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

#include "alignment.h"

#include <charconv>
#include <sstream>
#include <system_error>

#include "exceptions.h"

namespace paste_alignments {

// Alignment::FromStringFields helper.
//
namespace {

int StringViewToInteger(const std::string_view& s_view) {
  int result;
  std::stringstream error_message;

  if (!s_view.empty()) {
    std::string_view::const_iterator it{s_view.cbegin()};
    while (it != s_view.cend()) {
      if (!std::isdigit(*it)) {
        error_message << "Unable to convert field to integer: '" << s_view
                      << "'.";
        throw exceptions::ParsingError(error_message.str());
      }
      ++it;
    }
  }

  std::from_chars_result conversion_result = std::from_chars(
      s_view.data(), s_view.data() + s_view.size(), result);
  if (conversion_result.ec == std::errc::invalid_argument
      || conversion_result.ec == std::errc::result_out_of_range) {
    error_message << "Unable to convert field to integer: '" << s_view << "'.";
    throw exceptions::ParsingError(error_message.str());
  }

  return result;
}

} // namespace

// Alignment::FromStringFields.
//
Alignment Alignment::FromStringFields(int id,
                                      std::vector<std::string_view> fields) {
  std::stringstream error_message;
  if (fields.size() < 12) {
    error_message << "Not enough fields provided to create `Alignment` object."
                  << " Alignments require 12 fields, but only " << fields.size()
                  << " were provided. (id: " << id << ").";
    throw exceptions::ParsingError(error_message.str());
  }

  Alignment result{id};

  // Query coordinates.
  result.qstart_ = StringViewToInteger(fields.at(0));
  result.qend_ = StringViewToInteger(fields.at(1));
  if (result.qstart_ > result.qend_
      || result.qstart_ < 0
      || result.qend_ < 0) {
    error_message << "Invalid query start and end coordinates provide to create"
                  << " `Alignment` object: (qstart: " << result.qstart_
                  << ", qend: " << result.qend_ << "). (id: " << id << ").";
    throw exceptions::ParsingError(error_message.str());
  }

  // Subject coordinates.
  result.sstart_ = StringViewToInteger(fields.at(2));
  result.send_ = StringViewToInteger(fields.at(3));
  if (result.sstart_ < 0 || result.send_ < 0) {
    error_message << "Invalid subject start and end coordinates provide to"
                  << " create `Alignment` object: (sstart: " << result.sstart_
                  << ", send: " << result.send_ << "). (id: " << id << ").";
    throw exceptions::ParsingError(error_message.str());
  }
  if (result.sstart_ <= result.send_) {
    result.plus_strand_ = true;
  } else {
    std::swap(result.sstart_, result.send_);
    result.plus_strand_ = false;
  }

  // Identities, mismatches, gap openings and gap extensions.
  result.nident_ = StringViewToInteger(fields.at(4));
  result.mismatch_ = StringViewToInteger(fields.at(5));
  result.gapopen_ = StringViewToInteger(fields.at(6));
  result.gaps_ = StringViewToInteger(fields.at(7));
  if (result.nident_ < 0 || result.mismatch_ < 0
      || result.gapopen_ < 0 || result.gaps_ < 0) {
    error_message << "Invalid field value. Fields must not be negative:"
                  << " (nident: " << result.nident_ << ", mismatch: "
                  << result.mismatch_ << ", gapopen: " << result.gapopen_
                  << ", gaps: " << result.gaps_ << "). (id: " << id << ").";
    throw exceptions::ParsingError(error_message.str());
  }

  // Sequence lengths.
  result.qlen_ = StringViewToInteger(fields.at(8));
  result.slen_ = StringViewToInteger(fields.at(9));
  if (result.qlen_ <= 0 || result.slen_ <= 0) {
    error_message << "Invalid sequence length. Aligned sequences must have"
                  << " positive length: (qlen: " << result.qlen_ << ", slen: "
                  << result.slen_ << "). (id: " << id << ").";
    throw exceptions::ParsingError(error_message.str());
  }

  // Sequence alignment.
  result.qseq_ = fields.at(10);
  result.sseq_ = fields.at(11);
  if (result.qseq_.empty() || result.sseq_.empty()) {
    error_message << "Invalid sequence alignment. Alignment must be non-empty."
                  << " (id: " << id << ").";
    throw exceptions::ParsingError(error_message.str());
  }

  return result;
}

} // namespace paste_alignments