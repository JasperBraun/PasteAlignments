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

#include <sstream>

#include "exceptions.h"
#include "helpers.h"

namespace paste_alignments {

// AlignmentReader::FromFile
//
AlignmentReader AlignmentReader::FromFile(std::string file_name, int num_fields,
                                          long chunk_size) {
  AlignmentReader result;
  result.num_fields_ = helpers::TestPositive(num_fields);
  result.chunk_size_ = helpers::TestPositive(chunk_size);
  std::stringstream error_message;

  result.ifs_.open(file_name);
  if (result.ifs_.fail()) {
    error_message << "Unable to open file: '" << file_name << "'.";
    throw exceptions::InvalidInput(error_message.str());
  }

  result.data_.resize(result.chunk_size_);
  result.ReadChunk(result.chunk_size_);
  if (result.EndOfData()) {
    error_message << "File: '" << file_name << "' seems to be empty.";
    throw exceptions::InvalidInput(error_message.str());
  }

  char first_delimiter, second_delimiter;
  std::string_view::size_type temp_pos{result.current_pos_};
  result.next_qseqid_ = result.GetField(first_delimiter, temp_pos);
  result.next_sseqid_ = result.GetField(second_delimiter, temp_pos);
  result.current_pos_ = temp_pos;
  if (first_delimiter != '\t' || second_delimiter != '\t') {
    error_message << "Premature end of line or data for alignment id: "
                  << result.next_alignment_id_ << ". Expected "
                  << (num_fields + 2) << " columns, but found less than 3.";
    throw exceptions::ReadError(error_message.str());
  }
  return result;
}

// AlignmentReader::ReadBatch
//
AlignmentBatch AlignmentReader::ReadBatch(const ScoringSystem& scoring_system,
                                          const PasteParameters& parameters) {
  AlignmentBatch batch{next_qseqid_, next_sseqid_};
  std::vector<Alignment> alignments;
  char last_delimiter{'\t'};
  std::string_view::size_type temp_pos{current_pos_};

  while (next_qseqid_ == batch.Qseqid() && next_sseqid_ == batch.Sseqid()) {
    alignments.emplace_back(Alignment::FromStringFields(
        next_alignment_id_, GetAlignmentFields(last_delimiter, temp_pos),
        scoring_system, parameters));

    if (last_delimiter == '\t') {
      AdvanceToEndOfLine(temp_pos);
    } else if (last_delimiter == '\n') {
      current_pos_ = temp_pos;
    }
    ++next_alignment_id_;
    if (!ifs_.is_open() && (temp_pos == std::string_view::npos
                            || temp_pos >= current_chunk_.length())) {
      current_pos_ = std::string_view::npos;
      break;
    }
    temp_pos = current_pos_;

    next_qseqid_ = GetField(last_delimiter, temp_pos);
    if (last_delimiter != '\t') {
      std::stringstream error_message;
      error_message << "Premature end of line or data for alignment id: "
                    << next_alignment_id_ << ". Expected " << (num_fields_ + 2)
                    << " columns, but found only " << 1 << '.';
      throw exceptions::ReadError(error_message.str());
    }
    next_sseqid_ = GetField(last_delimiter, temp_pos);
    current_pos_ = temp_pos;
  }
  batch.ResetAlignments(std::move(alignments), parameters);
  return batch;
}

// AlignmentReader::ReadChunk
//
void AlignmentReader::ReadChunk(long suffix_start) {
  long suffix_size = std::max(0l, static_cast<long>(current_chunk_.length())
                                      - suffix_start);
  if (!ifs_.is_open()) {
    std::stringstream error_message;
    error_message << "Attempted to read data from closed file for alignment id:"
                  << next_alignment_id_ << '.';
    throw exceptions::ReadError(error_message.str());
  }

  // Save suffix, if any, by moving its contents to beginning of data.
  if (suffix_size > 0 && suffix_size < chunk_size_) {
    std::memcpy(data_.data(),
                data_.data() + suffix_start,
                suffix_size);
  }

  // Extract new data.
  ifs_.read(data_.data() + suffix_size, chunk_size_ - suffix_size);
  if (ifs_.gcount() == 0) {
    if (ifs_.bad() || (ifs_.fail() && !ifs_.eof())) {
      throw exceptions::ReadError("Something went wrong while reading from"
                                  " input file.");
    }
    ifs_.close();
  } else {
    current_chunk_ = std::string_view{
        data_.data(), static_cast<std::string_view::size_type>(ifs_.gcount())};
    ifs_.peek();
    if (ifs_.eof()) {
      ifs_.close();
    }
  }

  current_pos_ = 0;
}

// AlignmentReader::GetField
//
std::string_view AlignmentReader::GetField(
    char& delimiter, std::string_view::size_type& temp_pos) {
  if (EndOfData()) {
    std::stringstream error_message;
    error_message << "Attempted to extract data after end of data was reached"
                  << " for alignment id: " << next_alignment_id_ << '.';
    throw exceptions::ReadError(error_message.str());
  }

  std::string_view result;
  std::string_view::size_type field_end;
  field_end = current_chunk_.find_first_of("\t\n", temp_pos);

  // If no delimiter found at first try, extract more data, if any available.
  if (field_end == std::string_view::npos && ifs_.is_open()) {
    temp_pos -= current_pos_;
    ReadChunk(current_pos_);
    field_end = current_chunk_.find_first_of("\t\n", temp_pos);
  }

  // If still no delimiter found, either end of data was reached, or chunk size
  // is too small.
  if (field_end == std::string_view::npos) {
    if (ifs_.is_open()) { // Chunk size too small
      std::stringstream error_message;
      error_message << "Object's data capacity too small for alignment id: "
                    << next_alignment_id_ << '.';
      throw exceptions::ReadError(error_message.str());
    } else { // End of data
      delimiter = '\0';
      result = std::string_view{current_chunk_.data() + temp_pos,
                                current_chunk_.length() - temp_pos};
      temp_pos = std::string_view::npos;
    }

  // If delimiter is found, set current position to one past field's delimiter.
  } else {
    delimiter = current_chunk_.at(field_end);
    result = std::string_view{current_chunk_.data() + temp_pos,
                              field_end - temp_pos};
    temp_pos = field_end + 1;
  }
  return result;
}

// AlignmentReader::GetAlignmentFields
//
std::vector<std::string_view> AlignmentReader::GetAlignmentFields(
    char& delimiter, std::string_view::size_type& temp_pos) {
  std::vector<std::string_view> fields;
  for (int i = 0; i < num_fields_; ++i) {
    if (delimiter != '\t') {
      std::stringstream error_message;
      error_message << "Premature end of line or data for alignment id: "
                    << next_alignment_id_ << ". Expected " << (num_fields_ + 2)
                    << " columns, but found only " << (i + 2) << '.';
      throw exceptions::ReadError(error_message.str());
    }
    fields.emplace_back(GetField(delimiter, temp_pos));
  }
  return fields;
}

// AlignmentReader::AdvanceToEndOfLine
//
void AlignmentReader::AdvanceToEndOfLine(
    std::string_view::size_type& temp_pos) {
  if (ifs_.is_open() || (temp_pos != std::string_view::npos
                         && temp_pos < current_chunk_.length())) {
    temp_pos = current_chunk_.find('\n', temp_pos);
    if (temp_pos == std::string_view::npos && ifs_.is_open()) {
      ReadChunk(current_pos_);
      temp_pos -= current_pos_;
      temp_pos = current_chunk_.find('\n', temp_pos);
    }
    if (temp_pos == std::string_view::npos && ifs_.is_open()) {
      std::stringstream error_message;
      error_message << "Object's data capacity too small for alignment id: "
                    << next_alignment_id_ << '.';
      throw exceptions::ReadError(error_message.str());
    }
  }
  if (temp_pos != std::string_view::npos) {
    current_pos_ = temp_pos + 1;
  } else {
    current_pos_ = std::string_view::npos;
  }
}

// AlignmentReader::DebugString
//
std::string AlignmentReader::DebugString() const {
  std::stringstream ss;
  ss << "{chunk_size: " << chunk_size_ << ", num_fields: " << num_fields_
     << ", ifs_.is_open: " << std::boolalpha << ifs_.is_open()
     << ", data.length: " << data_.length() << ", current_chunk.length: "
     << current_chunk_.length() << ", current_pos: " << current_pos_
     << ", next_qseqid: " << next_qseqid_ << ", next_sseqid_: " << next_sseqid_
     << ", next_alignment_id: " << next_alignment_id_ << '}';
  return ss.str();
}

} // namespace paste_alignments