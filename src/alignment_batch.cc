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

namespace paste_alignments {

namespace {

// Returns true if `first` appears before `second` in `alignments` according to
// lexicographic key (raw score, pident), both descending. Two scores, or
// percent identities are considered the same if they are at most `epsilon`
// times the magnitude of the smaller non-zero value apart. Returns `true` if
// the two alignments tie in both score and percent identity according to radius
// `epsilon`.
//
template <const std::vector<Alignment>& alignments,
          const ScoringSystem& scoring_system>
bool ScoreComp(int first, int second, float epsilon = 0.05f) {
  float first_score, second_score, first_pident, second_pident;
  first_score = scoring_system.RawScore(alignments.at(first));
  second_score = scoring_system.RawScore(alignments.at(second));
  first_pident = alignments.at(first).pident();
  second_pident = alignments.at(second).pident();

  if (helpers::FuzzyFloatEquals(first_score, second_score, epsilon)) {
    if (helpers::FuzzyFloatEquals(first_pident, second_pident, epsilon)
        || first_pident > second_pident) {
      return true;
    }
  } else if (first_score > second_score) {
    return true;
  }
  return false;
}

// Indicates whether subject start of `first` precedes subject start of
// `second`. Returns true when tied.
//
template <const std::vector<Alignment>& alignments>
bool SstartComp(int first, int second) {
  return (alignments.at(first).sstart() <= alignments.at(second).sstart());
}

// Indicates whether subject end of `first` precedes subject end of
//  `second`. Returns true when tied.
//
template <const std::vector<Alignment>& alignments>
bool SendComp(int first, int second) {
  return (alignments.at(first).send() <= alignments.at(second).send());
}

} // namespace

// AlignmentBatch::ResetAlignments
//
void AlignmentBatch::ResetAlignments(
    std::vector<Alignment> alignments, const ScoringSystem& scoring_system) {
  std::vector<int> score_sorted, sstart_sorted, send_sorted;
  score_sorted.reserve(alignments.size());
  sstart_sorted.reserve(alignments.size());
  send_sorted.reserve(alignments.size());

  for (int i = 0; i < alignments.size(); ++i) {
    score_sorted.push_back(i);
    sstart_sorted.push_back(i);
    send_sorted.push_back(i);
  }

  std::sort(score_sorted.begin(), score_sorted.end(),
            ScoreComp<alignments, scoring_system>);
  std::sort(sstart_sorted.begin(), sstart_sorted.end(),
            SstartComp<alignments>);
  std::sort(send_sorted.begin(), send_sorted.end(),
            SendComp<alignments>);

  alignments_ = std::move(alignments);
  score_sorted_ = std::move(score_sorted);
  sstart_sorted_ = std::move(sstart_sorted);
  send_sorted_ = std::move(send_sorted);
}

// AlignmentBatch::operator==
//
bool AlignmentBatch::operator==(const AlignmentBatch& other) const {
  return (other.alignments_ == alignments_
          && other.score_sorted_ == score_sorted_
          && other.sstart_sorted_ == sstart_sorted_
          && other.send_sorted_ == send_sorted_);
}

// AlignmentBatch::DebugString.
//
std::string AlignmentBatch::DebugString() const {
  std::stringstream ss;
  ss << "{alignments: [";
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
  ss << "], sstart_sorted: [";
  if (!sstart_sorted_.empty()) {
    ss << sstart_sorted_.at(0);
    for (int i = 1; i < sstart_sorted_.size(); ++i) {
      ss << ", " << sstart_sorted_.at(i);
    }
  }
  ss << "], send_sorted: [";
  if (!send_sorted_.empty()) {
    ss << send_sorted_.at(0);
    for (int i = 1; i < send_sorted_.size(); ++i) {
      ss << ", " << send_sorted_.at(i);
    }
  }
  ss << "]}";
  return ss.str();
}

}