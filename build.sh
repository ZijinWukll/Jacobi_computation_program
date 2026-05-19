#!/bin/bash
set -e

# MSYS2 64-bit 编译器
CXX="/c/msys64/mingw64/bin/g++.exe"
CXXFLAGS="-std=c++17 -O3 -march=native -flto -Wall -Wextra -static -Iinclude"

export PATH="/c/msys64/mingw64/bin:$PATH"

case "${1:-all}" in
    test)
        echo "Compiling BigInt tests..."
        $CXX $CXXFLAGS -o test_bigint.exe test/test_bigint.cpp src/bigint.cpp
        echo "Running BigInt tests..."
        ./test_bigint.exe

        echo ""
        echo "Compiling Jacobi tests..."
        $CXX $CXXFLAGS -o test_jacobi.exe test/test_jacobi.cpp src/bigint.cpp src/jacobi.cpp
        echo "Running Jacobi tests..."
        ./test_jacobi.exe
        echo "All tests passed!"
        ;;

    main)
        echo "Compiling jacobi_main..."
        $CXX $CXXFLAGS -o jacobi_main.exe src/main.cpp src/bigint.cpp src/jacobi.cpp
        echo "Done: jacobi_main.exe"
        ;;

    all)
        bash "$0" main
        bash "$0" test
        ;;

    *)
        echo "Usage: ./build.sh {test|main|all}"
        echo "  test  — 编译并运行单元测试"
        echo "  main  — 编译主程序 jacobi_main.exe"
        echo "  all   — 编译主程序 + 运行测试"
        ;;
esac
