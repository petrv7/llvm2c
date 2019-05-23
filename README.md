# llvm2c

Translation of LLVM bitcode to C

## Dependencies

| Name | Version | 
| ---- | ------- |
| CMake | 2.8+ |
| LLVM | 6.0 - 7.0 |
| Clang | 6.0 - 7.0 |

## Installation

    git clone https://github.com/petrv7/llvm2c
    cd llvm2c
    mkdir build
    cd build
    cmake .. -DCMAKE_INSTALL_PREFIX=../
    make
    make install

## Authors

* **Petr Vitovský** - [petrv7](https://github.com/petrv7)
