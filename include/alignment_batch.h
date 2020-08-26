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

#include "helpers.h"

namespace paste_alignments {

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
  /// @parameter qlen The length of the query sequence.
  /// @parameter slen The length of the subject sequence.
  ///
  AlignmentBatch(std::string qseqid, std::string qsseqid, int qlen, int slen)
      : qseqid_{std::move(helpers::TestNonEmpty(qseqid))},
        sseqid_{std::move(helpers::TestNonEmpty(sseqid))},
        qlen_{helpers::TestPositive(qlen)}, slen_{helpers::TestPositive(slen)}
        {}
  
  /// @brief Copy constructor.
  ///
  AlignmentBatch(const AlignmentBatch& other) = default;

  /// @brief Move constructor.
  ///
  AlignmentBatch(AlignmentBatch&& other) = default;
  /// @}
  
  /// @name Assignment:
  ///
  /// @{
  
  /// @brief Copy assignment.
  ///
  AlignmentBatch& operator=(const AlignmentBatch& other) = default;

  /// @brief Move assignment.
  ///
  AlignmentBatch& operator=(AlignmentBatch&& other) = default;
  /// @}

  /// @name Accessors:
  ///
  /// @{

  /// @brief Number of alignments stored in the object.
  ///
  /// @exceptions Strong guarantee.
  ///
  inline size_type size() const {return alignments_.size();}
  
  /// @brief Alignments stored in the object.
  ///
  /// @exceptions Strong guarantee.
  ///
  inline const std::vector<Alignment>& alignments() const {return alignments_;}

  /// @brief Indices of stored alignments sorted by score.
  ///
  /// @details Indices refer to positions in vector returned by
  ///  `AlignmentBatch::alignments`.
  ///
  /// @exceptions Strong guarantee.
  ///
  inline const std::vector<int>& score_sorted() const {return score_sorted_;}

  /// @brief Indices of stored alignments sorted by subject start.
  ///
  /// @details Indices refer to positions in vector returned by
  ///  `AlignmentBatch::alignments`.
  ///
  /// @exceptions Strong guarantee.
  ///
  inline const std::vector<int>& sstart_sorted() const {return sstart_sorted_;}

  /// @brief Indices of stored alignments sorted by subject end.
  ///
  /// @details Indices refer to positions in vector returned by
  ///  `AlignmentBatch::alignments`.
  ///
  /// @exceptions Strong guarantee.
  ///
  inline const std::vector<int>& send_sorted() const {return send_sorted_;}

  /// @brief String-identifier of the aligned query sequence.
  ///
  /// @exceptions Strong guarantee.
  ///
  inline const std::string& qseqid() const {return qseqid_;}

  /// @brief String-identifier of the aligned subject sequence.
  ///
  /// @exceptions Strong guarantee.
  ///
  inline const std::string& sseqid() const {return sseqid_;}

  /// @brief Length of the aligned query sequence.
  ///
  /// @exceptions Strong guarantee.
  ///
  inline int qlen() const {return qlen_;}

  /// @brief Length of the aligned subject sequence.
  ///
  /// @exception Strong guarantee.
  ///
  inline int slen() const {return slen_;}
  /// @}

  /// @name Mutators:
  ///
  /// @{
  
  /// @brief Replaces stored alignments with contents of `alignments`.
  ///
  /// @parameter alignments The new contents of the object.
  /// @parameter scoring_system The scoring system by which to sort alignments.
  ///
  /// @exception Strong guarantee.
  ///
  void ResetAlignments(std::vector<Alignment> alignments,
                       const ScoringSystem& scoring_system);

  void PasteAlignments();
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
  int qlen_;
  int slen_;
  std::vector<Alignment> alignments_;
  std::vector<int> score_sorted_;
  std::vector<int> sstart_sorted_;
  std::vector<int> send_sorted_;
};

} // namespace paste_alignments

#endif // PASTE_ALIGNMENTS_ALIGNMENT_BATCH_H_