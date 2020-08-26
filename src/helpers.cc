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

#include "helpers.h"

namespace paste_alignments {

namespace helpers {

// StringViewToInteger
//
int StringViewToInteger(const std::string_view& s_view) {
  int result;
  std::stringstream error_message;

  if (!s_view.empty()) {
    std::string_view::const_iterator it{s_view.cbegin()};
    while (it != s_view.cend()) {
      if (!std::isdigit(*it)) {
        error_message << "Unable to convert field to non-negative integer: '"
                      << s_view << "'.";
        throw exceptions::ParsingError(error_message.str());
      }
      ++it;
    }
  }

  std::from_chars_result conversion_result = std::from_chars(
      s_view.data(), s_view.data() + s_view.size(), result);
  if (conversion_result.ec == std::errc::invalid_argument
      || conversion_result.ec == std::errc::result_out_of_range) {
    error_message << "Unable to convert field to non-negative integer: '"
                  << s_view << "'.";
    throw exceptions::ParsingError(error_message.str());
  }

  return result;
}

} // namespace helpers

} // namespace paste_alignments