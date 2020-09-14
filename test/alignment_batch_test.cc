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

#include "alignment_batch.h"

#define CATCH_CONFIG_MAIN
#define CATCH_CONFIG_COLOUR_NONE
#include "catch.h"

#include "string_conversions.h" // include after catch.h

#include <algorithm>
#include <random>
#include <unordered_set>
#include <vector>

// AlignmentBatch tests
//
// Test correctness for:
// * ResetAlignments
// * PasteAlignments
// 
// Test invariants for:
// * ResetAlignments
// * PasteAlignments
//
// Test exceptions for:
// * AlignmentBatch(string_view, string_view)
// * qseqid(string_view)
// * sseqid(string_view)
// * PasteAlignments

namespace paste_alignments {

namespace test {

bool ScoreComp(const Alignment& first, const Alignment& second,
               int first_pos, int second_pos,
               float epsilon = 0.05f) {
  float first_score, second_score, first_pident, second_pident;
  first_score = first.RawScore();
  second_score = second.RawScore();
  first_pident = first.Pident();
  second_pident = first.Pident();
  if (helpers::FuzzyFloatEquals(
          first_score, second_score, epsilon)) {
    if (helpers::FuzzyFloatEquals(
            first_pident, second_pident, epsilon)) {
      return first_pos < second_pos;
    } else if (first_pident > second_pident) {
      return true;
    }
  } else if (first_score > second_score) {
    return true;
  }
  return false;
}

// Obtains `AlignmentConfiguration` object for `left` and `right`.
//
AlignmentConfiguration GetConfiguration(const Alignment& left,
                                        const Alignment& right) {
  assert(left.PlusStrand() == right.PlusStrand());
  AlignmentConfiguration config;

  config.query_offset = right.Qstart() - left.Qend() - 1;
  if (left.PlusStrand()) {
    config.subject_offset = right.Sstart() - left.Send() - 1;
  } else {
    config.subject_offset = left.Sstart() - right.Send() - 1;
  }

  config.query_overlap = std::abs(std::min(0, config.query_offset));
  config.query_distance = std::max(0, config.query_offset);

  config.subject_overlap = std::abs(std::min(0, config.subject_offset));
  config.subject_distance = std::max(0, config.subject_offset);

  config.shift = std::abs(config.query_offset - config.subject_offset);
  config.left_length = left.Length();
  config.right_length = right.Length();
  config.pasted_length = config.left_length + config.right_length
                         + std::max(config.query_offset, config.subject_offset);
  return config;
}

namespace {

SCENARIO("Test correctness of AlignmentBatch::ResetAlignments.",
         "[AlignmentBatch][ResetAlignments][correctness]") {
  PasteParameters paste_parameters;

  GIVEN("An alignment batch and a scoring system.") {
    AlignmentBatch alignment_batch{"qseqid", "sseqid"};
    ScoringSystem scoring_system{ScoringSystem::Create(100000l, 1, 2, 1, 1)};

    WHEN("Alignments have various scores.") {
      std::vector<Alignment> alignments{
          // score 22, pident 96.0, sstart 1101, send 1125
          Alignment::FromStringFields(0, {"101", "125", "1101", "1125",
                                         "24", "1", "0", "0",
                                         "10000", "100000",
                                         "GCCCCAAAATTCCCCAAAATTCCCC",
                                         "ACCCCAAAATTCCCCAAAATTCCCC"},
                                      scoring_system, paste_parameters),
          // score 20, pident 100.0, sstart 1131, send 1150
          Alignment::FromStringFields(1, {"101", "120", "1131", "1150",
                                         "20", "0", "0", "0",
                                         "10000", "100000",
                                         "CCCCAAAATTCCCCAAAATT",
                                         "CCCCAAAATTCCCCAAAATT"},
                                      scoring_system, paste_parameters),
          // score 20, pident 80.0, sstart 1001, send 1050
          Alignment::FromStringFields(2, {"101", "150", "1001", "1050",
                                         "40", "10", "0", "0",
                                         "10000", "100000",
                                         "GGGGGGGGGGCCCCAAAATTCCCCAAAATTCCCCAAAATTCCCCAAAATT",
                                         "AAAAAAAAAACCCCAAAATTCCCCAAAATTCCCCAAAATTCCCCAAAATT"},
                                      scoring_system, paste_parameters),
          // score: 10, pident 100.0, sstart 2111, send 2120
          Alignment::FromStringFields(3, {"101", "110", "2111", "2120",
                                         "10", "0", "0", "0",
                                         "10000", "100000",
                                         "CCCCAAAATT",
                                         "CCCCAAAATT"},
                                      scoring_system, paste_parameters),
          // score 10, pident 80.0, sstart 1111, send 1135
          Alignment::FromStringFields(4, {"101", "125", "1111", "1135",
                                         "20", "5", "0", "0",
                                         "10000", "100000",
                                         "GGGGGCCCCAAAATTCCCCAAAATT",
                                         "AAAAACCCCAAAATTCCCCAAAATT"},
                                      scoring_system, paste_parameters),
          // score 10, pident 75.0, sstart 1121, send 1160
          Alignment::FromStringFields(5, {"101", "140", "1121", "1160",
                                         "30", "10", "0", "0",
                                         "10000", "100000",
                                         "GGGGGGGGGGCCCCAAAATTCCCCAAAATTCCCCAAAATT",
                                         "AAAAAAAAAACCCCAAAATTCCCCAAAATTCCCCAAAATT"},
                                      scoring_system, paste_parameters),
          // score 0, pident 66.66666667, sstart 1096, send 1110
          Alignment::FromStringFields(6, {"101", "115", "1096", "1110",
                                         "10", "5", "0", "0",
                                         "10000", "100000",
                                         "GGGGGCCCCAAAATT",
                                         "AAAAACCCCAAAATT"},
                                      scoring_system, paste_parameters),
          // score -10, pident 57.14285714, sstart 101, send 135
          Alignment::FromStringFields(7, {"101", "135", "101", "135",
                                         "20", "15", "0", "0",
                                         "10000", "100000",
                                         "GGGGGGGGGGGGGGGCCCCAAAATTCCCCAAAATT",
                                         "AAAAAAAAAAAAAAACCCCAAAATTCCCCAAAATT"},
                                      scoring_system, paste_parameters),
          // score -10, pident 50.0, sstart 201, send 220
          Alignment::FromStringFields(8, {"101", "120", "201", "220",
                                         "10", "10", "0", "0",
                                         "10000", "100000",
                                         "GGGGGGGGGGCCCCAAAATT",
                                         "AAAAAAAAAACCCCAAAATT"},
                                      scoring_system, paste_parameters),
          // score -20, pident 40.0, sstart 2101, send 2125
          Alignment::FromStringFields(9, {"101", "125", "2101", "2125",
                                         "10", "15", "0", "0",
                                         "10000", "100000",
                                         "GGGGGGGGGGGGGGGCCCCAAAATT",
                                         "AAAAAAAAAAAAAAACCCCAAAATT"},
                                      scoring_system, paste_parameters)};
      std::random_device rd;
      std::mt19937 g(rd());
      std::vector<std::vector<Alignment>> shuffled_alignments;
      for (int i = 0; i < 10; ++i) {
        shuffled_alignments.push_back(alignments);
        std::shuffle(shuffled_alignments.back().begin(),
                     shuffled_alignments.back().end(),
                     g);
      }
      int i = GENERATE(range(0, 10));
      float epsilon = GENERATE(0.05f, 0.15f);
      paste_parameters.float_epsilon = epsilon;
      alignment_batch.ResetAlignments(shuffled_alignments.at(i),
                                      paste_parameters);

      THEN("Object contains the new alignments.") {
        CHECK(alignment_batch.Alignments() == shuffled_alignments.at(i));
      }

      THEN("qseqid and sseqid didn't change.") {
        CHECK(alignment_batch.Qseqid() == "qseqid");
        CHECK(alignment_batch.Sseqid() == "sseqid");
      }
    }
  }

  GIVEN("A nonempty alignment batch.") {
    AlignmentBatch alignment_batch{"qseqid", "sseqid"};
    ScoringSystem scoring_system{ScoringSystem::Create(100000l, 1, 2, 1, 1)};
    std::vector<Alignment> old_alignments{
        // score 22, pident 96.0, sstart 1101, send 1125
        Alignment::FromStringFields(0, {"101", "125", "1101", "1125",
                                       "24", "1", "0", "0",
                                       "10000", "100000",
                                       "GCCCCAAAATTCCCCAAAATTCCCC",
                                       "ACCCCAAAATTCCCCAAAATTCCCC"},
                                    scoring_system, paste_parameters),
        // score 20, pident 100.0, sstart 1131, send 1150
        Alignment::FromStringFields(1, {"101", "120", "1131", "1150",
                                       "20", "0", "0", "0",
                                       "10000", "100000",
                                       "CCCCAAAATTCCCCAAAATT",
                                       "CCCCAAAATTCCCCAAAATT"},
                                    scoring_system, paste_parameters),
        // score 20, pident 80.0, sstart 1001, send 1050
        Alignment::FromStringFields(2, {"101", "150", "1001", "1050",
                                       "40", "10", "0", "0",
                                       "10000", "100000",
                                       "GGGGGGGGGGCCCCAAAATTCCCCAAAATTCCCCAAAATTCCCCAAAATT",
                                       "AAAAAAAAAACCCCAAAATTCCCCAAAATTCCCCAAAATTCCCCAAAATT"},
                                    scoring_system, paste_parameters),
        // score: 10, pident 100.0, sstart 2111, send 2120
        Alignment::FromStringFields(3, {"101", "110", "2111", "2120",
                                       "10", "0", "0", "0",
                                       "10000", "100000",
                                       "CCCCAAAATT",
                                       "CCCCAAAATT"},
                                    scoring_system, paste_parameters),
        // score 10, pident 80.0, sstart 1111, send 1135
        Alignment::FromStringFields(4, {"101", "125", "1111", "1135",
                                       "20", "5", "0", "0",
                                       "10000", "100000",
                                       "GGGGGCCCCAAAATTCCCCAAAATT",
                                       "AAAAACCCCAAAATTCCCCAAAATT"},
                                    scoring_system, paste_parameters)};
    alignment_batch.ResetAlignments(old_alignments, paste_parameters);

    WHEN("New alignments are set.") {
      std::vector<Alignment> new_alignments{
          // score 10, pident 75.0, sstart 1121, send 1160
          Alignment::FromStringFields(5, {"101", "140", "1121", "1160",
                                         "30", "10", "0", "0",
                                         "10000", "100000",
                                         "GGGGGGGGGGCCCCAAAATTCCCCAAAATTCCCCAAAATT",
                                         "AAAAAAAAAACCCCAAAATTCCCCAAAATTCCCCAAAATT"},
                                      scoring_system, paste_parameters),
          // score 0, pident 66.66666667, sstart 1096, send 1110
          Alignment::FromStringFields(6, {"101", "115", "1096", "1110",
                                         "10", "5", "0", "0",
                                         "10000", "100000",
                                         "GGGGGCCCCAAAATT",
                                         "AAAAACCCCAAAATT"},
                                      scoring_system, paste_parameters),
          // score -10, pident 57.14285714, sstart 101, send 135
          Alignment::FromStringFields(7, {"101", "135", "101", "135",
                                         "20", "15", "0", "0",
                                         "10000", "100000",
                                         "GGGGGGGGGGGGGGGCCCCAAAATTCCCCAAAATT",
                                         "AAAAAAAAAAAAAAACCCCAAAATTCCCCAAAATT"},
                                      scoring_system, paste_parameters),
          // score -10, pident 50.0, sstart 201, send 220
          Alignment::FromStringFields(8, {"101", "120", "201", "220",
                                         "10", "10", "0", "0",
                                         "10000", "100000",
                                         "GGGGGGGGGGCCCCAAAATT",
                                         "AAAAAAAAAACCCCAAAATT"},
                                      scoring_system, paste_parameters),
          // score -20, pident 40.0, sstart 2101, send 2125
          Alignment::FromStringFields(9, {"101", "125", "2101", "2125",
                                         "10", "15", "0", "0",
                                         "10000", "100000",
                                         "GGGGGGGGGGGGGGGCCCCAAAATT",
                                         "AAAAAAAAAAAAAAACCCCAAAATT"},
                                      scoring_system, paste_parameters)};
      alignment_batch.ResetAlignments(new_alignments, paste_parameters);

      THEN("Object contains the new alignments.") {
        CHECK(alignment_batch.Alignments() == new_alignments);
      }

      THEN("qseqid and sseqid didn't change.") {
        CHECK(alignment_batch.Qseqid() == "qseqid");
        CHECK(alignment_batch.Sseqid() == "sseqid");
      }
    }
  }
}

SCENARIO("Test invariant preservation by AlignmentBatch::ResetAlignments.",
         "[AlignmentBatch][ResetAlignments][invariants]") {
  PasteParameters paste_parameters;

  GIVEN("An alignment batch and a scoring system.") {
    AlignmentBatch alignment_batch{"qseqid", "sseqid"};
    ScoringSystem scoring_system{ScoringSystem::Create(100000l, 1, 2, 1, 1)};

    WHEN("Alignments have various scores.") {
      std::vector<Alignment> alignments{
          // score 22, pident 96.0, sstart 1101, send 1125
          Alignment::FromStringFields(0, {"101", "125", "1101", "1125",
                                         "24", "1", "0", "0",
                                         "10000", "100000",
                                         "GCCCCAAAATTCCCCAAAATTCCCC",
                                         "ACCCCAAAATTCCCCAAAATTCCCC"},
                                      scoring_system, paste_parameters),
          // score 20, pident 100.0, sstart 1131, send 1150
          Alignment::FromStringFields(1, {"101", "120", "1131", "1150",
                                         "20", "0", "0", "0",
                                         "10000", "100000",
                                         "CCCCAAAATTCCCCAAAATT",
                                         "CCCCAAAATTCCCCAAAATT"},
                                      scoring_system, paste_parameters),
          // score 20, pident 80.0, sstart 1001, send 1050
          Alignment::FromStringFields(2, {"101", "150", "1001", "1050",
                                         "40", "10", "0", "0",
                                         "10000", "100000",
                                         "GGGGGGGGGGCCCCAAAATTCCCCAAAATTCCCCAAAATTCCCCAAAATT",
                                         "AAAAAAAAAACCCCAAAATTCCCCAAAATTCCCCAAAATTCCCCAAAATT"},
                                      scoring_system, paste_parameters),
          // score: 10, pident 100.0, sstart 2111, send 2120
          Alignment::FromStringFields(3, {"101", "110", "2111", "2120",
                                         "10", "0", "0", "0",
                                         "10000", "100000",
                                         "CCCCAAAATT",
                                         "CCCCAAAATT"},
                                      scoring_system, paste_parameters),
          // score 10, pident 80.0, sstart 1111, send 1135
          Alignment::FromStringFields(4, {"101", "125", "1111", "1135",
                                         "20", "5", "0", "0",
                                         "10000", "100000",
                                         "GGGGGCCCCAAAATTCCCCAAAATT",
                                         "AAAAACCCCAAAATTCCCCAAAATT"},
                                      scoring_system, paste_parameters),
          // score 10, pident 75.0, sstart 1121, send 1160
          Alignment::FromStringFields(5, {"101", "140", "1121", "1160",
                                         "30", "10", "0", "0",
                                         "10000", "100000",
                                         "GGGGGGGGGGCCCCAAAATTCCCCAAAATTCCCCAAAATT",
                                         "AAAAAAAAAACCCCAAAATTCCCCAAAATTCCCCAAAATT"},
                                      scoring_system, paste_parameters),
          // score 0, pident 66.66666667, sstart 1096, send 1110
          Alignment::FromStringFields(6, {"101", "115", "1096", "1110",
                                         "10", "5", "0", "0",
                                         "10000", "100000",
                                         "GGGGGCCCCAAAATT",
                                         "AAAAACCCCAAAATT"},
                                      scoring_system, paste_parameters),
          // score -10, pident 57.14285714, sstart 101, send 135
          Alignment::FromStringFields(7, {"101", "135", "101", "135",
                                         "20", "15", "0", "0",
                                         "10000", "100000",
                                         "GGGGGGGGGGGGGGGCCCCAAAATTCCCCAAAATT",
                                         "AAAAAAAAAAAAAAACCCCAAAATTCCCCAAAATT"},
                                      scoring_system, paste_parameters),
          // score -10, pident 50.0, sstart 201, send 220
          Alignment::FromStringFields(8, {"101", "120", "201", "220",
                                         "10", "10", "0", "0",
                                         "10000", "100000",
                                         "GGGGGGGGGGCCCCAAAATT",
                                         "AAAAAAAAAACCCCAAAATT"},
                                      scoring_system, paste_parameters),
          // score -20, pident 40.0, sstart 2101, send 2125
          Alignment::FromStringFields(9, {"101", "125", "2101", "2125",
                                         "10", "15", "0", "0",
                                         "10000", "100000",
                                         "GGGGGGGGGGGGGGGCCCCAAAATT",
                                         "AAAAAAAAAAAAAAACCCCAAAATT"},
                                      scoring_system, paste_parameters)};
      std::random_device rd;
      std::mt19937 g(rd());
      std::vector<std::vector<Alignment>> shuffled_alignments;
      for (int i = 0; i < 10; ++i) {
        shuffled_alignments.push_back(alignments);
        std::shuffle(shuffled_alignments.back().begin(),
                     shuffled_alignments.back().end(),
                     g);
      }
      int i = GENERATE(range(0, 10));
      float epsilon = GENERATE(0.05f, 0.15f);
      paste_parameters.float_epsilon = epsilon;
      std::vector<int> unshuffled_alignment_pos(alignments.size(), -1);
      alignment_batch.ResetAlignments(shuffled_alignments.at(i),
                                      paste_parameters);
      std::unordered_set<int> alignment_positions;
      for (int i = 0; i < alignment_batch.Size(); ++i) {
        alignment_positions.insert(i);
      }

      THEN("Sorted lists contains precisely the alignment positions.") {
        CHECK(std::unordered_set<int>{alignment_batch.ScoreSorted().begin(),
                                      alignment_batch.ScoreSorted().end()}
              == alignment_positions);
        CHECK(std::unordered_set<int>{alignment_batch.QstartSorted().begin(),
                                      alignment_batch.QstartSorted().end()}
              == alignment_positions);
        CHECK(std::unordered_set<int>{alignment_batch.QendSorted().begin(),
                                      alignment_batch.QendSorted().end()}
              == alignment_positions);
      }

      THEN("Sorting by (score, pident) works as expected.") {
        for (int j = 0; j < alignment_batch.ScoreSorted().size() - 1; ++j) {
          CHECK(ScoreComp(alignment_batch.Alignments()
                              .at(alignment_batch.ScoreSorted().at(j)),
                          alignment_batch.Alignments()
                              .at(alignment_batch.ScoreSorted().at(j + 1)),
                          j, j + 1,
                          paste_parameters.float_epsilon));
        }
      }

      THEN("Sorting by qstart works as expected.") {
        for (int j = 0; j < alignment_batch.QstartSorted().size() - 1; ++j) {
          CHECK(alignment_batch.Alignments()
                    .at(alignment_batch.QstartSorted().at(j)).Qstart()
                <= alignment_batch.Alignments()
                      .at(alignment_batch.QstartSorted().at(j + 1)).Qstart());
        }
      }

      THEN("Sorting by qend works as expected.") {
        for (int j = 0; j < alignment_batch.QendSorted().size() - 1; ++j) {
          CHECK(alignment_batch.Alignments()
                    .at(alignment_batch.QendSorted().at(j)).Qend()
                <= alignment_batch.Alignments()
                      .at(alignment_batch.QendSorted().at(j + 1)).Qend());
        }
      }
    }
  }

  GIVEN("A nonempty alignment batch.") {
    AlignmentBatch alignment_batch{"qseqid", "sseqid"};
    ScoringSystem scoring_system{ScoringSystem::Create(100000l, 1, 2, 1, 1)};
    float epsilon = GENERATE(0.05f, 0.15f);
    paste_parameters.float_epsilon = epsilon;
    std::vector<Alignment> old_alignments{
        // score 22, pident 96.0, sstart 1101, send 1125
        Alignment::FromStringFields(0, {"101", "125", "1101", "1125",
                                       "24", "1", "0", "0",
                                       "10000", "100000",
                                       "GCCCCAAAATTCCCCAAAATTCCCC",
                                       "ACCCCAAAATTCCCCAAAATTCCCC"},
                                    scoring_system, paste_parameters),
        // score 20, pident 100.0, sstart 1131, send 1150
        Alignment::FromStringFields(1, {"101", "120", "1131", "1150",
                                       "20", "0", "0", "0",
                                       "10000", "100000",
                                       "CCCCAAAATTCCCCAAAATT",
                                       "CCCCAAAATTCCCCAAAATT"},
                                    scoring_system, paste_parameters),
        // score 20, pident 80.0, sstart 1001, send 1050
        Alignment::FromStringFields(2, {"101", "150", "1001", "1050",
                                       "40", "10", "0", "0",
                                       "10000", "100000",
                                       "GGGGGGGGGGCCCCAAAATTCCCCAAAATTCCCCAAAATTCCCCAAAATT",
                                       "AAAAAAAAAACCCCAAAATTCCCCAAAATTCCCCAAAATTCCCCAAAATT"},
                                    scoring_system, paste_parameters),
        // score: 10, pident 100.0, sstart 2111, send 2120
        Alignment::FromStringFields(3, {"101", "110", "2111", "2120",
                                       "10", "0", "0", "0",
                                       "10000", "100000",
                                       "CCCCAAAATT",
                                       "CCCCAAAATT"},
                                    scoring_system, paste_parameters),
        // score 10, pident 80.0, sstart 1111, send 1135
        Alignment::FromStringFields(4, {"101", "125", "1111", "1135",
                                       "20", "5", "0", "0",
                                       "10000", "100000",
                                       "GGGGGCCCCAAAATTCCCCAAAATT",
                                       "AAAAACCCCAAAATTCCCCAAAATT"},
                                    scoring_system, paste_parameters)};
    alignment_batch.ResetAlignments(old_alignments, paste_parameters);
    WHEN("New alignments are set.") {
      std::vector<Alignment> new_alignments{
          // score 10, pident 75.0, sstart 1121, send 1160
          Alignment::FromStringFields(5, {"101", "140", "1121", "1160",
                                         "30", "10", "0", "0",
                                         "10000", "100000",
                                         "GGGGGGGGGGCCCCAAAATTCCCCAAAATTCCCCAAAATT",
                                         "AAAAAAAAAACCCCAAAATTCCCCAAAATTCCCCAAAATT"},
                                      scoring_system, paste_parameters),
          // score 0, pident 66.66666667, sstart 1096, send 1110
          Alignment::FromStringFields(6, {"101", "115", "1096", "1110",
                                         "10", "5", "0", "0",
                                         "10000", "100000",
                                         "GGGGGCCCCAAAATT",
                                         "AAAAACCCCAAAATT"},
                                      scoring_system, paste_parameters),
          // score -10, pident 57.14285714, sstart 101, send 135
          Alignment::FromStringFields(7, {"101", "135", "101", "135",
                                         "20", "15", "0", "0",
                                         "10000", "100000",
                                         "GGGGGGGGGGGGGGGCCCCAAAATTCCCCAAAATT",
                                         "AAAAAAAAAAAAAAACCCCAAAATTCCCCAAAATT"},
                                      scoring_system, paste_parameters),
          // score -10, pident 50.0, sstart 201, send 220
          Alignment::FromStringFields(8, {"101", "120", "201", "220",
                                         "10", "10", "0", "0",
                                         "10000", "100000",
                                         "GGGGGGGGGGCCCCAAAATT",
                                         "AAAAAAAAAACCCCAAAATT"},
                                      scoring_system, paste_parameters),
          // score -20, pident 40.0, sstart 2101, send 2125
          Alignment::FromStringFields(9, {"101", "125", "2101", "2125",
                                         "10", "15", "0", "0",
                                         "10000", "100000",
                                         "GGGGGGGGGGGGGGGCCCCAAAATT",
                                         "AAAAAAAAAAAAAAACCCCAAAATT"},
                                      scoring_system, paste_parameters)};
      alignment_batch.ResetAlignments(new_alignments, paste_parameters);
      std::unordered_set<int> alignment_positions;
      for (int i = 0; i < alignment_batch.Size(); ++i) {
        alignment_positions.insert(i);
      }

      THEN("Sorted lists contain precisely the alignment positions.") {
        CHECK(std::unordered_set<int>{alignment_batch.ScoreSorted().begin(),
                                      alignment_batch.ScoreSorted().end()}
              == alignment_positions);
        CHECK(std::unordered_set<int>{alignment_batch.QstartSorted().begin(),
                                      alignment_batch.QstartSorted().end()}
              == alignment_positions);
        CHECK(std::unordered_set<int>{alignment_batch.QendSorted().begin(),
                                      alignment_batch.QendSorted().end()}
              == alignment_positions);
      }

      THEN("Sorting by (score, pident) works as expected.") {
        for (int j = 0; j < alignment_batch.ScoreSorted().size() - 1; ++j) {
          CHECK(ScoreComp(alignment_batch.Alignments()
                              .at(alignment_batch.ScoreSorted().at(j)),
                          alignment_batch.Alignments()
                              .at(alignment_batch.ScoreSorted().at(j + 1)),
                          j, j + 1,
                          paste_parameters.float_epsilon));
        }
      }

      THEN("Sorting by qstart works as expected.") {
        for (int j = 0; j < alignment_batch.QstartSorted().size() - 1; ++j) {
          CHECK(alignment_batch.Alignments()
                    .at(alignment_batch.QstartSorted().at(j)).Qstart()
                <= alignment_batch.Alignments()
                      .at(alignment_batch.QstartSorted().at(j + 1)).Qstart());
        }
      }

      THEN("Sorting by qend works as expected.") {
        for (int j = 0; j < alignment_batch.QendSorted().size() - 1; ++j) {
          CHECK(alignment_batch.Alignments()
                    .at(alignment_batch.QendSorted().at(j)).Qend()
                <= alignment_batch.Alignments()
                      .at(alignment_batch.QendSorted().at(j + 1)).Qend());
        }
      }
    }
  }
}

SCENARIO("Test exceptions thrown by"
         " AlignmentBatch::AlignmentBatch(string_view, string_view).",
         "[AlignmentBatch][AlignmentBatch(string_view, string_view)]"
         "[exceptions]") {

  THEN("An empty query sequence identifier causes an exception.") {
    CHECK_THROWS_AS(AlignmentBatch("", "sseqid"),
                    exceptions::UnexpectedEmptyString);
  }

  THEN("An empty subject sequence identifier causes an exception.") {
    CHECK_THROWS_AS(AlignmentBatch("qseqid", ""),
                    exceptions::UnexpectedEmptyString);
  }

  THEN("Empty query and subject sequence identifiers cause an exception.") {
    CHECK_THROWS_AS(AlignmentBatch("", ""), exceptions::UnexpectedEmptyString);
  }
}

SCENARIO("Test exceptions thrown by AlignmentBatch::qseqid(string_view).",
         "[AlignmentBatch][qseqid(string_view)][exceptions]") {

  THEN("An empty query sequence identifier causes an exception.") {
    AlignmentBatch alignment_batch{"qseqid", "sseqid"};
    CHECK_THROWS_AS(alignment_batch.Qseqid(""),
                    exceptions::UnexpectedEmptyString);
  }

  THEN("A nonempty identifier does not cause an exception.") {
    AlignmentBatch alignment_batch{"qseqid", "sseqid"};
    CHECK_NOTHROW(alignment_batch.Qseqid("new_qseqid"));
  }
}

SCENARIO("Test exceptions thrown by AlignmentBatch::sseqid(string_view).",
         "[AlignmentBatch][sseqid(string_view)][exceptions]") {

  THEN("An empty subject sequence identifier causes an exception.") {
    AlignmentBatch alignment_batch{"qseqid", "sseqid"};
    CHECK_THROWS_AS(alignment_batch.Sseqid(""),
                    exceptions::UnexpectedEmptyString);
  }

  THEN("A nonempty identifier does not cause an exception.") {
    AlignmentBatch alignment_batch{"qseqid", "sseqid"};
    CHECK_NOTHROW(alignment_batch.Sseqid("new_sseqid"));
  }
}

SCENARIO("Test correctness of AlignmentBatch::PasteAlignments.",
         "[AlignmentBatch][PasteAlignments][correctness]") {
  PasteParameters paste_parameters;
  AlignmentBatch alignment_batch{"qseqid", "sseqid"};
  ScoringSystem scoring_system{ScoringSystem::Create(100000l, 1, 2, 1, 1)};

  GIVEN("A1, A2, A3 (+), ungapped, all pastable, 0 paste gaps, 0 thresholds.") {
    std::vector<Alignment> alignments{
        Alignment::FromStringFields(0, {"101", "110", "1001", "1010",
                                        "10", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(1, {"111", "130", "1011", "1030",
                                        "20", "0", "0", "0",
                                        "10000", "100000",
                                        "CCCCCCCCCCCCCCCCCCCC",
                                        "CCCCCCCCCCCCCCCCCCCC"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(2, {"131", "145", "1031", "1045",
                                        "15", "0", "0", "0",
                                        "10000", "100000",
                                        "GGGGGGGGGGGGGGG",
                                        "GGGGGGGGGGGGGGG"},
                                       scoring_system, paste_parameters)};
    std::vector<Alignment> original{alignments};
    alignment_batch.ResetAlignments(std::move(alignments), paste_parameters);
    alignment_batch.PasteAlignments(scoring_system, paste_parameters);

    THEN("All are pasted into one and that one is marked final.") {
      original.at(1).PasteRight(original.at(2),
                                GetConfiguration(original.at(1),
                                                 original.at(2)),
                                scoring_system, paste_parameters);
      original.at(1).PasteLeft(original.at(0),
                               GetConfiguration(original.at(0),
                                                original.at(1)),
                               scoring_system, paste_parameters);
      original.at(1).IncludeInOutput(true);
      CHECK(original.at(0) == alignment_batch.Alignments().at(0));
      CHECK(original.at(1) == alignment_batch.Alignments().at(1));
      CHECK(original.at(2) == alignment_batch.Alignments().at(2));
    }
  }
}

} // namespace

} // namespace test

} // namespace paste_alignments