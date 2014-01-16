/*
 * Some of this code is based on llc and llvm-ld from the LLVM tools suite.
 * It is used by permission under the University of Illinois/NCSA Open Source
 * License.
 *
 * Copyright (c) 2003-2010 University of Illinois at Urbana-Champaign.
 *
 * Copyright 2011-2012 Google Inc.
 * Copyright 2011-2012 Shannon Weyrick <weyrick@mozek.us>
 * Copyright 2011-2012 Conrad Steenberg <conrad.steenberg@gmail.com>
 *
 *   This Source Code Form is subject to the terms of the Mozilla Public
 *   License, v. 2.0. If a copy of the MPL was not distributed with this
 *   file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *
 */

#include "Native.h"
#include "builder/BuilderOptions.h"

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/PassManager.h>
#include <llvm/Pass.h>
#include <llvm/ADT/Triple.h>
#include <llvm/Support/FormattedStream.h>
#include <llvm/LinkAllPasses.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Linker.h>
#include <llvm/Support/Program.h>
#include <llvm/Support/PathV1.h>
#include <llvm/Support/Host.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/Support/ToolOutputFile.h>
#include <llvm/Analysis/Verifier.h>
#include <llvm/Bitcode/ReaderWriter.h>

#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

using namespace llvm;
using namespace llvm::sys;
using namespace builder;
using namespace std;

#ifdef __APPLE__
#include "crt_externs.h" // _NSGetEnviron
#else
extern char **environ;
#endif

