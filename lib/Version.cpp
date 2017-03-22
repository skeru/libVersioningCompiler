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
#include "versioningCompiler/Version.hpp"
#include <dlfcn.h>
#include <cstdio>

using namespace vc;

// ----------------------------------------------------------------------------
// ----------------------- zero-parameters constructor ------------------------
// ----------------------------------------------------------------------------
Version::Version()
{
  autoremoveFilesEnable = true;
  functionName = "";
  fileName_src = "";
  fileName_IR = "";
  fileName_bin = "";
  symbol = nullptr;
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
    compiler->releaseSymbol(&lib_handle, &symbol);
  }
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
std::string Version::getTag() const
{
  return tag;
}

// ----------------------------------------------------------------------------
// --------------------- has generated intermediate file ----------------------
// ----------------------------------------------------------------------------
bool Version::hasGeneratedIR() const
{
  return (fileName_IR != "");
}

// ----------------------------------------------------------------------------
// ----------------------- has generated optimized file -----------------------
// ----------------------------------------------------------------------------
bool Version::hasOptimizedIR() const
{
  return (fileName_IR_opt != "");
}

// ----------------------------------------------------------------------------
// ------------------------ has generated binary file -------------------------
// ----------------------------------------------------------------------------
bool Version::hasGeneratedBin() const
{
  return (fileName_bin != "");
}

// ----------------------------------------------------------------------------
// ----------------------- has loaded function pointer ------------------------
// ----------------------------------------------------------------------------
bool Version::hasLoadedSymbol() const
{
  return (getSymbol() != nullptr);
}

// ----------------------------------------------------------------------------
// -------------------------- load function pointer ---------------------------
// ----------------------------------------------------------------------------
void Version::loadSymbol()
{
  if (!symbol && hasGeneratedBin()) {
    symbol = compiler->loadSymbol(fileName_bin, functionName, &lib_handle);
  }
  return;
}

