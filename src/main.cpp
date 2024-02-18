#include <array>
#include <cassert>
#include <clang-c/Index.h>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <vector>

#include "clang/AST/CommentVisitor.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"

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

  bool VisitDecl(clang::Decl *decl) {

    const clang::comments::FullComment *Comment = decl->getASTContext().getLocalCommentForDeclUncached(decl);
    if (Comment) {
      Comment->dump();
    }

    const clang::Decl::Kind kind = decl->getKind();

    switch (kind) {
    case (clang::Decl::Kind::Function): {
      clang::FunctionDecl const *const functionDecl = static_cast<clang::FunctionDecl *>(decl);
      std::cout << "Found function " << functionDecl->getQualifiedNameAsString() << std::endl;
      uint32_t const numParams = functionDecl->getNumParams();
      std::cout << "with num of args " << numParams << std::endl;
      for (uint32_t i = 0; i < numParams; i++) {
        clang::ParmVarDecl const *const parameterDecl = functionDecl->getParamDecl(i);
        visitParameter(parameterDecl);
      }

      return false;
    }
    case (clang::Decl::Kind::ParmVar): {
      assert(false);
      break;
    }
    default: {
    }
    }

    return true;
  }

  void visitParameter(clang::ParmVarDecl const *const parameterDecl) {

    clang::QualType const variableType = parameterDecl->getType();

    std::cout << "Found parameter " << parameterDecl->getQualifiedNameAsString() << " ";

    if (variableType->isBuiltinType()) {
      const clang::BuiltinType *builtinType = variableType->castAs<clang::BuiltinType>();
      if (builtinType->getKind() == clang::BuiltinType::Int) {
        std::cout << "type int ";
      }
      if (builtinType->getKind() == clang::BuiltinType::UInt) {
        std::cout << "type uint ";
      }
    }

    clang::Qualifiers const qualifier = variableType.getQualifiers();

    if (qualifier.hasConst()) {
      std::cout << "const ";
    }

    std::cout << std::endl;
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

  std::array<std::string, 1U> args{"-std=c++20"};

  // The source file(s) you wish to parse.
  std::vector<std::string> sourcePaths = {argv[1]};

  // Manually set up the CompilationDatabase and SourcePathList.
  clang::tooling::FixedCompilationDatabase compilations(".", args);
  clang::tooling::ClangTool tool(compilations, sourcePaths);

  return tool.run(clang::tooling::newFrontendActionFactory<DeclFindingAction>().get());
}
