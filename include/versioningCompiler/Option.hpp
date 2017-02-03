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
#ifndef LIB_VERSIONING_COMPILER_OPTION_HPP
#define LIB_VERSIONING_COMPILER_OPTION_HPP

#include <string>

namespace vc {

class Option
{

 public:
  /** \brief default constructor.
   *
   * \param optionTag option identifier.
   * It can be used to index the suported options and avoid duplicates and/or
   * sequence of inconsistent options, such as "-O2 -O3 -Ofast"
   *
   * \param optionPrefix string to prepend the option value.
   * This string should include also separators such as " ", "=".
   * Examples of optionPrefix are:
   * "-O" for `-O2`, "--std=" for `--std=c++11`, "--emit-llvm" for `--emit-llvm`
   *
   * \param val option value.
   * Examples are "2" for `-O2`, "3" for `-O3`.
   * Default value is an empty string.
   */
  Option(const std::string &optionTag,
         const std::string &optionPrefix,
         const std::string &val = "");

  Option(const Option& other) = default;

  std::string getTag() const;

  std::string getValue() const;

  std::string getPrefix() const;

  inline Option& operator= (const Option& other) = default;

  inline bool operator== (const Option& other) {
    return (getPrefix() + getValue()) == (other.getPrefix() + other.getValue());
  }

  inline bool operator< (const Option& other) {
    return (getPrefix() + getValue()) < (other.getPrefix() + other.getValue());
  }

 private:
  std::string tag;
  std::string value;
  std::string prefix;

};

} // end namespace vc

namespace std {
  template<>
  struct hash<vc::Option>
  {
    std::size_t operator()( const vc::Option& key ) {
      return hash<std::string>()(key.getPrefix() + key.getValue());
    }
  };
} // end namespace std

#endif /* end of include guard: LIB_VERSIONING_COMPILER_OPTION_HPP */
