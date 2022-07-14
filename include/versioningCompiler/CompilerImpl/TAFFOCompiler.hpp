/* Copyright 2019 Politecnico di Milano.
 * Developed by : Daniele Cattaneo
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
#ifndef LIB_VERSIONING_COMPILER_TAFFO_COMPILER_HPP
#define LIB_VERSIONING_COMPILER_TAFFO_COMPILER_HPP

#include "versioningCompiler/Compiler.hpp"
#include <string>


namespace vc {


class TAFFOCompiler : public Compiler
{
public:
  enum Language {
    C,
    CXX
  };

  TAFFOCompiler();

  TAFFOCompiler(
    const std::string &compilerID,
    const std::string &llvmInstallPrefix,
    Language lang = C,
    const std::string &taffoInstallPrefix = "",
    const std::string &libWorkingDir = ".",
    const std::string &log = "",
    const std::string &annotationInserterPath = "");

  TAFFOCompiler(
    const std::string &compilerID,
    const std::string &llvmOptPath,
    const std::string &llvmClangPath,
    const std::string &llvmLinkerPath,
    const std::string &taffoInstallPrefix = "",
    const std::string &libWorkingDir = ".",
    const std::string &log = "",
    const std::string &annotationInserterPath = "",
	const std::string &llvmLinkPath = "");

  inline virtual ~TAFFOCompiler() {}

  virtual bool hasOptimizer() const override;

  virtual std::string generateIR(
    const std::vector<std::string> &src,
    const std::vector<std::string> &func,
    const std::string &versionID,
    const opt_list_t options) const ;

  virtual std::string runOptimizer(
    const std::string &src_IR,
    const std::string &versionID,
    const opt_list_t options) const override;

  virtual std::string generateBin(
    const std::vector<std::string> &src,
    const std::vector<std::string> &func,
    const std::string &versionID,
    const opt_list_t options) const;

  virtual std::string getOptionString(const Option &o) const override;

  static Option getScalarAnnotationDefine(const std::string& define, double min, double max);

  void setDisableVRA(bool disableVRA) { noVRA = disableVRA; }
  bool getDisableVRA() { return noVRA; }

  void setRestrictiveFunctionCloning(bool rfc) { restrictFunClone = rfc; }
  bool getRestrictiveFunctionCloning() { return restrictFunClone; }

  void setDisableTypeMerging(bool dtm) { noTypeMerge = dtm; }
  bool setDisableTypeMerging() { return noTypeMerge; }
  
private:
  std::string llvmOptPath;
  std::string llvmClangPath;
  std::string llvmLinkerPath;
  std::string llvmLinkPath;
  std::string taffoInstallPrefix;
  std::string annotationInserterPath;
  std::string annotationFilePath;
  bool noVRA = false;
  bool restrictFunClone = false;
  bool noTypeMerge = false;
  
  struct Component
  {
    std::string libName;
    std::string optParamName;
    std::string envName;
  };
  static Component Init;
  static Component VRA;
  static Component DTA;
  static Component Conversion;
  
  void insertAnnotations(const std::vector<std::string>& src, const opt_list_t& options, const opt_list_t& annotationOptions) const;
  std::vector<std::string> copySources(const std::vector<std::string>& src, const std::string& versionID, const opt_list_t& options) const;
  std::string getLLVMLibExtension() const;
  std::string getInvocation(const Component& c) const;
  static void splitOptimizationOptions(const opt_list_t& in, opt_list_t& outOpt, opt_list_t& outRest);
  static void splitAnnotationOptions(const opt_list_t& in, opt_list_t& outOpt, opt_list_t& outRest);
};


} // end namespace vc

#endif /* end of include guard: LIB_VERSIONING_COMPILER_TAFFO_COMPILER_HPP */
