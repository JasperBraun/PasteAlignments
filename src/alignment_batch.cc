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
                                     const PasteParameters& parameters) {
  std::vector<int> score_sorted, left_diff_sorted;//, right_diff_sorted;
  score_sorted.reserve(alignments.size());
  left_diff_sorted.reserve(alignments.size());
  //right_diff_sorted.reserve(alignments.size());

  for (int i = 0; i < alignments.size(); ++i) {
    score_sorted.push_back(i);
    left_diff_sorted.push_back(i);
    //right_diff_sorted.push_back(i);
  }

  std::sort(score_sorted.begin(), score_sorted.end(),
            // Sort by lexicographic key (raw score, pident), both descending.
            // Consider two floats equal according to helpers::FuzzyFloatEquals.
            // Return true in case of tie.
            [&alignments = std::as_const(alignments),
             &epsilon = std::as_const(parameters.float_epsilon)](int first,
                                                                 int second) {
              float first_score, second_score, first_pident, second_pident;
              first_score = alignments.at(first).RawScore();
              second_score = alignments.at(second).RawScore();
              first_pident = alignments.at(first).Pident();
              second_pident = alignments.at(second).Pident();
              if (helpers::FuzzyFloatEquals(
                      first_score, second_score, epsilon)) {
                if (helpers::FuzzyFloatEquals(
                        first_pident, second_pident, epsilon)
                    || first_pident > second_pident) {
                  return true;
                }
              } else if (first_score > second_score) {
                return true;
              }
              return false;
            });
  std::sort(left_diff_sorted.begin(), left_diff_sorted.end(),
            [&alignments = std::as_const(alignments)](int first, int second) {
              return (alignments.at(first).LeftDiff()
                      <= alignments.at(second).LeftDiff());
            });
/*
  std::sort(right_diff_sorted.begin(), right_diff_sorted.end(),
            [alignments](int first, int second) {
              return (alignments.at(first).RightDiff()
                      <= alignments.at(second).RightDiff());
            });
*/
  alignments_ = std::move(alignments);
  score_sorted_ = std::move(score_sorted);
  left_diff_sorted_ = std::move(left_diff_sorted);
  //right_diff_sorted_ = std::move(right_diff_sorted);
}
/*
// AlignmentBatch::PasteAlignments
//
void AlignmentBatch::PasteAlignments(const PasteParameters& parameters,
                                     const ScoringSystem& scoring_system) {
  std::vector<Alignment> pasted_alignments;
  std::unordered_set<int> was_pasted;
  Alignment intermediate_alignment;
  Configuration config;
  std::vector<int>::iterator it{score_sorted_.begin()};
  while (it != score_sorted_.end()) {
    if (!was_pasted.count(*it)) {
      was_pasted.insert(*it);
      intermediate_alignment = alignments_.at(*it);
      bounds = GetBounds(intermediate_alignment);
      sstart_pos = binary_find(sstart_sorted_, (*it));
      send_pos = binary_find(send_sorted_, (*it));

      sstart_pos_pident_score = GetNextSstart(intermediate_alignment,
                                              sstart_sorted_, sstart_pos,
                                              bounds, alignments_,
                                              parameters, scoring_system,
                                              was_pasted);
      send_pos_pident_score = GetNextSend(intermediate_alignment, send_sorted_,
                                          send_pos, bounds, alignments_,
                                          parameters, scoring_system,
                                          was_pasted);
      config = GetConfiguration(sstart_pos_pident_score, send_pos_pident_score,
                                alignments_.size());
      while (config != Configuration::kNone) {
        switch (config) {
          case Configuration::kSstart: {
            Paste(alignments_.at(sstart_pos_pident_score.pos),
                  sstart_pos_pident_score.pasted_pident,
                  sstart_pos_pident_score.pasted_score,
                  intermediate_alignment);
            if (SatisfiesThresholds(sstart_pos_pident_score.pasted_pident,
                                    sstart_pos_pident_score.pasted_score,
                                    parameters.final_pident_threshold,
                                    parameters.final_score_threshold)) {
              was_pasted.insert(sstart_pos_pident_score.pos);
              alignments_.at(*it) = intermediate_alignment;
            }
            bounds = GetBounds(intermediate_alignment);
            sstart_pos_pident_score = GetNextSstart(
                intermediate_alignment, sstart_sorted_,
                sstart_pos_pident_score.sstart_pos, bounds, alignments_,
                parameters, scoring_system);
            break;
          }
          case Configuration::kSend: {
            Paste(alignments_.at(send_pos_pident_score.pos),
                  send_pos_pident_score.pasted_pident,
                  send_pos_pident_score.pasted_score,
                  intermediate_alignment);
            paste_alignments.insert(send_pos_pident_score.pos);
            bounds = GetBounds(intermediate_alignment);
            send_pos_pident_score = GetNextSend(
                intermediate_alignment, send_sorted_,
                send_pos_pident_score.send_pos, bounds, alignments_,
                parameters, scoring_system);
            break;
          }
          default: {
            assert(false);
          }
        }
      }
    }
    ++it;
  }
}*/

// AlignmentBatch::operator==
//
bool AlignmentBatch::operator==(const AlignmentBatch& other) const {
  return (other.qseqid_ == qseqid_
          && other.sseqid_ == sseqid_
          && other.alignments_ == alignments_
          && other.score_sorted_ == score_sorted_
          && other.left_diff_sorted_ == left_diff_sorted_);
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
  ss << "], left_diff_sorted: [";
  if (!left_diff_sorted_.empty()) {
    ss << left_diff_sorted_.at(0);
    for (int i = 1; i < left_diff_sorted_.size(); ++i) {
      ss << ", " << left_diff_sorted_.at(i);
    }
  }
/*
  ss << "], right_diff_sorted: [";
  if (!right_diff_sorted_.empty()) {
    ss << right_diff_sorted_.at(0);
    for (int i = 1; i < right_diff_sorted_.size(); ++i) {
      ss << ", " << right_diff_sorted_.at(i);
    }
  }
*/
  ss << "]}";
  return ss.str();
}

}