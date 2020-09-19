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

#include "alignment_reader.h"

#include <cassert>

#include "exceptions.h"
#include "helpers.h"

namespace paste_alignments {

// AlignmentReader read operations helpers.
//
namespace {

// Used during field extraction to indicate whether a field is expected to
// terminate with a `\t` before the end of a row.
//
enum class FieldTerminator{
  kTab, // Expects `\t` as field terminator.
  kAny // Field may not terminate with '\t'.
};

// Replaces contents of `row` with the next line from `is`.
//
// Basic guarantee. Both `is` and `row` are modified. Throws
// `exceptions::ReadError` if `failbit` or `badbit` of `is` are set during
// character extraction.
//
void ExtractRow(std::istream& is, std::string& row) {
  std::getline(is, row);
  if (is.fail() || is.bad()) {
    throw exceptions::ReadError("Something went wrong when attempting to"
                                   " read from input stream.");
  }
}

// Extracts data field from `row` starting at `start_pos` and terminated with
// '\t', or the end of the row.
//
// Strong guarantee. Throws `exceptions::ReadError` if
// * The field is empty.
// * No `\t` found beginning at `start_pos` and `terminator` is
//   `FieldTerminator::kTab`.
//
std::string_view GetNonEmptyField(const std::string& row,
                                  std::string::size_type start_pos,
                                  FieldTerminator terminator) {
  // Find end of field.
  std::string::size_type end_pos{row.find('\t', start_pos)};
  if (terminator == FieldTerminator::kTab && end_pos == std::string::npos) {
    std::stringstream error_message;
    error_message << "Unable to find tab-terminated field starting at position:"
                  << start_pos << " in row: '" << row << "'.";
    throw exceptions::ReadError(error_message.str());
  } else if (end_pos == std::string::npos) {
    end_pos = row.length();
  }

  // Create view of field.
  std::string_view field = std::string_view{row.data() + start_pos,
                                            end_pos - start_pos};
  if (field.empty()) {
    std::stringstream error_message;
    error_message << "Empty field starting at position:" << start_pos
                  << " in row: '" << row << "'.";
    throw exceptions::ReadError(error_message.str());
  }
  
  return field;
}

// Extracts first two fields from `row` and stores them in `first_field` and
// `second_field`.
//
// Basic guarantee. Throws `exceptions::ReadError` if
// * `row` does not contain at least 2 '\t' characters.
// * One of the first two fields is empty.
//
void ExtractFirstTwoFields(const std::string& row,
                           std::string_view& first_field,
                           std::string_view& second_field) {
  std::string::size_type pos{0};
  first_field = GetNonEmptyField(row, pos, FieldTerminator::kTab);
  assert(row.at(first_field.length()) == '\t');
  pos = first_field.length() + 1;
  second_field = GetNonEmptyField(row, pos, FieldTerminator::kTab);
  assert(row.at(first_field.length() + second_field.length() + 1) == '\t');
}

// Returns `num_fields` tab-delimited non-empty fields from `row` starting at
// `start_pos`.
//
// Strong guarantee. Throws `exceptions::ReadError` if
// * Not enough field delimiters are found in `row` starting at `start_pos`.
// * One of the fields is empty.
//
std::vector<std::string_view> GetFields(const std::string& row,
                                        std::string::size_type start_pos,
                                        int num_fields) {
  std::vector<std::string_view> fields;
  for (int i = 1; i < num_fields; ++i) {
    fields.push_back(GetNonEmptyField(row, start_pos, FieldTerminator::kTab));
    start_pos += fields.back().length() + 1; // Position one past delimiter.
  }
  fields.push_back(GetNonEmptyField(row, start_pos, FieldTerminator::kAny));
  return fields;
}

} // namespace

// AlignmentReader::FromIStream
//
AlignmentReader AlignmentReader::FromIStream(std::unique_ptr<std::istream> is,
                                             int num_fields) {
  AlignmentReader result;
  if (is == nullptr) {
    throw exceptions::ReadError("Attempted to create `AlignmentReader` object"
                                " without providing input stream; `nullptr` was"
                                " given.");
  }
  result.num_fields_ = helpers::TestPositive(num_fields);

  result.is_ = std::move(is);
  ExtractRow(*(result.is_), result.row_);

  ExtractFirstTwoFields(result.row_, result.next_qseqid_, result.next_sseqid_);
  return result;
}

// AlignmentReader::ReadBatch
//
AlignmentBatch AlignmentReader::ReadBatch(
    const ScoringSystem& scoring_system,
    const PasteParameters& paste_parameters) {
  // Precondition.
  if (end_of_data_) {
    std::stringstream error_message;
    error_message << "Attempted to read more alignments when end of data was"
                  << " reached after row " << (num_fields_ - 1) << '.';
    throw exceptions::ReadError(error_message.str());
  }

  assert(!next_qseqid_.empty() && !next_sseqid_.empty());
  AlignmentBatch batch{next_qseqid_, next_sseqid_};

  // Read batch's alignments.
  std::vector<Alignment> alignments;
  std::vector<std::string_view> fields;
  while (next_qseqid_ == batch.Qseqid() && next_sseqid_ == batch.Sseqid()) {

    // Convert row to alignments.
    fields = GetFields(row_, next_qseqid_.length() + next_sseqid_.length() + 2,
                       num_fields_);
    Alignment a{Alignment::FromStringFields(next_alignment_id_,
        std::move(fields),
        scoring_system,
        paste_parameters)};
    alignments.push_back(a);
    ++next_alignment_id_;

    // Read next row, or stop looking if end of data is reached.
    if (end_of_data_) {
      break;
    } else {
      ExtractRow(*is_, row_);
      ExtractFirstTwoFields(row_, next_qseqid_, next_sseqid_);
      end_of_data_ = (is_->peek() == std::istream::traits_type::eof());
    }
  }

  // Populate and return batch.
  batch.ResetAlignments(std::move(alignments), paste_parameters);
  return batch;
}

// AlignmentReader::DebugString
//
std::string AlignmentReader::DebugString() const {
  std::stringstream ss;
  ss << "{num_fields: " << num_fields_
     << ", end_of_data: " << std::boolalpha << end_of_data_
     << ", next_alignment_id: " << next_alignment_id_ 
     << ", row: " << row_
     << ", next_qseqid: " << next_qseqid_
     << ", next_sseqid_: " << next_sseqid_
     << '}';
  return ss.str();
}

} // namespace paste_alignments