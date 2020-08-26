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

#ifndef PASTE_ALIGNMENTS_HELPERS_H_
#define PASTE_ALIGNMENTS_HELPERS_H_

#include <charconv>
#include <cmath>
#include <sstream>
#include <string>
#include <system_error>

#include "exceptions.h"

namespace paste_alignments {

namespace helpers {

/// @addtogroup ArgParseConvert-Reference
///
/// @{

/// @brief Tests whether `i` is in the range [`first`, `last`].
///
/// @parameter first First coordinate of tested range.
/// @parameter last Last coordinate of tested range.
/// @parameter i Value to be tested.
///
/// @exceptions Strong guarantee. Throws `exceptions::OutOfRange` if `i` is not
///  in the range [`first`, `last`].
///
inline int TestInRange(int first, int last, int i) {
  std::stringstream error_message;
  if (i < first || last < i) {
    error_message << "Expected value in range: [" << first << ", " << last
                  << "], but was given: " << i << '.';
    throw exceptions::OutOfRange(error_message.str());
  }
  return i;
}

/// @brief Tests whether `i` is positive.
///
/// @parameter i Number to be tested.
///
/// @exceptions Strong guarantee. Throws `exceptions::OutOfRange` if `i` is not
///  positive.
///
inline int TestPositive(int i) {
  std::stringstream error_message;
  if (i <= 0) {
    error_message << "Expected positive value, but was given: " << i << '.';
    throw exceptions::OutOfRange(error_message.str());
  }
  return i;
}

/// @brief Tests whether `i` is positive.
///
/// @parameter i Number to be tested.
///
/// @exceptions Strong guarantee. Throws `exceptions::OutOfRange` if `i` is not
///  positive.
///
inline long TestPositive(long i) {
  std::stringstream error_message;
  if (i <= 0) {
    error_message << "Expected positive value, but was given: " << i << '.';
    throw exceptions::OutOfRange(error_message.str());
  }
  return i;
}

/// @brief Tests whether the string `s` is non-empty.
///
/// @parameter s String to be tested.
///
/// @exceptions Strong guarantee. Throws `exceptions::UnexpectedEmptyString` if
///  `s` is the empty string.
///
inline std::string TestNonEmpty(std::string& s) {
  if (s.empty()) {
    throw exceptions::UnexpectedEmptyString("Empty string given, where"
                                            " non-empty string expected.");
  }
  return s;
}

/// @brief Interprets `s_view` as non-negative integer.
///
/// @parameter s_view A non-empty string of digits.
///
/// @exceptions Strong guarantee. Throws exceptions::ParsingError if conversion
///  failed.
///
int StringViewToInteger(const std::string_view& s_view);

/// @brief Return `true` if the two numbers are at most `epsilon` times the
///  magnitude of the smaller apart, and `false` otherwise.
///
/// @parameter first First float to be compared.
/// @parameter second Second float to be compared.
/// @parameter epsilon The factor multiplying the smaller of the two magnitudes
///  to determine maximum distance.
///
/// @details If one of the two is 0.0, then epsilon is multipltied with the
///  magnitude of the other to determine max distance. If both are 0.0, the
///  function returns true.
///
/// @exceptions Strong guarantee.
///
inline bool FuzzyFloatEquals(float first, float second, float epsilon = 0.01f) {
  float max_distance;
  if (first == 0.0f && second == 0.0f) {
    return true;
  } else if (first == 0.0f) {
    max_distance = epsilon * (std::abs(second));
  } else if (second == 0.0f) {
    max_distance = epsilon * (std::abs(first));
  } else {
    max_distance = epsilon * std::min(std::abs(first), std::abs(second));
  }
  return (std::abs(first - second) <= max_distance);
}

/// @brief Return `true` if the two numbers are at most `epsilon` times the
///  magnitude of the smaller apart, and `false` otherwise.
///
/// @parameter first First double to be compared.
/// @parameter second Second double to be compared.
/// @parameter epsilon The factor multiplying the smaller of the two magnitudes
///  to determine maximum distance.
///
/// @details If one of the two is 0.0, then epsilon is multipltied with the
///  magnitude of the other to determine max distance. If both are 0.0, the
///  function returns true.
///
/// @exceptions Strong guarantee.
///
inline bool FuzzyDoubleEquals(double first, double second,
                              double epsilon = 0.01) {
  double max_distance;
  if (first == 0.0 && second == 0.0) {
    return true;
  } else if (first == 0.0) {
    max_distance = epsilon * (std::abs(second));
  } else if (second == 0.0) {
    max_distance = epsilon * (std::abs(first));
  } else {
    max_distance = epsilon * std::min(std::abs(first), std::abs(second));
  }
  return (std::abs(first - second) <= max_distance);
}

/// @brief Returns the gap extension cost used by Megablast for provided reward
///  and penalty values.
///
/// @parameter reward Match reward scoring parameter.
/// @parameter penalty Mismatch penalty scoring parameter.
///
/// @details Megablast uses the expression `(reward / 2) + penalty` to calculate
///  its gap-extension cost.
///
/// @exceptions Strong guarantee. Throws `exception::OutOfRange` if `reward`, or
///  `penalty` are not positive.
///
inline float MegablastExtendCost(int reward, int penalty) {
  TestPositive(reward);
  TestPositive(penalty);
  return (reward / 2.0f) + penalty;
}

/// @}

} // namespace helpers

} // namespace paste_alignments

#endif // PASTE_ALIGNMENTS_HELPERS_H_