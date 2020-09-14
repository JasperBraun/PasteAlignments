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

#ifndef PASTE_ALIGNMENTS_ALIGNMENT_H_
#define PASTE_ALIGNMENTS_ALIGNMENT_H_

#include <string>
#include <vector>

#include "helpers.h"
#include "paste_parameters.h"
#include "scoring_system.h"

namespace paste_alignments {

/// @addtogroup PasteAlignments-Reference
///
/// @{

/// @brief Describes the relative positions of two alignments.
///
/// @details Assumes that neither aligned query nor aligned subject region of
///  one is contained in the other and that
///
struct AlignmentConfiguration {

  /// @brief Offset between aligned query regions.
  ///
  int query_offset;

  /// @brief Amount of overlap between aligned query regions.
  ///
  int query_overlap;

  /// @brief Distance between aligned query regions.
  ///
  int query_distance;

  /// @brief Offset between aligned subject regions.
  ///
  int subject_offset;

  /// @brief Amount of overlap between aligned subject regions.
  ///
  int subject_overlap;

  /// @brief Distance between aligned subject regions.
  ///
  int subject_distance;

  /// @brief Shift between the query and subject offsets.
  ///
  int shift;

  /// @brief Length of left alignment.
  ///
  int left_length;

  /// @brief Length of right alignment.
  ///
  int right_length;

  /// @brief Length of pasted alignment.
  ///
  int pasted_length;

  /// @brief Returns a descriptive string of the object.
  ///
  /// @exceptions Strong guarantee.
  ///
  std::string DebugString() const;
};

/// @brief Contains data relevant for a sequence alignment.
///
/// @invariant All integral data members are non-negative.
/// @invariant `Qstart <= Qend`, and `Sstart <= Send`.
/// @invariant `Qlen` and `Slen` are positive.
/// @invariant `Qseq` and `Sseq` are non-empty.
/// @invariatn `Qseq` and `Sseq` have the same length.
/// @invariant `PastedIdentifiers` contains `Id`.
///
class Alignment {
 public:
  /// @name Factories:
  ///
  /// @{
  
  /// @brief Creates an `Alignment` from string representations of field values.
  ///
  /// @parameter id Identifier assigned to the object.
  /// @parameter fields String fields containing the alignment data.
  /// @parameter scoring_system Scoring system used to compute score, bitscore,
  ///  and evalue.
  /// @parameter paste_parameters Additional arguments used for handling
  ///  floating points.
  ///
  /// @details `fields` values are interpreted in the order:
  ///  qstart qend sstart send nident mismatch gapopen gaps qlen slen qseq sseq.
  ///  The object is considered to be on the minus strand if it's subject end
  ///  coordinate precedes its subject start coordinate. Fields in excess of 12
  ///  are ignored.
  ///
  /// @exceptions Strong guarantee. Throws `exceptions::ParsingError` if
  ///  * Fewer than 12 fields are provided.
  ///  * One of the fields, except qseq and sseq cannot be converted to integer.
  ///  * qstart is larger than qend coordinate.
  ///  * One of qstart, qend, sstart, or send is negative.
  ///  * One of nident, mismatch, gapopen, or gaps is negative.
  ///  * One of qlen, or slen is non-positive.
  ///  * One of qseq, or sseq is empty.
  ///  * Length of qseq is not the same as length of sseq.
  ///
  static Alignment FromStringFields(int id,
                                    std::vector<std::string_view> fields,
                                    const ScoringSystem& scoring_system,
                                    const PasteParameters& paste_parameters);
  /// @}

  /// @name Constructors:
  ///
  /// @{
  
  /// @brief Copy constructor.
  ///
  Alignment(const Alignment& other) = default;

  /// @brief Move constructor.
  ///
  Alignment(Alignment&& other) noexcept = default;
  /// @}
  
  /// @name Assignment:
  ///
  /// @{
  
  /// @brief Copy assignment.
  ///
  Alignment& operator=(const Alignment& other) = default;

  /// @brief Move assignment.
  ///
  Alignment& operator=(Alignment&& other) noexcept = default;
  /// @}

  /// @name Accessors:
  ///
  /// @{

  /// @brief Object's id.
  ///
  /// @exceptions Strong guarantee.
  ///
  inline int Id() const {return pasted_identifiers_.at(0);}

  /// @brief Identifiers of alignments that pasted together to make this object.
  ///
  /// @exceptions Strong guarantee.
  ///
  inline const std::vector<int>& PastedIdentifiers() const {
    return pasted_identifiers_;
  }

  /// @brief Query starting coordinate.
  ///
  /// @exceptions Strong guarantee.
  ///
  inline int Qstart() const {return qstart_;}

  /// @brief Query ending coordinate.
  ///
  /// @exceptions Strong guarantee.
  ///
  inline int Qend() const {return qend_;}

