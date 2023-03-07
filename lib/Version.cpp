/* Copyright 2017-2018 Politecnico di Milano.
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
#include "versioningCompiler/Version.hpp"

#include <cstdio>
#include <dlfcn.h>

using namespace vc;

// ----------------------------------------------------------------------------
// ----------------------- zero-parameters constructor ------------------------
// ----------------------------------------------------------------------------
Version::Version()
{
  autoremoveFilesEnable = true;
  functionName = {};
  fileName_src = {};
  fileName_IR = "";
  fileName_bin = "";
  symbol = {};
  tags = {};
  lib_handle = nullptr;
  uuid_t uuid;
  char tmp[128];
  uuid_generate(uuid);
  uuid_unparse(uuid, tmp);
  id = std::string(tmp);
}

// ----------------------------------------------------------------------------
// ---------------------------- default destructor ----------------------------
// ----------------------------------------------------------------------------
Version::~Version()
{
  if (lib_handle) {
    compiler->releaseSymbol(&lib_handle); // close the shared object
  }
  symbol.clear(); // invalide symbols
  if (autoremoveFilesEnable) {
    removeFile(fileName_bin);
    removeFile(fileName_IR_opt);
    removeFile(fileName_IR);
  }
}

// ----------------------------------------------------------------------------
// ---------------------------------- get ID ----------------------------------
// ----------------------------------------------------------------------------
std::string Version::getID() const
{
  return id;
}

// ----------------------------------------------------------------------------
// --------------------------------- get Tag ----------------------------------
// ----------------------------------------------------------------------------
std::vector<std::string> Version::getTags() const
{
  return tags;
}

// ----------------------------------------------------------------------------
// --------------------- has generated intermediate file ----------------------
// ----------------------------------------------------------------------------
bool Version::hasGeneratedIR() const
{
  return (!fileName_IR.empty());
}

// ----------------------------------------------------------------------------
// ----------------------- has generated optimized file -----------------------
// ----------------------------------------------------------------------------
bool Version::hasOptimizedIR() const
{
  return (!fileName_IR_opt.empty());
}

// ----------------------------------------------------------------------------
// ------------------------ has generated binary file -------------------------
// ----------------------------------------------------------------------------
bool Version::hasGeneratedBin() const
{
  return (!fileName_bin.empty());
}

// ----------------------------------------------------------------------------
// ----------------------- has loaded function pointer ------------------------
// ----------------------------------------------------------------------------
bool Version::hasLoadedSymbol() const
{
  return (!symbol.empty());
}

// ----------------------------------------------------------------------------
// -------------------------- load function pointer ---------------------------
// ----------------------------------------------------------------------------
void Version::loadSymbol()
{
  if (symbol.empty() && hasGeneratedBin()) {
    symbol = compiler->loadSymbols(fileName_bin, functionName, &lib_handle);
  }
  return;
}

// ----------------------------------------------------------------------------
// ----------------------------------- fold -----------------------------------
// ----------------------------------------------------------------------------
void Version::fold()
{
  if (lib_handle) {
    compiler->releaseSymbol(&lib_handle);
    symbol.clear();
  }
  return;
}

// ----------------------------------------------------------------------------
// ---------------------------------- reload ----------------------------------
// ----------------------------------------------------------------------------
void *Version::reload()
{
  if (lib_handle) {
    fold();
  }
  symbol.clear();
  loadSymbol();
  return getSymbol();
}

// ----------------------------------------------------------------------------
// --------------------------- get function pointer ---------------------------
// ----------------------------------------------------------------------------
void *Version::getSymbol() const
{
  return symbol.at(0);
}

// ----------------------------------------------------------------------------
// --------------------------- get function pointer ---------------------------
// ----------------------------------------------------------------------------
void *Version::getSymbol(const int index) const
{
  return symbol.at(index);
}

// ----------------------------------------------------------------------------
// --------------------------- get function pointer ---------------------------
// ----------------------------------------------------------------------------
std::vector<void *> Version::getSymbols() const
{
  return symbol;
}

// ----------------------------------------------------------------------------
// --------------------------- get function pointer ---------------------------
// ----------------------------------------------------------------------------
void *Version::getSymbol(const std::string& functionName) const
{
  using map_finder_t = decltype(mapFnToIndex)::const_iterator;
  map_finder_t it = mapFnToIndex.find(functionName);
  if (it != mapFnToIndex.end())
  {
    return getSymbol(it->second);
  }
  return nullptr;
}

// ----------------------------------------------------------------------------
// ----------------- prepare intermediate and optimized file ------------------
// ----------------------------------------------------------------------------
bool Version::prepareIR()
{
  if (! compiler->hasIRSupport()) {
    return false;
  }
  fileName_IR = compiler->generateIR(fileName_src, functionName, id,
                                     genIRoptionList);
  if (! hasGeneratedIR()) {
    return false;
  }
  if (compiler->hasOptimizer()) {
    fileName_IR_opt = compiler->runOptimizer(fileName_IR, id, optOptionList);
    return hasOptimizedIR();
  }
  return true;
}

// ----------------------------------------------------------------------------
// -------------------------- compile to binary file --------------------------
// ----------------------------------------------------------------------------
bool Version::compile()
{
  if (hasGeneratedBin() && hasLoadedSymbol()) {
    return true;
  }
  if (! hasGeneratedBin()) {
    std::vector<std::filesystem::path> src;
    if (!fileName_IR_opt.empty()) {
      src.clear();
      src.push_back(fileName_IR_opt);
    } else if (!fileName_IR.empty()) {
      src.clear();
      src.push_back(fileName_IR);
    } else {
      src = fileName_src;
    }
    fileName_bin = compiler->generateBin(src, functionName, id, optionList);
  }
  loadSymbol();
  return hasLoadedSymbol();
}

// ----------------------------------------------------------------------------
// ----------------- get ordered list of compilation options ------------------
// ----------------------------------------------------------------------------
const opt_list_t Version::getOptionList() const
{
  return optionList;
}

// ----------------------------------------------------------------------------
// ----------- get ordered list of intermediate generation options ------------
// ----------------------------------------------------------------------------
const opt_list_t Version::getGenIRoptionList() const
{
  return genIRoptionList;
}

// ----------------------------------------------------------------------------
// ------------------ get ordered list of optimizer options -------------------
// ----------------------------------------------------------------------------
const opt_list_t Version::getOptOptionList() const
{
  return optOptionList;
}

// ----------------------------------------------------------------------------
// ----------------------------- get compiler ID ------------------------------
// ----------------------------------------------------------------------------
std::string Version::getCompilerId() const
{
  return compiler->getId();
}

// ----------------------------------------------------------------------------
// --------------------------- get function name ------------------------------
// ----------------------------------------------------------------------------
std::string Version::getFunctionName() const
{
  return functionName.at(0);
}

// ----------------------------------------------------------------------------
// --------------------------- get function name ------------------------------
// ----------------------------------------------------------------------------
std::string Version::getFunctionName(const int index) const
{
  return functionName.at(index);
}

// ----------------------------------------------------------------------------
// --------------------------- get function name ------------------------------
// ----------------------------------------------------------------------------
std::vector<std::string> Version::getFunctionNames() const
{
  return functionName;
}

// ----------------------------------------------------------------------------
// --------------------------- get source filename ----------------------------
// ----------------------------------------------------------------------------
std::filesystem::path Version::getFileName_src() const
{
  return fileName_src.at(0);
}

// ----------------------------------------------------------------------------
// --------------------------- get source filename ----------------------------
// ----------------------------------------------------------------------------
std::filesystem::path Version::getFileName_src(const int index) const
{
  return fileName_src.at(index);
}

// ----------------------------------------------------------------------------
// --------------------------- get source filename ----------------------------
// ----------------------------------------------------------------------------
std::vector<std::filesystem::path> Version::getFileNames_src() const
{
  return fileName_src;
}

// ----------------------------------------------------------------------------
// ------------------------ get intermediate filename -------------------------
// ----------------------------------------------------------------------------
std::filesystem::path Version::getFileName_IR() const
{
  return fileName_IR;
}

// ----------------------------------------------------------------------------
// ------------------------- get optimized filename ---------------------------
// ----------------------------------------------------------------------------
std::filesystem::path Version::getFileName_IR_opt() const
{
  return fileName_IR_opt;
}

// ----------------------------------------------------------------------------
// --------------------------- get binary filename ----------------------------
// ----------------------------------------------------------------------------
std::filesystem::path Version::getFileName_bin() const
{
  return fileName_bin;
}

// ----------------------------------------------------------------------------
// ------------------------------ remove file ---------------------------------
// ----------------------------------------------------------------------------
bool Version::removeFile(const std::filesystem::path &fileName)
{
  if (fileName.empty()) {
    return true;
  }
  return remove(fileName.c_str()) != 0;
}

// ---------------------------------------------------------------------------
// ----------------------------- VERSION BUILDER -----------------------------
// ---------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// -------------------------- default constructor -----------------------------
// ----------------------------------------------------------------------------
Version::Builder::Builder()
{
  reset();
}

// ----------------------------------------------------------------------------
// ------------------- constructor from a Version object ----------------------
// ----------------------------------------------------------------------------
Version::Builder::Builder(const Version *v)
{
  _functionName = v->functionName;
  _fileName_src = v->fileName_src;
  _fileName_IR = v->fileName_IR;
  _optionList = v->optionList;
  _compiler = v->compiler;
  _genIROptionList = v->genIRoptionList;
  _optOptionList = v->optOptionList;
  _autoremoveFilesEnable = v->autoremoveFilesEnable;
}

// ----------------------------------------------------------------------------
// -------------------- proxy for the above contrusctor -----------------------
// ----------------------------------------------------------------------------
Version::Builder::Builder(const version_ptr_t v)
  : Builder(v.get()) { }

// ----------------------------------------------------------------------------
// ------------- constructor populating only mandatory parameters -------------
// ----------------------------------------------------------------------------
Version::Builder::Builder(const std::filesystem::path &fileName,
                          const std::string &functionName,
                          const compiler_ptr_t &compiler)
{
  _fileName_src.push_back(fileName);
  _functionName.push_back(functionName);
  _compiler = compiler;
}

// ----------------------------------------------------------------------------
// ------------- constructor populating only mandatory parameters -------------
// ----------------------------------------------------------------------------
Version::Builder::Builder(const std::vector<std::filesystem::path> &fileNames,
                          const std::vector<std::string> &functionNames,
                          const compiler_ptr_t &compiler)
{
  _fileName_src = fileNames;
  _functionName = functionNames;
  _compiler = compiler;
}

// ----------------------------------------------------------------------------
// ---------------------- Version object finalization -------------------------
// ----------------------------------------------------------------------------
version_ptr_t Version::Builder::build()
{
  _version_ptr = version_ptr_t(new Version());
  _version_ptr->tags = _tags;
  _version_ptr->functionName = _functionName;
  for (int i = 0; i < _functionName.size(); i++) { // build reverse index
    _version_ptr->mapFnToIndex[_functionName[i]] = i;
  }
  _version_ptr->fileName_src = _fileName_src;
  _version_ptr->fileName_IR = _fileName_IR;
  _version_ptr->compiler = _compiler;
  _version_ptr->optionList = _optionList;
  _version_ptr->genIRoptionList = _genIROptionList;
  _version_ptr->optOptionList = _optOptionList;
  _version_ptr->fileName_IR_opt = "";
  _version_ptr->autoremoveFilesEnable = _autoremoveFilesEnable;
  for (const auto &flag : _flagDefineList) {
    if (flag != "") {
      const Option flag_opt = getFunctionFlag(flag);
      _version_ptr->optionList.push_front(flag_opt);
      _version_ptr->genIRoptionList.push_front(flag_opt);
    }
  }
  return _version_ptr;
}

// ----------------------------------------------------------------------------
// --------------------------------- reset ------------------------------------
// ----------------------------------------------------------------------------
void Version::Builder::reset()
{
  _tags = {};
  _version_ptr = nullptr;
  _compiler = nullptr;
  _functionName.clear();
  _fileName_src.clear();
  _fileName_IR = "";
  _optionList.clear();
  _genIROptionList.clear();
  _optOptionList.clear();
  _flagDefineList.clear();
  _autoremoveFilesEnable = true;
  return;
}

// ----------------------------------------------------------------------------
// ----------------------------- add source file ------------------------------
// ----------------------------------------------------------------------------
void Version::Builder::addSourceFile(const std::filesystem::path &src)
{
  _fileName_src.push_back(src);
  return;
}

// ----------------------------------------------------------------------------
// ------------------------------- add metadata -------------------------------
// ----------------------------------------------------------------------------
void Version::Builder::addTag(const std::string &tag)
{
  _tags.push_back(tag);
  return;
}

// ----------------------------------------------------------------------------
// --------------------------- add include directory --------------------------
// ----------------------------------------------------------------------------
void Version::Builder::addIncludeDir(const std::filesystem::path &path)
{
  _genIROptionList.push_back(Option("includeDir", "-I", path.string()));
  _optionList.push_back(Option("includeDir", "-I", path.string()));
  return;
}

// ----------------------------------------------------------------------------
// ---------------------------- add link directory ----------------------------
// ----------------------------------------------------------------------------
void Version::Builder::addLinkingDir(const std::filesystem::path &path)
{
  _optionList.push_back(Option("linkDir", "-L", path.string()));
  return;
}

// ----------------------------------------------------------------------------
// ------------------------ set compilation options ---------------------------
// ----------------------------------------------------------------------------
void Version::Builder::options(const opt_list_t options)
{
  _optionList = options;
  return;
}

// ----------------------------------------------------------------------------
// ------------------ set intermediate generation options ---------------------
// ----------------------------------------------------------------------------
void Version::Builder::genIRoptions(const opt_list_t options)
{
  _genIROptionList = options;
  return;
}

// ----------------------------------------------------------------------------
// ------------------------- set optimizer options ----------------------------
// ----------------------------------------------------------------------------
void Version::Builder::optOptions(const opt_list_t options)
{
  _optOptionList = options;
  return;
}

// ----------------------------------------------------------------------------
// ----------------------- remove compilation option --------------------------
// ----------------------------------------------------------------------------
void Version::Builder::removeOption(const std::string &tag)
{
  auto check = [tag](const Option & _o) -> bool {return _o.getTag() == tag;};
  _optionList.remove_if(check);
  return;
}

// ----------------------------------------------------------------------------
// ------------------------ remove optimizer option ---------------------------
// ----------------------------------------------------------------------------
void Version::Builder::removeOptOption(const std::string &tag)
{
  auto check = [tag](const Option & _o) -> bool {return _o.getTag() == tag;};
  _optOptionList.remove_if(check);
  return;
}

// ----------------------------------------------------------------------------
// ------------------ remove intermediate generation option -------------------
// ----------------------------------------------------------------------------
void Version::Builder::removeGenIROption(const std::string &tag)
{
  auto check = [tag](const Option & _o) -> bool {return _o.getTag() == tag;};
  _genIROptionList.remove_if(check);
  return;
}

// ----------------------------------------------------------------------------
// ---------------------------- add function flag -----------------------------
// ----------------------------------------------------------------------------
void Version::Builder::addFunctionFlag(const std::string &flag)
{
  if (flag != "") {
    _flagDefineList.push_front(flag);
  }
  return;
}

// ----------------------------------------------------------------------------
// --------------- create version from shared object file name ----------------
// ----------------------------------------------------------------------------
version_ptr_t Version::Builder::createFromSO(const std::filesystem::path &sharedObject,
                                             const std::string &functionName,
                                             const compiler_ptr_t &compiler,
                                             bool autoremoveFilesEnable,
                                             const std::vector<std::string> &tags)
{
  version_ptr_t v = version_ptr_t(new Version());
  v->fileName_bin = sharedObject;
  v->autoremoveFilesEnable = autoremoveFilesEnable;
  v->functionName.push_back(functionName);
  v->compiler = compiler;
  v->tags = tags;
  v->compile();
  return v;
}

// ----------------------------------------------------------------------------
// --------------- create version from shared object file name ----------------
// ----------------------------------------------------------------------------
version_ptr_t Version::Builder::createFromSO(const std::filesystem::path &sharedObject,
                                             const std::vector<std::string> &functionNames,
                                             const compiler_ptr_t &compiler,
                                             bool autoremoveFilesEnable,
                                             const std::vector<std::string> &tags)
{
  version_ptr_t v = version_ptr_t(new Version());
  v->fileName_bin = sharedObject;
  v->autoremoveFilesEnable = autoremoveFilesEnable;
  v->functionName = functionNames;
  v->compiler = compiler;
  v->tags = tags;
  v->compile();
  return v;
}

// ----------------------------------------------------------------------------
// ---------------------------- get function flag -----------------------------
// ----------------------------------------------------------------------------
Option Version::Builder::getFunctionFlag(const std::string &flagName)
{
  const Option opt("enable_define", "-D", flagName);
  return opt;
}
