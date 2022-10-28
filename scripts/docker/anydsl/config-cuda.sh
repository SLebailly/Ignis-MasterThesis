# use Debug or Release
: ${BUILD_TYPE:=Release}

#: ${CMAKE_MAKE:=""}
#: ${MAKE:="make -j4"}

# use this for ninja instead of make
: ${CMAKE_MAKE:="-G Ninja -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DLLVM_PARALLEL_COMPILE_JOBS=8 -DLLVM_PARALLEL_LINK_JOBS=2"}
: ${MAKE:="ninja"}

# set this to true if you don't have a github account
: ${HTTPS:=true}

# set this to true if you want to download and build the required version of CMake
: ${CMAKE:=false}
: ${BRANCH_CMAKE:=v3.13.4}

# set this to false if you don't want to build with LLVM
# setting to false is meant to speed up debugging and not recommended for end users
: ${LLVM:=true}
: ${LLVM_TARGETS:="NVPTX;X86"}
: ${LLVM_GIT:=false}
: ${LLVM_GIT_REPO:=llvm}
: ${LLVM_GIT_BRANCH:=release/14.x}
: ${LLVM_SRC_VERSION:=14.0.6}

# set this to false if you don't want to build LLVM with RV support
: ${RV:=true}

# use this to debug thorin hash table performance
: ${THORIN_PROFILE:=false}

# set this to false if you don't build with LLVM
: ${RUNTIME_JIT:=true}

# set this to false to hide runtime debug output
: ${RUNTIME_DEBUG_OUTPUT:=false}

# set the default branches for each repository
: ${BRANCH_ARTIC:=master}
: ${BRANCH_IMPALA:=master}
: ${BRANCH_THORIN:=master}
: ${BRANCH_RV:=release/14.x}
: ${BRANCH_RUNTIME:=master}
: ${BRANCH_STINCILLA:=main}
: ${BRANCH_RODENT:=master}
: ${CLONE_RODENT:=false}
