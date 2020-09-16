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

#include "alignment_batch.h"

#include <algorithm>
#include <functional>
#include <unordered_set>

namespace paste_alignments {

// AlignmentBatch::ResetAlignments
//
void AlignmentBatch::ResetAlignments(std::vector<Alignment> alignments,
                                     const PasteParameters& paste_parameters) {
  std::vector<int> score_sorted, qstart_sorted, qend_sorted;
  score_sorted.reserve(alignments.size());
  qstart_sorted.reserve(alignments.size());
  qend_sorted.reserve(alignments.size());

  for (int i = 0; i < alignments.size(); ++i) {
    score_sorted.push_back(i);
    qstart_sorted.push_back(i);
    qend_sorted.push_back(i);
  }

  std::sort(score_sorted.begin(), score_sorted.end(),
            // Sort by lexicographic key (raw score, pident, id), both
            // descending. Consider two floats equal according to
            // helpers::FuzzyFloatEquals.
            [&alignments = std::as_const(alignments),
             &epsilon = std::as_const(paste_parameters.float_epsilon)](
                int first, int second) {
              float first_score, second_score, first_pident, second_pident;
              first_score = alignments.at(first).RawScore();
              second_score = alignments.at(second).RawScore();
              first_pident = alignments.at(first).Pident();
              second_pident = alignments.at(second).Pident();
              if (helpers::FuzzyFloatEquals(
                      first_score, second_score, epsilon)) {
                if (helpers::FuzzyFloatEquals(
                        first_pident, second_pident, epsilon)) {
                  return first < second;
                } else if (first_pident > second_pident) {
                  return true;
                }
              } else if (first_score > second_score) {
                return true;
              }
              return false;
            });
  std::sort(qstart_sorted.begin(), qstart_sorted.end(),
            [&alignments = std::as_const(alignments)](int first, int second) {
              return (alignments.at(first).Qstart()
                      < alignments.at(second).Qstart());
            });
  std::sort(qend_sorted.begin(), qend_sorted.end(),
            [alignments](int first, int second) {
              return (alignments.at(first).Qend()
                      < alignments.at(second).Qend());
            });

  alignments_ = std::move(alignments);
  score_sorted_ = std::move(score_sorted);
  qstart_sorted_ = std::move(qstart_sorted);
  qend_sorted_ = std::move(qend_sorted);
}

// Helper functions for AlignmentBatch::PasteAlignments
//
namespace {

// Information relevant for potential candidates for pasting.
//
struct PasteCandidate {

  // Position in either QstartSorted, or QendSorted.
  //
  int sorted_pos{-1};

  // Position in Alignments.
  //
  int alignment_pos;

  // Configuration of candidate with reference alignment.
  //
  AlignmentConfiguration config;

  // Pasted percent identity.
  //
  float pident;

  // Pasted raw score.
  //
  float score;
};

// Counts the types of matches and number of gaps in an alignment.
//
struct MatchCounts {

  // Number of identical matches.
  //
  int nident;

  // Number of mismatches.
  //
  int mismatch;

  // Number of gap openings.
  //
  int gapopen;

