```shell
sudo apt install libclang-dev libclang-cpp-dev
mkdir build
cd build
cmake ..
make
./ClangLearn ../sample/header.hpp
```

### Dump ast with clang
```shell
clang -Xclang -ast-dump -fsyntax-only ./sample/header.hpp
```

### use clang-check
```shell
clang-check -ast-dump sample/header.hpp  --extra-arg=-fparse-all-comments
```