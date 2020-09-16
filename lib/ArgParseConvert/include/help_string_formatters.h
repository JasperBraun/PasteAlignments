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

#ifndef ARG_PARSE_CONVERT_HELP_STRING_FORMATTERS_H_
#define ARG_PARSE_CONVERT_HELP_STRING_FORMATTERS_H_

#include <string>

#include "exceptions.h"
#include "parameter_map.h"

namespace arg_parse_convert {

/// @addtogroup ArgParseConvert-Reference
///
/// @{

/// @name Help string formatters
///
/// @{

/// @brief Returns a formatted help string for the parameters contained in
///  `parameter_map`.
///
/// @details Help string is a concatenation of `header`, help strings of
///  required parameters, help strings of non-required positional parameters,
///  help strings of non-required keyword parameters, help strings of flags, and
///  finally `footer`, in this order. `width` limits the line width for
///  descriptions of parameters, `parameter_indentation` determines how far the
///  names and argument lists of parameters are indented, and
///  `description_indentation` determines how far descriptions of parameters are
///  indented. Parameters within each of the sections appear in the same order
///  as originally inserted into the `parameter_map`.
///
/// @exceptions Strong guarantee. Throws `exceptions::HelpStringError` if
///  * `width` is not positive.
///  * One of `parameter_indentation`, or `description_indentation` is negative.
///  * One of `parameter_indentation`, or `description_indentation` is not less
///    than `width`.
///
std::string FormattedHelpString(const ParameterMap& parameter_map,
    std::string header = "", std::string footer = "", int width = 80,
    int parameter_indentation = 4, int description_indentation = 8);
/// @}

/// @}

} // namespace arg_parse_convert

#endif // ARG_PARSE_CONVERT_HELP_STRING_FORMATTERS_H_