  /// @brief Subject starting coordinate.
  ///
  /// @exceptions Strong guarantee.
  ///
  inline int Sstart() const {return sstart_;}

  /// @brief Subject ending coordinate.
  ///
  /// @exceptions Strong guarantee.
  ///
  inline int Send() const {return send_;}

  /// @brief Indicates if alignment is on plus strand of subject.
  ///
  /// @exceptions Strong guarantee.
  ///
  inline bool PlusStrand() const {return plus_strand_;}

  /// @brief Number of identical matches.
  ///
  /// @exceptions Strong guarantee.
  ///
  inline int Nident() const {return nident_;}

  /// @brief Number of mismatches.
  ///
  /// @exceptions Strong guarantee.
  ///
  inline int Mismatch() const {return mismatch_;}

  /// @brief Number of gap openings.
  ///
  /// @exceptions Strong guarantee.
  ///
  inline int Gapopen() const {return gapopen_;}

  /// @brief Total number of gaps.
  ///
  /// @exceptions Strong guarantee.
  ///
  inline int Gaps() const {return gaps_;}

  /// @brief Length of query sequence.
  ///
  /// @exceptions Strong guarantee.
  ///
  inline int Qlen() const {return qlen_;}

  /// @brief Length of subject sequence.
  ///
  /// @exceptions Strong guarantee.
  ///
  inline int Slen() const {return slen_;}

  /// @brief Query part of the sequence alignment.
  ///
  /// @exceptions Strong guarantee.
  ///
  inline const std::string& Qseq() const {return qseq_;}

  /// @brief Subject part of the sequence alignment.
  ///
  /// @exceptions Strong guarantee.
  ///
  inline const std::string& Sseq() const {return sseq_;}

  /// @brief Length of the alignment.
  ///
  /// @exceptions Strong guarantee.
  ///
  inline int Length() const {return qseq_.length();}

  /// @brief Alignment's percent identity.
  ///
  /// @exceptions Strong guarantee.
  ///
  inline float Pident() const {
    return pident_;
  }

  /// @brief Alignment's score.
  ///
  /// @exceptions Strong guarantee.
  ///
  inline float RawScore() const {
    return raw_score_;
  }

  /// @brief Alignment's bitscore.
  ///
  /// @exception Strong guarantee.
  ///
  inline float Bitscore() const {return bitscore_;}

  /// @brief Alignment's evalue.
  ///
  /// @exception Strong guarantee.
  ///
  inline double Evalue() const {return evalue_;}

  /// @brief Indicates whether alignment is flagged to be included in output.
  ///
  /// @exception Strong guarantee.
  ///
  inline bool IncludeInOutput() const {return include_in_output_;}

  /// @brief Position one-past-the-last aligned pair of maximal ungapped prefix.
  ///
  /// @details Prefix in query corresponds to suffix in subject if object is
  ///  negatively oriented.
  ///
  /// @exception Strong guarantee.
  ///
  inline int UngappedPrefixEnd() const {return ungapped_prefix_end_;}

  /// @brief Position of first aligned pair of maximal ungapped suffix.
  ///
  /// @details Suffix in query corresponds to prefix in subject if object is
  ///  negatively oriented.
  ///
  /// @exception Strong guarantee.
  ///
  inline int UngappedSuffixBegin() const {return ungapped_suffix_begin_;}

  /// @brief Indicates whether alignment satisfies both quality thesholds.
  ///
  /// @parameter pident_threshold Minimum percent identity alignment must have.
  /// @parameter score_threshold Minimum raw score alignment must have.
  /// @parameter parameters Additional arguments used for handling floating
  ///  points.
  ///
  /// @details Returns true if for both thresholds, the alignment's value is at
  ///  least as big as the thresholds minus `epsilon` times the magnitude of the
  ///  smaller non-zero value of the two (alignment's actual value, and
  ///  threshold).
  ///
  /// @exception Strong guarantee.
  ///
  inline bool SatisfiesThresholds(float pident_threshold, float score_threshold,
                                  const PasteParameters& parameters) const {
    return helpers::SatisfiesThresholds(pident_, raw_score_,
                                        pident_threshold, score_threshold,
                                        parameters.float_epsilon);
  }

  /// @brief Difference of start coordinates.
  ///
  /// @exceptions Strong guarantee.
  ///
  inline int LeftDiff() const {return (qstart_ - sstart_);}

  /// @brief Difference of end coordinates.
  ///
  /// @exceptions Strong guarantee.
  ///
  inline int RightDiff() const {return (qend_ - send_);}
  /// @}

  /// @name Mutators:
  ///
  /// @{

