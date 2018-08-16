# AutoPriv
**AutoPriv** is an LLVM-based compiler that tranforms programs to drop unneeded privileges.
The source code of **AutoPriv** is located in `lib/Transforms/PrivAnalysis`.

## How to compile
Before you compile, please download `clang 3.7.0` and copy it to `tools/clang` 
in this LLVM source tree.

1. Make a build directory and go into the build directory.
2. Run the `configure` file in this repository in the build directory. If you
use `clang` and `clang++` to compile and run into compilation erros, please 
switch to `gcc` and `g++`. You can specify them when run the `configure` file
like `../AutoPriv/configure CC=gcc CXX=g++`.
3. In the build directory, run `make ENABLE_OPTIMIZED=1 DISABLE_ASSERTIONS=1`.
You can add `-jn` to compile the source with multiple threads in parallel.

## How to run
Before you compile programs with **AutoPriv**, refactor the programs with the 
privimitives defined in `PrivLib/priv.c`. To be more specific, put a pair of 
`priv_raise()` and `priv_lower()` around any system calls or library function
calls that use some privilege(s).

First, compile the target program to LLVM bitcode; then run opt with library
`LLVMDataStructure.so`, `AssistDS.so` and `LLVMPrivAnalysis.so` loaded, which are all
located in `you_build_directory/lib`, and specify `-PrivRemoveInsert`. It's also
recommended to run the `internalize` and `simplifycfg` pass. Here is an exmaple,
`$(OPT) -load $(DSALIB) -load $(ASSISTDSLIB) -load $(OPTLIB)  $(INTERNOPT) -PrivRemoveInsert -simplifycfg program.bc -o program.opt.bc`. After you get the `.opt.bc` file, run `clang` with
`-lcap -lstdc++` and the priv-lib (e.g., `$(CLANG) program.opt.bc priv.c -o program -lcap -lstdc++`.
Last, give the final executable the capabilities it needs by running `setcap`.

If you have any questions about compiling and running **AutoPriv** and related
test programs, please contact Jie Zhou: jzhou41@cs.rochester.edu.

Have a nice privilege dropping. :-)

