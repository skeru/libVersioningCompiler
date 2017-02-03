# libVersioningCompiler

This library enables runtime compilation of source code and dynamic loading of
a specified C/C++ function.
It also provides support for versioning of the compiled functions.

This software is distributed under the LGPLv3 license.
See LICENSE.txt and LICENSE.LESSER.txt for the full text of the license.

Part of this software include third-party source code.
See additional license file(s) for those parts of this software
(LLVM_LICENSE.txt)

## Dependencies
libVersioningCompiler requires:
 - Ubuntu 16.04 or greater
 - any compiler compliant to the C++11 standard
 - cmake 3.0.2 or greater

external libraries:
 - dl
 - uuid-dev
 - (OPTIONAL) llvm-4.0-dev
 - (OPTIONAL) libclang-4.0-dev

Compiling without the OPTIONAL dependencies will disable some features,
like the Clang-as-a-library compiler implementation.

## How to install

libVersioningCompiler uses the cmake build system.
If you want to install libVersioningCompiler

clone the repository to `${LIBVC_ROOT}`

```
cd ${LIBVC_ROOT}
mkdir build
cd build
cmake .. [-DCMAKE_INSTALL_PREFIX="/path/to/your/custom/install/folder/"]
make
[sudo] make install
```

Please note that if you choose to install libVersioningCompiler in a custom
folder, you will need the FindLibVersioningCompiler.cmake module.
You will find a pre-cooked cmake module in `${LIBVC_ROOT}/config`.

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
aggregating the most common sequence of function calls.
All basic functionalities are summarized within 3 functions.
 - Initialize
 - Create Version object
 - Compile a Version object and load function symbol

## How to use it

### Common
Disregarding the APIs that are going to be exploited, these modifications
should be applied.

#### For each function to be compiled/versioned
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
compiled into a version from the code around it.
By default low-level APIs do not add any extra symbol definition during the
compilation; any extra definition must be manually specified.
High-level APIs include by default a definition of the uppercase name of the
compiled function. Every statement in the source file can be selectively
enabled or disabled by using
`#ifdef uppercaseFunctionName` | `#ifndef uppercaseFunctionName`.

### Low-level APIs
#### Include
`#include "versioningCompiler/Version.hpp"`

According to the chosen Compiler implementation, the corresponding include
should be added. E.g.

`#include "versioningCompiler/SystemCompilerOptimizer.hpp"`

#### Once on setup
Instantiate and configure all compilers that are going to be used.
```
// default system compiler
std::shared_ptr<vc::Compiler> cc = std::make_shared<vc::SystemCompiler>();

// /usr/bin/gcc using ./test.log as log file
std::shared_ptr<vc::Compiler> gcc = std::make_shared<vc::SystemCompiler>(
                                        "gcc",
                                        "gcc",
                                        ".",
                                        "./test.log",
                                        "/usr/bin",
                                        false
                                    );

// custom installation of LLVM/clang
// /clang-3.7.0/bin/clang as compiler and /clang-3.7.0/bin/opt as optimizer
std::shared_ptr<vc::Compiler> clang = std::make_shared<vc::SystemCompilerOptimizer>(
                                            "llvm/clang",
                                            "clang",
                                            "opt",
                                            ".",
                                            "./test.log",
                                            "/clang-3.7.0/bin",
                                            "/clang-3.7.0/bin"
                                          );
```

#### Configuring a Version object
Changing the configuration of a Version is impossible after its creation.
The configuration is done using a builder instance. Once a builder is
correctly configured, it is possible to take a snapshot of it by creating a
Version. Builder::build() will create a Version.
```
vc::Version::Builder builder;               // MANDATORY instantiate Builder
builder._functionName = FUNCTION_NAME;      // MANDATORY set function name
builder._fileName_src = FILENAME_SRC;       // MANDATORY set source file
builder._compiler = clang;  // MANDATORY set clang as compiler for the version
builder.options({                           // set compilation option list
                 vc::Option("o", "-O", "2") // add "-O2"
                });
builder._optOptionList = {                  // set optimizer option list
                          vc::Option("fp-contract", "-fp-contract=", "fast"),
                          vc::Option("inline", "-inline"),
                          vc::Option("unroll", "-loop-unroll"),
                          vc::Option("mem2reg", "-mem2reg")
                         };
std::shared_ptr<vc::Version> v = builder.build(); // MANDATORY finalize Version
```
After a Version finalization, the builder can be modified and reused to build
another Version or it can be destroyed.
Version objects constructed from the same builder will not be affected from
any kind of change to the Builder object.
It is also possible to clone a Version in a new Builder to initialize it.
```
// reuse builder from v
builder._compiler = cc;
std::shared_ptr<vc::Version> v2 = builder.build();

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
  fn_ptr = (signature_t) v->getSymbol();
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
High-level APIs are simple functions that wraps the low-level APIs.
They rely on builder and compiler static objects.
They apply the most common values to the low-level parameters.
Please consider to switch to Low-level APIs for a more fine-grained approach.

#### Include
`#include "versioningCompiler/Utils.hpp"`

#### Once on setup
` vc_utils_init();`

#### For every desired Version object
```
 std::shared_ptr<Version> v = vc::createVersion(FILENAME_SRC,
                                                FUNCTION_NAME,
                                                {Option("O", "-O", "2"), ... }
                                               );
 signature_t fn_ptr = (signature_t) vc::compileAndGetSymbol(v);
 if (fn_ptr) {  // check if correctly compiled and loaded symbol
   fn_ptr(42);  // run the compiled function version
 }
```
