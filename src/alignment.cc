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
                                      const PasteParameters& paste_parameters) {
  std::stringstream error_message;
  if (fields.size() >= 13
      || (paste_parameters.blind_mode && fields.size() >= 11)) {

    Alignment result{id};

    // Query coordinates.
    result.qstart_ = helpers::StringViewToInteger(fields.at(0));
    result.qend_ = helpers::StringViewToInteger(fields.at(1));
    if (result.qstart_ > result.qend_
        || result.qstart_ < 0
        || result.qend_ < 0) {
      error_message << "Invalid query start and end coordinates provide to"
                    << " create `Alignment` object: (qstart: " << result.qstart_
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
    result.length_ = helpers::StringViewToInteger(fields.at(10));
    if (result.qlen_ <= 0 || result.slen_ <= 0 || result.length_ <= 0) {
      error_message << "Invalid sequence length. Aligned sequences must have"
                    << " positive length: (qlen: " << result.qlen_ << ", slen: "
                    << result.slen_ << ", length: " << result.length_
                    << "). (id: " << id << ").";
      throw exceptions::ParsingError(error_message.str());
    }

    // Sequence alignment.
    if (!paste_parameters.blind_mode) {
      result.qseq_ = fields.at(11);
      result.sseq_ = fields.at(12);
      if (result.qseq_.empty() || result.sseq_.empty()) {
        error_message << "Invalid sequence alignment. Alignment must be"
                      << " non-empty. (id: " << id << ").";
        throw exceptions::ParsingError(error_message.str());
      } else if (result.qseq_.length() != result.sseq_.length()) {
        error_message << "Invalid sequence alignment. Both sides of the"
                      << " alignment must have the same length. (id: " << id
                      << ").";
        throw exceptions::ParsingError(error_message.str());
      } else if (result.qseq_.length() != result.length_) {
        error_message << "Alignment length must be the same as the length of"
                      << " either side of the alignment. (id: " << id << ").";
        throw exceptions::ParsingError(error_message.str());
      }
    }

    // Derived values.
    if (result.sstart_ <= result.send_) {
      result.plus_strand_ = true;
    } else {
      std::swap(result.sstart_, result.send_);
      result.plus_strand_ = false;
    }
    result.UpdateSimilarityMeasures(scoring_system, paste_parameters);
    result.ungapped_prefix_end_ = result.length_;
    result.ungapped_suffix_begin_ = 0;
    return result;

  } else {
    error_message << "Not enough fields provided to create `Alignment` object."
                  << " Alignments require 13 fields (11 if in blind mode), but"
                  << " only " << fields.size() << " were provided. (id: " << id
                  << ").";
    throw exceptions::ParsingError(error_message.str());
  }
}

// Alignment::PasteRight / Alignment::PasteLeft helper
//
namespace {

// Data determining how a sequence is partitioned into (portions of) the two
// pasted alignments' sequences, gaps and unknowns.
//
struct PastedPartition {
  int gap_begin;
  int gap_length;
  int unknown_begin;
  int unknown_length;
  int right_begin;
  int right_length;

  inline std::string DebugString() const {
    std::stringstream ss;
    ss << "(gap_begin=" << gap_begin
       << ", gap_length=" << gap_length
       << ", unknown_begin=" << unknown_begin
       << ", unknown_length=" << unknown_length
       << ", right_begin=" << right_begin
       << ", right_length=" << right_length
       << ')';
    return ss.str();
  }
};

// Determines partitioning of pasted sequence when maximizing ungapped suffix.
//
PastedPartition GetRightPartition(const AlignmentConfiguration& config) {
  PastedPartition result;

  // Left-prefix + gap + unknown + right
  result.gap_begin = config.left_length - std::max(config.query_overlap,
                                                   config.subject_overlap);
  result.gap_length = config.shift;
  result.unknown_begin = result.gap_begin + result.gap_length;
  result.unknown_length = std::min(config.query_distance,
                                   config.subject_distance);
  result.right_begin = result.unknown_begin + result.unknown_length;
  result.right_length = config.right_length;

  return result;
}

// Determines partitioning of pasted sequence when maximizing ungapped prefix.
//
PastedPartition GetLeftPartition(const AlignmentConfiguration& config) {
  PastedPartition result;

  // Left + unknown + gap + right-suffix
  result.unknown_begin = config.left_length;
  result.unknown_length = std::min(config.query_distance,
                                   config.subject_distance);
  result.gap_begin = result.unknown_begin + result.unknown_length;
  result.gap_length = config.shift;
  result.right_begin = result.gap_begin + result.gap_length;
  result.right_length = config.pasted_length - result.right_begin;

  return result;
}

// Pastes left and right strings together maximizing ungapped suffix.
//
std::string CombineRight(const std::string& left, const std::string& right,
                         const PastedPartition& partition, char gap_character) {
  std::string result;
  result.reserve(partition.gap_begin + partition.gap_length
                 + partition.unknown_length + partition.right_length);
  result.append(left.data(), partition.gap_begin);
  result.append(partition.gap_length, gap_character);
  result.append(partition.unknown_length, 'N');
  result.append(right.data(), right.length());
  return result;
}

// Pastes left and right strings together maximizing ungapped prefix.
//
std::string CombineLeft(const std::string& left, const std::string& right,
                         const PastedPartition& partition, char gap_character) {
  std::string result;
  result.reserve(left.length() + partition.unknown_length + partition.gap_length
                 + partition.right_length);
  result.append(left.data(), left.length());
  result.append(partition.unknown_length, 'N');
  result.append(partition.gap_length, gap_character);
  result.append(right.data() + right.length() - partition.right_length,
                partition.right_length);

  return result;
}

// Modifies the counts of identical matches, mismatches, gap opening and gaps to
// account for pasting `other` onto the object.
//
void AdjustCounts(int& nident, int& mismatch, int& gap_open, int& gaps,
                  const Alignment& other,
                  const AlignmentConfiguration& config) {
  nident += other.Nident() - std::max(config.query_overlap,
                                      config.subject_overlap);
  mismatch += other.Mismatch() + std::min(config.query_distance,
                                          config.subject_distance);
  gap_open += other.Gapopen();
  if (config.shift > 0) {
    gap_open += 1;
  }
  gaps += other.Gaps() + config.shift;
}

// Returns end of an ungapped prefix of alignment obtained by pasting left and
// right together. In most cases, this will be the end of the maximal prefix.
//
int GetPrefixEnd(const Alignment& left, const Alignment& right,
                 const PastedPartition& partition,
                 const AlignmentConfiguration& config) {
  // Move right prefix_end/suffix_begin to corresponding positions in pasted.
  int right_prefix_end_after_pasting{config.pasted_length
                                     - right.Length()
                                     + right.UngappedPrefixEnd()};
  int right_suffix_begin_after_pasting{config.pasted_length
                                       - right.Length()
                                       + right.UngappedSuffixBegin()};

  // Find where left alignment was chopped off during pasting.
  int left_end;
  if (partition.unknown_length > 0 && partition.gap_length > 0) {
    left_end = std::min(partition.unknown_begin, partition.gap_begin);
  } else if (partition.unknown_length > 0) {
    left_end = partition.unknown_begin;
  } else if (partition.gap_length > 0) {
    left_end = partition.gap_begin;
  } else {
    left_end = partition.right_begin;
  }

  // Portion of left included in pasted has gap.
  if (left_end > left.UngappedPrefixEnd()) {

    // Maximal prefix of pasted is same as maximal prefix of left.
    return left.UngappedPrefixEnd();

  // Portion of left included in pasted is ungapped.
  } else {

    // Pasting introduces gap.
    if (config.shift != 0) {

      // The introduced gap is the left-most gap in pasted.
      return partition.gap_begin;

    // Pasting doesn't introduce gap.
    } else {

      // Portion of right included in pasted is ungapped.
      if (right_suffix_begin_after_pasting <= partition.right_begin) {
        return config.pasted_length;

      // Maximal prefix of portion of right in pasted is right's maximal prefix.
      } else if (partition.right_begin < right_prefix_end_after_pasting) {
        return right_prefix_end_after_pasting;

      // Don't know about maximal prefix of portion of right included in pasted.
      } else {
        return partition.right_begin;
      }
    }
  }
}

// Returns begin of an ungapped suffix of alignment obtained by pasting left and
// right together. In most cases, this will be the begin of the maximal suffix.
//
int GetSuffixBegin(const Alignment& left, const Alignment& right,
                   const PastedPartition& partition,
                   const AlignmentConfiguration& config) {
  // Move right prefix_end/suffix_begin to corresponding positions in pasted.
  int right_suffix_begin_after_pasting{config.pasted_length
                                       - right.Length()
                                       + right.UngappedSuffixBegin()};

  // Find where left alignment was chopped off during pasting.
  int left_end;
  if (partition.unknown_length > 0 && partition.gap_length > 0) {
    left_end = std::min(partition.unknown_begin, partition.gap_begin);
  } else if (partition.unknown_length > 0) {
    left_end = partition.unknown_begin;
  } else if (partition.gap_length > 0) {
    left_end = partition.gap_begin;
  } else {
    left_end = partition.right_begin;
  }

  // Portion of right included in pasted has gap.
  if (partition.right_begin < right_suffix_begin_after_pasting) {
    // Maximal suffix of pasted is same as maximal suffix of right.
    return right_suffix_begin_after_pasting;

  // Portion of right included in pasted is ungapped.
  } else {

    // Pasting introduces gap.
    if (config.shift != 0) {

      // The introduced gap is the right-most gap in pasted.
      return partition.gap_begin + partition.gap_length;

    // Pasting doesn't introduce gap.
    } else {

      // Portion of left included in pasted is ungapped.
      if (left_end <= left.UngappedPrefixEnd()) {
        return 0;

      // Maximal suffix of portion of left in pasted is left's maximal prefix.
      } else if (left.UngappedSuffixBegin() < left_end) {
        return left.UngappedSuffixBegin();

      // Don't know about maximal suffix of portion of left included in pasted.
      } else {
        return left_end;
      }
    }
  }
}

} // namespace

// Alignment::PasteRight
//
void Alignment::PasteRight(const Alignment& other,
                           const AlignmentConfiguration& config,
                           const ScoringSystem& scoring_system,
                           const PasteParameters& paste_parameters) {
  // Invariant sanity checks.
  assert(qseq_.length() == sseq_.length());
  assert(other.Qseq().length() == other.Sseq().length());

  // Preconditions.
  if (qstart_ >= other.Qstart()
      || qend_ >= other.Qend()
      || (plus_strand_ && (sstart_ >= other.Sstart()
                           || send_ >= other.Send()))
      || ((!plus_strand_) && (sstart_ <= other.Sstart()
                              || send_ <= other.Send()))) {
    std::stringstream error_message;
    error_message << "Invalid configuration for pasting alignment "
                  << other.DebugString() << " onto the right of alignment "
                  << DebugString() << ". (config: " << config.DebugString()
                  << ')';
    throw exceptions::PastingError(error_message.str());
  }

  // Combine sequences.
  PastedPartition partition;
  partition = GetRightPartition(config);

  // Update ungapped prefix and suffix.
  int new_ungapped_prefix_end, new_ungapped_suffix_begin;
  new_ungapped_prefix_end = GetPrefixEnd((*this), other, partition, config);
  new_ungapped_suffix_begin = GetSuffixBegin((*this), other, partition, config);

  // Deploy changes.
  if (!paste_parameters.blind_mode) {
    std::string new_qseq, new_sseq;
    char query_gap_char, subject_gap_char;
    new_qseq.reserve(config.pasted_length);
    new_sseq.reserve(config.pasted_length);

    // Add gap characters on one side and unknown on other side of gap.
    if (config.query_offset > config.subject_offset) {
      query_gap_char = 'N';
      subject_gap_char = '-';
    } else {
      query_gap_char = '-';
      subject_gap_char = 'N';
    }
    new_qseq = CombineRight(qseq_, other.Qseq(), partition, query_gap_char);
    new_sseq = CombineRight(sseq_, other.Sseq(), partition, subject_gap_char);
    qseq_ = std::move(new_qseq);
    sseq_ = std::move(new_sseq);
  }
  pasted_identifiers_.insert(pasted_identifiers_.end(),
                             other.PastedIdentifiers().begin(),
                             other.PastedIdentifiers().end());
  length_ = config.pasted_length;
  qend_ = other.Qend();
  if (plus_strand_) {
    send_ = other.Send();
  } else {
    sstart_ = other.Sstart();
  }
  ungapped_prefix_end_ = new_ungapped_prefix_end;
  ungapped_suffix_begin_ = new_ungapped_suffix_begin;

  // Adjust counts of identities, mismatches and gaps.
  AdjustCounts(nident_, mismatch_, gapopen_, gaps_, other, config);
  UpdateSimilarityMeasures(scoring_system, paste_parameters);
}

// Alignment::PasteLeft
//
void Alignment::PasteLeft(const Alignment& other,
                          const AlignmentConfiguration& config,
                          const ScoringSystem& scoring_system,
                          const PasteParameters& paste_parameters) {
  // Invariant sanity checks.
  assert(qseq_.length() == sseq_.length());
  assert(other.Qseq().length() == other.Sseq().length());

  // Preconditions.
  if (plus_strand_ != other.PlusStrand()
      || qstart_ <= other.Qstart()
      || qend_ <= other.Qend()
      || (plus_strand_ && (sstart_ <= other.Sstart()
                           || send_ <= other.Send()))
      || (!plus_strand_ && (sstart_ >= other.Sstart()
                            || send_ >= other.Send()))) {
    std::stringstream error_message;
    error_message << "Invalid configuration for pasting alignment "
                  << other.DebugString() << " onto the left of alignment "
                  << DebugString() << '.';
    throw exceptions::PastingError(error_message.str());
  } 

  // Combine sequences.
  PastedPartition partition;
  partition = GetLeftPartition(config);

  // Update ungapped prefix and suffix.
  int new_ungapped_prefix_end, new_ungapped_suffix_begin;
  new_ungapped_prefix_end = GetPrefixEnd(other, (*this), partition, config);
  new_ungapped_suffix_begin = GetSuffixBegin(other, (*this), partition, config);

  // Deploy changes.
  if (!paste_parameters.blind_mode) {
    std::string new_qseq, new_sseq;
    char query_gap_char, subject_gap_char;
    new_qseq.reserve(config.pasted_length);
    new_sseq.reserve(config.pasted_length);

    // Add gap characters on one side and unknown on other side of gap.
    if (config.query_offset > config.subject_offset) {
      query_gap_char = 'N';
      subject_gap_char = '-';
    } else {
      query_gap_char = '-';
      subject_gap_char = 'N';
    }
    new_qseq = CombineLeft(other.Qseq(), qseq_, partition, query_gap_char);
    new_sseq = CombineLeft(other.Sseq(), sseq_, partition, subject_gap_char);
    qseq_ = std::move(new_qseq);
    sseq_ = std::move(new_sseq);
  }
  pasted_identifiers_.insert(pasted_identifiers_.end(),
                             other.PastedIdentifiers().begin(),
                             other.PastedIdentifiers().end());
  length_ = config.pasted_length;
  qstart_ = other.Qstart();
  if (plus_strand_) {
    sstart_ = other.Sstart();
  } else {
    send_ = other.Send();
  }
  ungapped_prefix_end_ = new_ungapped_prefix_end;
  ungapped_suffix_begin_ = new_ungapped_suffix_begin;

  // Adjust counts of identities, mismatches and gaps.
  AdjustCounts(nident_, mismatch_, gapopen_, gaps_, other, config);
  UpdateSimilarityMeasures(scoring_system, paste_parameters);
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
          && other.length_ == length_
          && other.qseq_ == qseq_
          && other.sseq_ == sseq_
          && helpers::FuzzyFloatEquals(other.pident_, pident_)
          && helpers::FuzzyFloatEquals(other.raw_score_, raw_score_)
          && helpers::FuzzyFloatEquals(other.bitscore_, bitscore_)
          && helpers::FuzzyDoubleEquals(other.evalue_, evalue_)
          && other.include_in_output_ == include_in_output_
          && other.ungapped_suffix_begin_ == ungapped_suffix_begin_
          && other.ungapped_prefix_end_ == ungapped_prefix_end_);
}

