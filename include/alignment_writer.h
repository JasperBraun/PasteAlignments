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

#ifndef PASTE_ALIGNMENTS_ALIGNMENT_WRITER_H_
#define PASTE_ALIGNMENTS_ALIGNMENT_WRITER_H_

namespace paste_alignments {

/// @brief Class for writing alignment data into data file.
///
/// @details Column order: qseqid, sseqid, qstart, qend, sstart, send, nident,
///  gapopen, qlen, qseq, pident, score, bitscore, evalue, identifiers
///
class AlignmentWriter {
 public:
  /// @name Factories:
  ///
  /// @{
  
  /// @brief Constructs object associating to it file named `file_name`.
  ///
  /// @parameter file_name The name of the file the object is associated with.
  ///
  /// @exceptions Basic guarantee. Attempts to create/open and truncate file
  ///  named `file_name`. Throws `exceptions::InvalidInput` if unable to open or
  ///  create file.
  ///
  inline static AlignmentWriter FromFile(std::string file_name) {
    AlignmentWriter result;

    result.ofs_.open(file_name, std::ofstream::trunc);
    if (result.ofs_.fail()) {
      std::stringstream error_message;
      error_message << "Unable to open file: '" << file_name << "'.";
      throw exceptions::InvalidInput(error_message.str());
    }

    return result;
  }
  /// @}
  
  /// @name Constructors:
  ///
  /// @{

  AlignmentWriter(const AlignmentWriter& other) = delete;

  /// @brief Move constructor.
  ///
  AlignmentWriter(AlignmentWriter&& other) = default;
  /// @}
  
  /// @name Assignment:
  ///
  /// @{

  AlignmentWriter& operator=(const AlignmentWriter& other) = delete;

  /// @brief Move assignment.
  ///
  AlignmentWriter& operator=(AlignmentWriter&& other) = default;
  /// @}

  /// @name Write operations:
  ///
  /// @{
  
  /// @brief Writes `AlignmentBatch` object into associated file.
  ///
  /// @exceptions Basic guarantee.
  ///
  void WriteBatch(AlignmentBatch batch, const PasteParameters& parameters);
  /// @}

  /// @name Other:
  ///
  /// @{
  
  /// @brief Returns a descriptive string of the object.
  ///
  /// @exceptions Strong guarantee.
  ///
  std::string DebugString() const;
  /// @}
 private:
  std::ofstream ofs_;
  bool line_break_{false};
};

} // namespace paste_alignments

#endif // PASTE_ALIGNMENTS_ALIGNMENT_WRITER_H_