  /// @brief Sets position one-past-the-last aligned pair of maximal ungapped
  ///  prefix.
  ///
  /// @details Prefix in query corresponds to suffix in subject if object is
  ///  negatively oriented.
  ///
  /// @exception Strong guarantee.
  ///
  inline void UngappedPrefixEnd(int value) {ungapped_prefix_end_ = value;}

  /// @brief Sets position of first aligned pair of maximal ungapped suffix.
  ///
  /// @details Suffix in query corresponds to prefix in subject if object is
  ///  negatively oriented.
  ///
  /// @exception Strong guarantee.
  ///
  inline void UngappedSuffixBegin(int value) {ungapped_suffix_begin_ = value;}
  
  /// @brief Sets flag for alignment to be included in output.
  ///
  /// @exceptions Strong guarantee.
  ///
  inline void IncludeInOutput(bool value) {include_in_output_ = value;}

  /// @brief (Re-)computes percent identity, raw score, bitscore, and evalue.
  ///
  /// @parameter scoring_system Scoring system used to compute score, bitscore,
  ///  and evalue.
  /// @parameter paste_parameters Additional arguments used for handling
  ///  floating points.
  ///
  /// @exceptions Basic guarantee.
  ///
  inline void UpdateSimilarityMeasures(
      const ScoringSystem& scoring_system,
      const PasteParameters& paste_parameters) {
    pident_ = helpers::Percentage(nident_, Length());
    raw_score_ = scoring_system.RawScore(nident_, mismatch_, gapopen_, gaps_);
    bitscore_ = scoring_system.Bitscore(raw_score_, paste_parameters);
    evalue_ = scoring_system.Evalue(raw_score_, qlen_, paste_parameters);
  }

  /// @brief Pastes another alignment onto the right of the object.
  ///
  /// @parameter other The alignment to paste onto the right.
  /// @parameter config The configuration describing the relative position of
  ///  the two alignments.
  /// @parameter scoring_system Scoring system used to compute score, bitscore,
  ///  and evalue.
  /// @parameter paste_parameters Additional arguments used for handling
  ///  floating points.
  ///
  /// @exceptions Basic guarantee. Throws `exceptions::PastingError` if
  ///  * The two alignments are not on the same strand.
  ///  * One of the two alignments' query or subject regions is contained in the
  ///    other's.
  ///  * `other` does not start after (or end after) this object in query.
  ///  * `other` does not start after (or end after) this object in subject and
  ///    the two alignments are on the plus strand.
  ///  * `other` does not start before (or end before) this object in subject
  ///    and the two alignments are on the minus strand.
  ///
  void PasteRight(const Alignment& other, const AlignmentConfiguration& config,
                  const ScoringSystem& scoring_system,
                  const PasteParameters& paste_parameters);

  /// @brief Pastes another alignment onto the left (in query) of the object.
  ///
  /// @parameter other The alignment to paste onto the left.
  /// @parameter config The configuration describing the relative position of
  ///  the two alignments.
  /// @parameter scoring_system Scoring system used to compute score, bitscore,
  ///  and evalue.
  /// @parameter paste_parameters Additional arguments used for handling
  ///  floating points.
  ///
  /// @exceptions Basic guarantee. Throws `exceptions::PastingError` if
  ///  * The two alignments are not on the same strand.
  ///  * One of the two alignments' query or subject regions is contained in the
  ///    other's.
  ///  * `other` does not start before (or end before) this object in query.
  ///  * `other` does not start before (or end before) this object in subject
  ///    and the two alignments are on the plus strand.
  ///  * `other does not start after (or end after) this object in subject and
  ///    the two alignments are on the minus strand.
  ///
  void PasteLeft(const Alignment& other, const AlignmentConfiguration& config,
                 const ScoringSystem& scoring_system,
                 const PasteParameters& paste_parameters);
  /// @}

  /// @name Other:
  ///
  /// @{

  /// @brief Compares the object to `other`.
  ///
  /// @exceptions Strong guarantee.
  ///
  bool operator==(const Alignment& other) const;

  /// @brief Returns a descriptive string of the object.
  ///
  /// @exceptions Strong guarantee.
  ///
  std::string DebugString() const;
  /// @}

 private:
  /// @brief Private constructor to force creation by factory.
  ///
  Alignment(int id) : pasted_identifiers_{id} {}

  std::vector<int> pasted_identifiers_;
  int qstart_;
  int qend_;
  int sstart_;
  int send_;
  bool plus_strand_;
  int nident_;
  int mismatch_;
  int gapopen_;
  int gaps_;
  int qlen_;
  int slen_;
  std::string qseq_;
  std::string sseq_;
  float pident_;
  float raw_score_;
  float bitscore_;
  double evalue_;
  bool include_in_output_{false};
  int ungapped_prefix_end_;
  int ungapped_suffix_begin_;
};
/// @}

} // namespace paste_alignments

#endif // PASTE_ALIGNMENTS_ALIGNMENT_H_