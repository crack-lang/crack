/*
 * Some of this code is based on llc and llvm-ld from the LLVM tools suite.
 * It is used by permission under the University of Illinois/NCSA Open Source
 * License.
 *
 * Copyright (c) 2003-2010 University of Illinois at Urbana-Champaign.
 *
 * Other parts Copyright 2010 Shannon Weyrick <weyrick@mozek.us>
 *
 */

#include "Native.h"
#include "builder/BuildOptions.h"

#include <llvm/LLVMContext.h>
#include <llvm/Module.h>
#include <llvm/PassManager.h>
#include <llvm/Pass.h>
#include <llvm/ADT/Triple.h>
#include <llvm/System/Host.h>
#include <llvm/Support/FormattedStream.h>
#include <llvm/Target/TargetData.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetRegistry.h>
#include <llvm/Target/TargetSelect.h>
#include <llvm/DerivedTypes.h>
#include <llvm/Instructions.h>
#include <llvm/Linker.h>
#include <llvm/System/Program.h>
#include <llvm/Support/IRBuilder.h>

#include <iostream>

using namespace llvm;
using namespace builder;
using namespace std;

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
/// Inputs:
///  InputFilename   - The name of the input bitcode file.
///  OutputFilename  - The name of the file to generate.
///  NativeLinkItems - The native libraries, files, code with which to link
///  LibPaths        - The list of directories in which to find libraries.
///  FrameworksPaths - The list of directories in which to find frameworks.
///  Frameworks      - The list of frameworks (dynamic libraries)
///  gcc             - The pathname to use for GGC.
///  envp            - A copy of the process's current environment.
///
/// Outputs:
///  None.
///
/// Returns non-zero value on error.
///
static int GenerateNative(const std::string &OutputFilename,
                          const std::string &InputFilename,
                          const vector<std::string> &LibPaths,
                          const Linker::ItemList &LinkItems,
                          const sys::Path &gcc, char ** const envp,
                          std::string& ErrMsg,
                          bool is64Bit,
                          bool verbose) {

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
  for (unsigned index = 0; index < LibPaths.size(); index++) {
    args.push_back("-L" + LibPaths[index]);
  }

  /*
  for (unsigned index = 0; index < FrameworkPaths.size(); index++) {
    args.push_back("-F" + FrameworkPaths[index]);
  }

  // Add the requested options
  for (unsigned index = 0; index < XLinker.size(); index++)
    args.push_back(XLinker[index]);
    */

  // Add in the libraries to link.
  for (unsigned index = 0; index < LinkItems.size(); index++)
    if (LinkItems[index].first != "crtend") {
      if (LinkItems[index].second)
        args.push_back("-l" + LinkItems[index].first);
      else
        args.push_back(LinkItems[index].first);
    }

  /*
  // Add in frameworks to link.
  for (unsigned index = 0; index < Frameworks.size(); index++) {
    args.push_back("-framework");
    args.push_back(Frameworks[index]);
  }
  */

  // Now that "args" owns all the std::strings for the arguments, call the c_str
  // method to get the underlying string array.  We do this game so that the
  // std::string array is guaranteed to outlive the const char* array.
  std::vector<const char *> Args;
  for (unsigned i = 0, e = args.size(); i != e; ++i)
    Args.push_back(args[i].c_str());
  Args.push_back(0);

  if (verbose) {
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
void createMain(llvm::Module *mod, const BuildOptions *o) {

    // script entry point we insert into main() function
    Function *scriptEntry = mod->getFunction(o->mainUnit+":main");
    assert(scriptEntry && "no main source file specified");

    // main cleanup function we insert into main() function, after script
    // is done
    Function *mainCleanup = mod->getFunction("main:cleanup");
    assert(mainCleanup && "no cleanup function");

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

void nativeCompile(llvm::Module *module, const BuildOptions *o) {

    // create int main(argc, argv) entry point
    createMain(module, o);

    // if we're dumping, return now that'd we've added main and finalized ir
    if (o->dump)
        return;

    BuildOptions::stringMap::const_iterator i = o->strOptions.find("outFile");
    assert(i != o->strOptions.end() && "no outFile");

    sys::Path oFile(i->second);
    sys::Path binFile(i->second);

    oFile.eraseSuffix();
    oFile.appendSuffix("o");

    binFile.eraseSuffix();

    // Initialize targets first, so that --version shows registered targets.
    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();

    Triple TheTriple(module->getTargetTriple());

    if (TheTriple.getTriple().empty())
        TheTriple.setTriple(sys::getHostTriple());

    const Target *TheTarget = 0;
    std::string Err;
    TheTarget = TargetRegistry::lookupTarget(TheTriple.getTriple(), Err);
    assert(TheTarget && "unable to select target machine");

    bool is64Bit(TheTriple.getArch() == Triple::x86_64);

    string FeaturesStr;
    std::auto_ptr<TargetMachine>
     target(TheTarget->createTargetMachine(TheTriple.getTriple(), FeaturesStr));
    assert(target.get() && "Could not allocate target machine!");
    TargetMachine &Target = *target.get();

    // Build up all of the passes that we want to do to the module.
    PassManager PM;

    // Add the target data from the target machine, if it exists, or the module.
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
            cerr << "Generating native object file:\n" << oFile.str() << "\n";

        // Ask the target to add backend passes as necessary.
        if (Target.addPassesToEmitFile(PM,
                                       FOS,
                                       TargetMachine::CGFT_ObjectFile,
                                       CodeGenOpt::None,
                                       false // do verify
                                       )) {
            cerr << "target does not support generation of this"
                    << " file type!\n";
            return;
        }

        PM.run(*module);
    }

    // note FOS needs to destruct before we can keep
    FDOut->keep();

    sys::Path gcc = sys::Program::FindProgramByName("gcc");
    assert(!gcc.isEmpty() && "Failed to find gcc");

    // libraries to link
    std::vector<string> LibPaths;
    LibPaths.assign(o->sourceLibPath.begin(), o->sourceLibPath.end());

    // if CRACK_LIB_PATH is set, add that
    char *elp = getenv("CRACK_LIB_PATH");
    if (elp) {
        // XXX split on :
        LibPaths.push_back(elp);
    }

    Linker::ItemList NativeLinkItems;

    // libcrack is required
    NativeLinkItems.push_back(pair<string,bool>("CrackLang",true));

    for (vector<string>::const_iterator i = o->sharedLibs.begin();
         i != o->sharedLibs.end();
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
                   (o->verbosity > 0)
                   );


}

} }
