#include "llvm/Pass.h"
#include "llvm/IR/Constants.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

// Command-line option to enable the analysis
static llvm::cl::opt<bool> AnalyzeMPIUniformParticipation(
    "analyze-mpi-uniform-participation",
    llvm::cl::desc("Analyze MPI uniform participation among processes"),
    llvm::cl::init(false)
);

namespace {

struct MPIUniformParticipationPass : public llvm::ModulePass {
    static char ID;
    MPIUniformParticipationPass() : ModulePass(ID) {}

    // Store detected MPI calls
    std::vector<std::tuple<std::string, std::string, int, int>> mpiCalls;

    bool runOnModule(llvm::Module &M) override {
        llvm::errs() << "[INFO] Starting MPI Uniform Participation Analysis...\n";

        for (llvm::Function &F : M) {
            llvm::errs() << "[DEBUG] Analyzing Function: " << F.getName() << "\n";

            for (llvm::BasicBlock &BB : F) {
                for (llvm::Instruction &I : BB) {
                    llvm::errs() << "[DEBUG] Instruction: " << I.getOpcodeName() << "\n";

                    if (auto *Call = llvm::dyn_cast<llvm::CallInst>(&I)) {
                        llvm::Function *Callee = Call->getCalledFunction();
                        if (!Callee) continue;

                        llvm::errs() << "[DEBUG] Found Call to: " << Callee->getName() << "\n";

                        if (Callee->getName().equals("MPI_Send") || Callee->getName().equals("MPI_Recv")) {
                            llvm::errs() << "[INFO] Detected MPI Call: " << Callee->getName() << "\n";

                            // Assuming the argument positions are correct:
                            llvm::Value *comm = Call->getArgOperand(5); // Assuming 6th argument is the communicator
                            llvm::Value *tag = Call->getArgOperand(4);  // Assuming 5th argument is the tag
                            llvm::Value *rank = Call->getArgOperand(3); // Assuming 4th argument is the rank

                            std::string commName = "UnknownComm";
                            if (auto *commConst = llvm::dyn_cast<llvm::GlobalValue>(comm)) {
                                commName = commConst->getName().str();
                            }

                            int tagVal = 0;
                            if (auto *tagConst = llvm::dyn_cast<llvm::ConstantInt>(tag)) {
                                tagVal = tagConst->getZExtValue();
                            }

                            int rankVal = 0;
                            if (auto *rankConst = llvm::dyn_cast<llvm::ConstantInt>(rank)) {
                                rankVal = rankConst->getZExtValue();
                            }

                            llvm::errs() << "[INFO] Detected MPI " << Callee->getName() << ": comm=" << commName
                                         << ", tag=" << tagVal << ", rank=" << rankVal << "\n";

                            // Store the MPI call information
                            mpiCalls.push_back(std::make_tuple(Callee->getName().str(), commName, tagVal, rankVal));
                        }
                    }
                }
            }
        }

        // Analyzing uniform participation
        llvm::errs() << "[INFO] Analyzing Uniform Participation Patterns...\n";

        for (const auto &call : mpiCalls) {
            const std::string &funcName = std::get<0>(call);
            const std::string &comm = std::get<1>(call);
            int tag = std::get<2>(call);
            int rank = std::get<3>(call);

            llvm::errs() << "[INFO] Detected " << funcName << ": comm=" << comm
                         << ", tag=" << tag << ", rank=" << rank << "\n";
        }

        llvm::errs() << "[INFO] Finished MPI Uniform Participation Analysis.\n";
        return false;
    }
};

}

char MPIUniformParticipationPass::ID = 0;
static llvm::RegisterPass<MPIUniformParticipationPass> X("mpi-uniform-participation", "MPI Uniform Participation Analysis", false, false);

