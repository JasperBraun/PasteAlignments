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

#include <sstream>
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
  ///  * Fewer than 12 fields were provided
  ///  * One of the fields, except the last two cannot be converted to integer
  ///  * Query start is larger than query end coordinate
  ///  * Query or subject start or end coordinates are negative
  ///  * Number of identities, mismatches, gap openings, or total gaps is
  ///    negative
  ///  * Query or subject length is non-positive
  ///  * Aligned part of query or subject sequence is empty. 
  ///
  static Alignment FromStringFields(int id,
                                    std::vector<std::string_view> fields);
  /// @}

  /// @name Constructors:
  ///
  /// @{
  
  /// @brief Copy constructor (defaulted).
  ///
  Alignment(const Alignment& other) = default;

  /// @brief Move constructor (defaulted).
  ///
  Alignment(Alignment&& other) = default;
  /// @}
  
  /// @name Assignment:
  ///
  /// @{
  
  /// @brief Copy assignment (defaulted).
  ///
  Alignment& operator=(const Alignment& other) = default;

  /// @brief Move assignment (defaulted).
  ///
  Alignment& operator=(Alignment&& other) = default;
  /// @}

  /// @name Accessors:
  ///
  /// @{

  /// @brief Object's id.
  ///
  inline int id() const {return id_;}

  /// @brief Query starting coordinate.
  ///
  inline int qstart() const {return qstart_;}

  /// @brief Query ending coordinate.
  ///
  inline int qend() const {return qend_;}

  /// @brief Subject starting coordinate.
  ///
  inline int sstart() const {return sstart_;}

  /// @brief Subject ending coordinate.
  ///
  inline int send() const {return send_;}

  /// @brief Indicates if alignment is on plus strand of subject.
  ///
  inline bool plus_strand() const {return plus_strand_;}

  /// @brief Number of identical matches.
  ///
  inline int nident() const {return nident_;}

  /// @brief Number of mismatches.
  ///
  inline int mismatch() const {return mismatch_;}

  /// @brief Number of gap openings.
  ///
  inline int gapopen() const {return gapopen_;}

  /// @brief Total number of gaps.
  ///
  inline int gaps() const {return gaps_;}

  /// @brief Length of query sequence.
  ///
  inline int qlen() const {return qlen_;}

  /// @brief Length of subject sequence.
  ///
  inline int slen() const {return slen_;}

  /// @brief Query part of the sequence alignment.
  ///
  inline const std::string& qseq() const {return qseq_;}

  /// @brief Subject part of the sequence alignment.
  ///
  inline const std::string& sseq() const {return sseq_;}
  /// @}

  /// @name Other:
  ///
  /// @{

  /// @brief Compares the object to `other`.
  ///
  inline bool operator==(const Alignment& other) const {
    return (other.qstart_ == qstart_
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
            && other.sseq_ == sseq_);
  }

  /// @brief Returns a descriptive string of the object.
  ///
  inline std::string DebugString() const {
    std::stringstream  ss;
    ss << std::boolalpha << "(id=" << id_
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
       << "')";
    return ss.str();
  }
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