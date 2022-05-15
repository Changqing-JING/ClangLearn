```shell
sudo apt install libclang-dev
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