// AlignmentConfiguration::DebugString
//
std::string AlignmentConfiguration::DebugString() const {
  std::stringstream ss;
  ss << "(query_offset=" << query_offset
     << ", query_overlap=" << query_overlap
     << ", query_distance=" << query_distance
     << ", subject_offset=" << subject_offset
     << ", subject_overlap=" << subject_overlap
     << ", subject_distance=" << subject_distance
     << ", shift=" << shift
     << ", left_length=" << left_length
     << ", right_length=" << right_length
     << ", pasted_length=" << pasted_length << ')';
  return ss.str();
}

// Alignment::DebugString
//
std::string Alignment::DebugString() const {
  std::stringstream  ss;
  ss << std::boolalpha << "(id=" << Id()
     << ", pasted_identifiers=[" << pasted_identifiers_.at(0);
  for (int i = 1; i < pasted_identifiers_.size(); ++i) {
    ss << ',' << pasted_identifiers_.at(i);
  }
  ss << "], qstart=" << qstart_
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
     << ", length=" << length_
     << ", qseq='" << qseq_
     << "', sseq='" << sseq_
     << "', pident=" << pident_
     << ", raw_score=" << raw_score_
     << ", bitscore=" << bitscore_
     << ", evalue=" << evalue_
     << ", include_in_output=" << include_in_output_
     << ", ungapped_prefix_end=" << ungapped_prefix_end_
     << ", ungapped_suffix_begin=" << ungapped_suffix_begin_
     << ')';
  return ss.str();
}

} // namespace paste_alignments