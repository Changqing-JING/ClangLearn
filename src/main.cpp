#include <clang-c/Index.h>
#include <iostream>

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

int main(int argc, const char *argv[]) {
  if (argc < 2) {
    std::cout << "no input file" << std::endl;
    exit(1);
  }
  CXIndex index = clang_createIndex(0, 0);
  CXTranslationUnit unit = clang_parseTranslationUnit(index, argv[1], nullptr, 0, nullptr, 0, CXTranslationUnit_None);
  if (unit == nullptr) {
    std::cerr << "Unable to parse translation unit. Quitting." << std::endl;
    exit(-1);
  }

  CXCursor cursor = clang_getTranslationUnitCursor(unit);
  clang_visitChildren(cursor, printCursor, nullptr);

  clang_disposeTranslationUnit(unit);
  clang_disposeIndex(index);
}