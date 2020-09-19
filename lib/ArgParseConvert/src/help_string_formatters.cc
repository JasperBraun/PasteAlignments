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

#include "help_string_formatters.h"

#include <algorithm>
#include <cassert>

namespace arg_parse_convert {

// Help string formatting helpers.
//
namespace {

// Returns formatted parameter description.
//
std::string FormattedDescription(const ParameterConfiguration& configuration,
                                 int width, int indentation) {
  assert(width > 0 && indentation >= 0 && width > indentation);
  std::string result;
  std::string_view description{configuration.description()};
  int offset{0}, text_width{width - indentation};
  std::string_view::size_type break_pos;
  std::string_view window{description.substr(0, text_width + 1)};
  while (window.length() > 0) {
    if (static_cast<int>(window.length()) > text_width) {
        //&& description.at(offset + text_width) != ' ') {
      break_pos = window.rfind(' ');
    } else {
      break_pos = std::string_view::npos;
    }
    if (break_pos == std::string_view::npos || break_pos > 0) {
      result.append(indentation, ' ');
      result.append(window.substr(0, break_pos));
      result.push_back('\n');
    }
    if (break_pos == std::string_view::npos
        || offset + break_pos + 1 >= description.length()) {
      break;
    //} else if (break_pos == std::string_view::npos) {
     // offset += text_width;
    } else {
      offset += break_pos + 1;
    }
    window = description.substr(offset, text_width + 1);
  }
  return result;
}

// Returns argument placeholder with a space at the beginning or and empty
// string, if placeholder is empty. If positional parameter, space prefix is
// omitted.
//
std::string Placeholder(const ParameterConfiguration& configuration) {
  std::string result;
  if (configuration.placeholder().length() > 0) {
    if (configuration.category() != ParameterCategory::kPositionalParameter) {
      result.push_back(' ');
    }
    result.append(configuration.placeholder());
  }
  return result;
}

// Returns default argument list of the form "( = ARG1 ARG2 ARG3)", or an empty
// string, if no default arguments are stored in `configuration`.
//
std::string DefaultArgumentList(const ParameterConfiguration& configuration) {
  std::string result;
  if (configuration.default_arguments().size() > 0) {
    result.append(" ( = ");
    result.append(configuration.default_arguments().at(0));
    for (int i = 1;
         i < static_cast<int>(configuration.default_arguments().size());
         ++i) {
      result.push_back(' ');
      result.append(configuration.default_arguments().at(i));
    }
    result.push_back(')');
  }
  return result;
}

// Returns prefix "-", or "--".
//
std::string HyphensPrefix(const std::string& name) {
  assert(!name.empty());
  if (name.length() == 1) {
    return "-";
  } else {
    return "--";
  }
}

// Returns list of names of the parameter of the form "--NAME1, -N, --NAME3".
//
std::string NamesList(ParameterConfiguration configuration) {
  assert(!configuration.names().empty());
  std::string result;
  result.append(HyphensPrefix(configuration.names().at(0)));
  result.append(configuration.names().at(0));
  for (int i = 1; i < static_cast<int>(configuration.names().size()); ++i) {
    result.append(", ");
    result.append(HyphensPrefix(configuration.names().at(i)));
    result.append(configuration.names().at(i));
  }
  return result;
}

// Constructs formatted help string for positional parameter.
//
std::string PositionalHelpString(const ParameterConfiguration& configuration,
                                 int width, int parameter_indentation,
                                 int description_indentation) {
  assert(configuration.category() == ParameterCategory::kPositionalParameter);
  std::string result;
  // Parameter signature.
  result.append(parameter_indentation, ' ');
  result.append(Placeholder(configuration));
  result.append(DefaultArgumentList(configuration));
  result.push_back('\n');
  // Description.
  result.append(FormattedDescription(configuration, width,
                                     description_indentation));
  return result;
}

// Constructs formatted help string for keyword parameter.
//
std::string KeywordHelpString(const ParameterConfiguration& configuration,
                              int width, int parameter_indentation,
                              int description_indentation) {
  assert(configuration.category() == ParameterCategory::kKeywordParameter);
  std::string result;
  // Parameter signature.
  result.append(parameter_indentation, ' ');
  result.append(NamesList(configuration));
  result.append(Placeholder(configuration));
  result.append(DefaultArgumentList(configuration));
  result.push_back('\n');
  // Description.
  result.append(FormattedDescription(configuration, width,
                                     description_indentation));
  return result;
}

// Constructs formatted help string for flag.
//
std::string FlagHelpString(const ParameterConfiguration& configuration,
                           int width, int parameter_indentation,
                           int description_indentation) {
  assert(configuration.category() == ParameterCategory::kFlag);
  std::string result;
  // Parameter signature.
  result.append(parameter_indentation, ' ');
  result.append(NamesList(configuration));
  result.push_back('\n');
  // Description.
  result.append(FormattedDescription(configuration, width,
                                     description_indentation));
  return result;
}

} // namespace

// FormattedHelpString
//
std::string FormattedHelpString(const ParameterMap& parameter_map,
    std::string header, std::string footer, int width,
    int parameter_indentation, int description_indentation) {
  int required_positional{0}, required_keyword{0};
  std::vector<int> sorted_required, sorted_keywords, sorted_flags;
  // Preconditions.
  std::stringstream error_message;
  if (width <= 0 || parameter_indentation < 0 || description_indentation < 0
      || width <= parameter_indentation || width <= description_indentation) {
    error_message << "Invalid help string formatting parameters (`width` = '"
                  << width << "', `parameter_indentation` = '"
                  << parameter_indentation << "', `description_indentation` = '"
                  << description_indentation << "').";
    throw exceptions::HelpStringError(error_message.str());
  }
  // Header.
  std::string result{std::move(header)};
  // Required parameters.
  if (!parameter_map.required_parameters().empty()) {
    sorted_required = std::vector<int>{
        parameter_map.required_parameters().cbegin(),
        parameter_map.required_parameters().cend()};
    std::sort(sorted_required.begin(), sorted_required.end());
    result.append("\nRequired parameters:\n");
    for (int id : sorted_required) {
      switch (parameter_map.GetConfiguration(id).category()) {
        case ParameterCategory::kPositionalParameter: {
          result.append(PositionalHelpString(parameter_map.GetConfiguration(id),
                                             width, parameter_indentation,
                                             description_indentation));
          required_positional += 1;
          break;
        }
        case ParameterCategory::kKeywordParameter: {
          result.append(KeywordHelpString(parameter_map.GetConfiguration(id),
                                          width, parameter_indentation,
                                          description_indentation));
          required_keyword += 1;
          break;
        }
        default: {
          assert(false);
          break;
        }
      }
    }
  }
  // Non-required positional parameters.
  if (static_cast<int>(parameter_map.positional_parameters().size())
      > required_positional) {
    result.append("\nOptional positional parameters:\n");
    for (const auto& pair : parameter_map.positional_parameters()) {
      if (!parameter_map.GetConfiguration(pair.second).IsRequired()) {
        result.append(PositionalHelpString(
            parameter_map.GetConfiguration(pair.second), width,
            parameter_indentation, description_indentation));
      }
    }
  }
  // Non-required keyword parameters.
  if (static_cast<int>(parameter_map.keyword_parameters().size())
      > required_keyword) {
    result.append("\nOptional keyword parameters:\n");
    sorted_keywords = std::vector<int>{
        parameter_map.keyword_parameters().cbegin(),
        parameter_map.keyword_parameters().cend()};
    std::sort(sorted_keywords.begin(), sorted_keywords.end());
    for (int id : sorted_keywords) {
      if (!parameter_map.GetConfiguration(id).IsRequired()) {
        result.append(KeywordHelpString(parameter_map.GetConfiguration(id),
                                        width, parameter_indentation,
                                        description_indentation));
      }
    }
  }
  // Flags.
  if (!parameter_map.flags().empty()) {
    result.append("\nFlags:\n");
    sorted_flags = std::vector<int>{
        parameter_map.flags().cbegin(),
        parameter_map.flags().cend()};
    std::sort(sorted_flags.begin(), sorted_flags.end());
    for (int id : sorted_flags) {
      result.append(FlagHelpString(parameter_map.GetConfiguration(id), width,
                                   parameter_indentation,
                                   description_indentation));
    }
  }
  // Footer.
  result.append(footer);
  return result;
}

} // namespace arg_parse_convert