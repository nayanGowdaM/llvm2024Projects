#include <clang/AST/ASTConsumer.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Frontend/FrontendPluginRegistry.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <clang/Rewrite/Frontend/Rewriters.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>
#include <llvm/Support/raw_ostream.h>
#include <fstream>
#include <iostream>
#include <string>

using namespace clang;
using namespace std;

namespace {

// Helper function to get the first child of a cursor
CXCursor getFirstChild(CXCursor parentCursor)
{
    CXCursor childCursor = clang_getNullCursor();
    clang_visitChildren(
        parentCursor,
        [](CXCursor c, CXCursor parent, CXClientData client_data)
        {
            CXCursor* cursor = static_cast<CXCursor*>(client_data);
            *cursor = c;
            return CXChildVisit_Break;
        },
        &childCursor
    );
    return childCursor;
}

// Helper function to extract binary operator from a cursor
std::string getBinaryOperator(CXTranslationUnit translationUnit, CXCursor expressionCursor)
{
    CXToken* expressionTokens;
    unsigned numExpressionTokens;
    clang_tokenize(translationUnit, clang_getCursorExtent(expressionCursor), &expressionTokens, &numExpressionTokens);

    CXCursor leftHandSideCursor = getFirstChild(expressionCursor);
    CXToken* leftHandSideTokens;
    unsigned numLeftHandSideTokens;
    clang_tokenize(translationUnit, clang_getCursorExtent(leftHandSideCursor), &leftHandSideTokens, &numLeftHandSideTokens);

    CXString operatorString = clang_getTokenSpelling(translationUnit, expressionTokens[numLeftHandSideTokens]);
    std::string operatorSymbol(clang_getCString(operatorString));

    clang_disposeString(operatorString);
    clang_disposeTokens(translationUnit, leftHandSideTokens, numLeftHandSideTokens);
    clang_disposeTokens(translationUnit, expressionTokens, numExpressionTokens);

    return operatorSymbol;
}

// Structure to hold edge and node counts
struct EdgeAndNodeCounter
{
    CXTranslationUnit translationUnit;
    int edges = 0;
    int nodes = 0;
};

// Callback function to count edges and nodes
CXChildVisitResult countEdgesAndNodesCallback(CXCursor cursor, CXCursor parent, CXClientData clientData)
{
    EdgeAndNodeCounter* counter = static_cast<EdgeAndNodeCounter*>(clientData);

    const CXCursorKind cursorKind = clang_getCursorKind(cursor);
    const std::vector<CXCursorKind> decision_kinds = {
        CXCursor_IfStmt, CXCursor_ForStmt, CXCursor_WhileStmt,
        CXCursor_DefaultStmt, CXCursor_CaseStmt,
        CXCursor_ConditionalOperator, CXCursor_BinaryOperator
    };

    if (std::find(decision_kinds.begin(), decision_kinds.end(), cursorKind) != decision_kinds.end())
    {
        if (cursorKind == CXCursor_BinaryOperator)
        {
            std::string operatorSymbol = getBinaryOperator(counter->translationUnit, cursor);
            if (operatorSymbol == "&&" || operatorSymbol == "||")
            {
                counter->edges += 2;
                counter->nodes += 1;
            }
        }
        else
        {
            counter->edges += 2;
            counter->nodes += 1;
        }
    }

    clang_visitChildren(cursor, countEdgesAndNodesCallback, clientData);
    return CXChildVisit_Continue;
}

// Count edges and nodes from a cursor
std::pair<int, int> countEdgesAndNodes(CXCursor cursor, CXTranslationUnit translationUnit)
{
    EdgeAndNodeCounter counter;
    counter.translationUnit = translationUnit;
    countEdgesAndNodesCallback(cursor, clang_getNullCursor(), &counter);
    return {counter.edges, counter.nodes};
}

// Compute cyclomatic complexity
int computeCyclomaticComplexity(CXCursor cursor, CXTranslationUnit translationUnit)
{
    auto counts = countEdgesAndNodes(cursor, translationUnit);
    const int edges = counts.first;
    const int nodes = counts.second + 1;
    return edges - nodes + 2;
}

// Create an unsaved file for Clang
CXUnsavedFile createUnsavedFile(const std::string& code)
{
    CXUnsavedFile unsaved_file;
    unsaved_file.Filename = "unsaved.c";
    unsaved_file.Contents = code.c_str();
    unsaved_file.Length = code.length();
    return unsaved_file;
}

// Parse translation unit
CXTranslationUnit parseTranslationUnit(CXIndex index, CXUnsavedFile* unsaved_file)
{
    CXTranslationUnit TU;
    CXErrorCode error = clang_parseTranslationUnit2(index, "unsaved.c", nullptr, 0, unsaved_file, 1, CXTranslationUnit_None, &TU);
    if (error != CXError_Success)
    {
        std::cerr << "Error: Unable to parse translation unit.\n";
        exit(1);
    }
    return TU;
}

// Structure for visiting children
struct ChildVisitor
{
    CXTranslationUnit cxTU;
};

// Callback function for visiting children
CXChildVisitResult visitChildren_callback(CXCursor cursor, CXCursor parent, CXClientData client_data)
{
    ChildVisitor* visitor = static_cast<ChildVisitor*>(client_data);

    if (clang_getCursorKind(cursor) == CXCursor_FunctionDecl)
    {
        CXSourceLocation location = clang_getCursorLocation(cursor);
        unsigned line, column;
        clang_getSpellingLocation(location, NULL, &line, &column, NULL);
        int complexity = computeCyclomaticComplexity(cursor, visitor->cxTU);
        
        CXString functionName = clang_getCursorSpelling(cursor);
        std::string functionNameStr = clang_getCString(functionName);
        clang_disposeString(functionName);

        std::ofstream outputFile("output.cy", std::ios_base::app);
        outputFile << line << " " << functionNameStr << " " << complexity << std::endl;
    }

    return CXChildVisit_Continue;
}

// Visit children of the root cursor
void visitChildren(CXCursor root_cursor, CXTranslationUnit TU)
{
    ChildVisitor visitor;
    visitor.cxTU = TU;
    clang_visitChildren(root_cursor, visitChildren_callback, &visitor);
}

} // end anonymous namespace

