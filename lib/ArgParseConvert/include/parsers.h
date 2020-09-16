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

#ifndef ARG_PARSE_CONVERT_PARSERS_H_
#define ARG_PARSE_CONVERT_PARSERS_H_

#include <string>
#include <vector>

#include "argument_map.h"
#include "exceptions.h"

namespace arg_parse_convert {

/// @addtogroup ArgParseConvert-Reference
///
/// @{

/// @name Parsers
///
/// @{

/// @brief Assigns members of `argv` to parameters registered with the
///  `ParameterMap` member of `arguments`.
///
/// @details Arguments with prefix `--` (except for the argument separator `--`
///  itself) must be names of keyword parameters or flags. Arguments with prefix
///  `-` indicate a list of characters, each of which is a name of a flag. The
///  last character may also be the name of a keyword parameter. All consecutive
///  argument lists following the name of a keyword parameter up to its maximum
///  number of arguments, are assigned to that keyword parameter. All other
///  contiguous lists of arguments are assigned to positional parameters.
///  Whenever the argument list of a positional parameter is interrupted (for
///  example, by a flag or a keyword), no more arguments are assigned to it and
///  the next positional parameter is assigned to next. The two-letter argument
///  `--` marks a separation point in the argument list, after which all
///  arguments are read as positional parameter arguments independent of their
///  prefix.
///
/// If a parameter was assigned one or more arguments prior to execution of this
///  function, the arguments that would be assigned to it by this function are
///  added to the return value instead.
///
/// @return A list of arguments which could not be assigned to any parameters.
///
/// @exceptions Basic guarantee. Throws `exceptions::ArgumentParsingError` when:
///  * Option list contains a character that is not the name of a flag, or the
///    the name of a keyword parameter in the case of the last letter.
///  * When a parameter begins with 2 hyphens, but the suffix after the 2 hypens
///    is not the name of a registered keyword parameter or flag.
///    
std::vector<std::string> ParseArgs(int argc, const char** argv,
                                   ArgumentMap& arguments);

/// @brief Assigns arguments listed in `is` to parameters registered with the
///  `ParameterMap` member of `arguments`.
///
/// @details Empty lines and lines starting with '#' are ignored. Parameter
///  names and arguments must be listed in the form 'name=arg1 arg2 ... argN'.
///  Flags must only be assigned a single argument (TRUE, True, true, or 1 for
///  setting the flag and FALSE, False, false, or 0 for explicitly unsetting the
///  flag). Flags and keywords must be listed by name (without hyphens), or they
///  are not recognized.
///
/// If a parameter was assigned one or more arguments prior to execution of this
///  function, the arguments that would be assigned to it by this function are
///  added to the return value instead. Arguments in the list of arguments
///  assigned to a parameter in excess of the parameter's maximum number of
///  arguments are also added to the return value.
///
/// @exceptions Basic guarantee. Throws `exceptions::ArgumentParsingError` when:
///  * Non-empty line not beginning with '#', does not contain '='.
///  * Name of parameter is not registered with the `ParameterMap` member of
///    `arguments`.
///  * A parameter name is listed without arguments.
///  * Something other than the prescribed strings was set as the argument of a
///    flag.
///
std::vector<std::string> ParseFile(std::istream& is, ArgumentMap& arguments);
/// @}

/// @}

} // namespace arg_parse_convert

#endif // ARG_PARSE_CONVERT_PARSERS_H_