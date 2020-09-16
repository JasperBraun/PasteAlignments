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
//#define CATCH_CONFIG_COLOUR_NONE
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
  ScoringSystem scoring_system{ScoringSystem::Create(100000l, 1, 2, 0, 0)};

  GIVEN("Empty batch.") {
    AlignmentBatch original_batch{alignment_batch};
    alignment_batch.PasteAlignments(scoring_system, paste_parameters);

    THEN("Nothing happens.") {
      CHECK(original_batch == alignment_batch);
    }
  }

  GIVEN("Batch with single alignment.") {
    Alignment a{
        Alignment::FromStringFields(0, {"101", "110", "1001", "1010",
                                        "10", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters)};
    Alignment original{a};
    WHEN("If alignment satisfies final thresholds.") {
      alignment_batch.ResetAlignments({std::move(a)}, paste_parameters);
      alignment_batch.PasteAlignments(scoring_system, paste_parameters);

      THEN("It's marked as final.") {
        original.IncludeInOutput(true);
        CHECK(alignment_batch.Size() == 1);
        CHECK(alignment_batch.Alignments().at(0) == original);
      }
    }

    WHEN("If alignment doesn't satisfy final thresholds.") {
      paste_parameters.final_score_threshold = (2.0f * a.RawScore() + 100.0f);
      alignment_batch.ResetAlignments({std::move(a)}, paste_parameters);
      alignment_batch.PasteAlignments(scoring_system, paste_parameters);

      THEN("It's not marked as final.") {
        original.IncludeInOutput(false);
        CHECK(alignment_batch.Size() == 1);
        CHECK(alignment_batch.Alignments().at(0) == original);
      }
    }
  }

  GIVEN("Two oppositely oriented runs of pastable alignments.") {
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
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(3, {"146", "160", "1046", "1060",
                                        "15", "0", "0", "0",
                                        "10000", "100000",
                                        "GGGGGGGGGGGGGGG",
                                        "GGGGGGGGGGGGGGG"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(4, {"41", "50", "1120", "1111",
                                        "10", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(5, {"51", "70", "1110", "1091",
                                        "20", "0", "0", "0",
                                        "10000", "100000",
                                        "CCCCCCCCCCCCCCCCCCCC",
                                        "CCCCCCCCCCCCCCCCCCCC"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(6, {"71", "85", "1090", "1076",
                                        "15", "0", "0", "0",
                                        "10000", "100000",
                                        "GGGGGGGGGGGGGGG",
                                        "GGGGGGGGGGGGGGG"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(7, {"86", "100", "1075", "1061",
                                        "15", "0", "0", "0",
                                        "10000", "100000",
                                        "GGGGGGGGGGGGGGG",
                                        "GGGGGGGGGGGGGGG"},
                                       scoring_system, paste_parameters)};
    std::vector<Alignment> original{alignments};
    alignment_batch.ResetAlignments(std::move(alignments), paste_parameters);
    alignment_batch.PasteAlignments(scoring_system, paste_parameters);

    THEN("The two runs are pasted independently.") {
      original.at(1).PasteRight(original.at(2),
                                GetConfiguration(original.at(1),
                                                 original.at(2)),
                                scoring_system, paste_parameters);
      original.at(1).PasteRight(original.at(3),
                                GetConfiguration(original.at(1),
                                                 original.at(3)),
                                scoring_system, paste_parameters);
      original.at(1).PasteLeft(original.at(0),
                                GetConfiguration(original.at(0),
                                                 original.at(1)),
                                scoring_system, paste_parameters);
      original.at(1).IncludeInOutput(true);
      original.at(5).PasteRight(original.at(6),
                                GetConfiguration(original.at(5),
                                                 original.at(6)),
                                scoring_system, paste_parameters);
      original.at(5).PasteRight(original.at(7),
                                GetConfiguration(original.at(5),
                                                 original.at(7)),
                                scoring_system, paste_parameters);
      original.at(5).PasteLeft(original.at(4),
                                GetConfiguration(original.at(4),
                                                 original.at(5)),
                                scoring_system, paste_parameters);
      original.at(5).IncludeInOutput(true);
      CHECK(original.at(0) == alignment_batch.Alignments().at(0));
      CHECK(original.at(1) == alignment_batch.Alignments().at(1));
      CHECK(original.at(2) == alignment_batch.Alignments().at(2));
      CHECK(original.at(3) == alignment_batch.Alignments().at(3));
      CHECK(original.at(4) == alignment_batch.Alignments().at(4));
      CHECK(original.at(5) == alignment_batch.Alignments().at(5));
      CHECK(original.at(6) == alignment_batch.Alignments().at(6));
      CHECK(original.at(7) == alignment_batch.Alignments().at(7));
    }
  }

  GIVEN("Two equally scoring alignments competing for another plus.") {
    paste_parameters.final_pident_threshold = 65.0f;
    std::vector<Alignment> alignments{
        Alignment::FromStringFields(0, {"101", "120", "1001", "1020",
                                        "20", "0", "0", "0",
                                        "10000", "100000",
                                        "CCCCCCCCCCCCCCCCCCCC",
                                        "CCCCCCCCCCCCCCCCCCCC"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(1, {"121", "160", "1021", "1060",
                                        "20", "20", "0", "0",
                                        "10000", "100000",
                                        "CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC",
                                        "CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(2, {"58", "100", "958", "1000",
                                        "22", "21", "0", "0",
                                        "10000", "100000",
                                        "CCCCCCCCCCCCCACCCCCCCCCCCCCCCCCCCCCCCCCCCCC",
                                        "CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC"},
                                       scoring_system, paste_parameters)};
    std::vector<Alignment> original{alignments};
    alignment_batch.ResetAlignments(std::move(alignments), paste_parameters);
    alignment_batch.PasteAlignments(scoring_system, paste_parameters);

    THEN("Percent identity breaks tie.") {
      original.at(0).PasteRight(original.at(1),
                                GetConfiguration(original.at(0),
                                                 original.at(1)),
                                scoring_system, paste_parameters);
      original.at(0).IncludeInOutput(true);
      CHECK(original.at(0) == alignment_batch.Alignments().at(0));
      CHECK(original.at(1) == alignment_batch.Alignments().at(1));
      CHECK(original.at(2) == alignment_batch.Alignments().at(2));
    }
  }

  GIVEN("Two equally scoring alignments competing for another minus.") {
    paste_parameters.final_pident_threshold = 65.0f;
    std::vector<Alignment> alignments{
        Alignment::FromStringFields(0, {"101", "120", "1020", "1001",
                                        "20", "0", "0", "0",
                                        "10000", "100000",
                                        "CCCCCCCCCCCCCCCCCCCC",
                                        "CCCCCCCCCCCCCCCCCCCC"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(1, {"121", "160", "1000", "961",
                                        "20", "20", "0", "0",
                                        "10000", "100000",
                                        "CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC",
                                        "CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(2, {"58", "100", "1063", "1021",
                                        "22", "21", "0", "0",
                                        "10000", "100000",
                                        "CCCCCCCCCCCCCACCCCCCCCCCCCCCCCCCCCCCCCCCCCC",
                                        "CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC"},
                                       scoring_system, paste_parameters)};
    std::vector<Alignment> original{alignments};
    alignment_batch.ResetAlignments(std::move(alignments), paste_parameters);
    alignment_batch.PasteAlignments(scoring_system, paste_parameters);

    THEN("Percent identity breaks tie.") {
      original.at(0).PasteRight(original.at(1),
                                GetConfiguration(original.at(0),
                                                 original.at(1)),
                                scoring_system, paste_parameters);
      original.at(0).IncludeInOutput(true);
      CHECK(original.at(0) == alignment_batch.Alignments().at(0));
      CHECK(original.at(1) == alignment_batch.Alignments().at(1));
      CHECK(original.at(2) == alignment_batch.Alignments().at(2));
    }
  }

  GIVEN("Alignments with gaps plus.") {
    std::vector<Alignment> alignments{
        Alignment::FromStringFields(0, {"101", "120", "1001", "1022",
                                        "18", "2", "1", "2",
                                        "10000", "100000",
                                        "CCCCCCCAACCCCCCC--CCCC",
                                        "CCCCCCCCCCCCCCCCGGCCCC"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(1, {"121", "142", "1021", "1040",
                                        "20", "0", "1", "2",
                                        "10000", "100000",
                                        "CCGGCCCCCCCCCCCCCCCCCC",
                                        "CC--CCCCCCCCCCCCCCCCCC"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(2, {"201", "220", "2001", "2022",
                                        "20", "0", "1", "2",
                                        "10000", "100000",
                                        "CCCCCCCCCCCCCCCC--CCCC",
                                        "CCCCCCCCCCCCCCCCGGCCCC"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(3, {"221", "242", "2021", "2040",
                                        "18", "2", "1", "2",
                                        "10000", "100000",
                                        "CCGGCCAACCCCCCCCCCCCCC",
                                        "CC--CCCCCCCCCCCCCCCCCC"},
                                       scoring_system, paste_parameters)};
        alignments.at(0).UngappedPrefixEnd(16);
        alignments.at(0).UngappedSuffixBegin(18);
        alignments.at(1).UngappedPrefixEnd(2);
        alignments.at(1).UngappedSuffixBegin(4);
        alignments.at(2).UngappedPrefixEnd(16);
        alignments.at(2).UngappedSuffixBegin(18);
        alignments.at(3).UngappedPrefixEnd(2);
        alignments.at(3).UngappedSuffixBegin(4);
    std::vector<Alignment> original{alignments};
    alignment_batch.ResetAlignments(std::move(alignments), paste_parameters);
    alignment_batch.PasteAlignments(scoring_system, paste_parameters);

    THEN("Alignments paste only if proper subset of ungapped end is chopped.") {
      original.at(2).PasteRight(original.at(3),
                                GetConfiguration(original.at(2),
                                                 original.at(3)),
                                scoring_system, paste_parameters);
      original.at(0).IncludeInOutput(true);
      original.at(1).IncludeInOutput(true);
      original.at(2).IncludeInOutput(true);
      CHECK(original.at(0) == alignment_batch.Alignments().at(0));
      CHECK(original.at(1) == alignment_batch.Alignments().at(1));
      CHECK(original.at(2) == alignment_batch.Alignments().at(2));
      CHECK(original.at(3) == alignment_batch.Alignments().at(3));
    }
  }

  GIVEN("Alignments with gaps minus.") {
    std::vector<Alignment> alignments{
        Alignment::FromStringFields(0, {"101", "120", "1060", "1039",
                                        "18", "2", "1", "2",
                                        "10000", "100000",
                                        "CCCCCCCAACCCCCCC--CCCC",
                                        "CCCCCCCCCCCCCCCCGGCCCC"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(1, {"121", "142", "1040", "1021",
                                        "20", "0", "1", "2",
                                        "10000", "100000",
                                        "CCGGCCCCCCCCCCCCCCCCCC",
                                        "CC--CCCCCCCCCCCCCCCCCC"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(2, {"201", "220", "2020", "1999",
                                        "20", "0", "1", "2",
                                        "10000", "100000",
                                        "CCCCCCCCCCCCCCCC--CCCC",
                                        "CCCCCCCCCCCCCCCCGGCCCC"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(3, {"221", "242", "2000", "1981",
                                        "18", "2", "1", "2",
                                        "10000", "100000",
                                        "CCGGCCAACCCCCCCCCCCCCC",
                                        "CC--CCCCCCCCCCCCCCCCCC"},
                                       scoring_system, paste_parameters)};
        alignments.at(0).UngappedPrefixEnd(16);
        alignments.at(0).UngappedSuffixBegin(18);
        alignments.at(1).UngappedPrefixEnd(2);
        alignments.at(1).UngappedSuffixBegin(4);
        alignments.at(2).UngappedPrefixEnd(16);
        alignments.at(2).UngappedSuffixBegin(18);
        alignments.at(3).UngappedPrefixEnd(2);
        alignments.at(3).UngappedSuffixBegin(4);
    std::vector<Alignment> original{alignments};
    alignment_batch.ResetAlignments(std::move(alignments), paste_parameters);
    alignment_batch.PasteAlignments(scoring_system, paste_parameters);

    THEN("Alignments paste only if proper subset of ungapped end is chopped.") {
      original.at(2).PasteRight(original.at(3),
                                GetConfiguration(original.at(2),
                                                 original.at(3)),
                                scoring_system, paste_parameters);
      original.at(0).IncludeInOutput(true);
      original.at(1).IncludeInOutput(true);
      original.at(2).IncludeInOutput(true);
      CHECK(original.at(0) == alignment_batch.Alignments().at(0));
      CHECK(original.at(1) == alignment_batch.Alignments().at(1));
      CHECK(original.at(2) == alignment_batch.Alignments().at(2));
      CHECK(original.at(3) == alignment_batch.Alignments().at(3));
    }
  }

  GIVEN("Gap tolerance; distance shifts; plus.") {
    paste_parameters.gap_tolerance = 4;
    std::vector<Alignment> alignments{
        Alignment::FromStringFields(0, {"101", "110", "1001", "1010",
                                        "10", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(1, {"111", "120", "1015", "1024",
                                        "10", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(2, {"126", "135", "1025", "1034",
                                        "10", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(3, {"136", "145", "1039", "1048",
                                        "10", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters)};
    std::vector<Alignment> original{alignments};
    alignment_batch.ResetAlignments(std::move(alignments), paste_parameters);
    alignment_batch.PasteAlignments(scoring_system, paste_parameters);

    THEN("Too long gaps are avoided.") {
      original.at(0).PasteRight(original.at(1),
                                GetConfiguration(original.at(0),
                                                 original.at(1)),
                                scoring_system, paste_parameters);
      original.at(2).PasteRight(original.at(3),
                                GetConfiguration(original.at(2),
                                                 original.at(3)),
                                scoring_system, paste_parameters);
      original.at(0).IncludeInOutput(true);
      original.at(2).IncludeInOutput(true);
      CHECK(original.at(0) == alignment_batch.Alignments().at(0));
      CHECK(original.at(1) == alignment_batch.Alignments().at(1));
      CHECK(original.at(2) == alignment_batch.Alignments().at(2));
      CHECK(original.at(3) == alignment_batch.Alignments().at(3));
    }
  }

  GIVEN("Gap tolerance; distance shifts; minus.") {
    paste_parameters.gap_tolerance = 4;
    std::vector<Alignment> alignments{
        Alignment::FromStringFields(0, {"101", "110", "1048", "1039",
                                        "10", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(1, {"111", "120", "1034", "1025",
                                        "10", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(2, {"126", "135", "1024", "1015",
                                        "10", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(3, {"136", "145", "1010", "1001",
                                        "10", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters)};
    std::vector<Alignment> original{alignments};
    alignment_batch.ResetAlignments(std::move(alignments), paste_parameters);
    alignment_batch.PasteAlignments(scoring_system, paste_parameters);

    THEN("Too long gaps are avoided.") {
      original.at(0).PasteRight(original.at(1),
                                GetConfiguration(original.at(0),
                                                 original.at(1)),
                                scoring_system, paste_parameters);
      original.at(2).PasteRight(original.at(3),
                                GetConfiguration(original.at(2),
                                                 original.at(3)),
                                scoring_system, paste_parameters);
      original.at(0).IncludeInOutput(true);
      original.at(2).IncludeInOutput(true);
      CHECK(original.at(0) == alignment_batch.Alignments().at(0));
      CHECK(original.at(1) == alignment_batch.Alignments().at(1));
      CHECK(original.at(2) == alignment_batch.Alignments().at(2));
      CHECK(original.at(3) == alignment_batch.Alignments().at(3));
    }
  }

  GIVEN("Gap tolerance; overlap shifts; plus.") {
    paste_parameters.gap_tolerance = 4;
    std::vector<Alignment> alignments{
        Alignment::FromStringFields(0, {"101", "110", "1001", "1010",
                                        "10", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(1, {"111", "120", "1007", "1016",
                                        "10", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(2, {"116", "125", "1017", "1026",
                                        "10", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(3, {"126", "135", "1023", "1032",
                                        "10", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters)};
    std::vector<Alignment> original{alignments};
    alignment_batch.ResetAlignments(std::move(alignments), paste_parameters);
    alignment_batch.PasteAlignments(scoring_system, paste_parameters);

    THEN("Too long gaps are avoided.") {
      original.at(0).PasteRight(original.at(1),
                                GetConfiguration(original.at(0),
                                                 original.at(1)),
                                scoring_system, paste_parameters);
      original.at(0).PasteRight(original.at(3),
                                GetConfiguration(original.at(0),
                                                 original.at(3)),
                                scoring_system, paste_parameters);
      original.at(0).IncludeInOutput(true);
      original.at(2).IncludeInOutput(true);
      CHECK(original.at(0) == alignment_batch.Alignments().at(0));
      CHECK(original.at(1) == alignment_batch.Alignments().at(1));
      CHECK(original.at(2) == alignment_batch.Alignments().at(2));
      CHECK(original.at(3) == alignment_batch.Alignments().at(3));
    }
  }

  GIVEN("Gap tolerance; overlap shifts; minus.") {
    paste_parameters.gap_tolerance = 4;
    std::vector<Alignment> alignments{
        Alignment::FromStringFields(0, {"101", "110", "1032", "1023",
                                        "10", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(1, {"111", "120", "1026", "1017",
                                        "10", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(2, {"116", "125", "1016", "1007",
                                        "10", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(3, {"126", "135", "1010", "1001",
                                        "10", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters)};
    std::vector<Alignment> original{alignments};
    alignment_batch.ResetAlignments(std::move(alignments), paste_parameters);
    alignment_batch.PasteAlignments(scoring_system, paste_parameters);

    THEN("Too long gaps are avoided.") {
      original.at(0).PasteRight(original.at(1),
                                GetConfiguration(original.at(0),
                                                 original.at(1)),
                                scoring_system, paste_parameters);
      original.at(0).PasteRight(original.at(3),
                                GetConfiguration(original.at(0),
                                                 original.at(3)),
                                scoring_system, paste_parameters);
      original.at(0).IncludeInOutput(true);
      original.at(2).IncludeInOutput(true);
      CHECK(original.at(0) == alignment_batch.Alignments().at(0));
      CHECK(original.at(1) == alignment_batch.Alignments().at(1));
      CHECK(original.at(2) == alignment_batch.Alignments().at(2));
      CHECK(original.at(3) == alignment_batch.Alignments().at(3));
    }
  }

  GIVEN("Gap tolerance; distance, overlap shifts; plus.") {
    paste_parameters.gap_tolerance = 4;
    std::vector<Alignment> alignments{
        Alignment::FromStringFields(0, {"101", "110", "1001", "1010",
                                        "10", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(1, {"113", "122", "1009", "1018",
                                        "10", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(2, {"120", "129", "1021", "1030",
                                        "10", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(3, {"132", "141", "1029", "1038",
                                        "10", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters)};
    std::vector<Alignment> original{alignments};
    alignment_batch.ResetAlignments(std::move(alignments), paste_parameters);
    alignment_batch.PasteAlignments(scoring_system, paste_parameters);

    THEN("Too long gaps are avoided.") {
      original.at(0).PasteRight(original.at(1),
                                GetConfiguration(original.at(0),
                                                 original.at(1)),
                                scoring_system, paste_parameters);
      original.at(2).PasteRight(original.at(3),
                                GetConfiguration(original.at(2),
                                                 original.at(3)),
                                scoring_system, paste_parameters);
      original.at(0).IncludeInOutput(true);
      original.at(2).IncludeInOutput(true);
      CHECK(original.at(0) == alignment_batch.Alignments().at(0));
      CHECK(original.at(1) == alignment_batch.Alignments().at(1));
      CHECK(original.at(2) == alignment_batch.Alignments().at(2));
      CHECK(original.at(3) == alignment_batch.Alignments().at(3));
    }
  }

  GIVEN("Gap tolerance; distance, overlap shifts; minus.") {
    paste_parameters.gap_tolerance = 4;
    std::vector<Alignment> alignments{
        Alignment::FromStringFields(0, {"101", "110", "1038", "1029",
                                        "10", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(1, {"113", "122", "1030", "1021",
                                        "10", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(2, {"120", "129", "1018", "1009",
                                        "10", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(3, {"132", "141", "1010", "1001",
                                        "10", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters)};
    std::vector<Alignment> original{alignments};
    alignment_batch.ResetAlignments(std::move(alignments), paste_parameters);
    alignment_batch.PasteAlignments(scoring_system, paste_parameters);

    THEN("Too long gaps are avoided.") {
      original.at(0).PasteRight(original.at(1),
                                GetConfiguration(original.at(0),
                                                 original.at(1)),
                                scoring_system, paste_parameters);
      original.at(2).PasteRight(original.at(3),
                                GetConfiguration(original.at(2),
                                                 original.at(3)),
                                scoring_system, paste_parameters);
      original.at(0).IncludeInOutput(true);
      original.at(2).IncludeInOutput(true);
      CHECK(original.at(0) == alignment_batch.Alignments().at(0));
      CHECK(original.at(1) == alignment_batch.Alignments().at(1));
      CHECK(original.at(2) == alignment_batch.Alignments().at(2));
      CHECK(original.at(3) == alignment_batch.Alignments().at(3));
    }
  }

  GIVEN("Final pident threshold; mismatches; plus.") {
    paste_parameters.final_pident_threshold = 90.0f;
    std::vector<Alignment> alignments{
        Alignment::FromStringFields(0, {"101", "120", "1001", "1020",
                                        "18", "2", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAACCAAAAAAAAA",
                                        "AAAAAAAAAAAAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(1, {"121", "130", "1021", "1030",
                                        "8", "2", "0", "0",
                                        "10000", "100000",
                                        "AAAACCAAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(2, {"131", "140", "1031", "1040",
                                        "10", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(3, {"141", "150", "1041", "1050",
                                        "8", "2", "0", "0",
                                        "10000", "100000",
                                        "AAAACCAAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters)};
    std::vector<Alignment> original{alignments};
    alignment_batch.ResetAlignments(std::move(alignments), paste_parameters);
    alignment_batch.PasteAlignments(scoring_system, paste_parameters);

    THEN("Alignment pasting rolls back to last final.") {
      original.at(0).PasteRight(original.at(1),
                                GetConfiguration(original.at(0),
                                                 original.at(1)),
                                scoring_system, paste_parameters);
      original.at(0).PasteRight(original.at(2),
                                GetConfiguration(original.at(0),
                                                 original.at(2)),
                                scoring_system, paste_parameters);
      original.at(0).IncludeInOutput(true);
      CHECK(original.at(0) == alignment_batch.Alignments().at(0));
      CHECK(original.at(1) == alignment_batch.Alignments().at(1));
      CHECK(original.at(2) == alignment_batch.Alignments().at(2));
      CHECK(original.at(3) == alignment_batch.Alignments().at(3));
    }
  }

  GIVEN("Final pident threshold; mismatches; minus.") {
    paste_parameters.final_pident_threshold = 90.0f;
    std::vector<Alignment> alignments{
        Alignment::FromStringFields(0, {"101", "120", "1050", "1031",
                                        "18", "2", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAACCAAAAAAAAA",
                                        "AAAAAAAAAAAAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(1, {"121", "130", "1030", "1021",
                                        "8", "2", "0", "0",
                                        "10000", "100000",
                                        "AAAACCAAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(2, {"131", "140", "1020", "1011",
                                        "10", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(3, {"141", "150", "1010", "1001",
                                        "8", "2", "0", "0",
                                        "10000", "100000",
                                        "AAAACCAAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters)};
    std::vector<Alignment> original{alignments};
    alignment_batch.ResetAlignments(std::move(alignments), paste_parameters);
    alignment_batch.PasteAlignments(scoring_system, paste_parameters);

    THEN("Alignment pasting rolls back to last final.") {
      original.at(0).PasteRight(original.at(1),
                                GetConfiguration(original.at(0),
                                                 original.at(1)),
                                scoring_system, paste_parameters);
      original.at(0).PasteRight(original.at(2),
                                GetConfiguration(original.at(0),
                                                 original.at(2)),
                                scoring_system, paste_parameters);
      original.at(0).IncludeInOutput(true);
      CHECK(original.at(0) == alignment_batch.Alignments().at(0));
      CHECK(original.at(1) == alignment_batch.Alignments().at(1));
      CHECK(original.at(2) == alignment_batch.Alignments().at(2));
      CHECK(original.at(3) == alignment_batch.Alignments().at(3));
    }
  }

  GIVEN("Final pident threshold; distance shifts; plus.") {
    paste_parameters.final_pident_threshold = 90.0f;
    std::vector<Alignment> alignments{
        Alignment::FromStringFields(0, {"101", "120", "1001", "1020",
                                        "20", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAAAAAAAAAAAA",
                                        "AAAAAAAAAAAAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(1, {"121", "126", "1025", "1030",
                                        "6", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAA",
                                        "AAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(2, {"127", "136", "1031", "1040",
                                        "10", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(3, {"139", "146", "1041", "1048",
                                        "8", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAA",
                                        "AAAAAAAA"},
                                       scoring_system, paste_parameters)};
    std::vector<Alignment> original{alignments};
    alignment_batch.ResetAlignments(std::move(alignments), paste_parameters);
    alignment_batch.PasteAlignments(scoring_system, paste_parameters);

    THEN("Alignment pasting rolls back to last final.") {
      original.at(0).PasteRight(original.at(1),
                                GetConfiguration(original.at(0),
                                                 original.at(1)),
                                scoring_system, paste_parameters);
      original.at(0).PasteRight(original.at(2),
                                GetConfiguration(original.at(0),
                                                 original.at(2)),
                                scoring_system, paste_parameters);
      original.at(0).IncludeInOutput(true);
      original.at(3).IncludeInOutput(true);
      CHECK(original.at(0) == alignment_batch.Alignments().at(0));
      CHECK(original.at(1) == alignment_batch.Alignments().at(1));
      CHECK(original.at(2) == alignment_batch.Alignments().at(2));
      CHECK(original.at(3) == alignment_batch.Alignments().at(3));
    }
  }

  GIVEN("Final pident threshold; distance shifts; minus.") {
    paste_parameters.final_pident_threshold = 90.0f;
    std::vector<Alignment> alignments{
        Alignment::FromStringFields(0, {"101", "120", "1048", "1029",
                                        "20", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAAAAAAAAAAAA",
                                        "AAAAAAAAAAAAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(1, {"121", "126", "1024", "1019",
                                        "6", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAA",
                                        "AAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(2, {"127", "136", "1018", "1009",
                                        "10", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(3, {"139", "146", "1006", "999",
                                        "8", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAA",
                                        "AAAAAAAA"},
                                       scoring_system, paste_parameters)};
    std::vector<Alignment> original{alignments};
    alignment_batch.ResetAlignments(std::move(alignments), paste_parameters);
    alignment_batch.PasteAlignments(scoring_system, paste_parameters);

    THEN("Alignment pasting rolls back to last final.") {
      original.at(0).PasteRight(original.at(1),
                                GetConfiguration(original.at(0),
                                                 original.at(1)),
                                scoring_system, paste_parameters);
      original.at(0).PasteRight(original.at(2),
                                GetConfiguration(original.at(0),
                                                 original.at(2)),
                                scoring_system, paste_parameters);
      original.at(0).IncludeInOutput(true);
      original.at(3).IncludeInOutput(true);
      CHECK(original.at(0) == alignment_batch.Alignments().at(0));
      CHECK(original.at(1) == alignment_batch.Alignments().at(1));
      CHECK(original.at(2) == alignment_batch.Alignments().at(2));
      CHECK(original.at(3) == alignment_batch.Alignments().at(3));
    }
  }

  GIVEN("Final pident threshold; overlap shifts; plus.") {
    paste_parameters.final_pident_threshold = 90.0f;
    std::vector<Alignment> alignments{
        Alignment::FromStringFields(0, {"101", "120", "1001", "1020",
                                        "20", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAAAAAAAAAAAA",
                                        "AAAAAAAAAAAAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(1, {"121", "130", "1017", "1026",
                                        "10", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(2, {"131", "140", "1027", "1036",
                                        "10", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(3, {"139", "148", "1037", "1046",
                                        "10", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters)};
    std::vector<Alignment> original{alignments};
    alignment_batch.ResetAlignments(std::move(alignments), paste_parameters);
    alignment_batch.PasteAlignments(scoring_system, paste_parameters);

    THEN("Alignment pasting rolls back to last final.") {
      original.at(0).PasteRight(original.at(1),
                                GetConfiguration(original.at(0),
                                                 original.at(1)),
                                scoring_system, paste_parameters);
      original.at(0).PasteRight(original.at(2),
                                GetConfiguration(original.at(0),
                                                 original.at(2)),
                                scoring_system, paste_parameters);
      original.at(0).IncludeInOutput(true);
      original.at(3).IncludeInOutput(true);
      CHECK(original.at(0) == alignment_batch.Alignments().at(0));
      CHECK(original.at(1) == alignment_batch.Alignments().at(1));
      CHECK(original.at(2) == alignment_batch.Alignments().at(2));
      CHECK(original.at(3) == alignment_batch.Alignments().at(3));
    }
  }

  GIVEN("Final pident threshold; overlap shifts; minus.") {
    paste_parameters.final_pident_threshold = 90.0f;
    std::vector<Alignment> alignments{
        Alignment::FromStringFields(0, {"101", "120", "1046", "1027",
                                        "20", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAAAAAAAAAAAA",
                                        "AAAAAAAAAAAAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(1, {"121", "130", "1030", "1021",
                                        "10", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(2, {"131", "140", "1020", "1011",
                                        "10", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(3, {"139", "148", "1010", "1001",
                                        "10", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters)};
    std::vector<Alignment> original{alignments};
    alignment_batch.ResetAlignments(std::move(alignments), paste_parameters);
    alignment_batch.PasteAlignments(scoring_system, paste_parameters);

    THEN("Alignment pasting rolls back to last final.") {
      original.at(0).PasteRight(original.at(1),
                                GetConfiguration(original.at(0),
                                                 original.at(1)),
                                scoring_system, paste_parameters);
      original.at(0).PasteRight(original.at(2),
                                GetConfiguration(original.at(0),
                                                 original.at(2)),
                                scoring_system, paste_parameters);
      original.at(0).IncludeInOutput(true);
      original.at(3).IncludeInOutput(true);
      CHECK(original.at(0) == alignment_batch.Alignments().at(0));
      CHECK(original.at(1) == alignment_batch.Alignments().at(1));
      CHECK(original.at(2) == alignment_batch.Alignments().at(2));
      CHECK(original.at(3) == alignment_batch.Alignments().at(3));
    }
  }

  GIVEN("Final pident threshold; overlap, distance shifts; plus.") {
    paste_parameters.final_pident_threshold = 90.0f;
    std::vector<Alignment> alignments{
        Alignment::FromStringFields(0, {"101", "120", "1001", "1020",
                                        "20", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAAAAAAAAAAAA",
                                        "AAAAAAAAAAAAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(1, {"123", "130", "1019", "1026",
                                        "8", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAA",
                                        "AAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(2, {"131", "140", "1027", "1036",
                                        "10", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(3, {"140", "148", "1038", "1046",
                                        "9", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAA",
                                        "AAAAAAAAA"},
                                       scoring_system, paste_parameters)};
    std::vector<Alignment> original{alignments};
    alignment_batch.ResetAlignments(std::move(alignments), paste_parameters);
    alignment_batch.PasteAlignments(scoring_system, paste_parameters);

    THEN("Alignment pasting rolls back to last final.") {
      original.at(0).PasteRight(original.at(1),
                                GetConfiguration(original.at(0),
                                                 original.at(1)),
                                scoring_system, paste_parameters);
      original.at(0).PasteRight(original.at(2),
                                GetConfiguration(original.at(0),
                                                 original.at(2)),
                                scoring_system, paste_parameters);
      original.at(0).IncludeInOutput(true);
      original.at(3).IncludeInOutput(true);
      CHECK(original.at(0) == alignment_batch.Alignments().at(0));
      CHECK(original.at(1) == alignment_batch.Alignments().at(1));
      CHECK(original.at(2) == alignment_batch.Alignments().at(2));
      CHECK(original.at(3) == alignment_batch.Alignments().at(3));
    }
  }

  GIVEN("Final pident threshold; overlap, distance shifts; minus.") {
    paste_parameters.final_pident_threshold = 90.0f;
    std::vector<Alignment> alignments{
        Alignment::FromStringFields(0, {"101", "120", "1046", "1027",
                                        "20", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAAAAAAAAAAAA",
                                        "AAAAAAAAAAAAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(1, {"123", "130", "1028", "1021",
                                        "8", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAA",
                                        "AAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(2, {"131", "140", "1020", "1011",
                                        "10", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(3, {"140", "148", "1009", "1001",
                                        "9", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAA",
                                        "AAAAAAAAA"},
                                       scoring_system, paste_parameters)};
    std::vector<Alignment> original{alignments};
    alignment_batch.ResetAlignments(std::move(alignments), paste_parameters);
    alignment_batch.PasteAlignments(scoring_system, paste_parameters);

    THEN("Alignment pasting rolls back to last final.") {
      original.at(0).PasteRight(original.at(1),
                                GetConfiguration(original.at(0),
                                                 original.at(1)),
                                scoring_system, paste_parameters);
      original.at(0).PasteRight(original.at(2),
                                GetConfiguration(original.at(0),
                                                 original.at(2)),
                                scoring_system, paste_parameters);
      original.at(0).IncludeInOutput(true);
      original.at(3).IncludeInOutput(true);
      CHECK(original.at(0) == alignment_batch.Alignments().at(0));
      CHECK(original.at(1) == alignment_batch.Alignments().at(1));
      CHECK(original.at(2) == alignment_batch.Alignments().at(2));
      CHECK(original.at(3) == alignment_batch.Alignments().at(3));
    }
  }

  GIVEN("Final pident threshold; distances; plus.") {
    paste_parameters.final_pident_threshold = 90.0f;
    std::vector<Alignment> alignments{
        Alignment::FromStringFields(0, {"101", "120", "1001", "1020",
                                        "20", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAAAAAAAAAAAA",
                                        "AAAAAAAAAAAAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(1, {"125", "130", "1025", "1030",
                                        "6", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAA",
                                        "AAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(2, {"131", "140", "1031", "1040",
                                        "10", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(3, {"143", "150", "1043", "1050",
                                        "8", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAA",
                                        "AAAAAAAA"},
                                       scoring_system, paste_parameters)};
    std::vector<Alignment> original{alignments};
    alignment_batch.ResetAlignments(std::move(alignments), paste_parameters);
    alignment_batch.PasteAlignments(scoring_system, paste_parameters);

    THEN("Alignment pasting rolls back to last final.") {
      original.at(0).PasteRight(original.at(1),
                                GetConfiguration(original.at(0),
                                                 original.at(1)),
                                scoring_system, paste_parameters);
      original.at(0).PasteRight(original.at(2),
                                GetConfiguration(original.at(0),
                                                 original.at(2)),
                                scoring_system, paste_parameters);
      original.at(0).IncludeInOutput(true);
      original.at(3).IncludeInOutput(true);
      CHECK(original.at(0) == alignment_batch.Alignments().at(0));
      CHECK(original.at(1) == alignment_batch.Alignments().at(1));
      CHECK(original.at(2) == alignment_batch.Alignments().at(2));
      CHECK(original.at(3) == alignment_batch.Alignments().at(3));
    }
  }

  GIVEN("Final pident threshold; distances; minus.") {
    paste_parameters.final_pident_threshold = 90.0f;
    std::vector<Alignment> alignments{
        Alignment::FromStringFields(0, {"101", "120", "1050", "1031",
                                        "20", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAAAAAAAAAAAA",
                                        "AAAAAAAAAAAAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(1, {"125", "130", "1026", "1021",
                                        "6", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAA",
                                        "AAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(2, {"131", "140", "1020", "1011",
                                        "10", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(3, {"143", "150", "1008", "1001",
                                        "8", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAA",
                                        "AAAAAAAA"},
                                       scoring_system, paste_parameters)};
    std::vector<Alignment> original{alignments};
    alignment_batch.ResetAlignments(std::move(alignments), paste_parameters);
    alignment_batch.PasteAlignments(scoring_system, paste_parameters);

    THEN("Alignment pasting rolls back to last final.") {
      original.at(0).PasteRight(original.at(1),
                                GetConfiguration(original.at(0),
                                                 original.at(1)),
                                scoring_system, paste_parameters);
      original.at(0).PasteRight(original.at(2),
                                GetConfiguration(original.at(0),
                                                 original.at(2)),
                                scoring_system, paste_parameters);
      original.at(0).IncludeInOutput(true);
      original.at(3).IncludeInOutput(true);
      CHECK(original.at(0) == alignment_batch.Alignments().at(0));
      CHECK(original.at(1) == alignment_batch.Alignments().at(1));
      CHECK(original.at(2) == alignment_batch.Alignments().at(2));
      CHECK(original.at(3) == alignment_batch.Alignments().at(3));
    }
  }

  GIVEN("Final pident threshold; gaps; plus.") {
    paste_parameters.final_pident_threshold = 90.0f;
    std::vector<Alignment> alignments{
        Alignment::FromStringFields(0, {"101", "118", "1001", "1020",
                                        "18", "0", "1", "2",
                                        "10000", "100000",
                                        "AAAAAAAAA--AAAAAAAAA",
                                        "AAAAAAAAAAAAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(1, {"119", "128", "1021", "1028",
                                        "8", "0", "1", "2",
                                        "10000", "100000",
                                        "AAAAAAAAAA",
                                        "AAAA--AAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(2, {"129", "138", "1029", "1038",
                                        "10", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(3, {"139", "146", "1039", "1048",
                                        "8", "0", "1", "2",
                                        "10000", "100000",
                                        "AAAA--AAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters)};
    alignments.at(0).UngappedPrefixEnd(9);
    alignments.at(0).UngappedSuffixBegin(11);
    alignments.at(1).UngappedPrefixEnd(4);
    alignments.at(1).UngappedSuffixBegin(6);
    alignments.at(3).UngappedPrefixEnd(4);
    alignments.at(3).UngappedSuffixBegin(6);
    std::vector<Alignment> original{alignments};
    alignment_batch.ResetAlignments(std::move(alignments), paste_parameters);
    alignment_batch.PasteAlignments(scoring_system, paste_parameters);

    THEN("Alignment pasting rolls back to last final.") {
      original.at(0).PasteRight(original.at(1),
                                GetConfiguration(original.at(0),
                                                 original.at(1)),
                                scoring_system, paste_parameters);
      original.at(0).PasteRight(original.at(2),
                                GetConfiguration(original.at(0),
                                                 original.at(2)),
                                scoring_system, paste_parameters);
      original.at(0).IncludeInOutput(true);
      CHECK(original.at(0) == alignment_batch.Alignments().at(0));
      CHECK(original.at(1) == alignment_batch.Alignments().at(1));
      CHECK(original.at(2) == alignment_batch.Alignments().at(2));
      CHECK(original.at(3) == alignment_batch.Alignments().at(3));
    }
  }

  GIVEN("Final pident threshold; gaps; minus.") {
    paste_parameters.final_pident_threshold = 90.0f;
    std::vector<Alignment> alignments{
        Alignment::FromStringFields(0, {"101", "118", "1048", "1029",
                                        "18", "0", "1", "2",
                                        "10000", "100000",
                                        "AAAAAAAAA--AAAAAAAAA",
                                        "AAAAAAAAAAAAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(1, {"119", "128", "1028", "1021",
                                        "8", "0", "1", "2",
                                        "10000", "100000",
                                        "AAAAAAAAAA",
                                        "AAAA--AAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(2, {"129", "138", "1020", "1011",
                                        "10", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(3, {"139", "146", "1010", "1001",
                                        "8", "0", "1", "2",
                                        "10000", "100000",
                                        "AAAA--AAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters)};
    alignments.at(0).UngappedPrefixEnd(9);
    alignments.at(0).UngappedSuffixBegin(11);
    alignments.at(1).UngappedPrefixEnd(4);
    alignments.at(1).UngappedSuffixBegin(6);
    alignments.at(3).UngappedPrefixEnd(4);
    alignments.at(3).UngappedSuffixBegin(6);
    std::vector<Alignment> original{alignments};
    alignment_batch.ResetAlignments(std::move(alignments), paste_parameters);
    alignment_batch.PasteAlignments(scoring_system, paste_parameters);

    THEN("Alignment pasting rolls back to last final.") {
      original.at(0).PasteRight(original.at(1),
                                GetConfiguration(original.at(0),
                                                 original.at(1)),
                                scoring_system, paste_parameters);
      original.at(0).PasteRight(original.at(2),
                                GetConfiguration(original.at(0),
                                                 original.at(2)),
                                scoring_system, paste_parameters);
      original.at(0).IncludeInOutput(true);
      CHECK(original.at(0) == alignment_batch.Alignments().at(0));
      CHECK(original.at(1) == alignment_batch.Alignments().at(1));
      CHECK(original.at(2) == alignment_batch.Alignments().at(2));
      CHECK(original.at(3) == alignment_batch.Alignments().at(3));
    }
  }

  GIVEN("Final score threshold; mismatches; plus.") {
    paste_parameters.final_score_threshold = 14.0f;
    std::vector<Alignment> alignments{
        Alignment::FromStringFields(0, {"101", "120", "1001", "1020",
                                        "18", "2", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAACCAAAAAAAAA",
                                        "AAAAAAAAAAAAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(1, {"121", "134", "1021", "1034",
                                        "6", "8", "0", "0",
                                        "10000", "100000",
                                        "AAACCCCCCCCAAA",
                                        "AAAAAAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(2, {"135", "144", "1035", "1044",
                                        "10", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(3, {"145", "154", "1045", "1054",
                                        "6", "4", "0", "0",
                                        "10000", "100000",
                                        "AACCCCAAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters)};
    std::vector<Alignment> original{alignments};
    alignment_batch.ResetAlignments(std::move(alignments), paste_parameters);
    alignment_batch.PasteAlignments(scoring_system, paste_parameters);

    THEN("Alignment pasting rolls back to last final.") {
      original.at(0).PasteRight(original.at(1),
                                GetConfiguration(original.at(0),
                                                 original.at(1)),
                                scoring_system, paste_parameters);
      original.at(0).PasteRight(original.at(2),
                                GetConfiguration(original.at(0),
                                                 original.at(2)),
                                scoring_system, paste_parameters);
      original.at(0).IncludeInOutput(true);
      CHECK(original.at(0) == alignment_batch.Alignments().at(0));
      CHECK(original.at(1) == alignment_batch.Alignments().at(1));
      CHECK(original.at(2) == alignment_batch.Alignments().at(2));
      CHECK(original.at(3) == alignment_batch.Alignments().at(3));
    }
  }

  GIVEN("Final score threshold; mismatches; minus.") {
    paste_parameters.final_score_threshold = 14.0f;
    std::vector<Alignment> alignments{
        Alignment::FromStringFields(0, {"101", "120", "1054", "1035",
                                        "18", "2", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAACCAAAAAAAAA",
                                        "AAAAAAAAAAAAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(1, {"121", "134", "1034", "1021",
                                        "6", "8", "0", "0",
                                        "10000", "100000",
                                        "AAACCCCCCCCAAA",
                                        "AAAAAAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(2, {"135", "144", "1020", "1011",
                                        "10", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(3, {"145", "154", "1010", "1001",
                                        "6", "4", "0", "0",
                                        "10000", "100000",
                                        "AACCCCAAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters)};
    std::vector<Alignment> original{alignments};
    alignment_batch.ResetAlignments(std::move(alignments), paste_parameters);
    alignment_batch.PasteAlignments(scoring_system, paste_parameters);

    THEN("Alignment pasting rolls back to last final.") {
      original.at(0).PasteRight(original.at(1),
                                GetConfiguration(original.at(0),
                                                 original.at(1)),
                                scoring_system, paste_parameters);
      original.at(0).PasteRight(original.at(2),
                                GetConfiguration(original.at(0),
                                                 original.at(2)),
                                scoring_system, paste_parameters);
      original.at(0).IncludeInOutput(true);
      CHECK(original.at(0) == alignment_batch.Alignments().at(0));
      CHECK(original.at(1) == alignment_batch.Alignments().at(1));
      CHECK(original.at(2) == alignment_batch.Alignments().at(2));
      CHECK(original.at(3) == alignment_batch.Alignments().at(3));
    }
  }

  GIVEN("Final score threshold; distance shifts; plus.") {
    paste_parameters.final_score_threshold = 20.0f;
    std::vector<Alignment> alignments{
        Alignment::FromStringFields(0, {"101", "120", "1001", "1020",
                                        "20", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAAAAAAAAAAAA",
                                        "AAAAAAAAAAAAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(1, {"121", "125", "1027", "1031",
                                        "5", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAA",
                                        "AAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(2, {"126", "135", "1032", "1041",
                                        "10", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(3, {"140", "147", "1042", "1049",
                                        "8", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAA",
                                        "AAAAAAAA"},
                                       scoring_system, paste_parameters)};
    std::vector<Alignment> original{alignments};
    alignment_batch.ResetAlignments(std::move(alignments), paste_parameters);
    alignment_batch.PasteAlignments(scoring_system, paste_parameters);

    THEN("Alignment pasting rolls back to last final.") {
      original.at(0).PasteRight(original.at(1),
                                GetConfiguration(original.at(0),
                                                 original.at(1)),
                                scoring_system, paste_parameters);
      original.at(0).PasteRight(original.at(2),
                                GetConfiguration(original.at(0),
                                                 original.at(2)),
                                scoring_system, paste_parameters);
      original.at(0).IncludeInOutput(true);
      CHECK(original.at(0) == alignment_batch.Alignments().at(0));
      CHECK(original.at(1) == alignment_batch.Alignments().at(1));
      CHECK(original.at(2) == alignment_batch.Alignments().at(2));
      CHECK(original.at(3) == alignment_batch.Alignments().at(3));
    }
  }

  GIVEN("Final score threshold; distance shifts; minus.") {
    paste_parameters.final_score_threshold = 20.0f;
    std::vector<Alignment> alignments{
        Alignment::FromStringFields(0, {"101", "120", "1049", "1030",
                                        "20", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAAAAAAAAAAAA",
                                        "AAAAAAAAAAAAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(1, {"121", "125", "1023", "1019",
                                        "5", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAA",
                                        "AAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(2, {"126", "135", "1018", "1009",
                                        "10", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(3, {"140", "147", "1008", "1001",
                                        "8", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAA",
                                        "AAAAAAAA"},
                                       scoring_system, paste_parameters)};
    std::vector<Alignment> original{alignments};
    alignment_batch.ResetAlignments(std::move(alignments), paste_parameters);
    alignment_batch.PasteAlignments(scoring_system, paste_parameters);

    THEN("Alignment pasting rolls back to last final.") {
      original.at(0).PasteRight(original.at(1),
                                GetConfiguration(original.at(0),
                                                 original.at(1)),
                                scoring_system, paste_parameters);
      original.at(0).PasteRight(original.at(2),
                                GetConfiguration(original.at(0),
                                                 original.at(2)),
                                scoring_system, paste_parameters);
      original.at(0).IncludeInOutput(true);
      CHECK(original.at(0) == alignment_batch.Alignments().at(0));
      CHECK(original.at(1) == alignment_batch.Alignments().at(1));
      CHECK(original.at(2) == alignment_batch.Alignments().at(2));
      CHECK(original.at(3) == alignment_batch.Alignments().at(3));
    }
  }

  GIVEN("Final score threshold; overlap shifts; plus.") {
    paste_parameters.final_score_threshold = 20.0f;
    std::vector<Alignment> alignments{
        Alignment::FromStringFields(0, {"101", "120", "1001", "1020",
                                        "20", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAAAAAAAAAAAA",
                                        "AAAAAAAAAAAAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(1, {"121", "130", "1015", "1024",
                                        "10", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(2, {"131", "141", "1025", "1035",
                                        "11", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAAA",
                                        "AAAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(3, {"138", "147", "1036", "1045",
                                        "10", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters)};
    std::vector<Alignment> original{alignments};
    alignment_batch.ResetAlignments(std::move(alignments), paste_parameters);
    alignment_batch.PasteAlignments(scoring_system, paste_parameters);

    THEN("Alignment pasting rolls back to last final.") {
      original.at(0).PasteRight(original.at(1),
                                GetConfiguration(original.at(0),
                                                 original.at(1)),
                                scoring_system, paste_parameters);
      original.at(0).PasteRight(original.at(2),
                                GetConfiguration(original.at(0),
                                                 original.at(2)),
                                scoring_system, paste_parameters);
      original.at(0).IncludeInOutput(true);
      CHECK(original.at(0) == alignment_batch.Alignments().at(0));
      CHECK(original.at(1) == alignment_batch.Alignments().at(1));
      CHECK(original.at(2) == alignment_batch.Alignments().at(2));
      CHECK(original.at(3) == alignment_batch.Alignments().at(3));
    }
  }

  GIVEN("Final score threshold; overlap shifts; minus.") {
    paste_parameters.final_score_threshold = 20.0f;
    std::vector<Alignment> alignments{
        Alignment::FromStringFields(0, {"101", "120", "1045", "1026",
                                        "20", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAAAAAAAAAAAA",
                                        "AAAAAAAAAAAAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(1, {"121", "130", "1031", "1022",
                                        "10", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(2, {"131", "141", "1021", "1011",
                                        "11", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAAA",
                                        "AAAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(3, {"138", "147", "1010", "1001",
                                        "10", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters)};
    std::vector<Alignment> original{alignments};
    alignment_batch.ResetAlignments(std::move(alignments), paste_parameters);
    alignment_batch.PasteAlignments(scoring_system, paste_parameters);

    THEN("Alignment pasting rolls back to last final.") {
      original.at(0).PasteRight(original.at(1),
                                GetConfiguration(original.at(0),
                                                 original.at(1)),
                                scoring_system, paste_parameters);
      original.at(0).PasteRight(original.at(2),
                                GetConfiguration(original.at(0),
                                                 original.at(2)),
                                scoring_system, paste_parameters);
      original.at(0).IncludeInOutput(true);
      CHECK(original.at(0) == alignment_batch.Alignments().at(0));
      CHECK(original.at(1) == alignment_batch.Alignments().at(1));
      CHECK(original.at(2) == alignment_batch.Alignments().at(2));
      CHECK(original.at(3) == alignment_batch.Alignments().at(3));
    }
  }

  GIVEN("Final score threshold; overlap, distance shifts; plus.") {
    paste_parameters.final_score_threshold = 20.0f;
    std::vector<Alignment> alignments{
        Alignment::FromStringFields(0, {"101", "120", "1001", "1020",
                                        "20", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAAAAAAAAAAAA",
                                        "AAAAAAAAAAAAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(1, {"123", "130", "1019", "1026",
                                        "8", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAA",
                                        "AAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(2, {"130", "139", "1028", "1037",
                                        "10", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(3, {"138", "145", "1040", "1047",
                                        "8", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAA",
                                        "AAAAAAAA"},
                                       scoring_system, paste_parameters)};
    std::vector<Alignment> original{alignments};
    alignment_batch.ResetAlignments(std::move(alignments), paste_parameters);
    alignment_batch.PasteAlignments(scoring_system, paste_parameters);

    THEN("Alignment pasting rolls back to last final.") {
      original.at(0).PasteRight(original.at(1),
                                GetConfiguration(original.at(0),
                                                 original.at(1)),
                                scoring_system, paste_parameters);
      original.at(0).PasteRight(original.at(2),
                                GetConfiguration(original.at(0),
                                                 original.at(2)),
                                scoring_system, paste_parameters);
      original.at(0).IncludeInOutput(true);
      CHECK(original.at(0) == alignment_batch.Alignments().at(0));
      CHECK(original.at(1) == alignment_batch.Alignments().at(1));
      CHECK(original.at(2) == alignment_batch.Alignments().at(2));
      CHECK(original.at(3) == alignment_batch.Alignments().at(3));
    }
  }

  GIVEN("Final score threshold; overlap, distance shifts; minus.") {
    paste_parameters.final_score_threshold = 20.0f;
    std::vector<Alignment> alignments{
        Alignment::FromStringFields(0, {"101", "120", "1047", "1028",
                                        "20", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAAAAAAAAAAAA",
                                        "AAAAAAAAAAAAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(1, {"123", "130", "1029", "1022",
                                        "8", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAA",
                                        "AAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(2, {"130", "139", "1020", "1011",
                                        "10", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(3, {"138", "145", "1008", "1001",
                                        "8", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAA",
                                        "AAAAAAAA"},
                                       scoring_system, paste_parameters)};
    std::vector<Alignment> original{alignments};
    alignment_batch.ResetAlignments(std::move(alignments), paste_parameters);
    alignment_batch.PasteAlignments(scoring_system, paste_parameters);

    THEN("Alignment pasting rolls back to last final.") {
      original.at(0).PasteRight(original.at(1),
                                GetConfiguration(original.at(0),
                                                 original.at(1)),
                                scoring_system, paste_parameters);
      original.at(0).PasteRight(original.at(2),
                                GetConfiguration(original.at(0),
                                                 original.at(2)),
                                scoring_system, paste_parameters);
      original.at(0).IncludeInOutput(true);
      CHECK(original.at(0) == alignment_batch.Alignments().at(0));
      CHECK(original.at(1) == alignment_batch.Alignments().at(1));
      CHECK(original.at(2) == alignment_batch.Alignments().at(2));
      CHECK(original.at(3) == alignment_batch.Alignments().at(3));
    }
  }

  GIVEN("Final score threshold; distances; plus.") {
    paste_parameters.final_score_threshold = 20.0f;
    std::vector<Alignment> alignments{
        Alignment::FromStringFields(0, {"101", "120", "1001", "1020",
                                        "20", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAAAAAAAAAAAA",
                                        "AAAAAAAAAAAAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(1, {"125", "130", "1025", "1030",
                                        "6", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAA",
                                        "AAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(2, {"135", "144", "1035", "1044",
                                        "10", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(3, {"149", "154", "1049", "1054",
                                        "6", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAA",
                                        "AAAAAA"},
                                       scoring_system, paste_parameters)};
    std::vector<Alignment> original{alignments};
    alignment_batch.ResetAlignments(std::move(alignments), paste_parameters);
    alignment_batch.PasteAlignments(scoring_system, paste_parameters);

    THEN("Alignment pasting rolls back to last final.") {
      original.at(0).PasteRight(original.at(1),
                                GetConfiguration(original.at(0),
                                                 original.at(1)),
                                scoring_system, paste_parameters);
      original.at(0).PasteRight(original.at(2),
                                GetConfiguration(original.at(0),
                                                 original.at(2)),
                                scoring_system, paste_parameters);
      original.at(0).IncludeInOutput(true);
      CHECK(original.at(0) == alignment_batch.Alignments().at(0));
      CHECK(original.at(1) == alignment_batch.Alignments().at(1));
      CHECK(original.at(2) == alignment_batch.Alignments().at(2));
      CHECK(original.at(3) == alignment_batch.Alignments().at(3));
    }
  }

  GIVEN("Final score threshold; distances; minus.") {
    paste_parameters.final_score_threshold = 20.0f;
    std::vector<Alignment> alignments{
        Alignment::FromStringFields(0, {"101", "120", "1054", "1035",
                                        "20", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAAAAAAAAAAAA",
                                        "AAAAAAAAAAAAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(1, {"125", "130", "1030", "1025",
                                        "6", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAA",
                                        "AAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(2, {"135", "144", "1020", "1011",
                                        "10", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(3, {"149", "154", "1006", "1001",
                                        "6", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAA",
                                        "AAAAAA"},
                                       scoring_system, paste_parameters)};
    std::vector<Alignment> original{alignments};
    alignment_batch.ResetAlignments(std::move(alignments), paste_parameters);
    alignment_batch.PasteAlignments(scoring_system, paste_parameters);

    THEN("Alignment pasting rolls back to last final.") {
      original.at(0).PasteRight(original.at(1),
                                GetConfiguration(original.at(0),
                                                 original.at(1)),
                                scoring_system, paste_parameters);
      original.at(0).PasteRight(original.at(2),
                                GetConfiguration(original.at(0),
                                                 original.at(2)),
                                scoring_system, paste_parameters);
      original.at(0).IncludeInOutput(true);
      CHECK(original.at(0) == alignment_batch.Alignments().at(0));
      CHECK(original.at(1) == alignment_batch.Alignments().at(1));
      CHECK(original.at(2) == alignment_batch.Alignments().at(2));
      CHECK(original.at(3) == alignment_batch.Alignments().at(3));
    }
  }

  GIVEN("Final score threshold; gaps; plus.") {
    paste_parameters.final_score_threshold = 13.0f;
    std::vector<Alignment> alignments{
        Alignment::FromStringFields(0, {"101", "118", "1001", "1020",
                                        "18", "0", "1", "2",
                                        "10000", "100000",
                                        "AAAAAAAAA--AAAAAAAAA",
                                        "AAAAAAAAAAAAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(1, {"119", "130", "1021", "1028",
                                        "8", "0", "1", "4",
                                        "10000", "100000",
                                        "AAAAAAAAAAAA",
                                        "AAAA----AAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(2, {"131", "142", "1029", "1044",
                                        "12", "0", "1", "4",
                                        "10000", "100000",
                                        "AAAAAA----AAAAAA",
                                        "AAAAAAAAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(3, {"143", "150", "1045", "1056",
                                        "8", "0", "1", "4",
                                        "10000", "100000",
                                        "AAAA----AAAA",
                                        "AAAAAAAAAAAA"},
                                       scoring_system, paste_parameters)};
    alignments.at(0).UngappedPrefixEnd(9);
    alignments.at(0).UngappedSuffixBegin(11);
    alignments.at(1).UngappedPrefixEnd(4);
    alignments.at(1).UngappedSuffixBegin(8);
    alignments.at(2).UngappedPrefixEnd(6);
    alignments.at(2).UngappedSuffixBegin(10);
    alignments.at(3).UngappedPrefixEnd(4);
    alignments.at(3).UngappedSuffixBegin(8);
    std::vector<Alignment> original{alignments};
    alignment_batch.ResetAlignments(std::move(alignments), paste_parameters);
    alignment_batch.PasteAlignments(scoring_system, paste_parameters);

    THEN("Alignment pasting rolls back to last final.") {
      original.at(0).PasteRight(original.at(1),
                                GetConfiguration(original.at(0),
                                                 original.at(1)),
                                scoring_system, paste_parameters);
      original.at(0).PasteRight(original.at(2),
                                GetConfiguration(original.at(0),
                                                 original.at(2)),
                                scoring_system, paste_parameters);
      original.at(0).IncludeInOutput(true);
      CHECK(original.at(0) == alignment_batch.Alignments().at(0));
      CHECK(original.at(1) == alignment_batch.Alignments().at(1));
      CHECK(original.at(2) == alignment_batch.Alignments().at(2));
      CHECK(original.at(3) == alignment_batch.Alignments().at(3));
    }
  }

  GIVEN("Final score threshold; gaps; minus.") {
    paste_parameters.final_score_threshold = 13.0f;
    std::vector<Alignment> alignments{
        Alignment::FromStringFields(0, {"101", "118", "1056", "1037",
                                        "18", "0", "1", "2",
                                        "10000", "100000",
                                        "AAAAAAAAA--AAAAAAAAA",
                                        "AAAAAAAAAAAAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(1, {"119", "130", "1036", "1029",
                                        "8", "0", "1", "4",
                                        "10000", "100000",
                                        "AAAAAAAAAAAA",
                                        "AAAA----AAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(2, {"131", "142", "1028", "1013",
                                        "12", "0", "1", "4",
                                        "10000", "100000",
                                        "AAAAAA----AAAAAA",
                                        "AAAAAAAAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(3, {"143", "150", "1012", "1001",
                                        "8", "0", "1", "4",
                                        "10000", "100000",
                                        "AAAA----AAAA",
                                        "AAAAAAAAAAAA"},
                                       scoring_system, paste_parameters)};
    alignments.at(0).UngappedPrefixEnd(9);
    alignments.at(0).UngappedSuffixBegin(11);
    alignments.at(1).UngappedPrefixEnd(4);
    alignments.at(1).UngappedSuffixBegin(8);
    alignments.at(2).UngappedPrefixEnd(6);
    alignments.at(2).UngappedSuffixBegin(10);
    alignments.at(3).UngappedPrefixEnd(4);
    alignments.at(3).UngappedSuffixBegin(8);
    std::vector<Alignment> original{alignments};
    alignment_batch.ResetAlignments(std::move(alignments), paste_parameters);
    alignment_batch.PasteAlignments(scoring_system, paste_parameters);

    THEN("Alignment pasting rolls back to last final.") {
      original.at(0).PasteRight(original.at(1),
                                GetConfiguration(original.at(0),
                                                 original.at(1)),
                                scoring_system, paste_parameters);
      original.at(0).PasteRight(original.at(2),
                                GetConfiguration(original.at(0),
                                                 original.at(2)),
                                scoring_system, paste_parameters);
      original.at(0).IncludeInOutput(true);
      CHECK(original.at(0) == alignment_batch.Alignments().at(0));
      CHECK(original.at(1) == alignment_batch.Alignments().at(1));
      CHECK(original.at(2) == alignment_batch.Alignments().at(2));
      CHECK(original.at(3) == alignment_batch.Alignments().at(3));
    }
  }

/*

  GIVEN("Ungapped, no distance, no overlap, no shift, ipid threshold plus.") {
    paste_parameters.intermediate_pident_threshold = 90.0f;
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
                                        "10", "5", "0", "0",
                                        "10000", "100000",
                                        "GGGAAAAAGGGGGGG",
                                        "GGGGGGGGGGGGGGG"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(3, {"146", "160", "1046", "1060",
                                        "15", "0", "0", "0",
                                        "10000", "100000",
                                        "GGGGGGGGGGGGGGG",
                                        "GGGGGGGGGGGGGGG"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(4, {"161", "172", "1061", "1072",
                                        "12", "0", "0", "0",
                                        "10000", "100000",
                                        "GGGGGGGGGGGG",
                                        "GGGGGGGGGGGG"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(5, {"173", "185", "1073", "1085",
                                        "13", "0", "0", "0",
                                        "10000", "100000",
                                        "GGGGGGGGGGGG",
                                        "GGGGGGGGGGGG"},
                                       scoring_system, paste_parameters)};
    std::vector<Alignment> original{alignments};
    alignment_batch.ResetAlignments(std::move(alignments), paste_parameters);
    alignment_batch.PasteAlignments(scoring_system, paste_parameters);

    THEN("Pasting stops where final pident threshold is violated.") {
      original.at(1).PasteLeft(original.at(0),
                                GetConfiguration(original.at(0),
                                                 original.at(1)),
                                scoring_system, paste_parameters);
      original.at(1).IncludeInOutput(true);
      original.at(3).PasteRight(original.at(4),
                                GetConfiguration(original.at(3),
                                                 original.at(4)),
                                scoring_system, paste_parameters);
      original.at(3).PasteRight(original.at(5),
                                GetConfiguration(original.at(3),
                                                 original.at(5)),
                                scoring_system, paste_parameters);
      original.at(3).PasteLeft(original.at(2),
                                GetConfiguration(original.at(2),
                                                 original.at(3)),
                                scoring_system, paste_parameters);
      original.at(3).IncludeInOutput(true);
      CHECK(original.at(0) == alignment_batch.Alignments().at(0));
      CHECK(original.at(1) == alignment_batch.Alignments().at(1));
      CHECK(original.at(2) == alignment_batch.Alignments().at(2));
      CHECK(original.at(3) == alignment_batch.Alignments().at(3));
      CHECK(original.at(4) == alignment_batch.Alignments().at(4));
      CHECK(original.at(5) == alignment_batch.Alignments().at(5));
    }
  }*/

  GIVEN("Intermediate pident threshold; mismatches; plus.") {
    paste_parameters.intermediate_pident_threshold = 90.0f;
    std::vector<Alignment> alignments{
        Alignment::FromStringFields(0, {"101", "120", "1001", "1020",
                                        "18", "2", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAACCAAAAAAAAA",
                                        "AAAAAAAAAAAAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(1, {"121", "130", "1021", "1030",
                                        "9", "1", "0", "0",
                                        "10000", "100000",
                                        "AAAAACAAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(2, {"131", "140", "1031", "1040",
                                        "8", "2", "0", "0",
                                        "10000", "100000",
                                        "AAACCAAAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(3, {"141", "150", "1041", "1050",
                                        "10", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters)};
    std::vector<Alignment> original{alignments};
    alignment_batch.ResetAlignments(std::move(alignments), paste_parameters);
    alignment_batch.PasteAlignments(scoring_system, paste_parameters);
    Alignment original_second{original.at(2)}, original_third{original.at(3)};

    THEN("Alignment pasting is interrupted.") {
      original.at(0).PasteRight(original.at(1),
                                GetConfiguration(original.at(0),
                                                 original.at(1)),
                                scoring_system, paste_parameters);
      original.at(3).PasteLeft(original.at(2),
                                GetConfiguration(original.at(2),
                                                 original.at(3)),
                                scoring_system, paste_parameters);
      original.at(0).IncludeInOutput(true);
      original.at(3).IncludeInOutput(true);
      CHECK(original.at(0) == alignment_batch.Alignments().at(0));
      CHECK(original.at(1) == alignment_batch.Alignments().at(1));
      CHECK(original.at(2) == alignment_batch.Alignments().at(2));
      CHECK(original.at(3) == alignment_batch.Alignments().at(3));
      original.at(0).PasteRight(original_second,
                                GetConfiguration(original.at(0),
                                                 original_second),
                                scoring_system, paste_parameters);
      original.at(0).PasteRight(original_third,
                                GetConfiguration(original.at(0),
                                                 original_third),
                                scoring_system, paste_parameters);
      CHECK(original.at(0).SatisfiesThresholds(
          paste_parameters.intermediate_pident_threshold,
          paste_parameters.intermediate_score_threshold,
          paste_parameters));
    }
  }

  GIVEN("Intermediate pident threshold; mismatches; minus.") {
    paste_parameters.intermediate_pident_threshold = 90.0f;
    std::vector<Alignment> alignments{
        Alignment::FromStringFields(0, {"101", "120", "1050", "1031",
                                        "18", "2", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAACCAAAAAAAAA",
                                        "AAAAAAAAAAAAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(1, {"121", "130", "1030", "1021",
                                        "9", "1", "0", "0",
                                        "10000", "100000",
                                        "AAAAACAAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(2, {"131", "140", "1020", "1011",
                                        "8", "2", "0", "0",
                                        "10000", "100000",
                                        "AAACCAAAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(3, {"141", "150", "1010", "1001",
                                        "10", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters)};
    std::vector<Alignment> original{alignments};
    alignment_batch.ResetAlignments(std::move(alignments), paste_parameters);
    alignment_batch.PasteAlignments(scoring_system, paste_parameters);
    Alignment original_second{original.at(2)}, original_third{original.at(3)};

    THEN("Alignment pasting is interrupted.") {
      original.at(0).PasteRight(original.at(1),
                                GetConfiguration(original.at(0),
                                                 original.at(1)),
                                scoring_system, paste_parameters);
      original.at(3).PasteLeft(original.at(2),
                                GetConfiguration(original.at(2),
                                                 original.at(3)),
                                scoring_system, paste_parameters);
      original.at(0).IncludeInOutput(true);
      original.at(3).IncludeInOutput(true);
      CHECK(original.at(0) == alignment_batch.Alignments().at(0));
      CHECK(original.at(1) == alignment_batch.Alignments().at(1));
      CHECK(original.at(2) == alignment_batch.Alignments().at(2));
      CHECK(original.at(3) == alignment_batch.Alignments().at(3));
      original.at(0).PasteRight(original_second,
                                GetConfiguration(original.at(0),
                                                 original_second),
                                scoring_system, paste_parameters);
      original.at(0).PasteRight(original_third,
                                GetConfiguration(original.at(0),
                                                 original_third),
                                scoring_system, paste_parameters);
      CHECK(original.at(0).SatisfiesThresholds(
          paste_parameters.intermediate_pident_threshold,
          paste_parameters.intermediate_score_threshold,
          paste_parameters));
    }
  }

  GIVEN("Intermediate pident threshold; distance shifts; plus.") {
    paste_parameters.intermediate_pident_threshold = 90.0f;
    std::vector<Alignment> alignments{
        Alignment::FromStringFields(0, {"101", "120", "1001", "1020",
                                        "20", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAAAAAAAAAAAA",
                                        "AAAAAAAAAAAAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(1, {"124", "130", "1021", "1027",
                                        "7", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAA",
                                        "AAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(2, {"131", "138", "1030", "1037",
                                        "8", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAA",
                                        "AAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(3, {"139", "148", "1038", "1047",
                                        "10", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters)};
    std::vector<Alignment> original{alignments};
    alignment_batch.ResetAlignments(std::move(alignments), paste_parameters);
    alignment_batch.PasteAlignments(scoring_system, paste_parameters);
    Alignment original_second{original.at(2)}, original_third{original.at(3)};

    THEN("Alignment pasting is interrupted.") {
      original.at(0).PasteRight(original.at(1),
                                GetConfiguration(original.at(0),
                                                 original.at(1)),
                                scoring_system, paste_parameters);
      original.at(3).PasteLeft(original.at(2),
                                GetConfiguration(original.at(2),
                                                 original.at(3)),
                                scoring_system, paste_parameters);
      original.at(0).IncludeInOutput(true);
      original.at(3).IncludeInOutput(true);
      CHECK(original.at(0) == alignment_batch.Alignments().at(0));
      CHECK(original.at(1) == alignment_batch.Alignments().at(1));
      CHECK(original.at(2) == alignment_batch.Alignments().at(2));
      CHECK(original.at(3) == alignment_batch.Alignments().at(3));
      original.at(0).PasteRight(original_second,
                                GetConfiguration(original.at(0),
                                                 original_second),
                                scoring_system, paste_parameters);
      original.at(0).PasteRight(original_third,
                                GetConfiguration(original.at(0),
                                                 original_third),
                                scoring_system, paste_parameters);
      CHECK(original.at(0).SatisfiesThresholds(
          paste_parameters.intermediate_pident_threshold,
          paste_parameters.intermediate_score_threshold,
          paste_parameters));
    }
  }

  GIVEN("Intermediate pident threshold; distance shifts; minus.") {
    paste_parameters.intermediate_pident_threshold = 90.0f;
    std::vector<Alignment> alignments{
        Alignment::FromStringFields(0, {"101", "120", "1047", "1028",
                                        "20", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAAAAAAAAAAAA",
                                        "AAAAAAAAAAAAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(1, {"124", "130", "1027", "1021",
                                        "7", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAA",
                                        "AAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(2, {"131", "138", "1018", "1011",
                                        "8", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAA",
                                        "AAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(3, {"139", "148", "1010", "1001",
                                        "10", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters)};
    std::vector<Alignment> original{alignments};
    alignment_batch.ResetAlignments(std::move(alignments), paste_parameters);
    alignment_batch.PasteAlignments(scoring_system, paste_parameters);
    Alignment original_second{original.at(2)}, original_third{original.at(3)};

    THEN("Alignment pasting is interrupted.") {
      original.at(0).PasteRight(original.at(1),
                                GetConfiguration(original.at(0),
                                                 original.at(1)),
                                scoring_system, paste_parameters);
      original.at(3).PasteLeft(original.at(2),
                                GetConfiguration(original.at(2),
                                                 original.at(3)),
                                scoring_system, paste_parameters);
      original.at(0).IncludeInOutput(true);
      original.at(3).IncludeInOutput(true);
      CHECK(original.at(0) == alignment_batch.Alignments().at(0));
      CHECK(original.at(1) == alignment_batch.Alignments().at(1));
      CHECK(original.at(2) == alignment_batch.Alignments().at(2));
      CHECK(original.at(3) == alignment_batch.Alignments().at(3));
      original.at(0).PasteRight(original_second,
                                GetConfiguration(original.at(0),
                                                 original_second),
                                scoring_system, paste_parameters);
      original.at(0).PasteRight(original_third,
                                GetConfiguration(original.at(0),
                                                 original_third),
                                scoring_system, paste_parameters);
      CHECK(original.at(0).SatisfiesThresholds(
          paste_parameters.intermediate_pident_threshold,
          paste_parameters.intermediate_score_threshold,
          paste_parameters));
    }
  }

  GIVEN("Intermediate pident threshold; overlap shifts; plus.") {
    paste_parameters.intermediate_pident_threshold = 90.0f;
    std::vector<Alignment> alignments{
        Alignment::FromStringFields(0, {"101", "120", "1001", "1020",
                                        "20", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAAAAAAAAAAAA",
                                        "AAAAAAAAAAAAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(1, {"118", "127", "1021", "1030",
                                        "10", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(2, {"128", "137", "1029", "1038",
                                        "10", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(3, {"138", "147", "1039", "1048",
                                        "10", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters)};
    std::vector<Alignment> original{alignments};
    alignment_batch.ResetAlignments(std::move(alignments), paste_parameters);
    alignment_batch.PasteAlignments(scoring_system, paste_parameters);
    Alignment original_second{original.at(2)}, original_third{original.at(3)};

    THEN("Alignment pasting is interrupted.") {
      original.at(0).PasteRight(original.at(1),
                                GetConfiguration(original.at(0),
                                                 original.at(1)),
                                scoring_system, paste_parameters);
      original.at(2).PasteRight(original.at(3),
                                GetConfiguration(original.at(2),
                                                 original.at(3)),
                                scoring_system, paste_parameters);
      original.at(0).IncludeInOutput(true);
      original.at(2).IncludeInOutput(true);
      CHECK(original.at(0) == alignment_batch.Alignments().at(0));
      CHECK(original.at(1) == alignment_batch.Alignments().at(1));
      CHECK(original.at(2) == alignment_batch.Alignments().at(2));
      CHECK(original.at(3) == alignment_batch.Alignments().at(3));
      original.at(0).PasteRight(original_second,
                                GetConfiguration(original.at(0),
                                                 original_second),
                                scoring_system, paste_parameters);
      original.at(0).PasteRight(original_third,
                                GetConfiguration(original.at(0),
                                                 original_third),
                                scoring_system, paste_parameters);
      CHECK(original.at(0).SatisfiesThresholds(
          paste_parameters.intermediate_pident_threshold,
          paste_parameters.intermediate_score_threshold,
          paste_parameters));
    }
  }

  GIVEN("Intermediate pident threshold; overlap shifts; minus.") {
    paste_parameters.intermediate_pident_threshold = 90.0f;
    std::vector<Alignment> alignments{
        Alignment::FromStringFields(0, {"101", "120", "1048", "1029",
                                        "20", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAAAAAAAAAAAA",
                                        "AAAAAAAAAAAAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(1, {"118", "127", "1028", "1019",
                                        "10", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(2, {"128", "137", "1020", "1011",
                                        "10", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(3, {"138", "147", "1010", "1001",
                                        "10", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters)};
    std::vector<Alignment> original{alignments};
    alignment_batch.ResetAlignments(std::move(alignments), paste_parameters);
    alignment_batch.PasteAlignments(scoring_system, paste_parameters);
    Alignment original_second{original.at(2)}, original_third{original.at(3)};

    THEN("Alignment pasting is interrupted.") {
      original.at(0).PasteRight(original.at(1),
                                GetConfiguration(original.at(0),
                                                 original.at(1)),
                                scoring_system, paste_parameters);
      original.at(2).PasteRight(original.at(3),
                                GetConfiguration(original.at(2),
                                                 original.at(3)),
                                scoring_system, paste_parameters);
      original.at(0).IncludeInOutput(true);
      original.at(2).IncludeInOutput(true);
      CHECK(original.at(0) == alignment_batch.Alignments().at(0));
      CHECK(original.at(1) == alignment_batch.Alignments().at(1));
      CHECK(original.at(2) == alignment_batch.Alignments().at(2));
      CHECK(original.at(3) == alignment_batch.Alignments().at(3));
      original.at(0).PasteRight(original_second,
                                GetConfiguration(original.at(0),
                                                 original_second),
                                scoring_system, paste_parameters);
      original.at(0).PasteRight(original_third,
                                GetConfiguration(original.at(0),
                                                 original_third),
                                scoring_system, paste_parameters);
      CHECK(original.at(0).SatisfiesThresholds(
          paste_parameters.intermediate_pident_threshold,
          paste_parameters.intermediate_score_threshold,
          paste_parameters));
    }
  }

  GIVEN("Intermediate pident threshold; overlap/distance shifts; plus.") {
    paste_parameters.intermediate_pident_threshold = 90.0f;
    std::vector<Alignment> alignments{
        Alignment::FromStringFields(0, {"101", "120", "1001", "1020",
                                        "20", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAAAAAAAAAAAA",
                                        "AAAAAAAAAAAAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(1, {"122", "130", "1019", "1027",
                                        "9", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAA",
                                        "AAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(2, {"130", "138", "1029", "1037",
                                        "9", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAA",
                                        "AAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(3, {"139", "148", "1038", "1047",
                                        "10", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters)};
    std::vector<Alignment> original{alignments};
    alignment_batch.ResetAlignments(std::move(alignments), paste_parameters);
    alignment_batch.PasteAlignments(scoring_system, paste_parameters);
    Alignment original_second{original.at(2)}, original_third{original.at(3)};

    THEN("Alignment pasting is interrupted.") {
      original.at(0).PasteRight(original.at(1),
                                GetConfiguration(original.at(0),
                                                 original.at(1)),
                                scoring_system, paste_parameters);
      original.at(3).PasteLeft(original.at(2),
                                GetConfiguration(original.at(2),
                                                 original.at(3)),
                                scoring_system, paste_parameters);
      original.at(0).IncludeInOutput(true);
      original.at(3).IncludeInOutput(true);
      CHECK(original.at(0) == alignment_batch.Alignments().at(0));
      CHECK(original.at(1) == alignment_batch.Alignments().at(1));
      CHECK(original.at(2) == alignment_batch.Alignments().at(2));
      CHECK(original.at(3) == alignment_batch.Alignments().at(3));
      original.at(0).PasteRight(original_second,
                                GetConfiguration(original.at(0),
                                                 original_second),
                                scoring_system, paste_parameters);
      original.at(0).PasteRight(original_third,
                                GetConfiguration(original.at(0),
                                                 original_third),
                                scoring_system, paste_parameters);
      CHECK(original.at(0).SatisfiesThresholds(
          paste_parameters.intermediate_pident_threshold,
          paste_parameters.intermediate_score_threshold,
          paste_parameters));
    }
  }

  GIVEN("Intermediate pident threshold; overlap/distance shifts; minus.") {
    paste_parameters.intermediate_pident_threshold = 90.0f;
    std::vector<Alignment> alignments{
        Alignment::FromStringFields(0, {"101", "120", "1047", "1028",
                                        "20", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAAAAAAAAAAAA",
                                        "AAAAAAAAAAAAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(1, {"122", "130", "1029", "1021",
                                        "9", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAA",
                                        "AAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(2, {"130", "138", "1019", "1011",
                                        "9", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAA",
                                        "AAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(3, {"139", "148", "1010", "1001",
                                        "10", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters)};
    std::vector<Alignment> original{alignments};
    alignment_batch.ResetAlignments(std::move(alignments), paste_parameters);
    alignment_batch.PasteAlignments(scoring_system, paste_parameters);
    Alignment original_second{original.at(2)}, original_third{original.at(3)};

    THEN("Alignment pasting is interrupted.") {
      original.at(0).PasteRight(original.at(1),
                                GetConfiguration(original.at(0),
                                                 original.at(1)),
                                scoring_system, paste_parameters);
      original.at(3).PasteLeft(original.at(2),
                                GetConfiguration(original.at(2),
                                                 original.at(3)),
                                scoring_system, paste_parameters);
      original.at(0).IncludeInOutput(true);
      original.at(3).IncludeInOutput(true);
      CHECK(original.at(0) == alignment_batch.Alignments().at(0));
      CHECK(original.at(1) == alignment_batch.Alignments().at(1));
      CHECK(original.at(2) == alignment_batch.Alignments().at(2));
      CHECK(original.at(3) == alignment_batch.Alignments().at(3));
      original.at(0).PasteRight(original_second,
                                GetConfiguration(original.at(0),
                                                 original_second),
                                scoring_system, paste_parameters);
      original.at(0).PasteRight(original_third,
                                GetConfiguration(original.at(0),
                                                 original_third),
                                scoring_system, paste_parameters);
      CHECK(original.at(0).SatisfiesThresholds(
          paste_parameters.intermediate_pident_threshold,
          paste_parameters.intermediate_score_threshold,
          paste_parameters));
    }
  }

  GIVEN("Intermediate pident threshold; distances; plus.") {
    paste_parameters.intermediate_pident_threshold = 90.0f;
    std::vector<Alignment> alignments{
        Alignment::FromStringFields(0, {"101", "120", "1001", "1020",
                                        "20", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAAAAAAAAAAAA",
                                        "AAAAAAAAAAAAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(1, {"124", "130", "1024", "1030",
                                        "7", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAA",
                                        "AAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(2, {"133", "140", "1033", "1040",
                                        "8", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAA",
                                        "AAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(3, {"141", "150", "1041", "1050",
                                        "10", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters)};
    std::vector<Alignment> original{alignments};
    alignment_batch.ResetAlignments(std::move(alignments), paste_parameters);
    alignment_batch.PasteAlignments(scoring_system, paste_parameters);
    Alignment original_second{original.at(2)}, original_third{original.at(3)};

    THEN("Alignment pasting is interrupted.") {
      original.at(0).PasteRight(original.at(1),
                                GetConfiguration(original.at(0),
                                                 original.at(1)),
                                scoring_system, paste_parameters);
      original.at(3).PasteLeft(original.at(2),
                                GetConfiguration(original.at(2),
                                                 original.at(3)),
                                scoring_system, paste_parameters);
      original.at(0).IncludeInOutput(true);
      original.at(3).IncludeInOutput(true);
      CHECK(original.at(0) == alignment_batch.Alignments().at(0));
      CHECK(original.at(1) == alignment_batch.Alignments().at(1));
      CHECK(original.at(2) == alignment_batch.Alignments().at(2));
      CHECK(original.at(3) == alignment_batch.Alignments().at(3));
      original.at(0).PasteRight(original_second,
                                GetConfiguration(original.at(0),
                                                 original_second),
                                scoring_system, paste_parameters);
      original.at(0).PasteRight(original_third,
                                GetConfiguration(original.at(0),
                                                 original_third),
                                scoring_system, paste_parameters);
      CHECK(original.at(0).SatisfiesThresholds(
          paste_parameters.intermediate_pident_threshold,
          paste_parameters.intermediate_score_threshold,
          paste_parameters));
    }
  }

  GIVEN("Intermediate pident threshold; distances; minus.") {
    paste_parameters.intermediate_pident_threshold = 90.0f;
    std::vector<Alignment> alignments{
        Alignment::FromStringFields(0, {"101", "120", "1050", "1031",
                                        "20", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAAAAAAAAAAAA",
                                        "AAAAAAAAAAAAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(1, {"124", "130", "1027", "1021",
                                        "7", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAA",
                                        "AAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(2, {"133", "140", "1018", "1011",
                                        "8", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAA",
                                        "AAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(3, {"141", "150", "1010", "1001",
                                        "10", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters)};
    std::vector<Alignment> original{alignments};
    alignment_batch.ResetAlignments(std::move(alignments), paste_parameters);
    alignment_batch.PasteAlignments(scoring_system, paste_parameters);
    Alignment original_second{original.at(2)}, original_third{original.at(3)};

    THEN("Alignment pasting is interrupted.") {
      original.at(0).PasteRight(original.at(1),
                                GetConfiguration(original.at(0),
                                                 original.at(1)),
                                scoring_system, paste_parameters);
      original.at(3).PasteLeft(original.at(2),
                                GetConfiguration(original.at(2),
                                                 original.at(3)),
                                scoring_system, paste_parameters);
      original.at(0).IncludeInOutput(true);
      original.at(3).IncludeInOutput(true);
      CHECK(original.at(0) == alignment_batch.Alignments().at(0));
      CHECK(original.at(1) == alignment_batch.Alignments().at(1));
      CHECK(original.at(2) == alignment_batch.Alignments().at(2));
      CHECK(original.at(3) == alignment_batch.Alignments().at(3));
      original.at(0).PasteRight(original_second,
                                GetConfiguration(original.at(0),
                                                 original_second),
                                scoring_system, paste_parameters);
      original.at(0).PasteRight(original_third,
                                GetConfiguration(original.at(0),
                                                 original_third),
                                scoring_system, paste_parameters);
      CHECK(original.at(0).SatisfiesThresholds(
          paste_parameters.intermediate_pident_threshold,
          paste_parameters.intermediate_score_threshold,
          paste_parameters));
    }
  }

  GIVEN("Intermediate pident threshold; gaps; plus.") {
    paste_parameters.intermediate_pident_threshold = 90.0f;
    std::vector<Alignment> alignments{
        Alignment::FromStringFields(0, {"101", "120", "1001", "1020",
                                        "18", "0", "1", "2",
                                        "10000", "100000",
                                        "AAAAAAAAA--AAAAAAAAA",
                                        "AAAAAAAAAAAAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(1, {"121", "130", "1021", "1030",
                                        "9", "0", "1", "1",
                                        "10000", "100000",
                                        "AAAAAAAAAA",
                                        "AAAAA-AAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(2, {"131", "140", "1031", "1040",
                                        "8", "0", "1", "2",
                                        "10000", "100000",
                                        "AAA--AAAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(3, {"141", "150", "1041", "1050",
                                        "10", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters)};
    alignments.at(0).UngappedPrefixEnd(9);
    alignments.at(0).UngappedSuffixBegin(11);
    alignments.at(1).UngappedPrefixEnd(5);
    alignments.at(1).UngappedSuffixBegin(6);
    alignments.at(2).UngappedPrefixEnd(3);
    alignments.at(2).UngappedSuffixBegin(5);
    std::vector<Alignment> original{alignments};
    alignment_batch.ResetAlignments(std::move(alignments), paste_parameters);
    alignment_batch.PasteAlignments(scoring_system, paste_parameters);
    Alignment original_second{original.at(2)}, original_third{original.at(3)};

    THEN("Alignment pasting is interrupted.") {
      original.at(0).PasteRight(original.at(1),
                                GetConfiguration(original.at(0),
                                                 original.at(1)),
                                scoring_system, paste_parameters);
      original.at(3).PasteLeft(original.at(2),
                                GetConfiguration(original.at(2),
                                                 original.at(3)),
                                scoring_system, paste_parameters);
      original.at(0).IncludeInOutput(true);
      original.at(3).IncludeInOutput(true);
      CHECK(original.at(0) == alignment_batch.Alignments().at(0));
      CHECK(original.at(1) == alignment_batch.Alignments().at(1));
      CHECK(original.at(2) == alignment_batch.Alignments().at(2));
      CHECK(original.at(3) == alignment_batch.Alignments().at(3));
      original.at(0).PasteRight(original_second,
                                GetConfiguration(original.at(0),
                                                 original_second),
                                scoring_system, paste_parameters);
      original.at(0).PasteRight(original_third,
                                GetConfiguration(original.at(0),
                                                 original_third),
                                scoring_system, paste_parameters);
      CHECK(original.at(0).SatisfiesThresholds(
          paste_parameters.intermediate_pident_threshold,
          paste_parameters.intermediate_score_threshold,
          paste_parameters));
    }
  }

  GIVEN("Intermediate pident threshold; gaps; minus.") {
    paste_parameters.intermediate_pident_threshold = 90.0f;
    std::vector<Alignment> alignments{
        Alignment::FromStringFields(0, {"101", "120", "1050", "1031",
                                        "18", "0", "1", "2",
                                        "10000", "100000",
                                        "AAAAAAAAA--AAAAAAAAA",
                                        "AAAAAAAAAAAAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(1, {"121", "130", "1030", "1021",
                                        "9", "0", "1", "1",
                                        "10000", "100000",
                                        "AAAAAAAAAA",
                                        "AAAAA-AAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(2, {"131", "140", "1020", "1011",
                                        "8", "0", "1", "2",
                                        "10000", "100000",
                                        "AAA--AAAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(3, {"141", "150", "1010", "1001",
                                        "10", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters)};
    alignments.at(0).UngappedPrefixEnd(9);
    alignments.at(0).UngappedSuffixBegin(11);
    alignments.at(1).UngappedPrefixEnd(5);
    alignments.at(1).UngappedSuffixBegin(6);
    alignments.at(2).UngappedPrefixEnd(3);
    alignments.at(2).UngappedSuffixBegin(5);
    std::vector<Alignment> original{alignments};
    alignment_batch.ResetAlignments(std::move(alignments), paste_parameters);
    alignment_batch.PasteAlignments(scoring_system, paste_parameters);
    Alignment original_second{original.at(2)}, original_third{original.at(3)};

    THEN("Alignment pasting is interrupted.") {
      original.at(0).PasteRight(original.at(1),
                                GetConfiguration(original.at(0),
                                                 original.at(1)),
                                scoring_system, paste_parameters);
      original.at(3).PasteLeft(original.at(2),
                                GetConfiguration(original.at(2),
                                                 original.at(3)),
                                scoring_system, paste_parameters);
      original.at(0).IncludeInOutput(true);
      original.at(3).IncludeInOutput(true);
      CHECK(original.at(0) == alignment_batch.Alignments().at(0));
      CHECK(original.at(1) == alignment_batch.Alignments().at(1));
      CHECK(original.at(2) == alignment_batch.Alignments().at(2));
      CHECK(original.at(3) == alignment_batch.Alignments().at(3));
      original.at(0).PasteRight(original_second,
                                GetConfiguration(original.at(0),
                                                 original_second),
                                scoring_system, paste_parameters);
      original.at(0).PasteRight(original_third,
                                GetConfiguration(original.at(0),
                                                 original_third),
                                scoring_system, paste_parameters);
      CHECK(original.at(0).SatisfiesThresholds(
          paste_parameters.intermediate_pident_threshold,
          paste_parameters.intermediate_score_threshold,
          paste_parameters));
    }
  }

  GIVEN("Intermediate score threshold; mismatches; plus.") {
    paste_parameters.intermediate_score_threshold = 14.0f;
    std::vector<Alignment> alignments{
        Alignment::FromStringFields(0, {"101", "120", "1001", "1020",
                                        "18", "2", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAACCAAAAAAAAA",
                                        "AAAAAAAAAAAAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(1, {"121", "132", "1021", "1032",
                                        "8", "4", "0", "0",
                                        "10000", "100000",
                                        "AAAACCCCAAAA",
                                        "AAAAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(2, {"133", "142", "1033", "1042",
                                        "6", "4", "0", "0",
                                        "10000", "100000",
                                        "AAACCCCAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(3, {"143", "156", "1043", "1056",
                                        "10", "4", "0", "0",
                                        "10000", "100000",
                                        "AAAAACCCCAAAAA",
                                        "AAAAAAAAAAAAAA"},
                                       scoring_system, paste_parameters)};
    std::vector<Alignment> original{alignments};
    alignment_batch.ResetAlignments(std::move(alignments), paste_parameters);
    alignment_batch.PasteAlignments(scoring_system, paste_parameters);
    Alignment original_second{original.at(2)}, original_third{original.at(3)};

    THEN("Alignment pasting is interrupted.") {
      original.at(0).PasteRight(original.at(1),
                                GetConfiguration(original.at(0),
                                                 original.at(1)),
                                scoring_system, paste_parameters);
      original.at(0).IncludeInOutput(true);
      original.at(3).IncludeInOutput(true);
      CHECK(original.at(0) == alignment_batch.Alignments().at(0));
      CHECK(original.at(1) == alignment_batch.Alignments().at(1));
      CHECK(original.at(2) == alignment_batch.Alignments().at(2));
      CHECK(original.at(3) == alignment_batch.Alignments().at(3));
      original.at(0).PasteRight(original_second,
                                GetConfiguration(original.at(0),
                                                 original_second),
                                scoring_system, paste_parameters);
      original.at(0).PasteRight(original_third,
                                GetConfiguration(original.at(0),
                                                 original_third),
                                scoring_system, paste_parameters);
      CHECK(original.at(0).SatisfiesThresholds(
          paste_parameters.intermediate_pident_threshold,
          paste_parameters.intermediate_score_threshold,
          paste_parameters));
    }
  }

  GIVEN("Intermediate score threshold; mismatches; minus.") {
    paste_parameters.intermediate_score_threshold = 14.0f;
    std::vector<Alignment> alignments{
        Alignment::FromStringFields(0, {"101", "120", "1056", "1037",
                                        "18", "2", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAACCAAAAAAAAA",
                                        "AAAAAAAAAAAAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(1, {"121", "132", "1036", "1025",
                                        "8", "4", "0", "0",
                                        "10000", "100000",
                                        "AAAACCCCAAAA",
                                        "AAAAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(2, {"133", "142", "1024", "1015",
                                        "6", "4", "0", "0",
                                        "10000", "100000",
                                        "AAACCCCAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(3, {"143", "156", "1014", "1001",
                                        "10", "4", "0", "0",
                                        "10000", "100000",
                                        "AAAAACCCCAAAAA",
                                        "AAAAAAAAAAAAAA"},
                                       scoring_system, paste_parameters)};
    std::vector<Alignment> original{alignments};
    alignment_batch.ResetAlignments(std::move(alignments), paste_parameters);
    alignment_batch.PasteAlignments(scoring_system, paste_parameters);
    Alignment original_second{original.at(2)}, original_third{original.at(3)};

    THEN("Alignment pasting is interrupted.") {
      original.at(0).PasteRight(original.at(1),
                                GetConfiguration(original.at(0),
                                                 original.at(1)),
                                scoring_system, paste_parameters);
      original.at(0).IncludeInOutput(true);
      original.at(3).IncludeInOutput(true);
      CHECK(original.at(0) == alignment_batch.Alignments().at(0));
      CHECK(original.at(1) == alignment_batch.Alignments().at(1));
      CHECK(original.at(2) == alignment_batch.Alignments().at(2));
      CHECK(original.at(3) == alignment_batch.Alignments().at(3));
      original.at(0).PasteRight(original_second,
                                GetConfiguration(original.at(0),
                                                 original_second),
                                scoring_system, paste_parameters);
      original.at(0).PasteRight(original_third,
                                GetConfiguration(original.at(0),
                                                 original_third),
                                scoring_system, paste_parameters);
      CHECK(original.at(0).SatisfiesThresholds(
          paste_parameters.intermediate_pident_threshold,
          paste_parameters.intermediate_score_threshold,
          paste_parameters));
    }
  }

  GIVEN("Intermediate score threshold; gaps; plus.") {
    paste_parameters.intermediate_score_threshold = 13.0f;
    std::vector<Alignment> alignments{
        Alignment::FromStringFields(0, {"101", "118", "1001", "1020",
                                        "18", "0", "1", "2",
                                        "10000", "100000",
                                        "AAAAAAAAA--AAAAAAAAA",
                                        "AAAAAAAAAAAAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(1, {"119", "132", "1021", "1030",
                                        "10", "0", "1", "4",
                                        "10000", "100000",
                                        "AAAAACCCCAAAAA",
                                        "AAAAA----AAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(2, {"133", "137", "1031", "1039",
                                        "5", "0", "1", "4",
                                        "10000", "100000",
                                        "AAA----AA",
                                        "AAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(3, {"138", "152", "1040", "1058",
                                        "15", "0", "1", "4",
                                        "10000", "100000",
                                        "AAAAAAA----AAAAAAAA",
                                        "AAAAAAAAAAAAAAAAAAA"},
                                       scoring_system, paste_parameters)};
    alignments.at(0).UngappedPrefixEnd(9);
    alignments.at(0).UngappedSuffixBegin(11);
    alignments.at(1).UngappedPrefixEnd(5);
    alignments.at(1).UngappedSuffixBegin(9);
    alignments.at(2).UngappedPrefixEnd(3);
    alignments.at(2).UngappedSuffixBegin(7);
    alignments.at(3).UngappedPrefixEnd(7);
    alignments.at(3).UngappedSuffixBegin(11);
    std::vector<Alignment> original{alignments};
    alignment_batch.ResetAlignments(std::move(alignments), paste_parameters);
    alignment_batch.PasteAlignments(scoring_system, paste_parameters);
    Alignment original_second{original.at(2)}, original_third{original.at(3)};

    THEN("Alignment pasting is interrupted.") {
      original.at(0).PasteRight(original.at(1),
                                GetConfiguration(original.at(0),
                                                 original.at(1)),
                                scoring_system, paste_parameters);
      original.at(0).IncludeInOutput(true);
      original.at(3).IncludeInOutput(true);
      CHECK(original.at(0) == alignment_batch.Alignments().at(0));
      CHECK(original.at(1) == alignment_batch.Alignments().at(1));
      CHECK(original.at(2) == alignment_batch.Alignments().at(2));
      CHECK(original.at(3) == alignment_batch.Alignments().at(3));
      original.at(0).PasteRight(original_second,
                                GetConfiguration(original.at(0),
                                                 original_second),
                                scoring_system, paste_parameters);
      original.at(0).PasteRight(original_third,
                                GetConfiguration(original.at(0),
                                                 original_third),
                                scoring_system, paste_parameters);
      CHECK(original.at(0).SatisfiesThresholds(
          paste_parameters.intermediate_pident_threshold,
          paste_parameters.intermediate_score_threshold,
          paste_parameters));
    }
  }

  GIVEN("Intermediate score threshold; gaps; minus.") {
    paste_parameters.intermediate_score_threshold = 13.0f;
    std::vector<Alignment> alignments{
        Alignment::FromStringFields(0, {"101", "118", "1058", "1039",
                                        "18", "0", "1", "2",
                                        "10000", "100000",
                                        "AAAAAAAAA--AAAAAAAAA",
                                        "AAAAAAAAAAAAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(1, {"119", "132", "1038", "1029",
                                        "10", "0", "1", "4",
                                        "10000", "100000",
                                        "AAAAACCCCAAAAA",
                                        "AAAAA----AAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(2, {"133", "137", "1028", "1020",
                                        "5", "0", "1", "4",
                                        "10000", "100000",
                                        "AAA----AA",
                                        "AAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(3, {"138", "152", "1019", "1001",
                                        "15", "0", "1", "4",
                                        "10000", "100000",
                                        "AAAAAAA----AAAAAAAA",
                                        "AAAAAAAAAAAAAAAAAAA"},
                                       scoring_system, paste_parameters)};
    alignments.at(0).UngappedPrefixEnd(9);
    alignments.at(0).UngappedSuffixBegin(11);
    alignments.at(1).UngappedPrefixEnd(5);
    alignments.at(1).UngappedSuffixBegin(9);
    alignments.at(2).UngappedPrefixEnd(3);
    alignments.at(2).UngappedSuffixBegin(7);
    alignments.at(3).UngappedPrefixEnd(7);
    alignments.at(3).UngappedSuffixBegin(11);
    std::vector<Alignment> original{alignments};
    alignment_batch.ResetAlignments(std::move(alignments), paste_parameters);
    alignment_batch.PasteAlignments(scoring_system, paste_parameters);
    Alignment original_second{original.at(2)}, original_third{original.at(3)};

    THEN("Alignment pasting is interrupted.") {
      original.at(0).PasteRight(original.at(1),
                                GetConfiguration(original.at(0),
                                                 original.at(1)),
                                scoring_system, paste_parameters);
      original.at(0).IncludeInOutput(true);
      original.at(3).IncludeInOutput(true);
      CHECK(original.at(0) == alignment_batch.Alignments().at(0));
      CHECK(original.at(1) == alignment_batch.Alignments().at(1));
      CHECK(original.at(2) == alignment_batch.Alignments().at(2));
      CHECK(original.at(3) == alignment_batch.Alignments().at(3));
      original.at(0).PasteRight(original_second,
                                GetConfiguration(original.at(0),
                                                 original_second),
                                scoring_system, paste_parameters);
      original.at(0).PasteRight(original_third,
                                GetConfiguration(original.at(0),
                                                 original_third),
                                scoring_system, paste_parameters);
      CHECK(original.at(0).SatisfiesThresholds(
          paste_parameters.intermediate_pident_threshold,
          paste_parameters.intermediate_score_threshold,
          paste_parameters));
    }
  }

  GIVEN("Intermediate score threshold; distances; plus.") {
    paste_parameters.intermediate_score_threshold = 20.0f;
    std::vector<Alignment> alignments{
        Alignment::FromStringFields(0, {"101", "120", "1001", "1020",
                                        "20", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAAAAAAAAAAAA",
                                        "AAAAAAAAAAAAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(1, {"124", "129", "1024", "1029",
                                        "6", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAA",
                                        "AAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(2, {"134", "139", "1034", "1039",
                                        "6", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAA",
                                        "AAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(3, {"144", "153", "1044", "1053",
                                        "10", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters)};
    std::vector<Alignment> original{alignments};
    alignment_batch.ResetAlignments(std::move(alignments), paste_parameters);
    alignment_batch.PasteAlignments(scoring_system, paste_parameters);
    Alignment original_second{original.at(2)}, original_third{original.at(3)};

    THEN("Alignment pasting is interrupted.") {
      original.at(0).PasteRight(original.at(1),
                                GetConfiguration(original.at(0),
                                                 original.at(1)),
                                scoring_system, paste_parameters);
      original.at(0).IncludeInOutput(true);
      original.at(2).IncludeInOutput(true);
      original.at(3).IncludeInOutput(true);
      CHECK(original.at(0) == alignment_batch.Alignments().at(0));
      CHECK(original.at(1) == alignment_batch.Alignments().at(1));
      CHECK(original.at(2) == alignment_batch.Alignments().at(2));
      CHECK(original.at(3) == alignment_batch.Alignments().at(3));
      original.at(0).PasteRight(original_second,
                                GetConfiguration(original.at(0),
                                                 original_second),
                                scoring_system, paste_parameters);
      original.at(0).PasteRight(original_third,
                                GetConfiguration(original.at(0),
                                                 original_third),
                                scoring_system, paste_parameters);
      CHECK(original.at(0).SatisfiesThresholds(
          paste_parameters.intermediate_pident_threshold,
          paste_parameters.intermediate_score_threshold,
          paste_parameters));
    }
  }

  GIVEN("Intermediate score threshold; distances; minus.") {
    paste_parameters.intermediate_score_threshold = 20.0f;
    std::vector<Alignment> alignments{
        Alignment::FromStringFields(0, {"101", "120", "1053", "1034",
                                        "20", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAAAAAAAAAAAA",
                                        "AAAAAAAAAAAAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(1, {"124", "129", "1030", "1025",
                                        "6", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAA",
                                        "AAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(2, {"134", "139", "1020", "1015",
                                        "6", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAA",
                                        "AAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(3, {"144", "153", "1010", "1001",
                                        "10", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters)};
    std::vector<Alignment> original{alignments};
    alignment_batch.ResetAlignments(std::move(alignments), paste_parameters);
    alignment_batch.PasteAlignments(scoring_system, paste_parameters);
    Alignment original_second{original.at(2)}, original_third{original.at(3)};

    THEN("Alignment pasting is interrupted.") {
      original.at(0).PasteRight(original.at(1),
                                GetConfiguration(original.at(0),
                                                 original.at(1)),
                                scoring_system, paste_parameters);
      original.at(0).IncludeInOutput(true);
      original.at(2).IncludeInOutput(true);
      original.at(3).IncludeInOutput(true);
      CHECK(original.at(0) == alignment_batch.Alignments().at(0));
      CHECK(original.at(1) == alignment_batch.Alignments().at(1));
      CHECK(original.at(2) == alignment_batch.Alignments().at(2));
      CHECK(original.at(3) == alignment_batch.Alignments().at(3));
      original.at(0).PasteRight(original_second,
                                GetConfiguration(original.at(0),
                                                 original_second),
                                scoring_system, paste_parameters);
      original.at(0).PasteRight(original_third,
                                GetConfiguration(original.at(0),
                                                 original_third),
                                scoring_system, paste_parameters);
      CHECK(original.at(0).SatisfiesThresholds(
          paste_parameters.intermediate_pident_threshold,
          paste_parameters.intermediate_score_threshold,
          paste_parameters));
    }
  }

  GIVEN("Intermediate score threshold; distance shifts; plus.") {
    paste_parameters.intermediate_score_threshold = 20.0f;
    std::vector<Alignment> alignments{
        Alignment::FromStringFields(0, {"101", "120", "1001", "1020",
                                        "20", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAAAAAAAAAAAA",
                                        "AAAAAAAAAAAAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(1, {"125", "134", "1021", "1030",
                                        "10", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(2, {"135", "142", "1035", "1042",
                                        "8", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAA",
                                        "AAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(3, {"147", "158", "1043", "1054",
                                        "12", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters)};
    std::vector<Alignment> original{alignments};
    alignment_batch.ResetAlignments(std::move(alignments), paste_parameters);
    alignment_batch.PasteAlignments(scoring_system, paste_parameters);
    Alignment original_second{original.at(2)}, original_third{original.at(3)};

    THEN("Alignment pasting is interrupted.") {
      original.at(0).PasteRight(original.at(1),
                                GetConfiguration(original.at(0),
                                                 original.at(1)),
                                scoring_system, paste_parameters);
      original.at(0).IncludeInOutput(true);
      original.at(2).IncludeInOutput(true);
      original.at(3).IncludeInOutput(true);
      CHECK(original.at(0) == alignment_batch.Alignments().at(0));
      CHECK(original.at(1) == alignment_batch.Alignments().at(1));
      CHECK(original.at(2) == alignment_batch.Alignments().at(2));
      CHECK(original.at(3) == alignment_batch.Alignments().at(3));
      original.at(0).PasteRight(original_second,
                                GetConfiguration(original.at(0),
                                                 original_second),
                                scoring_system, paste_parameters);
      original.at(0).PasteRight(original_third,
                                GetConfiguration(original.at(0),
                                                 original_third),
                                scoring_system, paste_parameters);
      CHECK(original.at(0).SatisfiesThresholds(
          paste_parameters.intermediate_pident_threshold,
          paste_parameters.intermediate_score_threshold,
          paste_parameters));
    }
  }

  GIVEN("Intermediate score threshold; distance shifts; minus.") {
    paste_parameters.intermediate_score_threshold = 20.0f;
    std::vector<Alignment> alignments{
        Alignment::FromStringFields(0, {"101", "120", "1054", "1035",
                                        "20", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAAAAAAAAAAAA",
                                        "AAAAAAAAAAAAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(1, {"125", "134", "1034", "1025",
                                        "10", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(2, {"135", "142", "1020", "1013",
                                        "8", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAA",
                                        "AAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(3, {"147", "158", "1012", "1001",
                                        "12", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters)};
    std::vector<Alignment> original{alignments};
    alignment_batch.ResetAlignments(std::move(alignments), paste_parameters);
    alignment_batch.PasteAlignments(scoring_system, paste_parameters);
    Alignment original_second{original.at(2)}, original_third{original.at(3)};

    THEN("Alignment pasting is interrupted.") {
      original.at(0).PasteRight(original.at(1),
                                GetConfiguration(original.at(0),
                                                 original.at(1)),
                                scoring_system, paste_parameters);
      original.at(0).IncludeInOutput(true);
      original.at(2).IncludeInOutput(true);
      original.at(3).IncludeInOutput(true);
      CHECK(original.at(0) == alignment_batch.Alignments().at(0));
      CHECK(original.at(1) == alignment_batch.Alignments().at(1));
      CHECK(original.at(2) == alignment_batch.Alignments().at(2));
      CHECK(original.at(3) == alignment_batch.Alignments().at(3));
      original.at(0).PasteRight(original_second,
                                GetConfiguration(original.at(0),
                                                 original_second),
                                scoring_system, paste_parameters);
      original.at(0).PasteRight(original_third,
                                GetConfiguration(original.at(0),
                                                 original_third),
                                scoring_system, paste_parameters);
      CHECK(original.at(0).SatisfiesThresholds(
          paste_parameters.intermediate_pident_threshold,
          paste_parameters.intermediate_score_threshold,
          paste_parameters));
    }
  }

  GIVEN("Intermediate score threshold; overlap shifts; plus.") {
    paste_parameters.intermediate_score_threshold = 20.0f;
    std::vector<Alignment> alignments{
        Alignment::FromStringFields(0, {"101", "120", "1001", "1020",
                                        "20", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAAAAAAAAAAAA",
                                        "AAAAAAAAAAAAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(1, {"117", "130", "1021", "1034",
                                        "14", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAAAAAA",
                                        "AAAAAAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(2, {"131", "142", "1031", "1042",
                                        "12", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAAAA",
                                        "AAAAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(3, {"139", "154", "1043", "1058",
                                        "16", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters)};
    std::vector<Alignment> original{alignments}, original2{alignments};
    alignment_batch.ResetAlignments(std::move(alignments), paste_parameters);
    alignment_batch.PasteAlignments(scoring_system, paste_parameters);

    THEN("Alignment pasting is skipped.") {
      original.at(0).PasteRight(original.at(1),
                                GetConfiguration(original.at(0),
                                                 original.at(1)),
                                scoring_system, paste_parameters);
      original.at(0).PasteRight(original.at(3),
                                GetConfiguration(original.at(0),
                                                 original.at(3)),
                                scoring_system, paste_parameters);
      original.at(0).IncludeInOutput(true);
      original.at(2).IncludeInOutput(true);
      CHECK(original.at(0) == alignment_batch.Alignments().at(0));
      CHECK(original.at(1) == alignment_batch.Alignments().at(1));
      CHECK(original.at(2) == alignment_batch.Alignments().at(2));
      CHECK(original.at(3) == alignment_batch.Alignments().at(3));
      original2.at(0).PasteRight(original2.at(1),
                                GetConfiguration(original2.at(0),
                                                 original2.at(1)),
                                scoring_system, paste_parameters);
      original2.at(0).PasteRight(original2.at(2),
                                GetConfiguration(original2.at(0),
                                                 original2.at(2)),
                                scoring_system, paste_parameters);
      original2.at(0).PasteRight(original2.at(3),
                                GetConfiguration(original2.at(0),
                                                 original2.at(3)),
                                scoring_system, paste_parameters);
      CHECK(original2.at(0).SatisfiesThresholds(
          paste_parameters.intermediate_pident_threshold,
          paste_parameters.intermediate_score_threshold,
          paste_parameters));
    }
  }

  GIVEN("Intermediate score threshold; overlap shifts; minus.") {
    paste_parameters.intermediate_score_threshold = 20.0f;
    std::vector<Alignment> alignments{
        Alignment::FromStringFields(0, {"101", "120", "1058", "1039",
                                        "20", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAAAAAAAAAAAA",
                                        "AAAAAAAAAAAAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(1, {"117", "130", "1038", "1025",
                                        "14", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAAAAAA",
                                        "AAAAAAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(2, {"131", "142", "1028", "1017",
                                        "12", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAAAA",
                                        "AAAAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(3, {"139", "154", "1016", "1001",
                                        "16", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters)};
    std::vector<Alignment> original{alignments}, original2{alignments};
    alignment_batch.ResetAlignments(std::move(alignments), paste_parameters);
    alignment_batch.PasteAlignments(scoring_system, paste_parameters);

    THEN("Alignment pasting is skipped.") {
      original.at(0).PasteRight(original.at(1),
                                GetConfiguration(original.at(0),
                                                 original.at(1)),
                                scoring_system, paste_parameters);
      original.at(0).PasteRight(original.at(3),
                                GetConfiguration(original.at(0),
                                                 original.at(3)),
                                scoring_system, paste_parameters);
      original.at(0).IncludeInOutput(true);
      original.at(2).IncludeInOutput(true);
      CHECK(original.at(0) == alignment_batch.Alignments().at(0));
      CHECK(original.at(1) == alignment_batch.Alignments().at(1));
      CHECK(original.at(2) == alignment_batch.Alignments().at(2));
      CHECK(original.at(3) == alignment_batch.Alignments().at(3));
      original2.at(0).PasteRight(original2.at(1),
                                GetConfiguration(original2.at(0),
                                                 original2.at(1)),
                                scoring_system, paste_parameters);
      original2.at(0).PasteRight(original2.at(2),
                                GetConfiguration(original2.at(0),
                                                 original2.at(2)),
                                scoring_system, paste_parameters);
      original2.at(0).PasteRight(original2.at(3),
                                GetConfiguration(original2.at(0),
                                                 original2.at(3)),
                                scoring_system, paste_parameters);
      CHECK(original2.at(0).SatisfiesThresholds(
          paste_parameters.intermediate_pident_threshold,
          paste_parameters.intermediate_score_threshold,
          paste_parameters));
    }
  }

  GIVEN("Intermediate score threshold; overlap, distance shifts; plus.") {
    paste_parameters.intermediate_score_threshold = 20.0f;
    std::vector<Alignment> alignments{
        Alignment::FromStringFields(0, {"101", "120", "1001", "1020",
                                        "20", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAAAAAAAAAAAA",
                                        "AAAAAAAAAAAAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(1, {"123", "134", "1019", "1030",
                                        "12", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAAAA",
                                        "AAAAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(2, {"133", "142", "1033", "1042",
                                        "10", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(3, {"141", "154", "1045", "1058",
                                        "14", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAAAAAA",
                                        "AAAAAAAAAAAAAA"},
                                       scoring_system, paste_parameters)};
    std::vector<Alignment> original{alignments};
    alignment_batch.ResetAlignments(std::move(alignments), paste_parameters);
    alignment_batch.PasteAlignments(scoring_system, paste_parameters);
    Alignment original_second{original.at(2)}, original_third{original.at(3)};

    THEN("Alignment pasting is interrupted.") {
      original.at(0).PasteRight(original.at(1),
                                GetConfiguration(original.at(0),
                                                 original.at(1)),
                                scoring_system, paste_parameters);
      original.at(0).IncludeInOutput(true);
      original.at(2).IncludeInOutput(true);
      original.at(3).IncludeInOutput(true);
      CHECK(original.at(0) == alignment_batch.Alignments().at(0));
      CHECK(original.at(1) == alignment_batch.Alignments().at(1));
      CHECK(original.at(2) == alignment_batch.Alignments().at(2));
      CHECK(original.at(3) == alignment_batch.Alignments().at(3));
      original.at(0).PasteRight(original_second,
                                GetConfiguration(original.at(0),
                                                 original_second),
                                scoring_system, paste_parameters);
      original.at(0).PasteRight(original_third,
                                GetConfiguration(original.at(0),
                                                 original_third),
                                scoring_system, paste_parameters);
      CHECK(original.at(0).SatisfiesThresholds(
          paste_parameters.intermediate_pident_threshold,
          paste_parameters.intermediate_score_threshold,
          paste_parameters));
    }
  }

  GIVEN("Intermediate score threshold; overlap, distance shifts; minus.") {
    paste_parameters.intermediate_score_threshold = 20.0f;
    std::vector<Alignment> alignments{
        Alignment::FromStringFields(0, {"101", "120", "1058", "1039",
                                        "20", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAAAAAAAAAAAA",
                                        "AAAAAAAAAAAAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(1, {"123", "134", "1040", "1029",
                                        "12", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAAAA",
                                        "AAAAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(2, {"133", "142", "1026", "1017",
                                        "10", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(3, {"141", "154", "1014", "1001",
                                        "14", "0", "0", "0",
                                        "10000", "100000",
                                        "AAAAAAAAAAAAAA",
                                        "AAAAAAAAAAAAAA"},
                                       scoring_system, paste_parameters)};
    std::vector<Alignment> original{alignments};
    alignment_batch.ResetAlignments(std::move(alignments), paste_parameters);
    alignment_batch.PasteAlignments(scoring_system, paste_parameters);
    Alignment original_second{original.at(2)}, original_third{original.at(3)};

    THEN("Alignment pasting is interrupted.") {
      original.at(0).PasteRight(original.at(1),
                                GetConfiguration(original.at(0),
                                                 original.at(1)),
                                scoring_system, paste_parameters);
      original.at(0).IncludeInOutput(true);
      original.at(2).IncludeInOutput(true);
      original.at(3).IncludeInOutput(true);
      CHECK(original.at(0) == alignment_batch.Alignments().at(0));
      CHECK(original.at(1) == alignment_batch.Alignments().at(1));
      CHECK(original.at(2) == alignment_batch.Alignments().at(2));
      CHECK(original.at(3) == alignment_batch.Alignments().at(3));
      original.at(0).PasteRight(original_second,
                                GetConfiguration(original.at(0),
                                                 original_second),
                                scoring_system, paste_parameters);
      original.at(0).PasteRight(original_third,
                                GetConfiguration(original.at(0),
                                                 original_third),
                                scoring_system, paste_parameters);
      CHECK(original.at(0).SatisfiesThresholds(
          paste_parameters.intermediate_pident_threshold,
          paste_parameters.intermediate_score_threshold,
          paste_parameters));
    }
  }
}

} // namespace

} // namespace test

} // namespace paste_alignments