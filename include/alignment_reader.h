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

#include <fstream>
#include <string>
#include <vector>

#include "alignment.h"
#include "alignment_batch.h"
#include "scoring_system.h"

namespace paste_alignments {

/// @addtogroup PasteAlignments-Reference
///
/// @{

/// @brief Class for reading data in a tab-delimited file into `AlignmentBatch`
///  objects.
///
/// @details Data file must have query and subject identifiers in its first two
///  columns, and at least as many additional columns as required by `Alignment`
///  constructor. Those additional columns must also be in the order expected
///  by `Alignment` constructor. Any columns in excess to the first two plus
///  those required by `Alignment` constructor are ignored. Objects of this
///  class read the file in chunks, storing each chunk internally. Rows must not
///  be longer than what fits into the internal chunk.
///
/// @invariant `chunk_size_` is positive.
/// @invariant `chunk_size_` is the same as `data_.capacity()`.
/// @invariant `current_pos_` is a valid position in `current_chunk_`, or equal
///  to `current_chunk_.length()` or `std::string_view::npos`.
/// @invariant `current_chunk_` is a string view of a prefix of `data_`.
///
class AlignmentReader {
 public:
  /// @name Factories:
  ///
  /// @{
  
  /// @brief Constructs object associating to it file named `file_name`.
  ///
  /// @parameter file_name The name of the file the object is associated with.
  /// @parameter num_fields The number of data fields `Alignment` objects are
  ///  constructed from during certain read operations. Some read operations
  ///  will skip fields in excess of this number (+2 for the sequence
  ///  identifiers) per row.
  /// @parameter chunk_size The size of the chunk of data stored in the object
  ///  at any given time. ***This parameter must always be at least as large as
  ///  the number of characters in the longest row of data in the input file.
  ///  Otherwise, read operations will throw exceptions when encountering a row
  ///  in the input file that is longer than the value of this parameter.***
  ///
  /// @exceptions Basic guarantee. May partially read file named `file_name`.
  ///  * Throws `exceptions::InvalidInput` if unable to open file, or if file is
  ///    empty.
  ///  * Throws `exceptions::OutOfRange` if `num_fields`, or `chunk_size` are
  ///    not positive.
  ///  * Throws `exceptions::ReadError` if `std::ifstream::read` operation
  ///    signals a failure other than eof.
  ///  * Throws `exceptions::ReadError`, or `exceptions::OutOfRange` if first
  ///    two fields in first row do not fit into internal data chunk.
  ///
  static AlignmentReader FromFile(std::string file_name,
                                  int num_fields = 12,
                                  long chunk_size = 128l * 1000l * 1000l);
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

  /// @name Accessors:
  ///
  /// @{
  
  /// @brief Size of the chunks read from file.
  ///
  /// @exceptions Strong guarantee.
  ///
  inline long ChunkSize() const {return chunk_size_;}

  /// @brief Number of data fields `Alignment` objects are constructed from.
  ///
  /// @exceptions Strong guarantee.
  ///
  inline int NumFields() const {return num_fields_;}

  /// @brief Indicates whether the entire input data was processed.
  ///
  /// @exceptions Strong guarantee.
  ///
  inline bool EndOfData() const {
    return (!ifs_.is_open() && (current_pos_ == std::string_view::npos
                                || current_pos_ >= current_chunk_.length()));
  }
  /// @}

  /// @name Read operations:
  ///
  /// @{

  /// @brief Returns a batch of alignments.
  ///
  /// @parameter scoring_system The scoring system by which to sort alignments.
  /// @parameter epsilon Parameter used by `helpers::FuzzyFloatEquals` to
  ///  determine when two floats are "equal".
  ///
  /// @details `AlignmentBatch` object corresponds to the current row and the
  ///  following run of rows with the same first two column values.
  ///
  /// @exceptions
  ///  * Throws `exceptions::ReadError` if
  ///    - Executed after end of data was already reached.
  ///    - `std::ifstream::read` operation signals a failure other than eof.
  ///  * Throws `exceptions::ReadError`, or `exceptions::OutOfRange` if a row
  ///    does not fit into object's internal data chunk.
  ///  * `ReadBatch::FromStringFields` may throw.
  ///  * 
  ///
  AlignmentBatch ReadBatch(const ScoringSystem& scoring_system,
                           const PasteParameters& parameters);
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
  /// @brief Private constructor to force creation by factory.
  ///
  AlignmentReader() = default;

