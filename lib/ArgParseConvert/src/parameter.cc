// Copyright (c) 2020 Jasper Braun
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include "parameter.h"

#include <sstream>

namespace arg_parse_convert {

namespace {

// Maps integers to string representation of `ParameterConfiguration`
// enumerators
//
const char* kParameterCategory[]{
  "kPositionalParameter",
  "kKeywordParameter",
  "kFlag"
};

}

// ParameterConfiguration::DebugString
//
std::string ParameterConfiguration::DebugString() const {
  std::stringstream ss;
  ss << "{names: [";
  if (!names_.empty()) {
    ss << names_.at(0);
    for (const std::string& name : names_) {
      ss << ", " << name;
    }
  }
  ss << "], category: "
     << kParameterCategory[static_cast<int>(category_)]
     << ", default arguments: [";
  if (!default_arguments_.empty()) {
    ss << default_arguments_.at(0);
    for (const std::string& argument : default_arguments_) {
      ss << ", " << argument;
    }
  }
  ss << "], position: " << position_
     << ", min number of arguments: " << min_num_arguments_
     << ", max number of arguments: " << max_num_arguments_
     << ", description: " << description_
     << ", argument placeholder: " << argument_placeholder_
     << "}.";
  return ss.str();
}

} // namespace arg_parse_convert