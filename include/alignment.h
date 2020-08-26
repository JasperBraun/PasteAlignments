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

namespace paste_alignments {

/// @brief Contains data relevant for a sequence alignment.
///
/// @invariant All integral data members are non-negative.
/// @invariant `qstart <= qend`, and `sstart <= send`.
/// @invariant `qlen` and `slen` are positive.
/// @invariant `qseq` and `sseq` are non-empty.
///
class Alignment {
 public:
  /// @name Factories:
  ///
  /// @{
  
  /// @brief Creates an `Alignment` from string representations of field values.
  ///
  /// @details Object is assigned provided `id`. `fields` values are interpreted
  ///  in the order:
  ///  qstart qend sstart send nident mismatch gapopen gaps qlen slen qseq sseq.
  ///  The object is considered to be on the minus strand if it's subject end
  ///  coordinate precedes its subject start coordinate. Fields in excess of 12
  ///  are ignored.
  ///
  /// @exceptions Strong guarantee. Throws `exceptions::ParsingError` if
  ///  * Fewer than 12 fields are provided
  ///  * One of the fields, except qseq and sseq cannot be converted to integer
  ///  * qstart is larger than qend coordinate
  ///  * One of qstart, qend, sstart, or send is negative
  ///  * One of nident, mismatch, gapopen, or gaps is negative
  ///  * One of qlen, or slen is non-positive
  ///  * One of qseq, or sseq is empty
  ///
  static Alignment FromStringFields(int id,
                                    std::vector<std::string_view> fields);
  /// @}

  /// @name Constructors:
  ///
  /// @{
  
  /// @brief Copy constructor.
  ///
  Alignment(const Alignment& other) = default;

  /// @brief Move constructor.
  ///
  Alignment(Alignment&& other) = default;
  /// @}
  
  /// @name Assignment:
  ///
  /// @{
  
  /// @brief Copy assignment.
  ///
  Alignment& operator=(const Alignment& other) = default;

  /// @brief Move assignment.
  ///
  Alignment& operator=(Alignment&& other) = default;
  /// @}

  /// @name Accessors:
  ///
  /// @{

  /// @brief Object's id.
  ///
  /// @exceptions Strong guarantee.
  ///
  inline int Id() const {return id_;}

  /// @brief Query starting coordinate.
  ///
  /// @exceptions Strong guarantee.
  ///
  inline int qstart() const {return qstart_;}

  /// @brief Query ending coordinate.
  ///
  /// @exceptions Strong guarantee.
  ///
  inline int qend() const {return qend_;}

  /// @brief Subject starting coordinate.
  ///
  /// @exceptions Strong guarantee.
  ///
  inline int sstart() const {return sstart_;}

  /// @brief Subject ending coordinate.
  ///
  /// @exceptions Strong guarantee.
  ///
  inline int send() const {return send_;}

  /// @brief Indicates if alignment is on plus strand of subject.
  ///
  /// @exceptions Strong guarantee.
  ///
  inline bool PlusStrand() const {return plus_strand_;}

  /// @brief Number of identical matches.
  ///
  /// @exceptions Strong guarantee.
  ///
  inline int nident() const {return nident_;}

  /// @brief Number of mismatches.
  ///
  /// @exceptions Strong guarantee.
  ///
  inline int mismatch() const {return mismatch_;}

  /// @brief Number of gap openings.
  ///
  /// @exceptions Strong guarantee.
  ///
  inline int gapopen() const {return gapopen_;}

  /// @brief Total number of gaps.
  ///
  /// @exceptions Strong guarantee.
  ///
  inline int gaps() const {return gaps_;}

  /// @brief Length of query sequence.
  ///
  /// @exceptions Strong guarantee.
  ///
  inline int qlen() const {return qlen_;}

  /// @brief Length of subject sequence.
  ///
  /// @exceptions Strong guarantee.
  ///
  inline int slen() const {return slen_;}

  /// @brief Query part of the sequence alignment.
  ///
  /// @exceptions Strong guarantee.
  ///
  inline const std::string& qseq() const {return qseq_;}

  /// @brief Subject part of the sequence alignment.
  ///
  /// @exceptions Strong guarantee.
  ///
  inline const std::string& sseq() const {return sseq_;}

  /// @brief Length of the alignment.
  ///
  /// @exceptions Strong guarantee.
  ///
  inline int length() const {return qseq_.length();}

  /// @brief Returns the alignment's percent identity.
  ///
  /// @exceptions Strong guarantee.
  ///
  inline float pident() const {
    return (static_cast<float>(nident_) / static_cast<float>(qseq_.length())
            * 100.0f);
  }
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
  Alignment(int id) : id_{id} {}

  int id_;
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
};

} // namespace paste_alignments

#endif // PASTE_ALIGNMENTS_ALIGNMENT_H_