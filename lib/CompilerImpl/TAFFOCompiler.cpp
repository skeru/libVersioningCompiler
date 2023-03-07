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
#include "versioningCompiler/CompilerImpl/TAFFOCompiler.hpp"

using namespace vc;

TAFFOCompiler::Component TAFFOCompiler::Init = {"TaffoInitializer",
                                                "-taffoinit", "INITLIB"};
TAFFOCompiler::Component TAFFOCompiler::VRA = {"TaffoVRA", "-taffoVRA",
                                               "VRALIB"};
TAFFOCompiler::Component TAFFOCompiler::DTA = {"TaffoDTA", "-taffodta",
                                               "TUNERLIB"};
TAFFOCompiler::Component TAFFOCompiler::Conversion = {"LLVMFloatToFixed",
                                                      "-flttofix", "PASSLIB"};

TAFFOCompiler::TAFFOCompiler(
    const std::string &compilerID, const std::filesystem::path &llvmOptPath,
    const std::filesystem::path &llvmClangPath,
    const std::filesystem::path &llvmLinkerPath,
    const std::filesystem::path &taffoInstallPrefix,
    const std::filesystem::path &libWorkingDir,
    const std::filesystem::path &log,
    const std::filesystem::path &annotationInserterPath,
    const std::filesystem::path &llvmLinkPath)
    : Compiler(compilerID, "", libWorkingDir, log, "", true),
      llvmOptPath(llvmOptPath), llvmClangPath(llvmClangPath),
      llvmLinkerPath(llvmLinkerPath), llvmLinkPath(llvmLinkPath),
      taffoInstallPrefix(taffoInstallPrefix),
      annotationInserterPath(annotationInserterPath) {
  if (const char *e_llvmdir = getenv("LLVM_DIR")) {
    const std::filesystem::path e_llvmdir_path =
        std::filesystem::u8path(e_llvmdir);
    if (this->llvmOptPath.empty())
      this->llvmOptPath = e_llvmdir_path / std::filesystem::u8path("bin/opt");
    if (this->llvmClangPath.empty())
      this->llvmClangPath =
          e_llvmdir_path / std::filesystem::u8path("bin/clang");
    if (this->llvmLinkerPath.empty())
      this->llvmLinkerPath =
          e_llvmdir_path / std::filesystem::u8path("bin/clang");
    if (this->llvmLinkerPath.empty())
      this->llvmLinkPath = e_llvmdir_path / std::filesystem::u8path("bin/link");
  }
}

TAFFOCompiler::TAFFOCompiler(
    const std::string &compilerID,
    const std::filesystem::path &llvmInstallPrefix, Language lang,
    const std::filesystem::path &taffoInstallPrefix,
    const std::filesystem::path &libWorkingDir,
    const std::filesystem::path &log,
    const std::filesystem::path &annotationInserterPath)
    : Compiler(compilerID, "", libWorkingDir, log, "", true),
      taffoInstallPrefix(taffoInstallPrefix),
      annotationInserterPath(annotationInserterPath) {
  std::filesystem::path llvmPfx;
  if (llvmInstallPrefix.empty()) {
    if (const char *e_llvmdir = getenv("LLVM_DIR")) {
      llvmPfx = std::filesystem::u8path(e_llvmdir);
    } else {
      llvmPfx = "/usr";
    }
  } else {
    llvmPfx = llvmInstallPrefix;
  }

  switch (lang) {
  case C:
    if (const char *e_clang = getenv("CLANG"))
      llvmClangPath = std::filesystem::u8path(e_clang);
    else
      llvmClangPath = llvmPfx / std::filesystem::path("bin/clang");
    break;
  case CXX:
    if (const char *e_clangxx = getenv("CLANGXX"))
      llvmClangPath = std::filesystem::u8path(e_clangxx);
    else
      llvmClangPath = llvmPfx / std::filesystem::path("bin/clang++");
    break;
  }

  llvmOptPath = llvmPfx / std::filesystem::path("bin/opt");
  llvmLinkPath = llvmPfx / std::filesystem::path("bin/llvm-link");
  llvmLinkerPath = llvmClangPath;
}

TAFFOCompiler::TAFFOCompiler()
    : TAFFOCompiler("taffo", std::filesystem::u8path("/usr/bin/opt"),
                    std::filesystem::u8path("/usr/bin/clang"),
                    std::filesystem::u8path("/usr/bin/clang"),
                    std::filesystem::u8path(""), std::filesystem::u8path("."),
                    std::filesystem::u8path("")) {}

bool TAFFOCompiler::hasOptimizer() const { return false; }

std::string TAFFOCompiler::getLLVMLibExtension() const {
#ifdef __APPLE__
  return "dylib";
#else
  return "so";
#endif
}

