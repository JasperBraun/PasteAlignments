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

#define CATCH_CONFIG_MAIN
#define CATCH_CONFIG_COLOUR_NONE
#include "catch.h"

#include "string_conversions.h" // include after catch.h

#include <limits>

#include "exceptions.h"

// AlignmentReader tests
//
// Test correctness for:
// * FromFile
// * ReadBatch
//
// Test exceptions for:
// * FromFile
// * ReadBatch

namespace paste_alignments {

namespace test {

// Creates a vector of alignments from `alignment_input_data` with consecutive
// run of id's starting at `start_id`.
//
std::vector<Alignment> MakeAlignments(
    const std::vector<std::vector<std::string>>& alignment_input_data,
    int start_id, const ScoringSystem& scoring_system,
    const PasteParameters& paste_parameters) {
  std::vector<Alignment> result;
  int id{start_id};
  std::vector<std::string_view> fields;
  for (const std::vector<std::string>& row : alignment_input_data) {
    fields.clear();
    for (const std::string& field : row) {
      fields.push_back(std::string_view{field});
    }
    result.push_back(Alignment::FromStringFields(id, fields, scoring_system,
                     paste_parameters));
    ++id;
  }
  return result;
}

namespace {

SCENARIO("Test correctness of AlignmentReader::FromFile.",
         "[AlignmentReader][FromFile][correctness]") {

  GIVEN("A valid input file.") {
    std::string input_file{"test_data/valid_alignment_file.tsv"};

    WHEN("Default number of fields and chunk size.") {
      AlignmentReader reader{AlignmentReader::FromFile(input_file)};

      THEN("Field number and chunk size are at default values.") {
        CHECK(reader.NumFields() == 12);
        CHECK(reader.ChunkSize() == 128l * 1000l * 1000l);
      }
    }

    WHEN("Non-default number of fields and default chunk size.") {
      int num_fields = GENERATE(take(5, random(1, 100)));
      AlignmentReader reader{AlignmentReader::FromFile(input_file, num_fields)};

      THEN("Field number is the specified non-default number.") {
        CHECK(reader.NumFields() == num_fields);
        CHECK(reader.ChunkSize() == 128l * 1000l * 1000l);
      }
    }

    WHEN("Default number of fields and non-default chunk size.") {
      long chunk_size
          = GENERATE(take(5, random(128l, 128l * 1000l * 1000l)));
      AlignmentReader reader{AlignmentReader::FromFile(input_file, 12,
                                                       chunk_size)};

      THEN("Field number is the specified non-default number.") {
        CHECK(reader.NumFields() == 12);
        CHECK(reader.ChunkSize() == chunk_size);
      }
    }

    WHEN("Non-default number of fields and chunk size.") {
      int num_fields = GENERATE(take(3, random(1, 100)));
      long chunk_size
          = GENERATE(take(3, random(128l, 128l * 1000l * 1000l)));
      AlignmentReader reader{AlignmentReader::FromFile(input_file, num_fields,
                                                       chunk_size)};

      THEN("Field number is the specified non-default number.") {
        CHECK(reader.NumFields() == num_fields);
        CHECK(reader.ChunkSize() == chunk_size);
      }
    }
  }
}

SCENARIO("Test exceptions thrown by AlignmentReader::FromFile.",
         "[AlignmentReader][FromFile][exceptions]") {

  THEN("Not being able to open the input file causes exception.") {
    CHECK_THROWS_AS(AlignmentReader::FromFile("invalid-file-name"),
                    exceptions::InvalidInput);
  }

  THEN("An empty file causes exception.") {
    CHECK_THROWS_AS(AlignmentReader::FromFile("empty_data_file.tsv"),
                    exceptions::InvalidInput);
  }

  THEN("Zero number of fields causes exception.") {
    CHECK_THROWS_AS(AlignmentReader::FromFile("test_data/valid_alignment_file.tsv", 0),
                    exceptions::OutOfRange);
  }

  THEN("Negative number of fields causes exception.") {
    int num_fields = GENERATE(take(5, random(-100, -1)));
    CHECK_THROWS_AS(AlignmentReader::FromFile("test_data/valid_alignment_file.tsv",
                                              num_fields),
                    exceptions::OutOfRange);
  }

  THEN("Zero chunk size causes exception.") {
    CHECK_THROWS_AS(AlignmentReader::FromFile("test_data/valid_alignment_file.tsv", 12,
                                              0l),
                    exceptions::OutOfRange);
  }

  THEN("Negative number of fields causes exception.") {
    long chunk_size = GENERATE(take(5, random(-1000l, -1l)));
    CHECK_THROWS_AS(AlignmentReader::FromFile("test_data/valid_alignment_file.tsv", 12,
                                              chunk_size),
                    exceptions::OutOfRange);
  }

  THEN("Using too small chunk size causes exception.") {
    CHECK_THROWS_AS(AlignmentReader::FromFile("test_data/valid_alignment_file.tsv", 12,
                                              1l),
                    exceptions::ReadError);
  }
}

SCENARIO("Test correctness of AlignmentReader::ReadBatch.",
         "[AlignmentReader][ReadBatch][correctness]") {

  GIVEN("A valid input file.") {
    std::string input_file{"test_data/valid_alignment_file.tsv"};

    // Pairs of identifiers in their order of appearance in input file.
    std::vector<std::pair<std::string, std::string>> sequence_identifiers{
        {"qseq1", "sseq1"}, {"qseq1", "sseq2"}, {"qseq2", "sseq2"},
        {"sseq2", "qseq2"}, {"qseq1", "sseq1"}, {"qseq4", "sseq4"}
    };

    // Alignments are the same for each pair of sequence identifiers.
    std::vector<std::vector<std::string>> alignment_input_data{
        {"101", "125", "1101", "1125", "24", "1", "0", "0", "10000", "100000", "GCCCCAAAATTCCCCAAAATTCCCC", "ACCCCAAAATTCCCCAAAATTCCCC"},
        {"101", "120", "1131", "1150", "20", "0", "0", "0", "10000", "100000", "CCCCAAAATTCCCCAAAATT", "CCCCAAAATTCCCCAAAATT"},
        {"101", "150", "1001", "1050", "40", "10", "0", "0", "10000", "100000", "GGGGGGGGGGCCCCAAAATTCCCCAAAATTCCCCAAAATTCCCCAAAATT", "AAAAAAAAAACCCCAAAATTCCCCAAAATTCCCCAAAATTCCCCAAAATT"},
        {"101", "110", "2111", "2120", "10", "0", "0", "0", "10000", "100000", "CCCCAAAATT", "CCCCAAAATT"},
        {"101", "125", "1111", "1135", "20", "5", "0", "0", "10000", "100000", "GGGGGCCCCAAAATTCCCCAAAATT", "AAAAACCCCAAAATTCCCCAAAATT"},
        {"101", "140", "1121", "1160", "30", "10", "0", "0", "10000", "100000", "GGGGGGGGGGCCCCAAAATTCCCCAAAATTCCCCAAAATT", "AAAAAAAAAACCCCAAAATTCCCCAAAATTCCCCAAAATT"},
        {"101", "115", "1096", "1110", "10", "5", "0", "0", "10000", "100000", "GGGGGCCCCAAAATT", "AAAAACCCCAAAATT"},
        {"101", "135", "101", "135", "20", "15", "0", "0", "10000", "100000", "GGGGGGGGGGGGGGGCCCCAAAATTCCCCAAAATT", "AAAAAAAAAAAAAAACCCCAAAATTCCCCAAAATT"},
        {"101", "120", "201", "220", "10", "10", "0", "0", "10000", "100000", "GGGGGGGGGGCCCCAAAATT", "AAAAAAAAAACCCCAAAATT"},
        {"101", "125", "2101", "2125", "10", "15", "0", "0", "10000", "100000", "GGGGGGGGGGGGGGGCCCCAAAATT", "AAAAAAAAAAAAAAACCCCAAAATT"}
    };

    WHEN("Run repeatedly with default various scoring systems.") {
      AlignmentReader reader{AlignmentReader::FromFile(input_file)};
      ScoringSystem scoring_system = GENERATE(ScoringSystem::Create(100000l, 1, 2, 1, 1),
                                              ScoringSystem::Create(10000000l, 2, 3, 0, 0));

      float epsilon = GENERATE(0.05f, 0.15f);
      PasteParameters paste_parameters;
      paste_parameters.float_epsilon = epsilon;

      THEN("Each run of equal first two columns will constitute one batch.") {
        for (int i = 0; i < sequence_identifiers.size(); ++i) {
          AlignmentBatch true_output_batch{reader.ReadBatch(scoring_system,
                                                            paste_parameters)};
          AlignmentBatch expected_output_batch{
              sequence_identifiers.at(i).first,
              sequence_identifiers.at(i).second};
          expected_output_batch.ResetAlignments(
              MakeAlignments(alignment_input_data, 0 + 10 * i, scoring_system,
                             paste_parameters),
              paste_parameters);
          CHECK(true_output_batch == expected_output_batch);
        }
      }
    }
  }

  GIVEN("A valid input file with additional columns and trailing newline.") {
    std::string input_file{
        "test_data/valid_alignment_file_with_additional_columns.tsv"};

    // Pairs of identifiers in their order of appearance in input file.
    std::vector<std::pair<std::string, std::string>> sequence_identifiers{
        {"qseq1", "sseq1"}, {"qseq1", "sseq2"}, {"qseq2", "sseq2"},
        {"sseq2", "qseq2"}, {"qseq1", "sseq1"}, {"qseq4", "sseq4"}
    };

    // Alignments are the same for each pair of sequence identifiers.
    std::vector<std::vector<std::string>> alignment_input_data{
        {"101", "125", "1101", "1125", "24", "1", "0", "0", "10000", "100000", "GCCCCAAAATTCCCCAAAATTCCCC", "ACCCCAAAATTCCCCAAAATTCCCC"},
        {"101", "120", "1131", "1150", "20", "0", "0", "0", "10000", "100000", "CCCCAAAATTCCCCAAAATT", "CCCCAAAATTCCCCAAAATT"},
        {"101", "150", "1001", "1050", "40", "10", "0", "0", "10000", "100000", "GGGGGGGGGGCCCCAAAATTCCCCAAAATTCCCCAAAATTCCCCAAAATT", "AAAAAAAAAACCCCAAAATTCCCCAAAATTCCCCAAAATTCCCCAAAATT"},
        {"101", "110", "2111", "2120", "10", "0", "0", "0", "10000", "100000", "CCCCAAAATT", "CCCCAAAATT"},
        {"101", "125", "1111", "1135", "20", "5", "0", "0", "10000", "100000", "GGGGGCCCCAAAATTCCCCAAAATT", "AAAAACCCCAAAATTCCCCAAAATT"},
        {"101", "140", "1121", "1160", "30", "10", "0", "0", "10000", "100000", "GGGGGGGGGGCCCCAAAATTCCCCAAAATTCCCCAAAATT", "AAAAAAAAAACCCCAAAATTCCCCAAAATTCCCCAAAATT"},
        {"101", "115", "1096", "1110", "10", "5", "0", "0", "10000", "100000", "GGGGGCCCCAAAATT", "AAAAACCCCAAAATT"},
        {"101", "135", "101", "135", "20", "15", "0", "0", "10000", "100000", "GGGGGGGGGGGGGGGCCCCAAAATTCCCCAAAATT", "AAAAAAAAAAAAAAACCCCAAAATTCCCCAAAATT"},
        {"101", "120", "201", "220", "10", "10", "0", "0", "10000", "100000", "GGGGGGGGGGCCCCAAAATT", "AAAAAAAAAACCCCAAAATT"},
        {"101", "125", "2101", "2125", "10", "15", "0", "0", "10000", "100000", "GGGGGGGGGGGGGGGCCCCAAAATT", "AAAAAAAAAAAAAAACCCCAAAATT"}
    };

    WHEN("Run repeatedly with default scoring system.") {
      AlignmentReader reader{AlignmentReader::FromFile(input_file)};
      ScoringSystem scoring_system = GENERATE(ScoringSystem::Create(100000l, 1, 2, 1, 1),
                                              ScoringSystem::Create(10000000l, 2, 3, 0, 0));
      float epsilon = GENERATE(0.05f, 0.15f);
      PasteParameters paste_parameters;
      paste_parameters.float_epsilon = epsilon;

      THEN("Each run of equal first two columns will constitute one batch.") {
        for (int i = 0; i < sequence_identifiers.size(); ++i) {
          AlignmentBatch true_output_batch{reader.ReadBatch(scoring_system,
                                                            paste_parameters)};
          AlignmentBatch expected_output_batch{
              sequence_identifiers.at(i).first,
              sequence_identifiers.at(i).second};
          expected_output_batch.ResetAlignments(
              MakeAlignments(alignment_input_data, 0 + 10 * i, scoring_system,
                             paste_parameters),
              paste_parameters);
          CHECK(true_output_batch == expected_output_batch);
        }
      }
    }
  }
}

SCENARIO("Test exceptions thrown by AlignmentReader::ReadBatch.",
         "[AlignmentReader][ReadBatch][exceptions]") {

  GIVEN("A valid input file.") {
    std::string input_file{"test_data/valid_alignment_file.tsv"};
    ScoringSystem scoring_system{ScoringSystem::Create(100000l, 1, 2, 1, 1)};
    PasteParameters paste_parameters;

    THEN("Call when already at the end of the data causes exception.") {
      AlignmentReader reader{AlignmentReader::FromFile(input_file)};
      while (!reader.EndOfData()) {
        reader.ReadBatch(scoring_system, paste_parameters);
      }

      REQUIRE(reader.EndOfData());
      CHECK_THROWS_AS(reader.ReadBatch(scoring_system, paste_parameters),
                      exceptions::ReadError);
    }

    THEN("Not allowing enough capacity for an input row causes exeception.") {
      AlignmentReader reader{AlignmentReader::FromFile(input_file, 12, 100l)};
      CHECK_THROWS_AS(reader.ReadBatch(scoring_system, paste_parameters),
                      exceptions::ReadError);
    }

    THEN("Expecting more fields than given in the file causes exception.") {
      AlignmentReader reader{AlignmentReader::FromFile(input_file, 14)};
      CHECK_THROWS_AS(reader.ReadBatch(scoring_system, paste_parameters),
                      exceptions::ReadError);
    }
  }
}

} // namespace

} // namespace test

} // namespace paste_alignments