class CyclomaticComplexityVisitor : public RecursiveASTVisitor<CyclomaticComplexityVisitor> {
public:
    explicit CyclomaticComplexityVisitor(ASTContext *Context, std::ofstream &OutFile)
        : Context(Context), OutFile(OutFile) {}

    bool VisitFunctionDecl(FunctionDecl *FD) {
        if (FD->hasBody()) {
            int complexity = computeCyclomaticComplexity(FD);
            OutFile << FD->getNameInfo().getName().getAsString() << " " << complexity << "\n";
        }
        return true;
    }

private:
    ASTContext *Context;
    std::ofstream &OutFile;

    int computeCyclomaticComplexity(FunctionDecl *FD) {
        int complexity = 1; // Base complexity is 1
        Stmt *Body = FD->getBody();

        // Count decision points
        for (auto it = Body->child_begin(); it != Body->child_end(); ++it) {
            if (*it) {
                complexity += countDecisionPoints(*it);
            }
        }
        return complexity;
    }

    int countDecisionPoints(Stmt *S) {
        int points = 0;
        if (isa<IfStmt>(S) || isa<ForStmt>(S) || isa<WhileStmt>(S) || isa<SwitchStmt>(S)) {
            points++;
        } else if (isa<BinaryOperator>(S)) {
            BinaryOperator *BO = cast<BinaryOperator>(S);
            if (BO->isLogicalOp()) {
                points++;
            }
        }

        for (auto it = S->child_begin(); it != S->child_end(); ++it) {
            if (*it) {
                points += countDecisionPoints(*it);
            }
        }
        return points;
    }
};

class CyclomaticComplexityConsumer : public ASTConsumer {
public:
    explicit CyclomaticComplexityConsumer(ASTContext *Context, std::ofstream &OutFile)
        : Visitor(Context, OutFile) {}

    virtual void HandleTranslationUnit(ASTContext &Context) {
        Visitor.TraverseDecl(Context.getTranslationUnitDecl());
    }

private:
    CyclomaticComplexityVisitor Visitor;
};

class CyclomaticComplexityAction : public PluginASTAction {
protected:
    std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI, llvm::StringRef) override {
        OutFile.open("output.cy");
        return std::make_unique<CyclomaticComplexityConsumer>(&CI.getASTContext(), OutFile);
    }

    bool ParseArgs(const CompilerInstance &CI, const std::vector<std::string> &args) override {
        return true;
    }

    void EndSourceFileAction() override {
        OutFile.close();
    }

private:
    std::ofstream OutFile;
};

static FrontendPluginRegistry::Add<CyclomaticComplexityAction>
X("analyze-cyclomatic-complexity", "Analyze cyclomatic complexity of functions");