std::string
TAFFOCompiler::getInvocation(const TAFFOCompiler::Component &c) const {
  std::filesystem::path path;
  if (!taffoInstallPrefix.empty()) {
    path = taffoInstallPrefix / std::filesystem::path("lib") /
           std::filesystem::path(c.libName + "." + getLLVMLibExtension());
  } else if (const char *envpath = getenv(c.envName.c_str())) {
    path = std::filesystem::u8path(envpath);
  }
  if (exists(path))
    return llvmOptPath.string() + " -load " + path.string() + " " +
           c.optParamName;
  return "";
}

void TAFFOCompiler::splitOptimizationOptions(const opt_list_t &in,
                                             opt_list_t &outOpt,
                                             opt_list_t &outRest) {
  for (const auto opt : in) {
    if (opt.getPrefix() == "-O")
      outOpt.push_back(opt);
    else
      outRest.push_back(opt);
  }
}

void TAFFOCompiler::splitAnnotationOptions(const opt_list_t &in,
                                           opt_list_t &outOpt,
                                           opt_list_t &outRest) {
  for (const auto opt : in) {
    if (opt.getTag() == "AnnotationInserter")
      outOpt.push_back(opt);
    else
      outRest.push_back(opt);
  }
}

void TAFFOCompiler::insertAnnotations(
    const std::vector<std::filesystem::path> &src, const opt_list_t &options,
    const opt_list_t &annotationOptions) const {
  if (annotationInserterPath.empty())
    return;

  std::filesystem::path annotationFilePath("./annotations.json");
  std::string annotationContent("");
  std::string insertionExtraFlags("");
  for (const auto opt : annotationOptions) {
    if (opt.getPrefix() == "annotationFile") {
      annotationFilePath = std::filesystem::u8path(opt.getValue());
      continue;
    }

    if (opt.getPrefix() == "annotationJSON") {
      annotationContent = opt.getValue();
      continue;
    }
    insertionExtraFlags += " " + opt.getPrefix() + opt.getValue();
  }

  for (const auto &src_file : src) {

    std::string raw_cmd =
        annotationInserterPath.string() + " \"" + src_file.string() + "\" -i ";

    if (annotationContent.empty())
      raw_cmd += "-f=" + annotationFilePath.string();
    else
      raw_cmd += "-j=" + annotationContent;

    raw_cmd += " -- ";

    for (auto &o : options)
      raw_cmd = raw_cmd + " " + getOptionString(o);

    raw_cmd += insertionExtraFlags;

    Compiler::log_exec(raw_cmd);
  }
}

std::vector<std::filesystem::path>
TAFFOCompiler::copySources(const std::vector<std::filesystem::path> &src,
                           const std::string &versionID,
                           const opt_list_t &options) const {
  std::vector<std::filesystem::path> toReturn;
  std::string opt;
  for (auto &o : options) {
    opt = opt + " " + getOptionString(o);
  }

  int i = 0;
  for (auto &file : src) {

    std::filesystem::path newFile =
        Compiler::generateTemporaryFileName(file, versionID, i);
    std::string command = llvmClangPath.string() + " " + file.string() +
                          " -E -o \"" + newFile.string() + "\" " + opt;

    toReturn.push_back(newFile);
    log_exec(command);
    i++;
  }

  return toReturn;
}