  // Total number of gaps.
  //
  int gaps;
};

// Assumes qend_sorted is not empty, each number in qend_sorted is in the range
// [0, alignments.size()), and qend == alignments.at(i).Qend(), for at least one
// i in qend_sorted.
//
int FindFirstLessQend(int qend, const std::vector<int>& qend_sorted,
                      const std::vector<Alignment>& alignments) {
  assert(!qend_sorted.empty());
  if (qend_sorted.size() == 1) {
    assert(alignments.at(qend_sorted.at(0)).Qend() == qend);
    return -1;
  }

  int left{0}, right{static_cast<int>(qend_sorted.size())}, median;
  do {
    median = left + (right - left) / 2;
    if (alignments.at(qend_sorted.at(median)).Qend() > qend) {
      assert(median > 0); // may fail if qend not in qend_sorted
      right = median;
    } else if (alignments.at(qend_sorted.at(median)).Qend() < qend) {
      assert(median < qend_sorted.size() - 1); // may fail if qend not in qend_sorted
      left = median;
    }
  } while (alignments.at(qend_sorted.at(median)).Qend() != qend);
  while (median > 0 && alignments.at(qend_sorted.at(median)).Qend() == qend) {
    --median;
  }
  return median;
}

// Assumes qstart_sorted is not empty, each number in qstart_sorted is in the
// range [0, alignments.size()), and qstart == alignments.at(i).Qstart(), for at
// least one i in qstart_sorted.
//
int FindFirstGreaterQstart(int qstart, const std::vector<int>& qstart_sorted,
                           const std::vector<Alignment>& alignments) {
  assert(!qstart_sorted.empty());
  if (qstart_sorted.size() == 1) {
    assert(alignments.at(qstart_sorted.at(0)).Qstart() == qstart);
    return -1;
  }

  int left{0}, right{static_cast<int>(qstart_sorted.size())}, median;
  do {
    median = left + (right - left) / 2;
    if (alignments.at(qstart_sorted.at(median)).Qstart() > qstart) {
      assert(median > 0); // may fail if qstart not in qstart_sorted
      right = median;
    } else if (alignments.at(qstart_sorted.at(median)).Qstart() < qstart) {
      assert(median < qstart_sorted.size() - 1); // may fail if qstart not in qstart_sorted
      left = median;
    }
  } while (alignments.at(qstart_sorted.at(median)).Qstart() != qstart);
  while (median < qstart_sorted.size()
         && alignments.at(qstart_sorted.at(median)).Qstart() == qstart) {
    ++median;
  }
  if (median == qstart_sorted.size()) {
    return -1;
  } else {
    return median;
  }
}

// When an alignment is further than this bound in query or subject to
// `alignment`, then the two cannot be pasted together.
//
int GetDistanceBound(const Alignment& alignment,
                     const ScoringSystem& scoring_system,
                     const PasteParameters& paste_parameters) {
  return ((alignment.RawScore() / scoring_system.Penalty())
          + static_cast<float>(paste_parameters.gap_tolerance));
}

// Indicates whether `first` is the better candidate for pasting.
//
bool BetterCandidate(const PasteCandidate& first,
                     const PasteCandidate& second,
                     const PasteParameters& parameters) {
  assert(first.sorted_pos != -1 || second.sorted_pos != -1);
  if (first.sorted_pos == -1) {return false;}
  if (second.sorted_pos == -1) {return true;}
  bool first_final, second_final;
  first_final = helpers::SatisfiesThresholds(
      first.pident, first.score,
      parameters.final_pident_threshold, parameters.final_score_threshold,
      parameters.float_epsilon);
  second_final = helpers::SatisfiesThresholds(
      second.pident, second.score,
      parameters.final_pident_threshold, parameters.final_score_threshold,
      parameters.float_epsilon);
  if (first_final && !second_final) {
    return true;
  } else if (second_final && !first_final) {
    return false;
  } else if (helpers::FuzzyFloatEquals(first.score, second.score,
                                       parameters.float_epsilon)) {
    if (helpers::FuzzyFloatEquals(first.pident, second.pident,
                                  parameters.float_epsilon)) {
      return first.alignment_pos < second.alignment_pos;
    } else if (first.pident > second.pident) {
      return true;
    }
  } else if (first.score > second.score) {
    return true;
  }
  return false;
}

// Obtains `AlignmentConfiguration` object for `left` and `right`.
//
AlignmentConfiguration GetConfiguration(const Alignment& left,
                                        const Alignment& right) {
  assert(left.PlusStrand() == right.PlusStrand());
  AlignmentConfiguration config;

  config.query_offset = right.Qstart() - left.Qend() - 1;
  if (left.PlusStrand()) {
    config.subject_offset = right.Sstart() - left.Send() - 1;
  } else {
    config.subject_offset = left.Sstart() - right.Send() - 1;
  }

  config.query_overlap = std::abs(std::min(0, config.query_offset));
  config.query_distance = std::max(0, config.query_offset);

  config.subject_overlap = std::abs(std::min(0, config.subject_offset));
  config.subject_distance = std::max(0, config.subject_offset);

  config.shift = std::abs(config.query_offset - config.subject_offset);
  config.left_length = left.Length();
  config.right_length = right.Length();
  config.pasted_length = config.left_length + config.right_length
                         + std::max(config.query_offset, config.subject_offset);
  return config;
}

// Gets correct nident, mismatch, gapopen, and gaps counts for the alignment
// otained by pasting `first` and `second`.
//
MatchCounts GetCounts(const Alignment& first, const Alignment& second,
                      const AlignmentConfiguration& config) {
  MatchCounts result;

  result.nident = first.Nident() + second.Nident()
                  - std::max(config.query_overlap, config.subject_overlap);
  result.mismatch = first.Mismatch() + second.Mismatch()
                    + std::min(config.query_distance, config.subject_distance);
  result.gapopen = first.Gapopen() + second.Gapopen();
  if (config.shift > 0) {
    result.gapopen += 1;
  }
  result.gaps = first.Gaps() + second.Gaps() + config.shift;

  return result;
}

// Searches for next pastable alignment to the left of `alignment `in query.
// Assumes that `candidate_sorted_pos` is in the range [-1, qend_sorted.size()).
//
PasteCandidate FindLeftCandidate(int candidate_sorted_pos,
                                 const Alignment& alignment,
                                 int distance_bound,
                                 const std::vector<int>& qend_sorted,
                                 const std::vector<Alignment>& alignments,
                                 const std::unordered_set<int>& used,
                                 const ScoringSystem& scoring_system,
                                 const PasteParameters& paste_parameters) {
  assert(-1 <= candidate_sorted_pos);
  assert(candidate_sorted_pos < static_cast<int>(qend_sorted.size()));
  int result_distance, result_qstart, max_overlap;
  MatchCounts counts;
  bool result_plus_strand;
  PasteCandidate result;
  result.sorted_pos = candidate_sorted_pos;
  if (result.sorted_pos == -1) {
    result.sorted_pos = FindFirstLessQend(
        alignment.Qend(), qend_sorted, alignments);
  }

  while (result.sorted_pos != -1) {
    result.alignment_pos = qend_sorted.at(result.sorted_pos);
    result_distance = alignment.Qstart()
                      - alignments.at(result.alignment_pos).Qend()
                      - 1;
    result_qstart = alignments.at(result.alignment_pos).Qstart();
    result_plus_strand = alignments.at(result.alignment_pos).PlusStrand();

    if (result_distance > distance_bound) {
      result.sorted_pos = -1;
    } else if (alignment.PlusStrand() == result_plus_strand
               && result_qstart < alignment.Qstart()
               && !used.count(result.alignment_pos)) {
      result.config = GetConfiguration(alignments.at(result.alignment_pos),
                                       alignment);
      max_overlap = std::max(result.config.query_overlap,
                             result.config.subject_overlap);
      if (result.config.shift <= paste_parameters.gap_tolerance
          && max_overlap < alignment.UngappedPrefixEnd()) {
        counts = GetCounts(alignment, alignments.at(result.alignment_pos),
                           result.config);
        result.pident = helpers::Percentage(counts.nident,
                                            result.config.pasted_length);
        result.score = scoring_system.RawScore(counts.nident, counts.mismatch,
                                               counts.gapopen, counts.gaps);
        if (helpers::SatisfiesThresholds(
                result.pident, result.score,
                paste_parameters.intermediate_pident_threshold,
                paste_parameters.intermediate_score_threshold,
                paste_parameters.float_epsilon)) {
          break;
        }
      }
      result.sorted_pos -= 1;
    } else {
      result.sorted_pos -= 1;
    }
  }
  return result;
}

// Searches for next pastable alignment to the right of `alignment `in query.
// Assumes that `candidate_sorted_pos` is in the range
// [-1, qstart_sorted.size()).
//
PasteCandidate FindRightCandidate(int candidate_sorted_pos,
                                  const Alignment& alignment,
                                  int distance_bound,
                                  const std::vector<int>& qstart_sorted,
                                  const std::vector<Alignment>& alignments,
                                  const std::unordered_set<int>& used,
                                  const ScoringSystem& scoring_system,
                                  const PasteParameters& paste_parameters) {
  assert(-1 <= candidate_sorted_pos);
  assert(candidate_sorted_pos < static_cast<int>(qstart_sorted.size()));
  int result_distance, result_qend, max_overlap, alignment_suffix_length;
  MatchCounts counts;
  bool result_plus_strand;
  PasteCandidate result;
  result.sorted_pos = candidate_sorted_pos;
  if (result.sorted_pos == -1) {
    result.sorted_pos = FindFirstGreaterQstart(
        alignment.Qstart(), qstart_sorted, alignments);
  }

  while (result.sorted_pos != -1) {
    result.alignment_pos = qstart_sorted.at(result.sorted_pos);
    result_distance = alignments.at(result.alignment_pos).Qstart()
                      - alignment.Qend()
                      - 1;
    result_qend = alignments.at(result.alignment_pos).Qend();
    result_plus_strand = alignments.at(result.alignment_pos).PlusStrand();
    if (result_distance > distance_bound) {
      result.sorted_pos = -1;
    } else if (alignment.PlusStrand() == result_plus_strand
               && alignment.Qend() < result_qend
               && !used.count(result.alignment_pos)) {
      result.config = GetConfiguration(alignment,
                                       alignments.at(result.alignment_pos));
      max_overlap = std::max(result.config.query_overlap,
                             result.config.subject_overlap);
      alignment_suffix_length = alignment.Length()
                                - alignment.UngappedSuffixBegin();
      if (result.config.shift <= paste_parameters.gap_tolerance
          && max_overlap < alignment_suffix_length) {
        counts = GetCounts(alignment, alignments.at(result.alignment_pos),
                           result.config);
        result.pident = helpers::Percentage(counts.nident,
                                            result.config.pasted_length);
        result.score = scoring_system.RawScore(counts.nident, counts.mismatch,
                                               counts.gapopen, counts.gaps);
        if (helpers::SatisfiesThresholds(
                result.pident, result.score,
                paste_parameters.intermediate_pident_threshold,
                paste_parameters.intermediate_score_threshold,
                paste_parameters.float_epsilon)) {
          break;
        }
      }
      result.sorted_pos += 1;
    } else {
      result.sorted_pos += 1;
    }
    if (result.sorted_pos == qstart_sorted.size()) {
      result.sorted_pos = -1;
    }
  }
  return result;
}

} // namespace

// AlignmentBatch::PasteAlignments
//
void AlignmentBatch::PasteAlignments(const ScoringSystem& scoring_system,
                                     const PasteParameters& paste_parameters) {
  assert(alignments_.size() == Size());
  assert(score_sorted_.size() == Size());
  assert(qstart_sorted_.size() == Size());
  assert(qend_sorted_.size() == Size());

  if (alignments_.empty()) {return;}
  std::unordered_set<int> used, temp_used;
  PasteCandidate left_candidate, right_candidate;
  int query_distance_bound;

  for (int i : score_sorted_) {
    if (!used.count(i)) {

      // Initialize search parameters.
      used.insert(i);
      temp_used.clear();
      Alignment current{alignments_.at(i)};
      query_distance_bound = GetDistanceBound(current, scoring_system,
                                              paste_parameters);
      left_candidate = FindLeftCandidate(left_candidate.sorted_pos, current,
                                         query_distance_bound,
                                         qend_sorted_, alignments_, used,
                                         scoring_system, paste_parameters);
      right_candidate = FindRightCandidate(right_candidate.sorted_pos, current,
                                           query_distance_bound, qstart_sorted_,
                                           alignments_, used, scoring_system,
                                           paste_parameters);

      // Begin search left and right.
      while (left_candidate.sorted_pos != -1
             || right_candidate.sorted_pos != -1) {

        // Prefer pasting more promising candidate.
        if (BetterCandidate(left_candidate, right_candidate,
                            paste_parameters)) {
          current.PasteLeft(alignments_.at(left_candidate.alignment_pos),
                            left_candidate.config, scoring_system,
                            paste_parameters);
          temp_used.insert(left_candidate.alignment_pos);
          left_candidate.sorted_pos -= 1;
        } else {
          current.PasteRight(alignments_.at(right_candidate.alignment_pos),
                             right_candidate.config, scoring_system,
                             paste_parameters);
          temp_used.insert(right_candidate.alignment_pos);
          right_candidate.sorted_pos += 1;
          if (right_candidate.sorted_pos == Size()) {
            right_candidate.sorted_pos = -1;
          }
        }

        // Make accumulated temporary pastes permanent if final thresholds met.
        if (current.SatisfiesThresholds(paste_parameters.final_pident_threshold,
                                        paste_parameters.final_score_threshold,
                                        paste_parameters)) {
          alignments_.at(i) = current;
          used.merge(temp_used);
        }

        // Adjust search parameters.
        query_distance_bound = GetDistanceBound(current, scoring_system,
                                                paste_parameters);
        if (left_candidate.sorted_pos != -1) {
          left_candidate = FindLeftCandidate(left_candidate.sorted_pos, current,
                                             query_distance_bound, qend_sorted_,
                                             alignments_, used, scoring_system,
                                             paste_parameters);
        }
        if (right_candidate.sorted_pos != -1) {
          right_candidate = FindRightCandidate(right_candidate.sorted_pos,
                                               current, query_distance_bound,
                                               qstart_sorted_, alignments_,
                                               used, scoring_system,
                                               paste_parameters);
        }
      }

      // Update whether or not alignment is to be included in output.
      alignments_.at(i).IncludeInOutput(alignments_.at(i).SatisfiesThresholds(
          paste_parameters.final_pident_threshold,
          paste_parameters.final_score_threshold,
          paste_parameters));
    }
  }
}

// AlignmentBatch::operator==
//
bool AlignmentBatch::operator==(const AlignmentBatch& other) const {
  return (other.qseqid_ == qseqid_
          && other.sseqid_ == sseqid_
          && other.alignments_ == alignments_
          && other.score_sorted_ == score_sorted_
          && other.qstart_sorted_ == qstart_sorted_
          && other.qend_sorted_ == qend_sorted_);
}

// AlignmentBatch::DebugString.
//
std::string AlignmentBatch::DebugString() const {
  std::stringstream ss;
  ss << "{qseqid: " << qseqid_ << ", sseqid: " << sseqid_
     << ", alignments: [";
  if (!alignments_.empty()) {
    ss << alignments_.at(0).DebugString();
    for (int i = 1; i < alignments_.size(); ++i) {
      ss << ", " << alignments_.at(i).DebugString();
    }
  }
  ss << "], score_sorted: [";
  if (!score_sorted_.empty()) {
    ss << score_sorted_.at(0);
    for (int i = 1; i < score_sorted_.size(); ++i) {
      ss << ", " << score_sorted_.at(i);
    }
  }
  ss << "], qstart_sorted_: [";
  if (!qstart_sorted_.empty()) {
    ss << qstart_sorted_.at(0);
    for (int i = 1; i < qstart_sorted_.size(); ++i) {
      ss << ", " << qstart_sorted_.at(i);
    }
  }

  ss << "], qend_sorted_: [";
  if (!qend_sorted_.empty()) {
    ss << qend_sorted_.at(0);
    for (int i = 1; i < qend_sorted_.size(); ++i) {
      ss << ", " << qend_sorted_.at(i);
    }
  }

  ss << "]}";
  return ss.str();
}

}