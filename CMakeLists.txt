cmake_minimum_required(VERSION 3.5)

project(ClangLearn)


find_package(LLVM REQUIRED)


aux_source_directory(src sourceFiles)

add_executable(${PROJECT_NAME} ${sourceFiles})

target_include_directories(${PROJECT_NAME} PRIVATE ${LLVM_INCLUDE_DIRS})

target_link_libraries(${PROJECT_NAME} PRIVATE clang)