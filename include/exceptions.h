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

#ifndef PASTE_ALIGNMENTS_EXCEPTIONS_H_
#define PASTE_ALIGNMENTS_EXCEPTIONS_H_

namespace paste_alignments {

namespace exceptions {

/// @brief Base class of exceptions thrown by entities in the `paste_alignments`
///  namespace.
///
/// @details Only exceptions of a final derived type or exceptions from STL
///  operations are thrown.
///
struct BaseException : public std::logic_error {
  using std::logic_error::logic_error;
};

/// @brief Thrown when failing to parse input data.
///
struct ParsingError final : public BaseException {
  using BaseException::BaseException;
};

/// @brief Thrown when a value doesn't fall in a prescribed range.
///
struct OutOfRange final : public BaseException {
  using BaseException::BaseException;
};

/// @brief Thrown when an empty string is used where a nonempty string was
///  required.
///
struct UnexpectedEmptyString final : public BaseException {
  using BaseException::BaseException;
};

/// @brief Thrown when error related to alignment score calculation occurs.
///
struct ScoringError final : public BaseException {
  using BaseException::BaseException;
};
/*
/// @brief Thrown by some functions when an invalid argument is given.
///
struct InvalidInput final : public BaseException {
  using BaseException::BaseException;
};*/

/// @brief Thrown when error occurred while reading input data.
///
struct ReadError final : public BaseException {
  using BaseException::BaseException;
};

/// @brief Thrown when error occurred while pasting alignments.
///
struct PastingError final : public BaseException {
  using BaseException::BaseException;
};

} // namespace exceptions

} // namespace paste_alignments

#endif // PASTE_ALIGNMENTS_EXCEPTIONS_H_