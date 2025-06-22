# libVersioningCompiler

This library enables runtime compilation of source code and dynamic loading of
a specified C/C++ function.
It also provides support for versioning of the compiled functions.

This software is distributed under the LGPLv3 license.
See [LICENSE.txt](LICENSE.txt) and [LICENSE.LESSER.txt](LICENSE.LESSER.txt) for full license details.

An introductory video is available on [YouTube](https://www.youtube.com/watch?v=1p8IajxOgoY).

## Dependencies

To build and use libVersioningCompiler, the following are required:

- Operating System:
  - Ubuntu 20.04 LTS or newer
  - Arch Linux, Manjaro or Fedora
  - macOS (tested on Sonoma)

- Build Tools & Libraries:
  - A C++17-compliant compiler
  - CMake ≥ 3.20
  - zlib
  - libuuid

- Optional: LLVM ≥ 15 (tested up to LLVM 20)

Compiling without LLVM certain features (e.g., Clang-as-a-library) will be unavailable.

## How to install the dependencies

### Ubuntu

For Ubuntu Noble, add the Kitware repository:

```bash
sudo apt-get install --yes ca-certificates gnupg software-properties-common wget
wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | sudo tee /etc/apt/trusted.gpg.d/kitware.gpg >/dev/null
sudo apt-add-repository 'deb https://apt.kitware.com/ubuntu/ noble main'
```

Install CMake via Snap and the dependencies:

```bash
sudo snap install cmake --classic
sudo apt-get install --yes build-essential zlib1g-dev uuid-dev
```

For Ubuntu Focal the procedure is the same, however, change `noble` to `focal` and Snap is not needed anymore.


#### Optional: Install LLVM and Clang

Install [LLVM](https://llvm.org/) 15 or greater (tested up to LLVM 20) and libclang (if needed, on many distributions it come bundled with LLVM/Clang), either using [LLVM installer](https://apt.llvm.org/#llvmsh) or via [Ubuntu packages](https://packages.ubuntu.com/search?keywords=llvm).

To install via the installer (replace `${LLVM_V}` with the version number):

```bash
wget https://apt.llvm.org/llvm.sh
chmod +x llvm.sh
[sudo] ./llvm.sh ${LLVM_V} all
```

Eventually, set the preferred LLVM/Clang version:

```bash
[optional][sudo] update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-${LLVM_V} 100
[optional][sudo] update-alternatives --install /usr/bin/clang clang /usr/bin/clang-${LLVM_V} 100
[optional][sudo] update-alternatives --install /usr/bin/opt opt /usr/bin/opt-${LLVM_V} 100
```

### Arch Linux / Manjaro

Install the necessary packages:

```bash
sudo pacman -S cmake llvm zlib [clang]
```

Note: `libuuid` is usually included by default and doesn't require separate installation.

### MacOS

Install [homebrew](https://brew.sh) if not already installed, then:

```bash
brew install cmake ossp-uuid zlib [llvm]
```

Replace `[llvm]` with `llvm` if you wish to install it.

## How to install libVersioningCompiler

If you want to install libVersioningCompiler, install the dependencies, then, assuming cloned the repository in `${LIBVC_ROOT}`, do:

```bash
cd ${LIBVC_ROOT}
cmake -S . -Bbuild [-DCMAKE_INSTALL_PREFIX="/path/to/your/custom/install/folder/"] [-G "make/Ninja/whatever generator"]
cmake --build build
./build/libVC_testUtils &&./build/libVC_test &&./build/libVC_testJit
[sudo] cmake --build build --target install
```

### Using a custom LLVM installation

To build libVersioningCompiler with a custom LLVM installation, follow the same steps as the standard build, but modify the second line of the process using one of the following options:

```bash
# Option 1: Specify the LLVM binaries directory
cmake -S . -B build -DLLVM_TOOLS_BINARY_DIR="/path/to/llvm/bin" \ 
    [-DCMAKE_INSTALL_PREFIX="/path/to/your/custom/install/folder/"] \ 
    [-G "make/Ninja/whatever generator"]

# Option 2: Specify the exact path to llvm-config
cmake -S . -B build -DLLVM_CONFIG_EXECUTABLE="/path/to/llvm/bin/llvm-config" \ 
    [-DCMAKE_INSTALL_PREFIX="/path/to/your/custom/install/folder/"] \ 
    [-G "make/Ninja/whatever generator"]
```

### Integrating libVersioningCompiler into other projects

Please note that if you choose to install libVersioningCompiler in a custom
folder, you will need the FindLibVersioningCompiler.cmake module.
You will find a pre-cooked cmake module in `${LIBVC_ROOT}/config`.

The above-mentioned cmake module will export also cmake variables which are
useful to build an application that uses libVersioningCompiler, including

```
$LIBVC_INCLUDE_DIRS      = Include path for the header files of libVersioningCompiler
$LIBVC_LIBRARIES         = Link these libraries to use libVersioningCompiler
$LIBVC_LIB_DIR           = Extra libraries directories
$HAVE_CLANG_LIB_COMPILER = Set to true if libVersioningCompiler can have Clang as a library enabled
```

If you choose to do not use this cmake module, every time you want to use
libVersioningCompiler in another application you have to manually specify:

- include path for the headers
- `libVersioningCompiler` library file
- `libVersioningCompiler`'s dependencies (see detailed list above)

## How to build libVersioningCompiler

### Linux

Assuming libVersioningCompiler had been cloned into `./libVersioningCompiler` directory:

#### Option 1: Basic Build Using System Compiler

If you want to build libVersioningCompiler using verbose flags and a custom compiler:

```bash
cd libVersioningCompiler
cmake -D CMAKE_VERBOSE_MAKEFILE=1 -D LIBCLANG_FIND_VERBOSE=1 -D LLVM_FIND_VERBOSE=1 -D LLVM_FIND_VERBOSE=1 -D JIT_ENABLE=0 -S . -B build
cmake --build build -v
```

You can optionally set a specific compiler using `CXX=/path/to/your/compiler` or `CC=/path/to/your/compiler`.

#### Option 2: Build with a Custom LLVM Installation

If you want to build using a custom LLVM/Clang installation, specify paths manually:

```bash
cmake -S . -B build \
-D LLVM_FOUND=1 \
-D LLVM_LIBRARY_DIR="/path/to/llvm/lib" \
-D LLVM_INCLUDE_DIR="/path/to/llvm/include" \
-D LLVM_TOOLS_BINARY_DIR="/path/to/llvm/bin" \
-D LLVM_VERSION_MAJOR=<major_version> \
-D LLVM_PACKAGE_VERSION="<full_version>" \
-D ENABLE_JIT=0

cmake --build build -v
```

Notes:

- `LLVM_FOUND=1` means to not look for a system installed LLVM version
- `LLVM_LIBRARY_DIR` specifies where to find LLVM libraries
- `LLVM_INCLUDE_DIR` specifies where to find LLVM headers
- `LLVM_TOOLS_BINARY_DIR` specifies where to find llvm binaries (`llvm-config`, `opt`, etc)
- `LLVM_VERSION_MAJOR`: set to your LLVM's major version (e.g., 15, 17, 20)
- `LLVM_PACKAGE_VERSION`: full version string returned by `llvm-config --version`
- `ENABLE_JIT` option to include or exclude the JIT compiler, currently not ported to LLVM 18 and upwards

Libclang will be found after defining those variables.

### MacOS

Tested with MacOS Monterey:

```bash
cmake -S . -B build \
-D CMAKE_VERBOSE_MAKEFILE=1 -D LIBCLANG_FIND_VERBOSE=1 -D LLVM_FIND_VERBOSE=1 -D LLVM_FIND_VERBOSE=1 \
-D LLVM_FOUND=1 \
-D LLVM_LIBRARY_DIR="/opt/homebrew/opt/llvm/lib" \
-D LLVM_INCLUDE_DIR="/opt/homebrew/opt/llvm/include" \
-D LLVM_TOOLS_BINARY_DIR="/opt/homebrew/opt/llvm/bin" \
-D LLVM_VERSION_MAJOR=<major_version> \
-D LLVM_SHARED_MODE="static" \
-D LLVM_PACKAGE_VERSION="$(/opt/homebrew/opt/llvm/bin/llvm-config --version)" \
-D ENABLE_JIT=0

 cmake --build build -v
```

For full customization, check the CMakeLists.txt and FindLLVM.cmake to explore available flags and module options.

## Compatibility with LLVM versions prior to 15
The project has legacy versions that are compatible with LLVM 13 to 15. Instructions are provided inside the repository. In order to go back to the legacy libVersioningCompiler versions:

```bash
cd ${LIBVC_ROOT}
git checkout b085535f83dc051edeede68905d2eca6f5cdd65c
```
## Essential classes

- Version :
  it represents a specific version of a function (compiled or to be compiled).
  It holds references to the source, intermediate representations, compiled
  files on disk and the dynamically loaded function pointer.
  It also holds the configuration parameters used to create that function.
  It has a unique identifier randomly generated.

- Compiler :
  it represents a compiler instance used to compile a Version.
  This library supports different compilers, they all derive from the Compiler
  abstract class.

- Option :
  it represents an options passed to the compiler.
  It holds a string identifier for the category that can be used to track
  incompatible options (e.g. -O1 -Ofast are both optimization levels).
  The option is internally represented as a couple of strings
  <prefix>,<value> which are usually queued.

## Low-level and High-level APIs

This library comes with two different flavors: low-level and high-level APIs.

Low-level APIs allow full control over the compilation process.
Through these APIs it is possible to incrementally construct a Version by
selecting the configuration parameter by parameter.
It is possible to create clones or partial clones of Version objects.
It is possible to configure and select different compilers.
It is possible to exploit split-compilation techniques.
It is possible to decide the exact moment when the compiler is invoked.
It is possible to debug at fine grain.

High-level APIs allow to reduce the verbosity of some operations by
aggregating the most common sequence of low-level function calls.
All basic functionalities are summarized within 3 functions.

- Initialize
- Create Version object
- Compile a Version object and load function symbol

## How to use it

### Common

Disregarding the APIs that are going to be exploited, these modifications
should be applied.

#### For each function to be compiled/versioned

You have to provide a few additional pieces of information to the library,
such as

- name of the function to be compiled
- name of the source file to be compiled
- signature of the function to be compiled
- ensure the function to be compiled will have C linkage

In the host code.

```
#define FILENAME_SRC "Kernel.cpp"
#define FUNCTION_NAME "myLovedFunction"
typedef int (*signature_t)(int);
```

where `signature_t` is a type definition for a function pointer having the
same signature as the function to be compiled/versioned.

Decoration of the source code of the function declaration:
if not compiled as C source code, C linkage must be enforced.
An example follows

```
#ifdef MYLOVEDFUNCTION  // uppercase function name
extern "C"
#endif
int myLovedFunction(int parameter);
```

The enforcing is applied through the `extern "C"` prefix.
In order to apply this variation only when this source code is compiled with
libVersioningCompiler, it can be wrapped with a define.
This technique can also be applied to isolate the source code that must be
compiled into a Version from the code around it.
By default low-level APIs do not add any extra symbol definition during the
compilation; any extra definition must be manually specified.
High-level APIs include by default a definition of the uppercase name of the
compiled function. Every statement in the source file can be selectively
enabled or disabled by using
`#ifdef uppercaseFunctionName` | `#ifndef uppercaseFunctionName`.

### Low-level APIs

Before proceeding any further, be sure you have understood the Common part,
which must be addressed in both cases of high-level and low-level APIs.

#### Include

`#include "versioningCompiler/Version.hpp"`

In addition -- according to the chosen Compiler implementation --
the corresponding include should be added. E.g.

`#include "versioningCompiler/SystemCompilerOptimizer.hpp"`

#### Once on setup

Instantiate and configure all compilers that are going to be used.
Please take a look at the source code documentation of each `Compiler`
for a more detailed explanation of the constructor parameters.

```
// default system compiler
vc::compiler_ptr_t cc = vc::make_compiler<vc::SystemCompiler>();

// /usr/bin/gcc using ./test.log as log file
vc::compiler_ptr_t gcc = vc::make_compiler<vc::SystemCompiler>(
                                        "gcc",
                                        std::filesystem::u8path("gcc"),
                                        std::filesystem::u8path("."),
                                        std::filesystem::u8path("./test.log"),
                                        std::filesystem::u8path("/usr/bin"),
                                        false
                                        );

// custom installation of LLVM/clang
// Clang as compiler and LLVM opt as optimizer
vc::compiler_ptr_t clang = vc::make_compiler<vc::SystemCompilerOptimizer>(
                                            "llvm/clang",
                                            std::filesystem::u8path("clang"),
                                            std::filesystem::u8path("opt"),
                                            std::filesystem::u8path("."),
                                            std::filesystem::u8path("./test.log"),
                                            std::filesystem::u8path("/usr/bin"), // compiler path
                                            std::filesystem::u8path("/usr/bin")  // optimizer path
                                          );
```

#### Configuring a Version object

Changing the configuration of a Version is impossible after its creation.
The configuration is done using a builder instance. Once a builder is
correctly configured, it is possible to take a snapshot of it by creating a
Version. Builder::build() will create a Version.

```
vc::Version::Builder builder;                    // MANDATORY instantiate Builder
builder._functionName.push_back(FUNCTION_NAME);  // MANDATORY set function name
builder._fileName_src.push_back(FILENAME_SRC);   // MANDATORY set source file
builder._compiler = clang;  // MANDATORY set clang as compiler for the version
builder.options({                                // set compilation option list
                 vc::Option("o", "-O", "2")      // add "-O2"
                });
builder._optOptionList = {                       // set optimizer option list
                          vc::Option("fp-contract", "-fp-contract=", "fast"),
                          vc::Option("inline", "-inline"),
                          vc::Option("unroll", "-loop-unroll"),
                          vc::Option("mem2reg", "-mem2reg")
                         };
vc::version_ptr_t v = builder.build();           // MANDATORY finalize Version
```

After a Version finalization, the builder can be modified and reused to build
another Version or it can be destroyed.
Version objects constructed from the same builder will not be affected from
any kind of change to the Builder object.
It is also possible to clone a Version in a new Builder to initialize it.

```
// reuse builder from v
builder._compiler = cc;
vc::version_ptr_t v2 = builder.build();

// initialize another_builder using v2 configuration
vc::Version::Builder another_builder = vc::Version::Builder(v2);
```

#### Compiling a Version object

Once a Version object is finalized it holds the configuration that will be
later used to compile ad optimize that function. It is not compiled yet.
This allows to control separately the configuration and the compilation tasks.
Given a Version object, it can be compiled by calling the `compile()` method.

```
signature_t fn_ptr;
bool ok = v->compile();
if (! ok) {
  // handle compilation error
} else {
  fn_ptr = (signature_t) v->getSymbol(0);
}
```

#### Split-compilation

If the compiler supports split-compilation, it is possible to generate an
intermediate representation via the optimizer.

```
if (clang->hasIRSupport())  // check if the compiler has IR support
{
  v->prepareIR();           // generate IR and run the optimizer.
  // Options for IR generation and optimization have to be specified
  // during the configuration of the Version object
  v->compile();             // compile the optimized IR and load symbol
}
```

### High-level APIs

Before proceeding any further, be sure you have understood the Common part,
which must be addressed in both cases of high-level and low-level APIs.

High-level APIs are simple functions that wraps the low-level APIs.
They rely on builder and compiler static objects.
They apply the most common values to the low-level parameters.
Please consider to switch to Low-level APIs for a more fine-grained approach.

#### Include

`#include "versioningCompiler/Utils.hpp"`

#### Once, on setup

` vc::vc_utils_init();`

#### For every desired Version object

```
 vc::version_ptr_t v = vc::createVersion(FILENAME_SRC,
                                         FUNCTION_NAME,
                                         {vc::Option("O", "-O", "2"), ... }
                                        );
 signature_t fn_ptr = (signature_t) vc::compileAndGetSymbol(v);
 if (fn_ptr) {  // check if correctly compiled and loaded symbol
   fn_ptr(42);  // run the compiled function version
 }
```
