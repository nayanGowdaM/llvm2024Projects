#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/ToolOutputFile.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Linker/Linker.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Bitcode/BitcodeReader.h"
#include "llvm/Bitcode/BitcodeWriter.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/CommandLine.h"
#include <fstream>
#include <iomanip>

using namespace llvm;

// Command-line options
static cl::opt<std::string> InputFilename(cl::Positional, cl::desc("<input .c file>"), cl::init("-"));
static cl::opt<std::string> OutputFilename("o", cl::desc("Specify output filename"), cl::value_desc("filename"));

namespace {

struct InstrPass : public ModulePass {
  static char ID;
  InstrPass() : ModulePass(ID) {}

  bool runOnModule(Module &M) override {
    // Construct output filename
    std::string moduleName = InputFilename;
    std::string outputFilename = moduleName.substr(0, moduleName.find_last_of('.')) + ".ic";

    // Open file to write the output
    std::ofstream outFile(outputFilename);

    // Define column width for alignment
    const int colWidth = 15;

    // Write table header
    outFile << std::setw(colWidth) << "Function Name" 
            << " | " << std::setw(colWidth) << "Arithmetic" 
            << " | " << std::setw(colWidth) << "Logical" 
            << " | " << std::setw(colWidth) << "Comparison" 
            << " | " << std::setw(colWidth) << "Memory" 
            << " | " << std::setw(colWidth) << "Control Flow" 
            << " | " << std::setw(colWidth) << "Function Call" 
            << "\n";
    outFile << std::setw(colWidth) << std::setfill('-') << "" 
            << " | " << std::setw(colWidth) << "" 
            << " | " << std::setw(colWidth) << "" 
            << " | " << std::setw(colWidth) << "" 
            << " | " << std::setw(colWidth) << "" 
            << " | " << std::setw(colWidth) << "" 
            << " | " << std::setw(colWidth) << "" 
            << "\n";
    outFile << std::setfill(' ');

    bool hasNonZeroCounts = false;

    for (auto &F : M) {
      int arithmeticCount = 0;
      int logicalCount = 0;
      int comparisonCount = 0;
      int memoryCount = 0;
      int controlFlowCount = 0;
      int functionCallCount = 0;

      for (auto &BB : F) {
        for (auto &I : BB) {
          if (isa<BinaryOperator>(&I)) {
            switch (I.getOpcode()) {
              case Instruction::Add:
              case Instruction::FAdd:
              case Instruction::Sub:
              case Instruction::FSub:
              case Instruction::Mul:
              case Instruction::FMul:
              case Instruction::UDiv:
              case Instruction::SDiv:
              case Instruction::FDiv:
              case Instruction::URem:
              case Instruction::SRem:
              case Instruction::FRem:
                arithmeticCount++;
                break;

              case Instruction::And:
              case Instruction::Or:
              case Instruction::Xor:
              case Instruction::Shl:
              case Instruction::LShr:
              case Instruction::AShr:
                logicalCount++;
                break;

              default:
                break;
            }
          } else if (isa<ICmpInst>(&I) || isa<FCmpInst>(&I)) {
            comparisonCount++;
          } else if (isa<LoadInst>(&I) || isa<StoreInst>(&I) ||
                     isa<AllocaInst>(&I) || isa<GetElementPtrInst>(&I)) {
            memoryCount++;
          } else if (isa<BranchInst>(&I) || isa<SwitchInst>(&I) || isa<PHINode>(&I)) {
            controlFlowCount++;
          } else if (isa<CallInst>(&I) || isa<InvokeInst>(&I) || isa<ReturnInst>(&I)) {
            functionCallCount++;
          }
        }
      }

      // Write function data only if at least one count is non-zero
      if (arithmeticCount > 0 || logicalCount > 0 || comparisonCount > 0 ||
          memoryCount > 0 || controlFlowCount > 0 || functionCallCount > 0) {
        outFile << std::setw(colWidth) << F.getName().str() 
                << " | " << std::setw(colWidth) << arithmeticCount 
                << " | " << std::setw(colWidth) << logicalCount 
                << " | " << std::setw(colWidth) << comparisonCount 
                << " | " << std::setw(colWidth) << memoryCount 
                << " | " << std::setw(colWidth) << controlFlowCount 
                << " | " << std::setw(colWidth) << functionCallCount 
                << "\n";
        hasNonZeroCounts = true;
      }
    }

    if (!hasNonZeroCounts) {
      outFile << "No functions with non-zero counts found.\n";
    }

    // Close the file
    outFile.close();
    return false;
  }
};

}

char InstrPass::ID = 0;
static RegisterPass<InstrPass> X("analyze-instr-pass", "Instruction Count Analysis Pass");

int main(int argc, char **argv) {
  // Parse command line options
  cl::ParseCommandLineOptions(argc, argv, "InstrPass\n");

  LLVMContext Context;
  SMDiagnostic Err;
  
  // Parse the input .c file using Clang
  std::string ErrMsg;
  std::string BitcodeFilename = InputFilename.substr(0, InputFilename.find_last_of('.')) + ".bc";
  
  // Compile the .c file to LLVM bitcode
  std::string ClangCmd = "clang -O0 -emit-llvm -c " + InputFilename + " -o " + BitcodeFilename;
  if (system(ClangCmd.c_str()) != 0) {
    errs() << "Error: failed to compile " + InputFilename + "\n";
    return 1;
  }

  // Load the bitcode file
  std::unique_ptr<Module> M = parseIRFile(BitcodeFilename, Err, Context);
  if (!M) {
    errs() << "Error reading bitcode file: " << BitcodeFilename << "\n";
    return 1;
  }

  // Create a pass manager and run the pass
  legacy::PassManager PM;
  PM.add(new InstrPass());
  PM.run(*M);

  return 0;
}