namespace builder { namespace mvll {

typedef std::vector<std::pair<std::string,bool> > ItemList;

static void PrintCommand(const std::vector<const char*> &args) {
  std::vector<const char*>::const_iterator I = args.begin(), E = args.end();
  for (; I != E; ++I)
    if (*I)
      //cerr << "'" << *I << "'" << " ";
      cerr << *I << " ";
  cerr << "\n";
}


/// GenerateNative - generates a native object file from the
/// specified bitcode file.
///
static int GenerateNative(const std::string &OutputFilename,
                          const std::string &InputFilename,
                          const vector<std::string> &LibPaths,
                          const ItemList &LinkItems,
                          const sys::Path &gcc, char ** const envp,
                          std::string& ErrMsg,
                          bool is64Bit,
                          const BuilderOptions *o) {

  // Run GCC to assemble and link the program into native code.
  //
  // Note:
  //  We can't just assemble and link the file with the system assembler
  //  and linker because we don't know where to put the _start symbol.
  //  GCC mysteriously knows how to do it.
  std::vector<std::string> args;
  args.push_back(gcc.c_str());
  args.push_back("-O3");

  BuilderOptions::StringMap::const_iterator i = o->optionMap.find("PIE");
  if (i != o->optionMap.end()) {
      args.push_back("-fPIC");
      args.push_back("-pie");
  }

  if (is64Bit)
      args.push_back("-m64");

  args.push_back("-o");
  args.push_back(OutputFilename);
  args.push_back(InputFilename);

#ifdef __linux__
  args.push_back("-Wl,--add-needed");
#endif

  // Add in the library and framework paths
  if (o->verbosity > 3) {
      cerr << "Native link paths:" << endl;
      // verbose linker
      args.push_back("-Wl,--verbose");
  }

  for (unsigned index = 0; index < LibPaths.size(); index++) {
      if (o->verbosity > 2)
          cerr << LibPaths[index] << endl;
      args.push_back("-L" + LibPaths[index]);
#ifdef __linux__
      // XXX we add all lib paths as rpaths as well. this can potentially
      // cause conflicts where foo/baz.so and bar/baz.so exist as crack
      // extensions, but the runtime loader loads foo/baz.so for both since
      // it shows up first matching the rpath. this needs a better fix.
      char *rp = realpath(LibPaths[index].c_str(), NULL);
      if (!rp) {
          switch (errno) {
              case EACCES:
              case ELOOP:
              case ENAMETOOLONG:
              case ENOENT:
              case ENOTDIR:
                  break;
              default:
                  perror("realpath");
          }
          continue;
      } else {
        args.push_back("-Wl,-rpath=" + string(rp));
        free(rp);
      }
#endif
  }

  // Add in the libraries to link.
  if (o->verbosity > 2)
      cerr << "Native link libraries:" << endl;
  for (unsigned index = 0; index < LinkItems.size(); index++)
    if (LinkItems[index].first != "crtend") {
      if (o->verbosity > 2)
          cerr << LinkItems[index].first << endl;
      if (LinkItems[index].second)
        args.push_back("-l" + LinkItems[index].first);
      else
        args.push_back(LinkItems[index].first);
    }

  // Now that "args" owns all the std::strings for the arguments, call the c_str
  // method to get the underlying string array.  We do this game so that the
  // std::string array is guaranteed to outlive the const char* array.
  std::vector<const char *> Args;
  for (unsigned i = 0, e = args.size(); i != e; ++i)
    Args.push_back(args[i].c_str());
  Args.push_back(0);

  if (o->verbosity) {
      cerr << "Generating Native Executable With:\n";
      PrintCommand(Args);
  }

  // Run the compiler to assembly and link together the program.
  int R = sys::Program::ExecuteAndWait(
    gcc, &Args[0], 0, 0, 0, 0, &ErrMsg);

  return R;
}


/*

  define i32 @main(i32 %argc, i8** %argv) nounwind {
    %1 = alloca i32, align 4
    %2 = alloca i32, align 4
    %3 = alloca i8**, align 8
    store i32 0, i32* %1
    store i32 %argc, i32* %2, align 4
    store i8** %argv, i8*** %3, align 8

    XXX entry XXX
    XXX cleanup XXX

    %5 = load i32* %1
    ret i32 %5
  }

*/
void createMain(llvm::Module *mod, const BuilderOptions *o,
                llvm::Value *vtableBaseTypeBody,
                const string &mainModuleName
                ) {

    Function *scriptEntry = mod->getFunction(mainModuleName + ":main");
    assert(scriptEntry && "no main source file specified");

    // main cleanup function we insert into main() function, after script
    // is done
    Function *mainCleanup = mod->getFunction("main:cleanup");
    assert(mainCleanup && "no cleanup function");

    Function *crackLangInit = mod->getFunction("crack.lang:main");

    // Type Definitions

    // argc
    std::vector<Type *>main_args;
    IntegerType *argcType = IntegerType::get(mod->getContext(), 32);
    main_args.push_back(argcType);

    // argv
    PointerType* pc = PointerType::get(
            IntegerType::get(mod->getContext(), 8), 0);
    PointerType* argvType = PointerType::get(pc, 0);
    main_args.push_back(argvType);

    // int main(int, char**)
    FunctionType* main_func_ty = FunctionType::get(
            IntegerType::get(mod->getContext(), 32),
            main_args,
            /*isVarArg=*/false);

    // global argc and argv, which our __getArgv and __getArgc functions
    // will return. these are set by main
    GlobalVariable *gargc =
            new GlobalVariable(*mod, argcType, false,
                               GlobalValue::PrivateLinkage,
                               Constant::getNullValue(argcType),
                               "global_argc"
                               );
    GlobalVariable *gargv =
            new GlobalVariable(*mod, argvType, false,
                               GlobalValue::PrivateLinkage,
                               Constant::getNullValue(argvType),
                               "global_argv"
                               );


    // Function Declarations
    Function* func_main = Function::Create(
            main_func_ty,
            GlobalValue::ExternalLinkage,
            "main", mod);
    func_main->setCallingConv(CallingConv::C);

    // result value constant
    // XXX this needs to adjust when we start allowing return values
    ConstantInt* retValConst = ConstantInt::get(
            mod->getContext(), APInt(32, StringRef("0"), 10));

    // Function Definitions
    // Function: main (func_main)
    {
        Function::arg_iterator args = func_main->arg_begin();
        Value* int32_argc = args++;
        int32_argc->setName("argc");
        Value* ptr_argv = args++;
        ptr_argv->setName("argv");

        BasicBlock* label_13 = BasicBlock::Create(mod->getContext(), "entry",
                                                  func_main,0);
        BasicBlock *cleanupBlock = BasicBlock::Create(mod->getContext(),
                                                      "cleanup",
                                                      func_main
                                                      ),
                   *lpBlock = BasicBlock::Create(mod->getContext(),
                                                 "lp",
                                                 func_main
                                                 ),
                   *endBlock = BasicBlock::Create(mod->getContext(),
                                                  "end",
                                                  func_main
                                                  ),
                   *lpPostCleanupBlock = BasicBlock::Create(mod->getContext(),
                                                            "lp",
                                                            func_main
                                                            );

        // alloc return value, argc, argv
        AllocaInst* ptr_14 = new AllocaInst(
                IntegerType::get(mod->getContext(), 32), "", label_13);

        new StoreInst(retValConst, ptr_14, false, label_13);
        new StoreInst(int32_argc, gargc, false, label_13);
        new StoreInst(ptr_argv, gargv, false, label_13);

        IRBuilder<> builder(label_13);

        // call crack.lang init function
        // note if crackLangInit isn't set, we skip it. this happens for
        // non bootstrapped scripts
        if (crackLangInit) {
            BasicBlock *block = BasicBlock::Create(mod->getContext(),
                                                   "main",
                                                   func_main
                                                   );
            builder.CreateInvoke(crackLangInit, block, lpBlock);
            builder.SetInsertPoint(block);
        }

        // call main script initialization function
        builder.CreateInvoke(scriptEntry, cleanupBlock, lpBlock);

        // cleanup function
        builder.SetInsertPoint(cleanupBlock);
        builder.CreateInvoke(mainCleanup, endBlock, lpPostCleanupBlock);

        // exception handling stuff
        Function *crkExFunc = mod->getFunction("__CrackExceptionPersonality");
        Type *i8PtrType = builder.getInt8PtrTy();
        Type *exStructType =
            StructType::get(i8PtrType, builder.getInt32Ty(), NULL);

        BasicBlock *uncaughtHandlerBlock =
            BasicBlock::Create(mod->getContext(), "uncaught_exception",
                               func_main
                               );

        // first landing pad does cleanups
        builder.SetInsertPoint(lpBlock);
        LandingPadInst *lp =
          builder.CreateLandingPad(exStructType, crkExFunc, 1);
        lp->addClause(Constant::getNullValue(i8PtrType));
        builder.CreateInvoke(mainCleanup, uncaughtHandlerBlock,
                             lpPostCleanupBlock
                             );

        // get our uncaught exception function (for some reason this doesn't
        // work when we do it from anywhere else)
        FunctionType *funcType =
            FunctionType::get(Type::getInt1Ty(mod->getContext()), false);
        Constant *uncaughtFuncConst =
            mod->getOrInsertFunction("__CrackUncaughtException", funcType);
        Function *uncaughtFunc = cast<Function>(uncaughtFuncConst);

        // post cleanup landing pad
        builder.SetInsertPoint(lpPostCleanupBlock);
        lp = builder.CreateLandingPad(exStructType, crkExFunc, 1);
        lp->addClause(Constant::getNullValue(i8PtrType));
        builder.CreateBr(uncaughtHandlerBlock);

        // uncaught exception handler
        builder.SetInsertPoint(uncaughtHandlerBlock);
        builder.CreateCall(uncaughtFunc);
        builder.CreateBr(endBlock);

        // return value
        builder.SetInsertPoint(endBlock);
        LoadInst* int32_21 = builder.CreateLoad(ptr_14);
        builder.CreateRet(int32_21);

    }

    // now fill in the bodies of the __getArgv and __getArgc functions
    // these simply return the values of the globals we setup in main
    // int puts(char *)
    std::vector<Type *>args;
    FunctionType *argv_ftype = FunctionType::get(
            argvType,
            args, false);
    FunctionType* argc_ftype = FunctionType::get(
            argcType,
            args, false);
    Constant *c = mod->getOrInsertFunction("__getArgv", argv_ftype);
    Function *f = llvm::cast<llvm::Function>(c);
    IRBuilder<> builder(mod->getContext());
    builder.SetInsertPoint(BasicBlock::Create(mod->getContext(), "", f));
    Value *v = builder.CreateLoad(gargv);
    builder.CreateRet(v);

    c = mod->getOrInsertFunction("__getArgc", argc_ftype);
    f = llvm::cast<llvm::Function>(c);
    builder.SetInsertPoint(BasicBlock::Create(mod->getContext(), "", f));
    v = builder.CreateLoad(gargc);
    builder.CreateRet(v);

}

void optimizeLink(llvm::Module *module, bool verify) {

    // see llvm's opt tool

    // module pass manager
    PassManager Passes;

    // Add an appropriate TargetData instance for this module...
    DataLayout *DL = 0;
    const std::string &ModuleDataLayout = module->getDataLayout();
    if (!ModuleDataLayout.empty())
        DL = new DataLayout(ModuleDataLayout);

    if (DL)
        Passes.add(DL);

//    createStandardLTOPasses(&Passes, /*Internalize=*/ false,
//                                     /*RunInliner=*/ false, // XXX breaks exceptions
//                                     /*VerifyEach=*/ verify);

//    createStandardAliasAnalysisPasses(&Passes);
    Passes.add(createTypeBasedAliasAnalysisPass());
    Passes.add(createBasicAliasAnalysisPass());

    // Now that composite has been compiled, scan through the module, looking
    // for a main function.  If main is defined, mark all other functions
    // internal.
//    Passes.add(createInternalizePass(true));

    // Propagate constants at call sites into the functions they call.  This
    // opens opportunities for globalopt (and inlining) by substituting function
    // pointers passed as arguments to direct uses of functions.
    Passes.add(createIPSCCPPass());

    // Now that we internalized some globals, see if we can hack on them!
    Passes.add(createGlobalOptimizerPass());

    // Linking modules together can lead to duplicated global constants, only
    // keep one copy of each constant...
    Passes.add(createConstantMergePass());

    // Remove unused arguments from functions...
    Passes.add(createDeadArgEliminationPass());

    // Reduce the code after globalopt and ipsccp.  Both can open up significant
    // simplification opportunities, and both can propagate functions through
    // function pointers.  When this happens, we often have to resolve varargs
    // calls, etc, so let instcombine do this.
    Passes.add(createInstructionCombiningPass());

    // Inline small functions
//    Passes.add(createFunctionInliningPass());

    Passes.add(createPruneEHPass());   // Remove dead EH info.
    // Optimize globals again if we ran the inliner.
//    Passes.add(createGlobalOptimizerPass());
    Passes.add(createGlobalDCEPass()); // Remove dead functions.

    // If we didn't decide to inline a function, check to see if we can
    // transform it to pass arguments by value instead of by reference.
    Passes.add(createArgumentPromotionPass());

    // The IPO passes may leave cruft around.  Clean up after them.
    Passes.add(createInstructionCombiningPass());
    Passes.add(createJumpThreadingPass());
    // Break up allocas
    Passes.add(createScalarReplAggregatesPass());

    // Run a few AA driven optimizations here and now, to cleanup the code.
    Passes.add(createFunctionAttrsPass()); // Add nocapture.
    Passes.add(createGlobalsModRefPass()); // IP alias analysis.

// XXX The LICM pass appears to cause function calls to "oper bind" to be
// removed after reference counts were converted to atomic_int.  I suspect
// this may be a bug in the optimization pass, but tracking it down is a low
// priority.
//    Passes.add(createLICMPass());      // Hoist loop invariants.
    Passes.add(createGVNPass());       // Remove redundancies.
    Passes.add(createMemCpyOptPass()); // Remove dead memcpys.

    // Nuke dead stores.
    Passes.add(createDeadStoreEliminationPass());

    // Cleanup and simplify the code after the scalar optimizations.
    Passes.add(createInstructionCombiningPass());

    Passes.add(createJumpThreadingPass());

    // Delete basic blocks, which optimization passes may have killed.
    Passes.add(createCFGSimplificationPass());

    // Now that we have optimized the program, discard unreachable functions.
    Passes.add(createGlobalDCEPass());

    // the old code used to inject a verify after every pass, for now we save
    // some time by doing the verify once at the end.
    if (verify)
      Passes.add(createVerifierPass());

    Passes.run(*module);

}

// optimize
void optimizeUnit(llvm::Module *module, int optimizeLevel) {

    // see llvm's opt tool

    // module pass manager
    PassManager Passes;

    // Add an appropriate TargetData instance for this module...
    DataLayout *DL = 0;
    const std::string &ModuleDataLayout = module->getDataLayout();
    if (!ModuleDataLayout.empty())
        DL = new DataLayout(ModuleDataLayout);

    if (DL)
        Passes.add(DL);

    // function pass manager
    OwningPtr<PassManager> FPasses;
    FPasses.reset(new PassManager());
    if (DL)
        FPasses->add(new DataLayout(*DL));

    if (optimizeLevel > 0) {
        FPasses->add(createCFGSimplificationPass());
        FPasses->add(createScalarReplAggregatesPass());
        FPasses->add(createEarlyCSEPass());
    }

    llvm::Pass *InliningPass = 0;

    /*
    // XXX inlining currently breaks exceptions
    if (optimizeLevel > 1) {
        unsigned Threshold = 200;
        if (optimizeLevel > 2)
            Threshold = 250;
        InliningPass = createFunctionInliningPass(Threshold);
    } else {
        InliningPass = createAlwaysInlinerPass();
    }
    */

//    createStandardModulePasses(&Passes, optimizeLevel,
//                               /*OptimizeSize=*/ false,
//                               /*UnitAtATime*/ true,
//                               /*UnrollLoops=*/ optimizeLevel > 1,
//                               /*SimplifyLibCalls*/ true,
//                               /*HaveExceptions=*/ true,
//                               InliningPass);


    Passes.add(createGlobalOptimizerPass());     // Optimize out global vars
    Passes.add(createIPSCCPPass());              // IP SCCP
    Passes.add(createDeadArgEliminationPass());  // Dead argument elimination

    Passes.add(createInstructionCombiningPass());  // Clean up after IPCP & DAE
    Passes.add(createCFGSimplificationPass());     // Clean up after IPCP & DAE

    // Start of CallGraph SCC passes.
    Passes.add(createPruneEHPass());             // Remove dead EH info
//    if (InliningPass)
//        Passes.add(InliningPass);
    Passes.add(createFunctionAttrsPass());       // Set readonly/readnone attrs
    if (optimizeLevel > 2)
        Passes.add(createArgumentPromotionPass());  // Scalarize uninlined fn args

    // Start of function pass.
    // Break up aggregate allocas, using SSAUpdater.
    Passes.add(createScalarReplAggregatesPass(-1, false));
    Passes.add(createEarlyCSEPass());  // Catch trivial redundancies
    Passes.add(createSimplifyLibCallsPass());  // Library Call Optimizations
    Passes.add(createJumpThreadingPass());  // Thread jumps.
    Passes.add(createCorrelatedValuePropagationPass());  // Propagate conditionals
    Passes.add(createCFGSimplificationPass());  // Merge & remove BBs
    Passes.add(createInstructionCombiningPass());  // Combine silly seq's

    Passes.add(createTailCallEliminationPass());  // Eliminate tail calls
    Passes.add(createCFGSimplificationPass());  // Merge & remove BBs
    Passes.add(createReassociatePass());  // Reassociate expressions
    Passes.add(createLoopRotatePass());  // Rotate Loop
    Passes.add(createLICMPass());  // Hoist loop invariants
    Passes.add(createLoopUnswitchPass(optimizeLevel < 3));
    Passes.add(createInstructionCombiningPass());
    Passes.add(createIndVarSimplifyPass());  // Canonicalize indvars
    Passes.add(createLoopIdiomPass());  // Recognize idioms like memset.
    Passes.add(createLoopDeletionPass());  // Delete dead loops
    if (optimizeLevel > 1)
        Passes.add(createLoopUnrollPass());  // Unroll small loops
    Passes.add(createInstructionCombiningPass());  // Clean up after the unroller
    if (optimizeLevel > 1)
        Passes.add(createGVNPass());  // Remove redundancies
    Passes.add(createMemCpyOptPass());  // Remove memcpy / form memset
    Passes.add(createSCCPPass());  // Constant prop with SCCP

    // Run instcombine after redundancy elimination to exploit opportunities
    // opened up by them.
    Passes.add(createInstructionCombiningPass());
    Passes.add(createJumpThreadingPass());  // Thread jumps
    Passes.add(createCorrelatedValuePropagationPass());
    Passes.add(createDeadStoreEliminationPass());  // Delete dead stores
    Passes.add(createAggressiveDCEPass());  // Delete dead instructions
    Passes.add(createCFGSimplificationPass());  // Merge & remove BBs

    Passes.add(createStripDeadPrototypesPass());  // Get rid of dead prototypes

    // GlobalOpt already deletes dead functions and globals, at -O3 try a
    // late pass of GlobalDCE.  It is capable of deleting dead cycles.
    if (optimizeLevel > 2)
          Passes.add(createGlobalDCEPass());  // Remove dead fns and globals.

    if (optimizeLevel > 1)
        Passes.add(createConstantMergePass());  // Merge dup global constants

    FPasses->run(*module);
    Passes.run(*module);

}

void nativeCompile(llvm::Module *module,
                   const BuilderOptions *o,
                   const vector<string> &sharedLibs,
                   const vector<string> &libPaths) {

    BuilderOptions::StringMap::const_iterator i = o->optionMap.find("out");
    assert(i != o->optionMap.end() && "no out");

    sys::Path oFile(i->second);
    sys::Path binFile(i->second);

    // see if we should output an object file/native binary,
    // native assembly, or llvm bitcode
    TargetMachine::CodeGenFileType cgt = TargetMachine::CGFT_ObjectFile;
    bool doBitcode(false);
    oFile.appendSuffix("o");

    i = o->optionMap.find("codeGen");
    if (i != o->optionMap.end()) {
        if (i->second == "llvm") {
            // llvm bitcode
            oFile.eraseSuffix();
            oFile.appendSuffix("bc");
            doBitcode = true;
        }
        else if (i->second == "asm") {
            // native assembly
            oFile.eraseSuffix();
            oFile.appendSuffix("s");
            cgt = TargetMachine::CGFT_AssemblyFile;
        }
    }

    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();

    Triple TheTriple(module->getTargetTriple());

    if (TheTriple.getTriple().empty())
        TheTriple.setTriple(sys::getDefaultTargetTriple());

    const Target *TheTarget = 0;
    std::string Err;
    TheTarget = TargetRegistry::lookupTarget(TheTriple.getTriple(), Err);
    assert(TheTarget && "unable to select target machine");

    // XXX insufficient
    bool is64Bit(TheTriple.getArch() == Triple::x86_64);

    string FeaturesStr;
    string CPU;

    TargetOptions options;

    // position independent executables
    i = o->optionMap.find("PIE");
    Reloc::Model relocModel = Reloc::Default;
    if (i != o->optionMap.end()) {
        options.PositionIndependentExecutable = 1;
        relocModel = Reloc::PIC_;
    }

    std::auto_ptr<TargetMachine>
            target(TheTarget->createTargetMachine(TheTriple.getTriple(),
                                                  CPU,
                                                  FeaturesStr,
                                                  options,
                                                  relocModel
                                                  )
                   );
    assert(target.get() && "Could not allocate target machine!");
    TargetMachine &Target = *target.get();

    // Build up all of the passes that we want to do to the module.
    PassManager PM;

    // Add the target data from the target machine or the module.
    if (const DataLayout *DL = Target.getDataLayout())
        PM.add(new DataLayout(*DL));
    else
        PM.add(new DataLayout(module));

    // Override default to generate verbose assembly.
    Target.setAsmVerbosityDefault(true);

    std::string error;
    unsigned OpenFlags = 0;
    OpenFlags |= raw_fd_ostream::F_Binary;
    tool_output_file *FDOut = new tool_output_file(oFile.str().c_str(),
                                                   Err,
                                                   OpenFlags);
    if (!Err.empty()) {
        cerr << error << '\n';
        delete FDOut;
        return;
    }

    {
        formatted_raw_ostream FOS(FDOut->os());

        // note, we expect optimizations to be done by now, so we don't do
        // any here
        if (o->verbosity)
            cerr << "Generating file:\n" << oFile.str() << "\n";

        if (doBitcode) {
            // llvm bitcode
            WriteBitcodeToFile(module, FOS);
        }
        else {
            // native
            if (Target.addPassesToEmitFile(PM,
                                           FOS,
                                           cgt,
                                           o->debugMode // do verify
                                           )) {
                cerr << "target does not support generation of this"
                        << " file type!\n";
                return;
            }

            PM.run(*module);
        }
    }

    // note FOS needs to destruct before we can keep
    FDOut->keep();
    delete FDOut;

    // if we created llvm or native assembly file we're done
    if (doBitcode ||
        cgt == TargetMachine::CGFT_AssemblyFile)
        return;

    // if we reach here, we finish generating a native binary with gcc
    sys::Path gcc = sys::Program::FindProgramByName("gcc");
    assert(!gcc.isEmpty() && "Failed to find gcc");

    vector<string> LibPaths(libPaths);

    // if CRACK_LIB_PATH is set, add that
    char *elp = getenv("CRACK_LIB_PATH");
    if (elp) {
        // XXX split on :
        LibPaths.push_back(elp);
    }

    ItemList NativeLinkItems;

    for (vector<string>::const_iterator i = sharedLibs.begin();
         i != sharedLibs.end();
         ++i
         ) {
        if (path::has_parent_path(*i)) {
            LibPaths.push_back(path::parent_path(*i));
        }

#if __GNUC__ > 4 && __GNUC_MINOR__ > 2
        NativeLinkItems.push_back(pair<string,bool>(":"+
                                                    string(path::stem(*i))+
                                                    string(path::extension(*i)),
                                                    true // .so
                                                    ));
#else
        string rtp = string(path::stem(*i)) + string(path::extension(*i));
        Path sPath;
        bool foundModule = false;

        // We have to manually search for the linkitem
        for (unsigned index = 0; index < LibPaths.size(); index++) {
            sPath = Path(LibPaths[index]);
            sPath.appendComponent(rtp);

            if (o->verbosity > 2)
                cerr << "search: " << sPath.str() << endl;

            char *rp = realpath(sPath.c_str(), NULL);
            if (!rp) {
                switch (errno) {
                    case EACCES:
                    case ELOOP:
                    case ENAMETOOLONG:
                    case ENOENT:
                    case ENOTDIR:
                        break;
                    default:
                        perror("realpath");
                }
                continue;
            } else {
                free(rp);
                foundModule = true;
                break;
           }
        }

        NativeLinkItems.push_back(
            pair<string,bool>(foundModule ? sPath.str() : rtp, false)
        );

#endif
    }

    // native runtime lib is required
    NativeLinkItems.push_back(pair<string,bool>("CrackNativeRuntime",true));
#ifdef __APPLE__
    // osx requires explicit link to debug
    NativeLinkItems.push_back(pair<string,bool>("CrackDebugTools",true));
#endif

    string ErrMsg;

#ifdef __APPLE__
    char ***envp_ = _NSGetEnviron();
    char **envp = *envp_;
#else
    char **envp = ::environ;
#endif

    GenerateNative(binFile.str(),
                   oFile.str(),
                   LibPaths,
                   NativeLinkItems,
                   gcc,
                   envp,
                   ErrMsg,
                   is64Bit,
                   o
                   );


}

} }
