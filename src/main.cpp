#include <array>
#include <clang-c/Index.h>
#include <iostream>
#include <sstream>

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
      clang::NamedDecl *const namedDecl = dynamic_cast<clang::NamedDecl *>(decl);
      std::cout << "Found function " << namedDecl->getQualifiedNameAsString() <<std::endl;
      break;
    }
    case(clang::Decl::Kind::ParmVar):{
      clang::NamedDecl *const namedDecl = dynamic_cast<clang::NamedDecl *>(decl);
      clang::VarDecl* const valDecl =  dynamic_cast<clang::VarDecl *>(decl);
      clang::QualType const variableType = valDecl->getType();
      
      
      std::cout << "Found parameter " << namedDecl->getQualifiedNameAsString() ;

      if(variableType->isBuiltinType()){
        const clang::BuiltinType* builtinType = variableType->castAs<clang::BuiltinType>();
        if(builtinType->getKind() == clang::BuiltinType::Int){
          std::cout<<"type int ";
        }
        if(builtinType->getKind() == clang::BuiltinType::UInt){
          std::cout<<"type uint ";
        }
      }

      clang::Qualifiers const qualifier = variableType.getQualifiers();

      if(qualifier.hasConst()){
        std::cout<<"const ";
      }

      

      std::cout<<std::endl;
      break;
    }
    default:{

    }

   
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
