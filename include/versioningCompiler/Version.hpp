/* Copyright 2017 Politecnico di Milano.
 * Developed by : Stefano Cherubin
 *                PhD student, Politecnico di Milano
 *                <first_name>.<family_name>@polimi.it
 *
 * This file is part of libVersioningCompiler
 *
 * libVersioningCompiler is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation, either version 3
 * of the License, or (at your option) any later version.
 *
 * libVersioningCompiler is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libVersioningCompiler. If not, see <http://www.gnu.org/licenses/>
 */
#ifndef LIB_VERSIONING_COMPILER_VERSION_HPP
#define LIB_VERSIONING_COMPILER_VERSION_HPP

#include "versioningCompiler/Compiler.hpp"
#include "versioningCompiler/Option.hpp"

#include <list>
#include <memory>
#include <string>
#include <uuid/uuid.h>

namespace vc {

/** \brief A Version object represents a configuration setup used to compile
 * a function.
 * It also holds references to intermediate and compiled files.
 *
 * Configuration setup is composed by:
 * - source file name
 * - target function name
 * - ordered list of intermediate file generation options
 * - ordered list of optimizer options
 * - ordered list of compilation options
 * - compiler and optimizer
 *
 * Configuration setup is lifetime immutable.
 *
 * A Version object can be configured only through a Version::Builder.
 */
class Version
{
 public:
  class Builder;

  /** \brief String representation of the Version unique identifier. */
  std::string getID() const;

  /** \brief User defined string description of the Version. */
  std::string getTag() const;

  /** \brief Return true if an IR representation of this Version is available.
   * False otherwise.
   */
  bool hasGeneratedIR() const;

  /** \brief Return true if an optimized IR representation of this Version is
   * available. False otherwise.
   */
  bool hasOptimizedIR() const;

  /** \brief Return true if the binary code of this Version is available.
   * False otherwise.
   */
  bool hasGeneratedBin() const;

  /** \brief Return true if the symbol was correctly loaded and
   * it is currently available.
   * False otherwise.
   */
  bool hasLoadedSymbol() const;

  /** \brief Return the symbol, if was correctly loaded.
   * nullptr otherwise.
   *
   * Please note that this symbol will stay valid only as long as the Version
   * object is still alive.
   * Closing the associated binary shared object will invalide this pointer.
   */
  void *getSymbol() const;

  /** \brief Closes the shared object to save memory resources.
   *
   * After folding a Version it is not possible to access its symbol before
   * reloading it.
   */
  void fold();

  /** \brief Reloads a shared object after a fold. If the Version is not
   * folded, this method will fold it and than load the function pointer again.
   *
   * \return the reloaded function pointer. nullptr on failure.
   */
  void *reload();

  /** \brief Generate the LLVM-IR code of the function.
   *
   * The compiler is invoked only if a LLVM-IR file is not yet available.
   *
   * \return true if a LLVM-IR file was made available. False elsewhere.
   */
  bool prepareIR();

  /** \brief Generate the binary code of the function and load it.
   *
   * The compiler is invoked only if a binary file is not yet available.
   *
   * \return true if a symbol was made available. False elsewhere.
   */
  bool compile();

  /** \brief ordered list of options used to build this version. */
  const opt_list_t getOptionList() const;

  /** \brief ordered list of options used to generate the IR for this version.
   */
  const opt_list_t getGenIRoptionList() const;

  /** \brief ordered list of options used to run the optimizer on this version.
   */
  const opt_list_t getOptOptionList() const;

  /** \brief Compiler used to compile this Version. */
  std::string getCompilerId() const;

  /** \brief name of the versioned function. */
  std::string getFunctionName() const;

  /** \brief file name where the source code, if available, is stored. */
  std::string getFileName_src() const;

  /** \brief file name where the IR, if available, is stored. */
  std::string getFileName_IR() const;

  /** \brief file name where the optimized IR, if available, is stored. */
  std::string getFileName_IR_opt() const;

  /** \brief file name where the binary, if available, is stored. */
  std::string getFileName_bin() const;

  inline bool operator== (const Version& other) {
    return getID() == other.getID();
  }

  inline bool operator< (const Version& other) {
    return getID() < other.getID();
  }

  /** default destructor*/
  ~Version();

 private:
  /** Version constructor is supposed to be called only from the
   * Version::Builder::build() method
   */
  Version();

  /** \brief unique identifier. */
  std::string id;

  /** \brief User-defined description. */
  std::string tag;

  /** \brief Remove files when the object is deallocated. */
  bool autoremoveFilesEnable;

