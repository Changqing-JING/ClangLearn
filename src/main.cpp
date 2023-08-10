#include <array>
#include <clang-c/Index.h>
#include <iostream>
#include <sstream>

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/AST/CommentVisitor.h"

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/Basic/SourceManager.h"

#include "clang/AST/Comment.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"

class DeclVisitor : public clang::RecursiveASTVisitor<DeclVisitor> {

public:
  DeclVisitor(clang::SourceManager &SourceManager) : SourceManager(SourceManager) {
  }

  bool VisitNamedDecl(clang::NamedDecl *NamedDecl) {
    std::cout << "Found " << NamedDecl->getQualifiedNameAsString() << " at "
              << "\n";
    const clang::comments::FullComment *Comment =
              NamedDecl->getASTContext().getLocalCommentForDeclUncached(NamedDecl);
    if(Comment){
      Comment->dump();
    }
    return true;
  }

private:
  clang::SourceManager &SourceManager;
  std::string getDeclLocation(clang::SourceLocation Loc) const {
    std::ostringstream OSS;
    OSS << SourceManager.getFilename(Loc).str() << ":" << SourceManager.getSpellingLineNumber(Loc) << ":" << SourceManager.getSpellingColumnNumber(Loc);
    return OSS.str();
  }
};

class DeclFinder : public clang::ASTConsumer {
  clang::SourceManager &SourceManager;
  DeclVisitor Visitor;

public:
  DeclFinder(clang::SourceManager &SM) : SourceManager(SM), Visitor(SM) {
  }

  void HandleTranslationUnit(clang::ASTContext &Context) final {
    clang::TranslationUnitDecl *D = Context.getTranslationUnitDecl();
    auto Decls = D->decls();

    for (auto &Decl : Decls) {
      const auto &FileID = SourceManager.getFileID(Decl->getLocation());
      if (FileID != SourceManager.getMainFileID())
        continue;
      Visitor.TraverseDecl(Decl);
    }
  }
};

namespace clang {
class ASTConsumer;
}

class DeclFindingAction : public clang::ASTFrontendAction {
public:
  std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance &CI, clang::StringRef) final {
    return std::unique_ptr<clang::ASTConsumer>(new DeclFinder(CI.getSourceManager()));
  }
};

std::ostream &operator<<(std::ostream &stream, const CXString &str) {
  stream << clang_getCString(str);
  clang_disposeString(str);
  return stream;
}

CXChildVisitResult printCursor(CXCursor cursor, CXCursor parent, CXClientData client_data) {
  CXCursorKind cursorKind = clang_getCursorKind(cursor);
  std::cout << "Cursor '" << clang_getCursorSpelling(cursor) << "' of kind '" << clang_getCursorKindSpelling(cursorKind) << std::endl;
  clang_visitChildren(cursor, printCursor, nullptr);
  return CXChildVisit_Recurse;
}

llvm::cl::OptionCategory FindDeclCategory("find-decl options");

int main(int argc, const char **argv) {

  llvm::Expected<clang::tooling::CommonOptionsParser> option = clang::tooling::CommonOptionsParser::create(argc, argv, FindDeclCategory);

  auto files = option->getSourcePathList();
  clang::tooling::ClangTool tool(option->getCompilations(), files);

  return tool.run(clang::tooling::newFrontendActionFactory<DeclFindingAction>().get());
}
