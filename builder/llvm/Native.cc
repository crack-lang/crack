/*
 * Some of this code is based on llc and llvm-ld from the LLVM tools suite.
 * It is used by permission under the University of Illinois/NCSA Open Source
 * License.
 *
 * Copyright (c) 2003-2010 University of Illinois at Urbana-Champaign.
 *
 * Other parts Copyright 2010-2011 Shannon Weyrick <weyrick@mozek.us>
 *
 */

#include "Native.h"
#include "builder/BuilderOptions.h"

#include <llvm/LLVMContext.h>
#include <llvm/Module.h>
#include <llvm/PassManager.h>
#include <llvm/Pass.h>
#include <llvm/ADT/Triple.h>
#include <llvm/System/Host.h>
#include <llvm/Support/StandardPasses.h>
#include <llvm/Support/FormattedStream.h>
#include <llvm/LinkAllPasses.h>
#include <llvm/Target/TargetData.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetRegistry.h>
#include <llvm/Target/TargetSelect.h>
#include <llvm/DerivedTypes.h>
#include <llvm/Instructions.h>
#include <llvm/Linker.h>
#include <llvm/System/Program.h>
#include <llvm/Support/IRBuilder.h>
#include <llvm/Bitcode/ReaderWriter.h>

#include <iostream>
#include <stdlib.h>

using namespace llvm;
using namespace builder;
using namespace std;

extern char **environ;