  /** \brief ordered list of options used to build this version. */
  opt_list_t optionList;

  /** \brief ordered list of options used to generate the IR for this version.
   */
  opt_list_t genIRoptionList;

  /** \brief ordered list of options used to run the optimizer on this version.
   */
  opt_list_t optOptionList;

  /** \brief Compiler used to compile this Version. */
  compiler_ptr_t compiler;

  /** \brief name of the versioned function. */
  std::string functionName;

  /** \brief file name where the source code, if available, is stored. */
  std::string fileName_src;

  /** \brief file name where the IR, if available, is stored. */
  std::string fileName_IR;

  /** \brief file name where the optimized IR, if available, is stored. */
  std::string fileName_IR_opt;

  /** \brief file name where the binary, if available, is stored. */
  std::string fileName_bin;

  /** \brief Loaded symbol, if available. */
  void *symbol;

  void *lib_handle;

  bool removeFile(const std::string &fileName);

  /** \brief Loads function pointer symbol from the shared object.
   * Shared object must already exists.
   */
  void loadSymbol();
};

typedef std::shared_ptr<Version> version_ptr_t;

/** \brief Version::Builder is used to configure a new version object.
 *
 * Once configuration is done, builder can finalize a Version object through
 * Version::Builder::build() method.
 */
class Version::Builder
{
 public:
  /** \brief constructs a Builder by cloning an existing Version. */
  Builder(const Version *v);

  /** \brief constructs a Builder by cloning an existing Version. */
  Builder(const version_ptr_t v);

  /** \brief constructs a Builder and populate the mandatory parameters. */
  Builder(const std::string &fileName,
          const std::string &functionName,
          const compiler_ptr_t &compiler);

  /** \brief default constructor. */
  Builder();

  /** \brief construct a Version using an already existing shared object. */
  static version_ptr_t createFromSO(const std::string &sharedObject,
                                    const std::string &functionName,
                                    const compiler_ptr_t &compiler,
                                    const bool autoremoveFilesEnable = true,
                                    const std::string &tag = "");

  /** \brief actually create an immutable object Version. */
  version_ptr_t build();

  /** \brief Reset builder to its default values. */
  void reset();

  /** \brief Remove from the option list all options with a given tag. */
  void removeOption(const std::string &tag);

  /** \brief Remove from optimizer Option list all options with a given tag.
   */
  void removeOptOption(const std::string &tag);

  /** \brief Remove from gen_IR Option list all options with a given tag.
   */
  void removeGenIROption(const std::string &tag);

  /** \brief Reset compilation Option list to a new list. */
  void options(const opt_list_t options);

  /** \brief Reset gen_IR Option list to a new list. */
  void genIRoptions(const opt_list_t options);

  /** \brief Reset optimizer Option list to a new list. */
  void optOptions(const opt_list_t options);

  /** \brief Insert a define in the compilation stages to enable the
   * compilation of the given function.
   * By default it is the name of the function, uppercase.
   */
  void addFunctionFlag(const std::string &flag = "");

  /** \brief User defined tag to describe the version. */
  std::string _tag;

  /** \brief Remove compiled files from disk when Version object is freed. */
  bool _autoremoveFilesEnable;

  /** \brief Compiler to be used to compile this Version. */
  compiler_ptr_t _compiler;

  /** \brief name of the versioned function. */
  std::string _functionName;

  /** \brief Defines that should be enabled to compile the given function. */
  std::list<std::string> _flagDefineList;

  /** \brief file name where the source code, if available, is stored. */
  std::string _fileName_src;

  /** \brief file name where the IR, if available, is stored. */
  std::string _fileName_IR;

  /** \brief ordered list of options to be used to build this version. */
  opt_list_t _optionList;

  /** \brief ordered list of options to be used to generate the IR. */
  opt_list_t _genIROptionList;

  /** \brief ordered list of options to be used to build this version. */
  opt_list_t _optOptionList;

 private:
  /** \brief shared pointer to the object to be built. */
  version_ptr_t _version_ptr;

  /** \brief Returns a flag to be enabled in order to compile the given function.
   *
   * That define depends on the source code.
   */
  static Option getFunctionFlag(const std::string &flagName);

};

} // end namespace vc

namespace std {
  template<>
  struct hash<vc::Version>
  {
    std::size_t operator()( const vc::Version& key ) {
      return hash<std::string>()(key.getID());
    }
  };
} // end namespace std

#endif /* end of include guard: LIB_VERSIONING_COMPILER_VERSION_HPP */
