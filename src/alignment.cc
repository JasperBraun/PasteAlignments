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

#include <sstream>

#include "exceptions.h"
#include "helpers.h"

namespace paste_alignments {

// Alignment::FromStringFields.
//
Alignment Alignment::FromStringFields(int id,
                                      std::vector<std::string_view> fields,
                                      const ScoringSystem& scoring_system,
                                      const PasteParameters& parameters) {
  std::stringstream error_message;
  if (fields.size() < 12) {
    error_message << "Not enough fields provided to create `Alignment` object."
                  << " Alignments require 12 fields, but only " << fields.size()
                  << " were provided. (id: " << id << ").";
    throw exceptions::ParsingError(error_message.str());
  }

  Alignment result{id};

  // Query coordinates.
  result.qstart_ = helpers::StringViewToInteger(fields.at(0));
  result.qend_ = helpers::StringViewToInteger(fields.at(1));
  if (result.qstart_ > result.qend_
      || result.qstart_ < 0
      || result.qend_ < 0) {
    error_message << "Invalid query start and end coordinates provide to create"
                  << " `Alignment` object: (qstart: " << result.qstart_
                  << ", qend: " << result.qend_ << "). (id: " << id << ").";
    throw exceptions::ParsingError(error_message.str());
  }

  // Subject coordinates.
  result.sstart_ = helpers::StringViewToInteger(fields.at(2));
  result.send_ = helpers::StringViewToInteger(fields.at(3));
  if (result.sstart_ < 0 || result.send_ < 0) {
    error_message << "Invalid subject start and end coordinates provide to"
                  << " create `Alignment` object: (sstart: " << result.sstart_
                  << ", send: " << result.send_ << "). (id: " << id << ").";
    throw exceptions::ParsingError(error_message.str());
  }

  // Identities, mismatches, gap openings and gap extensions.
  result.nident_ = helpers::StringViewToInteger(fields.at(4));
  result.mismatch_ = helpers::StringViewToInteger(fields.at(5));
  result.gapopen_ = helpers::StringViewToInteger(fields.at(6));
  result.gaps_ = helpers::StringViewToInteger(fields.at(7));
  if (result.nident_ < 0 || result.mismatch_ < 0
      || result.gapopen_ < 0 || result.gaps_ < 0) {
    error_message << "Invalid field value. Fields must not be negative:"
                  << " (nident: " << result.nident_ << ", mismatch: "
                  << result.mismatch_ << ", gapopen: " << result.gapopen_
                  << ", gaps: " << result.gaps_ << "). (id: " << id << ").";
    throw exceptions::ParsingError(error_message.str());
  }

  // Sequence lengths.
  result.qlen_ = helpers::StringViewToInteger(fields.at(8));
  result.slen_ = helpers::StringViewToInteger(fields.at(9));
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

  // Derived values.
  if (result.sstart_ <= result.send_) {
    result.plus_strand_ = true;
  } else {
    std::swap(result.sstart_, result.send_);
    result.plus_strand_ = false;
  }
  result.length_ = result.qseq_.length();
  result.pident_ = 100.0f * static_cast<float>(result.nident_)
                   / static_cast<float>(result.length_);
  result.raw_score_ = scoring_system.RawScore(result.nident_, result.mismatch_,
                                              result.gapopen_, result.gaps_);
  result.bitscore_ = scoring_system.Bitscore(result.raw_score_, parameters);
  result.evalue_ = scoring_system.Evalue(result.raw_score_, result.qlen_,
                                         parameters);
  result.ungapped_suffix_begin_ = 0;
  result.ungapped_prefix_end_ = result.qseq_.length();
  return result;
}

// Alignment::operator==
//
bool Alignment::operator==(const Alignment& other) const {
  return (other.pasted_identifiers_ == pasted_identifiers_
          && other.qstart_ == qstart_
          && other.qend_ == qend_
          && other.sstart_ == sstart_
          && other.send_ == send_
          && other.plus_strand_ == plus_strand_
          && other.nident_ == nident_
          && other.mismatch_ == mismatch_
          && other.gapopen_ == gapopen_
          && other.gaps_ == gaps_
          && other.qlen_ == qlen_
          && other.slen_ == slen_
          && other.qseq_ == qseq_
          && other.sseq_ == sseq_
          && other.length_ == length_
          && helpers::FuzzyFloatEquals(other.pident_, pident_)
          && helpers::FuzzyFloatEquals(other.raw_score_, raw_score_)
          && helpers::FuzzyFloatEquals(other.bitscore_, bitscore_)
          && helpers::FuzzyDoubleEquals(other.evalue_, evalue_)
          && other.include_in_output_ == include_in_output_
          && other.ungapped_suffix_begin_ == ungapped_suffix_begin_
          && other.ungapped_prefix_end_ == ungapped_prefix_end_);
}

// Alignment::DebugString
//
std::string Alignment::DebugString() const {
  std::stringstream  ss;
  ss << std::boolalpha << "(id=" << Id()
     << ", qstart=" << qstart_
     << ", qend=" << qend_
     << ", sstart=" << sstart_
     << ", send=" << send_
     << ", plus_strand=" << plus_strand_
     << ", nident=" << nident_
     << ", mismatch=" << mismatch_
     << ", gapopen=" << gapopen_
     << ", gaps=" << gaps_
     << ", qlen=" << qlen_
     << ", slen=" << slen_
     << ", qseq='" << qseq_
     << "', sseq='" << sseq_
     << "', pasted_identifiers=[" << pasted_identifiers_.at(0);
  for (int i = 1; i < pasted_identifiers_.size(); ++i) {
    ss << ',' << pasted_identifiers_.at(i);
  }
  ss << "])";
  return ss.str();
}

} // namespace paste_alignments