namespace builder { namespace mvll {

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
                          const Linker::ItemList &LinkItems,
                          const sys::Path &gcc, char ** const envp,
                          std::string& ErrMsg,
                          bool is64Bit,
                          int verbosity) {

  // Run GCC to assemble and link the program into native code.
  //
  // Note:
  //  We can't just assemble and link the file with the system assembler
  //  and linker because we don't know where to put the _start symbol.
  //  GCC mysteriously knows how to do it.
  std::vector<std::string> args;
  args.push_back(gcc.c_str());
  args.push_back("-O3");

  if (is64Bit)
      args.push_back("-m64");

  args.push_back("-o");
  args.push_back(OutputFilename);
  args.push_back(InputFilename);

  // Add in the library and framework paths
  if (verbosity > 2)
      cerr << "Native link paths:" << endl;
  for (unsigned index = 0; index < LibPaths.size(); index++) {
      if (verbosity > 2)
          cerr << LibPaths[index] << endl;
      args.push_back("-L" + LibPaths[index]);
  }

  // Add in the libraries to link.
  if (verbosity > 2)
      cerr << "Native link libraries:" << endl;
  for (unsigned index = 0; index < LinkItems.size(); index++)
    if (LinkItems[index].first != "crtend") {
      if (verbosity > 2)
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

  if (verbosity) {
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
void createMain(llvm::Module *mod, const BuilderOptions *o) {

    // script entry point we insert into main() function
    BuilderOptions::StringMap::const_iterator i = o->optionMap.find("mainUnit");
    assert(i != o->optionMap.end() && "no mainUnit");

    Function *scriptEntry = mod->getFunction(i->second+":main");
    assert(scriptEntry && "no main source file specified");

    // main cleanup function we insert into main() function, after script
    // is done
    Function *mainCleanup = mod->getFunction("main:cleanup");
    assert(mainCleanup && "no cleanup function");

    Function *crackLangInit = mod->getFunction("crack.lang:main");

    // Type Definitions

    // argc
    std::vector<const Type*>main_args;
    const IntegerType *argcType = IntegerType::get(mod->getContext(), 32);
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

        // alloc return value, argc, argv
        AllocaInst* ptr_14 = new AllocaInst(
                IntegerType::get(mod->getContext(), 32), "", label_13);

        new StoreInst(retValConst, ptr_14, false, label_13);
        new StoreInst(int32_argc, gargc, false, label_13);
        new StoreInst(ptr_argv, gargv, false, label_13);

        // call crack.lang init function
        // note if crackLangInit isn't set, we skip it. this happens for
        // non bootstrapped scripts
        if (crackLangInit)
            CallInst::Create(crackLangInit, "", label_13);

        // call main script initialization function
        CallInst::Create(scriptEntry, "", label_13);

        // cleanup function
        CallInst::Create(mainCleanup, "", label_13);

        // return value
        LoadInst* int32_21 = new LoadInst(ptr_14, "", false, label_13);
        ReturnInst::Create(mod->getContext(), int32_21, label_13);

    }

    // now fill in the bodies of the __getArgv and __getArgc functions
    // these simply return the values of the globals we setup in main
    // int puts(char *)
    std::vector<const Type*>args;
    FunctionType* argv_ftype = FunctionType::get(
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
    TargetData *TD = 0;
    const std::string &ModuleDataLayout = module->getDataLayout();
    if (!ModuleDataLayout.empty())
        TD = new TargetData(ModuleDataLayout);

    if (TD)
        Passes.add(TD);

    createStandardLTOPasses(&Passes, /*Internalize=*/ false,
                                     /*RunInliner=*/ false, // XXX breaks exceptions
                                     /*VerifyEach=*/ verify);
    Passes.run(*module);

}

// optimize
void optimizeUnit(llvm::Module *module, int optimizeLevel) {

    // see llvm's opt tool

    // module pass manager
    PassManager Passes;

    // Add an appropriate TargetData instance for this module...
    TargetData *TD = 0;
    const std::string &ModuleDataLayout = module->getDataLayout();
    if (!ModuleDataLayout.empty())
        TD = new TargetData(ModuleDataLayout);

    if (TD)
        Passes.add(TD);

    // function pass manager
    OwningPtr<PassManager> FPasses;
    FPasses.reset(new PassManager());
    if (TD)
        FPasses->add(new TargetData(*TD));

    createStandardFunctionPasses(FPasses.get(), optimizeLevel);

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

    createStandardModulePasses(&Passes, optimizeLevel,
                               /*OptimizeSize=*/ false,
                               /*UnitAtATime*/ true,
                               /*UnrollLoops=*/ optimizeLevel > 1,
                               /*SimplifyLibCalls*/ true,
                               /*HaveExceptions=*/ true,
                               InliningPass);

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

    oFile.eraseSuffix();
    binFile.eraseSuffix();

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
        TheTriple.setTriple(sys::getHostTriple());

    const Target *TheTarget = 0;
    std::string Err;
    TheTarget = TargetRegistry::lookupTarget(TheTriple.getTriple(), Err);
    assert(TheTarget && "unable to select target machine");

    // XXX insufficient
    bool is64Bit(TheTriple.getArch() == Triple::x86_64);

    string FeaturesStr;
    std::auto_ptr<TargetMachine>
            target(TheTarget->createTargetMachine(TheTriple.getTriple(),
                                                  FeaturesStr));
    assert(target.get() && "Could not allocate target machine!");
    TargetMachine &Target = *target.get();

    // Build up all of the passes that we want to do to the module.
    PassManager PM;

    // Add the target data from the target machine or the module.
    if (const TargetData *TD = Target.getTargetData())
        PM.add(new TargetData(*TD));
    else
        PM.add(new TargetData(module));

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
                                           CodeGenOpt::None,
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

    Linker::ItemList NativeLinkItems;

    // libcrack is required
    NativeLinkItems.push_back(pair<string,bool>("CrackLang",true));

    for (vector<string>::const_iterator i = sharedLibs.begin();
         i != sharedLibs.end();
         ++i) {

         // split out directories
         sys::Path lib(*i);

         if (!lib.getDirname().empty()) {
             LibPaths.push_back(lib.getDirname());
         }

         NativeLinkItems.push_back(pair<string,bool>(":"+
                                                     string(lib.getBasename())+
                                                     "."+
                                                     string(lib.getSuffix()),
                                                     true // .so
                                                     ));
    }

    string ErrMsg;
    char **envp = ::environ;

    GenerateNative(binFile.str(),
                   oFile.str(),
                   LibPaths,
                   NativeLinkItems,
                   gcc,
                   envp,
                   ErrMsg,
                   is64Bit,
                   o->verbosity
                   );


}

} }
