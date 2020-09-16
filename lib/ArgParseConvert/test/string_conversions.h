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

#ifndef ARG_PARSE_CONVERT_TEST_STRING_CONVERSIONS_H_
#define ARG_PARSE_CONVERT_TEST_STRING_CONVERSIONS_H_

#include "catch.h"
#include "arg_parse_convert.h"

namespace Catch {

template<>
struct StringMaker<arg_parse_convert::ParameterConfiguration> {
  static std::string convert(
      const arg_parse_convert::ParameterConfiguration& configuration) {
    return configuration.DebugString();
  }
};

template<class ParameterType>
struct StringMaker<arg_parse_convert::Parameter<ParameterType>> {
  static std::string convert(
      const arg_parse_convert::Parameter<ParameterType>& parameter) {
    return parameter.DebugString();
  }
};

template<>
struct StringMaker<arg_parse_convert::ParameterMap> {
  static std::string convert(
      const arg_parse_convert::ParameterMap& map) {
    return map.DebugString();
  }
};

template<>
struct StringMaker<arg_parse_convert::ArgumentMap> {
  static std::string convert(
      const arg_parse_convert::ArgumentMap& map) {
    return map.DebugString();
  }
};

} // namespace Catch

#endif // ARG_PARSE_CONVERT_TEST_STRING_CONVERSIONS_H_