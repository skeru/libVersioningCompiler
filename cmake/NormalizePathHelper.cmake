# Copyright 2022 Politecnico di Milano.
# Developed by : Moreno Giussani
#                Ms student, Politecnico di Milano
#                <first_name>.<family_name>@mail.polimi.it
#
# This file is part of libVersioningCompiler
#
# libVersioningCompiler is free software: you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public License
# as published by the Free Software Foundation, either version 3
# of the License, or (at your option) any later version.
#
# libVersioningCompiler is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with libVersioningCompiler. If not, see <http://www.gnu.org/licenses/>
#

# Helper for path normalization and multi OS support

function(convert_to_native_normalized_path element)
  cmake_path(CONVERT "${element}" TO_NATIVE_PATH_LIST element NORMALIZE)
  set(${element}
      "${element}"
      PARENT_SCOPE)
endfunction(convert_to_native_normalized_path)

# Helper for path normalization in a list and multi OS compatibility

function(list_convert_to_native_normalized_path var_list)
  set(tmp_list "")
  foreach(elem ${${var_list}})
    convert_to_native_normalized_path(elem)
    list(APPEND tmp_list ${elem})
  endforeach()
  set(${var_list}
      "${tmp_list}"
      PARENT_SCOPE)
endfunction(list_convert_to_native_normalized_path)
