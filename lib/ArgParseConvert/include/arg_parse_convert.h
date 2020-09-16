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

#ifndef ARG_PARSE_CONVERT_ARG_PARSE_CONVERT_H_
#define ARG_PARSE_CONVERT_ARG_PARSE_CONVERT_H_

#include "argument_map.h"
#include "conversion_functions.h"
#include "exceptions.h"
#include "help_string_formatters.h"
#include "parameter.h"
#include "parameter_map.h"
#include "parsers.h"

/// @defgroup ArgParseConvert-Reference
///
/// @{

/// @brief Library namespace.
///
/// @details All types and functions defined by the library reside inside this
///  namespace.
///
namespace arg_parse_convert {}

/// @brief Contains library-specific exceptions.
///
/// @details All non-STL exceptions thrown by library functions are derived
///  from `BaseError`.
///
namespace arg_parse_convert::exceptions {}

/// @brief Contains common conversion functions.
///
/// @details For compatibility,  wrappers around standard library string
///  conversion functions contained in this namespace must be used as conversion
///  functions instead of using the corresponding standad library functions
///  directly.
///
namespace arg_parse_convert::converters {}
/// @}

#endif // ARG_PARSE_CONVERT_ARG_PARSE_CONVERT_H_