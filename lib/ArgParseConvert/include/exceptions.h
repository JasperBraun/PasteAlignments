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

#ifndef ARG_PARSE_CONVERT_EXCEPTIONS_H_
#define ARG_PARSE_CONVERT_EXCEPTIONS_H_

#include <stdexcept>

namespace arg_parse_convert {

namespace exceptions {

/// @brief Base class for all custom exceptions thrown by entities in the
///  `arg_parse_convert` namespace.
///
/// @details Only exceptions of a final derived type or exceptions from STL
///  operations are thrown.
///
struct BaseError : public std::logic_error {
  using std::logic_error::logic_error;
};

/// @brief Exception thrown while configuring a parameter.
///
struct ParameterConfigurationError final : public BaseError {
  using BaseError::BaseError;
};

/// @brief Exception thrown while accessing a parameter.
///
struct ParameterAccessError final : public BaseError {
  using BaseError::BaseError;
};

/// @brief Exception thrown while registering a parameter.
///
struct ParameterRegistrationError final : public BaseError {
  using BaseError::BaseError;
};

/// @brief Exception thrown while obtaining parameter value(s).
///
struct ValueAccessError final : public BaseError {
  using BaseError::BaseError;
};

/// @brief Exception thrown while parsing arguments.
///
struct ArgumentParsingError final : public BaseError {
  using BaseError::BaseError;
};

/// @brief Exception thrown while generating help-strings.
///
struct HelpStringError final : public BaseError {
  using BaseError::BaseError;
};

} // namespace exceptions

} // namespace arg_parse_convert

#endif // ARG_PARSE_CONVERT_EXCEPTIONS_H_