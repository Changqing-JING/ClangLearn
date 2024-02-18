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
  explicit DeclVisitor(clang::SourceManager &SourceManager) noexcept : SourceManager(SourceManager) {
  }

  bool VisitFunctionDecl(clang::FunctionDecl *functionDecl) const {

    dumpComment(functionDecl);

    std::cout << "Found function " << functionDecl->getQualifiedNameAsString() << std::endl;
    clang::AttrVec attrs = functionDecl->getAttrs();

    for (uint32_t i = 0; i < attrs.size(); i++) {
      clang::Attr *attr = attrs[i];
      std::cout << attr->getAttrName()->getName().str() << std::endl;
    }

    uint32_t const numParams = functionDecl->getNumParams();
    std::cout << "with num of args " << numParams << std::endl;
    for (uint32_t i = 0; i < numParams; i++) {
      clang::ParmVarDecl const *const parameterDecl = functionDecl->getParamDecl(i);
      dumpParameter(parameterDecl);
    }

    return true;
  }
  bool VisitDecl(clang::Decl *decl) {

    const clang::Decl::Kind kind = decl->getKind();

    switch (kind) {

    default: {
    }
    }

    return true;
  }

  bool VisitEnumDecl(clang::EnumDecl *enumDecl) const {
    if (enumDecl->isScoped()) { // Check if the enum is a scoped enum (enum class).
      // Do something with the enum class declaration.
      std::cout << "Found enum class: " << enumDecl->getNameAsString() << std::endl;

      // If you also want to visit each enumerator of the enum class, you can do that here.
      for (clang::EnumConstantDecl *enumerator : enumDecl->enumerators()) {
        std::cout << "  Enumerator: " << enumerator->getNameAsString();
        llvm::APSInt value = enumerator->getInitVal();

        // Print the value
        std::cout << " = " << value.getExtValue() << std::endl;
      }
    }

    // Return true to continue visiting other nodes in the AST.
    return true;
  }

private:
  clang::SourceManager &SourceManager;
  std::string getDeclLocation(clang::SourceLocation Loc) const {
    std::ostringstream OSS;
    OSS << SourceManager.getFilename(Loc).str() << ":" << SourceManager.getSpellingLineNumber(Loc) << ":" << SourceManager.getSpellingColumnNumber(Loc);
    return OSS.str();
  }

  static void dumpParameter(clang::ParmVarDecl const *const parameterDecl) {

    clang::QualType const variableType = parameterDecl->getType();

    std::cout << "Found parameter " << parameterDecl->getQualifiedNameAsString() << " ";

    clang::Qualifiers const qualifier = variableType.getQualifiers();

    if (qualifier.hasConst()) {
      std::cout << "const ";
    }

    switch (variableType->getTypeClass()) {
    case (clang::Type::TypeClass::Builtin): {
      const clang::BuiltinType *const builtinType = variableType->castAs<clang::BuiltinType>();
      dumpBuiltinType(builtinType);
      break;
    }
    case (clang::Type::TypeClass::Pointer): {
      std::cout << "type pointer ";
      const clang::PointerType *const pointerType = variableType->castAs<clang::PointerType>();
      if (pointerType->isFunctionPointerType()) {
        std::cout << "to function ";
      }
      break;
    }
    case (clang::Type::TypeClass::Typedef): {
      const clang::TypedefType *const typeDef = variableType->castAs<clang::TypedefType>();
      clang::QualType underLayerType = typeDef->desugar();

      while (underLayerType->isTypedefNameType()) {
        assert(underLayerType->getTypeClass() == clang::Type::TypeClass::Typedef);
        underLayerType = underLayerType->castAs<clang::TypedefType>()->desugar();
      }
      if (underLayerType->isBuiltinType()) {
        const clang::BuiltinType *const builtinType = underLayerType->castAs<clang::BuiltinType>();
        dumpBuiltinType(builtinType);
      } else {
        std::cout << "unsupported type";
      }

      break;
    }
    case (clang::Type::TypeClass::Elaborated): {
      std::cout << "type Elaborated ";
      const clang::ElaboratedType *const elaboratedType = variableType->castAs<clang::ElaboratedType>();
      clang::CXXRecordDecl *decl = elaboratedType->getAsCXXRecordDecl();
      std::cout << decl->getName().str();
      break;
    }
    default: {
      break;
    }
    }

    std::cout << std::endl;
  }

  static void dumpBuiltinType(clang::BuiltinType const *const builtinType) {
    switch (builtinType->getKind()) {
    case (clang::BuiltinType::Int): {
      std::cout << "type int ";
      break;
    }
    case (clang::BuiltinType::UInt): {
      std::cout << "type uint ";
      break;
    }
    case (clang::BuiltinType::ULong): {
      std::cout << "type ulong ";
      break;
    }
    case (clang::BuiltinType::ULongLong): {
      std::cout << "type ULongLong ";
      break;
    }
    default: {
      break;
    }
    }
  }

  static void dumpComment(clang::Decl const *const decl) {
    const clang::comments::FullComment *Comment = decl->getASTContext().getLocalCommentForDeclUncached(decl);
    if (Comment) {
      Comment->dump();
    }
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

  std::array<std::string, 2U> args{"-std=c++20", "--target=wasm32"};

  // The source file(s) you wish to parse.
  std::vector<std::string> sourcePaths = {argv[1]};

  // Manually set up the CompilationDatabase and SourcePathList.
  clang::tooling::FixedCompilationDatabase compilations(".", args);
  clang::tooling::ClangTool tool(compilations, sourcePaths);

  return tool.run(clang::tooling::newFrontendActionFactory<DeclFindingAction>().get());
}