  /// @brief Reads the next chunk of characters from input file, saving suffix
  ///  at start of new chunk.
  ///
  /// @parameter suffix_start Size of the suffix of the current chunk to be
  ///  saved by moving it to the beginning of the new chunk. If set to a number
  ///  equal to or greater than the length of the current chunk, no suffix will
  ///  be saved.
  ///  
  /// @details Resets current position to 0.
  ///
  /// @exceptions Basic guarantee.
  ///  * Throws `exceptions::ReadError` if
  ///    - `ifs_` is not open.
  ///    - `std::ifstream::read` operation signals a failure other than eof.
  ///
  void ReadChunk(long suffix_start);

  /// @brief Returns the next data field following `temp_pos`.
  ///
  /// @parameter delimiter A `char` reference in which the delimiter following
  ///  the returned field is stored. If end of data is reached, its value is set
  ///  to '\0'.
  /// @parameter temp_pos The position in the current chunk at which the field
  ///  starts.
  ///
  /// @details Advances `temp_pos` to one past the delimiter following the
  ///  returned field, or to `std::string_view::npos` if end of data is reached.
  ///  If the function must extract more data using `ReadChunk`, it saves the
  ///  prefix starting at current position.
  ///
  /// @exceptions Basic guarantee.
  ///  * Throws `exceptions::ReadError` if
  ///    - Executed after end of data was already reached.
  ///    - `std::ifstream::read` operation signals a failure other than eof.
  ///  * Throws `exceptions::ReadError`, or `exceptions::OutOfRange` if current
  ///    row does not fit into internal data chunk.
  ///
  std::string_view GetField(char& delimiter,
                            std::string_view::size_type& temp_pos);

  /// @brief Returns a list of `num_fields_` data fields starting at current
  ///  position and sets `delimiter` to the delimiter following the fields.
  ///
  /// @parameter delimiter A `char` reference in which the delimiter following
  ///  the last of the returned fields is stored. If end of data is reached, its
  ///  value is set to '\0'.
  /// @parameter temp_pos The position in the current chunk at which the field
  ///  starts.
  ///
  /// @details Advances `temp_pos` to one past the delimiter following the
  ///  returned field, or to `std::string_view::npos` if end of data is reached.
  ///  If the function must extract more data using `ReadChunk`, it saves the
  ///  prefix starting at current position.
  ///
  /// @exceptions Basic guarantee.
  ///  * Throws `exceptions::ReadError` if
  ///    - Executed after end of data was already reached.
  ///    - `std::ifstream::read` operation signals a failure other than eof.
  ///  * Throws `exceptions::ReadError`, or `exceptions::OutOfRange` if a row
  ///    does not fit into internal data chunk.
  ///
  std::vector<std::string_view> GetAlignmentFields(
    char& delimiter, std::string_view::size_type& temp_pos);

  /// @brief Advances current position to one after the next newline character.
  ///
  /// @parameter temp_pos The position in current chunk at which search for
  ///  newline character begins.
  ///
  /// @details current position is advanced to the character following the next
  ///  newline character after `temp_pos`, or set to `std::string_view::npos`,
  ///  if no newline character was found.
  ///
  /// @exceptions Basic guarantee.
  ///  * Throws exceptions::ReadError` if
  ///    - `std::ifstream::read` operation signals a failure other than eof.
  ///    - If the current row does not fit into object's internal data chunk.
  ///
  void AdvanceToEndOfLine(std::string_view::size_type& temp_pos);

  long chunk_size_;
  int num_fields_;
  std::ifstream ifs_;
  std::string data_;
  std::string_view current_chunk_;
  std::string_view::size_type current_pos_{std::string_view::npos};
  std::string next_qseqid_;
  std::string next_sseqid_;
  int next_alignment_id_{0};
};
/// @}

} // namespace paste_alignments

#endif // PASTE_ALIGNMENTS_ALIGNMENT_READER_H_