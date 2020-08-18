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
class Alignment {
 public:
  /// @name Factories:
  ///
  /// @{
  
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