std::filesystem::path
TAFFOCompiler::generateIR(const std::vector<std::filesystem::path> &src,
                          const std::vector<std::string> &func,
                          const std::string &versionID,
                          const opt_list_t options) const {
  opt_list_t optOpts;
  opt_list_t normalOpts;
  opt_list_t annotationOptions;
  opt_list_t compilationOptions;
  splitAnnotationOptions(options, annotationOptions, compilationOptions);
  splitOptimizationOptions(compilationOptions, optOpts, normalOpts);

  std::vector<std::filesystem::path> copied =
      copySources(src, versionID, normalOpts);
  insertAnnotations(copied, compilationOptions, annotationOptions);

  std::string compileOptions = "";
  for (auto &o : normalOpts)
    compileOptions = compileOptions + " " + getOptionString(o);

  for (const auto &src_file : copied) {
    std::string raw_cmd = llvmClangPath.string() +
                          " -c -S -emit-llvm -O0 -o \"" + src_file.string() +
                          "_0_clang" + "\" " + " \"" + src_file.string() +
                          "\"" + compileOptions;
    Compiler::log_exec(raw_cmd);
  }

  std::string raw_bitcode =
      Compiler::getBitcodeFileName(versionID + "_1_clang");
  std::string llvm_link_command =
      llvmLinkPath.string() + " -o \"" + raw_bitcode + "\" ";

  for (const auto &src_file : copied)
    llvm_link_command += "\"" + src_file.string() + "_0_clang" + "\" ";

  Compiler::log_exec(llvm_link_command);
  if (!exists(raw_bitcode))
    return "";

  std::string init_bitcode =
      Compiler::getBitcodeFileName(versionID + "_2_init");
  std::string init_cmd = getInvocation(Init) + " -S -o \"" + init_bitcode +
                         "\" \"" + raw_bitcode + "\"";
  if (noVRA)
    init_cmd += " -manualrange";
  if (restrictFunClone)
    init_cmd += " -manualclone";
  Compiler::log_exec(init_cmd);
  if (!exists(init_bitcode))
    return "";

  std::string vra_bitcode;
  if (!noVRA) {
    vra_bitcode = Compiler::getBitcodeFileName(versionID + "_3_vra");
    std::string vra_cmd = getInvocation(VRA) + " -S -o \"" + vra_bitcode +
                          "\" \"" + init_bitcode + "\"";
    Compiler::log_exec(vra_cmd);
    if (!exists(vra_bitcode))
      return "";
  } else {
    vra_bitcode = init_bitcode;
  }

  std::string dta_bitcode = Compiler::getBitcodeFileName(versionID + "_4_dta");
  std::string dta_cmd = getInvocation(DTA) + " -S -o \"" + dta_bitcode +
                        "\" \"" + vra_bitcode + "\"";
  if (noTypeMerge)
    dta_cmd += " -notypemerge";
  Compiler::log_exec(dta_cmd);
  if (!exists(dta_bitcode))
    return "";

  std::string conv_bitcode;
  conv_bitcode = Compiler::getBitcodeFileName(versionID + "_5_conv");
  std::string conv_cmd = getInvocation(Conversion) + " -dce -S -o \"" +
                         conv_bitcode + "\" \"" + dta_bitcode + "\"";
  Compiler::log_exec(conv_cmd);
  if (!exists(conv_bitcode))
    return "";

  std::string end_bitcode = Compiler::getBitcodeFileName(versionID);
  std::string end_cmd = llvmClangPath.string() + " -c -S -emit-llvm -o \"" +
                        end_bitcode + "\" \"" + conv_bitcode + "\"";
  for (auto &o : optOpts) {
    end_cmd = end_cmd + " " + getOptionString(o);
  }
  Compiler::log_exec(end_cmd);
  if (!exists(end_bitcode))
    return "";

  return end_bitcode;
}

std::filesystem::path
TAFFOCompiler::runOptimizer(const std::filesystem::path &src_IR,
                            const std::string &versionID,
                            const opt_list_t options) const {
  std::string error = __STRING(__FUNCTION__) ": ";
  error = error + "TAFFO compiler does not support optimizer";
  Compiler::unsupported(error);
  return "";
}

std::filesystem::path
TAFFOCompiler::generateBin(const std::vector<std::filesystem::path> &src,
                           const std::vector<std::string> &func,
                           const std::string &versionID,
                           const opt_list_t options) const {
  // std::string bitcode = generateIR(src, func, versionID, options);
  // if (bitcode.empty())
  //   return "";
  std::string bitcode = "";
  for (const auto &file : src) {
    bitcode = bitcode + "\"" + file.string() + "\" ";
  }

  // system call - command construction
  std::filesystem::path binaryFile =
      Compiler::getSharedObjectFileName(versionID);
  std::string command = llvmLinkerPath.string() + " -fPIC -shared -o \"" +
                        binaryFile.string() + "\" " + bitcode;
  for (const auto &opt : options) {
    command = command + " " + getOptionString(opt);
  }
  log_exec(command);
  if (!exists(binaryFile))
    return "";

  return binaryFile;
}

inline std::string TAFFOCompiler::getOptionString(const Option &o) const {
  std::string tmp_val = o.getValue();
  if (tmp_val.length() > 2 &&
      // escape whitespaces with double quotes
      (tmp_val.find(" ") != std::string::npos ||
       tmp_val.find("\t") != std::string::npos) &&
      // ...but not if already within quotes
      !(tmp_val[0] == tmp_val[tmp_val.length() - 1] &&
        (tmp_val[0] == '\"' || tmp_val[0] == '\''))) {
    tmp_val = "\"" + tmp_val + "\"";
  }
  return o.getPrefix() + tmp_val;
}

Option TAFFOCompiler::getScalarAnnotationDefine(const std::string &define,
                                                double min, double max) {
  std::string tag = "TAFFO_" + define;
  std::string lhs = "-D" + define + "=";
  std::string rhs = "'\"scalar(range(" + std::to_string(min) + ", " +
                    std::to_string(max) + "))\"'";
  return Option(tag, lhs, rhs);
}
