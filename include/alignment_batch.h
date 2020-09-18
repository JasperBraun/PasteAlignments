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

#ifndef PASTE_ALIGNMENTS_ALIGNMENT_BATCH_H_
#define PASTE_ALIGNMENTS_ALIGNMENT_BATCH_H_

#include <string>
#include <vector>

#include "alignment.h"
#include "helpers.h"
#include "scoring_system.h"

namespace paste_alignments {

/// @addtogroup PasteAlignments-Reference
///
/// @{

/// @brief Container for alignments between a query and a subject sequence.
///
/// @details Alignments can be accessed directly, or sorted by one of:
///  * (raw score, pident) lexicographically descending. This is done using
///    `helpers::FuzzyFloatLess` for comparison of floating points. Ties are
///    broken arbitrarily by the positions of the alignments in `Alignments()`.
///  * Query start coordinate ascending.
///  * Query end coordinate ascending.
///
/// @invariant The collections ScoreSorted, QstartSorted, and QendSorted each
///  consist precisely of the set of integers `{0, 1, ..., Size() - 1}` (as
///  second coordinate for QstartSorted and QendSorted).
///
class AlignmentBatch {
 public:
  using size_type = std::vector<Alignment>::size_type;
  /// @name Constructors:
  ///
  /// @{
  
  /// @brief Constructs object to store alignments between a query and a subject
  ///  sequence.
  ///
  /// @parameter qseqid A string-identifier for the query sequence.
  /// @parameter sseqid A string-identifier for the subject sequence.
  ///
  /// @exceptions Throws `exceptions::UnexpectedEmptyString` if `qseqid` or
  ///  `sseqid` is empty.
  ///
  AlignmentBatch(std::string_view qseqid, std::string_view sseqid)
      : qseqid_{helpers::TestNonEmpty(qseqid)},
        sseqid_{helpers::TestNonEmpty(sseqid)}
        {}
  
  /// @brief Copy constructor.
  ///
  AlignmentBatch(const AlignmentBatch& other) = default;

  /// @brief Move constructor.
  ///
  AlignmentBatch(AlignmentBatch&& other) noexcept = default;
  /// @}
  
  /// @name Assignment:
  ///
  /// @{
  
  /// @brief Copy assignment.
  ///
  AlignmentBatch& operator=(const AlignmentBatch& other) = default;

  /// @brief Move assignment.
  ///
  AlignmentBatch& operator=(AlignmentBatch&& other) noexcept = default;
  /// @}

  /// @name Accessors:
  ///
  /// @{

  /// @brief Number of alignments stored in the object.
  ///
  /// @exceptions Strong guarantee.
  ///
  inline size_type Size() const {return alignments_.size();}
  
  /// @brief Alignments stored in the object.
  ///
  /// @exceptions Strong guarantee.
  ///
  inline const std::vector<Alignment>& Alignments() const {return alignments_;}

  /// @brief Indices of stored alignments sorted by score.
  ///
  /// @details Indices refer to positions in vector returned by
  ///  `AlignmentBatch::Alignments`.
  ///
  /// @exceptions Strong guarantee.
  ///
  inline const std::vector<int>& ScoreSorted() const {return score_sorted_;}

  /// @brief Indices of stored alignments sorted by query start coordinate.
  ///
  /// @details First coordinate of each item is the query start coordinate of
  ///  the original alignment, and second coordinate refers to positions in
  ///  vector returned by `AlignmentBatch::Alignments`.
  ///
  /// @exceptions Strong guarantee.
  ///
  inline const std::vector<std::pair<int,int>>& QstartSorted() const {
    return qstart_sorted_;
  }

  /// @brief Indices of stored alignments sorted by query end coordinate.
  ///
  /// @details First coordinate of each item is the query end coordinate of the
  ///  original alignment, and second coordinate refers to positions in vector
  ///  returned by `AlignmentBatch::Alignments`.
  ///
  /// @exceptions Strong guarantee.
  ///
  inline const std::vector<std::pair<int,int>>& QendSorted() const {
    return qend_sorted_;
  }

  /// @brief String-identifier of the aligned query sequence.
  ///
  /// @exceptions Strong guarantee.
  ///
  inline const std::string& Qseqid() const {return qseqid_;}

  /// @brief String-identifier of the aligned subject sequence.
  ///
  /// @exceptions Strong guarantee.
  ///
  inline const std::string& Sseqid() const {return sseqid_;}
  /// @}

  /// @name Mutators:
  ///
  /// @{

  /// @brief Sets the query sequence string-identifier of the object to `id`.
  ///
  /// @parameter qseqid The new query sequence string-identifier.
  ///
  /// @exceptions Throws `exceptions::UnexpectedEmptyString` if `id` is
  ///  empty.
  ///
  inline void Qseqid(std::string_view id) {qseqid_ = helpers::TestNonEmpty(id);}

  /// @brief Sets the subject sequence string-identifier of the object to `id`.
  ///
  /// @parameter qseqid The new subject sequence string-identifier.
  ///
  /// @exceptions Throws `exceptions::UnexpectedEmptyString` if `id` is
  ///  empty.
  ///
  inline void Sseqid(std::string_view id) {sseqid_ = helpers::TestNonEmpty(id);}
  
  /// @brief Replaces stored alignments with contents of `alignments`.
  ///
  /// @parameter alignments The new contents of the object.
  /// @parameter parameters Additional arguments to handle floating points.
  ///
  /// @exception Strong guarantee.
  ///
  void ResetAlignments(std::vector<Alignment> alignments,
                       const PasteParameters& paste_parameters);

  /// @brief Pastes alignments in pastable configuration together.
  ///
  /// @parameter scoring_system Used to compute raw score, bitscore, and evalue
  ///  for pasted alignments.
  /// @parameter paste_parameters Contains thresholds which define what
  ///  alignment pairs count as 'pastable'.
  ///
  /// @details Attempts to extend each alignment to the left and right via
  ///  pasting. Alignments are processed in `ScoreSorted` order. Alignments
  ///  pasted onto others are not processed/pasted again. Alignments which after
  ///  pasting satisfy final thresholds are marked using the
  ///  `Alignment::IncludeInOutput` function member.
  ///
  /// @exceptions Basic guarantee. Position of pasted alignments in
  ///  `ScoreSorted`, `QstartSorted` and `QendSorted` may not agree with the
  ///  corresponding orders after execution of this function.
  ///
  void PasteAlignments(const ScoringSystem& scoring_system,
                       const PasteParameters& paste_parameters);
  /// @}

  /// @name Other:
  ///
  /// @{

  /// @brief Compares the object to `other`.
  ///
  /// @exceptions Strong guarantee.
  ///
  bool operator==(const AlignmentBatch& other) const;

  /// @brief Returns a descriptive string of the object.
  ///
  /// @exceptions Strong guarantee.
  ///
  std::string DebugString() const;
  /// @}
 private:
  std::string qseqid_;
  std::string sseqid_;
  std::vector<Alignment> alignments_;
  std::vector<int> score_sorted_;
  std::vector<std::pair<int,int>> qstart_sorted_;
  std::vector<std::pair<int,int>> qend_sorted_;
};
/// @}

} // namespace paste_alignments

#endif // PASTE_ALIGNMENTS_ALIGNMENT_BATCH_H_