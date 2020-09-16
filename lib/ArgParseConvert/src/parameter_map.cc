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

#include "parameter_map.h"

#include <sstream>

namespace arg_parse_convert {

// ParameterMap::DebugString
//
std::string ParameterMap::DebugString() const {
  std::stringstream ss;
  ss << "{size: " << size() << ", required: [";
  if (required_parameters_.size() > 0) {
    for (int id : required_parameters_) {
      ss << '{';
      for (const std::string& name
           : parameter_configurations_.at(id).names()) {
        ss << name << ',';
      }
      ss << "}";
    }
  }
  ss << "], positional: [";
  if (positional_parameters_.size() > 0) {
    for (auto pair : positional_parameters_) {
      ss << '(' << pair.first << ',';
      ss << '{';
      for (const std::string& name
           : parameter_configurations_.at(pair.second).names()) {
        ss << name << ',';
      }
      ss << "})";
    }
  }
  ss << "], keyword: [";
  if (keyword_parameters_.size() > 0) {
    for (int id : keyword_parameters_) {
      ss << '{';
      for (const std::string& name
           : parameter_configurations_.at(id).names()) {
        ss << name << ',';
      }
      ss << "}";
    }
  }
  ss << "], flags: [";
  if (flags_.size() > 0) {
    for (int id : flags_) {
      ss << '{';
      for (const std::string& name
           : parameter_configurations_.at(id).names()) {
        ss << name << ',';
      }
      ss << "}";
    }
  }
  ss << "]}";
  return ss.str();
}

} // namespace arg_parse_convert