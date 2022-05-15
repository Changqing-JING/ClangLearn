#include <clang-c/Index.h>
#include <iostream>

std::ostream &operator<<(std::ostream &stream, const CXString &str) {
  stream << clang_getCString(str);
  clang_disposeString(str);
  return stream;
}

int main(int argc, const char *argv[]) {
  if (argc < 2) {
    std::cout << "no input file" << std::endl;
    exit(1);
  }
  CXIndex index = clang_createIndex(0, 0);
  CXTranslationUnit unit = clang_parseTranslationUnit(
      index, argv[1], nullptr, 0, nullptr, 0, CXTranslationUnit_None);
  if (unit == nullptr) {
    std::cerr << "Unable to parse translation unit. Quitting." << std::endl;
    exit(-1);
  }

  CXCursor cursor = clang_getTranslationUnitCursor(unit);
  clang_visitChildren(
      cursor,
      [](CXCursor c, CXCursor parent, CXClientData client_data) {
        CXCursorKind cursorKind = clang_getCursorKind(c);
        std::cout << "Cursor '" << clang_getCursorSpelling(c) << "' of kind '"
                  << clang_getCursorKindSpelling(cursorKind) << std::endl;
        return CXChildVisit_Recurse;
      },
      nullptr);

  clang_disposeTranslationUnit(unit);
  clang_disposeIndex(index);
}