// ----------------------------------------------------------------------------
// ----------------------------------- fold -----------------------------------
// ----------------------------------------------------------------------------
void Version::fold()
{
  if (lib_handle) {
    compiler->releaseSymbol(&lib_handle, &symbol);
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
  symbol = nullptr;
  loadSymbol();
  return getSymbol();
}

// ----------------------------------------------------------------------------
// --------------------------- get function pointer ---------------------------
// ----------------------------------------------------------------------------
void *Version::getSymbol() const
{
  return symbol;
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
    std::string src;
    if (fileName_IR_opt != "") {
      src = fileName_IR_opt;
    } else if (fileName_IR != "") {
      src = fileName_IR;
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
const std::list<Option> Version::getOptionList() const
{
  return optionList;
}

// ----------------------------------------------------------------------------
// ----------- get ordered list of intermediate generation options ------------
// ----------------------------------------------------------------------------
const std::list<Option> Version::getGenIRoptionList() const
{
  return genIRoptionList;
}

// ----------------------------------------------------------------------------
// ------------------ get ordered list of optimizer options -------------------
// ----------------------------------------------------------------------------
const std::list<Option> Version::getOptOptionList() const
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
  return functionName;
}

// ----------------------------------------------------------------------------
// --------------------------- get source filename ----------------------------
// ----------------------------------------------------------------------------
std::string Version::getFileName_src() const
{
  return fileName_src;
}

// ----------------------------------------------------------------------------
// ------------------------ get intermediate filename -------------------------
// ----------------------------------------------------------------------------
std::string Version::getFileName_IR() const
{
  return fileName_IR;
}

// ----------------------------------------------------------------------------
// ------------------------- get optimized filename ---------------------------
// ----------------------------------------------------------------------------
std::string Version::getFileName_IR_opt() const
{
  return fileName_IR_opt;
}

// ----------------------------------------------------------------------------
// --------------------------- get binary filename ----------------------------
// ----------------------------------------------------------------------------
std::string Version::getFileName_bin() const
{
  return fileName_bin;
}

// ----------------------------------------------------------------------------
// ------------------------------ remove file ---------------------------------
// ----------------------------------------------------------------------------
bool Version::removeFile(const std::string &fileName)
{
  if (fileName == "") {
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
Version::Builder::Builder(const std::shared_ptr<Version> v)
  : Builder(v.get()) { }

// ----------------------------------------------------------------------------
// ------------- constructor populating only mandatory parameters -------------
// ----------------------------------------------------------------------------
Version::Builder::Builder(const std::string &fileName,
                          const std::string &functionName,
                          const std::shared_ptr<Compiler> &compiler)
{
  _fileName_src = fileName;
  _functionName = functionName,
  _compiler = compiler;
}

// ----------------------------------------------------------------------------
// ---------------------- Version object finalization -------------------------
// ----------------------------------------------------------------------------
std::shared_ptr<Version> Version::Builder::build()
{
  _version_ptr = std::shared_ptr<Version>(new Version());
  _version_ptr->tag = _tag;
  _version_ptr->functionName = _functionName;
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
  _tag = "";
  _compiler = nullptr;
  _functionName = "";
  _fileName_src = "";
  _fileName_IR = "";
  _optionList.clear();
  _genIROptionList.clear();
  _optOptionList.clear();
  _flagDefineList.clear();
  _autoremoveFilesEnable = true;
}

// ----------------------------------------------------------------------------
// ------------------------ set compilation options ---------------------------
// ----------------------------------------------------------------------------
void Version::Builder::options(const std::list<Option> options)
{
  _optionList = options;
}

// ----------------------------------------------------------------------------
// ------------------ set intermediate generation options ---------------------
// ----------------------------------------------------------------------------
void Version::Builder::genIRoptions(const std::list<Option> options)
{
  _genIROptionList = options;
}

// ----------------------------------------------------------------------------
// ------------------------- set optimizer options ----------------------------
// ----------------------------------------------------------------------------
void Version::Builder::optOptions(const std::list<Option> options)
{
  _optOptionList = options;
}

// ----------------------------------------------------------------------------
// ----------------------- remove compilation option --------------------------
// ----------------------------------------------------------------------------
void Version::Builder::removeOption(const std::string &tag)
{
  auto check = [tag](const Option & _o) -> bool {return _o.getTag() == tag;};
  _optionList.remove_if(check);
}

// ----------------------------------------------------------------------------
// ------------------------ remove optimizer option ---------------------------
// ----------------------------------------------------------------------------
void Version::Builder::removeOptOption(const std::string &tag)
{
  auto check = [tag](const Option & _o) -> bool {return _o.getTag() == tag;};
  _optOptionList.remove_if(check);
}

// ----------------------------------------------------------------------------
// ------------------ remove intermediate generation option -------------------
// ----------------------------------------------------------------------------
void Version::Builder::removeGenIROption(const std::string &tag)
{
  auto check = [tag](const Option & _o) -> bool {return _o.getTag() == tag;};
  _genIROptionList.remove_if(check);
}

// ----------------------------------------------------------------------------
// ---------------------------- add function flag -----------------------------
// ----------------------------------------------------------------------------
void Version::Builder::addFunctionFlag(const std::string &flag)
{
  std::string tmp = flag;
  if (tmp == "") {
    tmp = _functionName;
    for (auto &c : tmp) {
      c = toupper(c);
    }
  }
  if (tmp != "") {
    _flagDefineList.push_front(tmp);
  }
  return;
}

// ----------------------------------------------------------------------------
// --------------- create version from shared object file name ----------------
// ----------------------------------------------------------------------------
std::shared_ptr<Version>
Version::Builder::createFromSO(const std::string &sharedObject,
                               const std::string &functionName,
                               const std::shared_ptr<Compiler> &compiler,
                               bool autoremoveFilesEnable,
                               const std::string &tag)
{
  std::shared_ptr<Version> v = std::shared_ptr<Version>(new Version());
  v->fileName_bin = sharedObject;
  v->autoremoveFilesEnable = autoremoveFilesEnable;
  v->functionName = functionName;
  v->compiler = compiler;
  v->tag = tag;
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
