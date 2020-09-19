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

#ifndef PASTE_ALIGNMENTS_ALIGNMENT_READER_H_
#define PASTE_ALIGNMENTS_ALIGNMENT_READER_H_

#include <memory>

#include "alignment.h"
#include "alignment_batch.h"

namespace paste_alignments {

/// @addtogroup PasteAlignments-Reference
///
/// @{

/// @brief Class for reading data in a tab-delimited file into `AlignmentBatch`
///  objects.
///
/// @details Data file must have query and subject identifiers in its first two
///  columns, and at least as many additional columns as required by
///  `AlignmentFromStringFields` (in the required order). Excess columns are
///  ignored.
///
class AlignmentReader {
 public:
  /// @name Factories:
  ///
  /// @{

  /// @name Creates an `AlignmentReader` object with the input stream `is`
  ///  associated to it.
  ///
  /// @parameter is Input stream to be associated with the return object.
  /// @parameter num_fields The number of fields per row expected to be read and
  ///  passed to `Alignment::FromStringFields`.
  ///
  /// @exceptions Basic guarantee. Modifies `is`.
  ///  * Throws `exceptions::OutOfRange` if `num_fields` is not positive.
  ///  * Throws `exceptions::ReadError` if
  ///    - `is` compares to `nullptr`.
  ///    - While extracting first line from `is`, `failbit` or `badbit` are set.
  ///    - First line in `is` does not contain at least 2 '\t' characters.
  ///    - One of the first two fields in the first line of `is` is empty.
  ///  * `Alignment::FromStringFields` may throw.
  ///
  static AlignmentReader FromIStream(std::unique_ptr<std::istream> is,
                                     int num_fields = 13);
  /// @}

  /// @name Constructors:
  ///
  /// @{
  
  AlignmentReader(const AlignmentReader& other) = delete;
  
  /// @brief Move constructor.
  ///
  AlignmentReader(AlignmentReader&& other) = default;
  /// @}
  
  /// @name Assignment:
  ///
  /// @{
  
  AlignmentReader& operator=(const AlignmentReader& other) = delete;
  
  /// @brief Move assignment.
  ///
  AlignmentReader& operator=(AlignmentReader&& other) = default;
  /// @}

  /// @name Read operations:
  ///
  /// @{
  
  /// @brief Indicates whether the end of data in the associated input stream
  ///  was reached.
  ///
  /// @details End of data is reached when stream operations cause `eofbit`,
  ///
  /// @exceptions Strong guarantee.
  ///
  inline bool EndOfData() const {return end_of_data_;}

  /// @brief Returns the next batch of alignments read from the associated input
  ///  stream.
  ///
  /// @parameter scoring_system The scoring system by which to sort alignments.
  /// @parameter paste_parameters Used by `Alignment::FromStringFields` and
  ///  `AlignmentBatch::ResetAlignments`.
  ///
  /// @exceptions Basic guarantee. Throws `exceptions::ParsingError` if
  ///  * Function is called after end of data is was reached.
  ///  * A row does not contain enough fields.
  ///  * An extracted field is empty.
  ///
  AlignmentBatch ReadBatch(const ScoringSystem& scoring_system,
                           const PasteParameters& paste_parameters);
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
  /// @brief Private default constructor to force factory creation
  ///
  AlignmentReader() = default;

  int num_fields_; // Number of fields passed to `Alignment::FromStringFields`.
  bool end_of_data_{false};
  long next_alignment_id_{1};
  std::unique_ptr<std::istream> is_;
  std::string row_;
  std::string_view next_qseqid_; // Must be non-empty if end_of_data_ is false.
  std::string_view next_sseqid_; // Must be non-empty if end_of_data_ is false.
};
/// @}

} // namespace paste_alignments

#endif // PASTE_ALIGNMENTS_ALIGNMENT_READER_H_