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

#include "alignment.h"

#define CATCH_CONFIG_MAIN
#define CATCH_CONFIG_COLOUR_NONE
#include "catch.h"

#include "string_conversions.h" // include after catch.h

#include <cmath>

#include "exceptions.h"
#include "paste_parameters.h"
#include "scoring_system.h"

// Alignment tests
//
// Test correctness for:
// * FromStringFields
// * PasteRight
// * PasteLeft
//
// Test invariants for:
// * PasteRight
// * PasteLeft
//
// Test exceptions for:
// * FromStringFields // ensures invariants
// * PasteRight
// * PasteLeft

// string fields for alignments are (in this order):
//
// qstart qend sstart send
// nident mismatch gapopen gaps
// qlen slen
// qseq sseq
//
// default scoring (megablast):
//
// reward: 1, penalty: 2, gapopen: 0, gapextend: 2.5

namespace paste_alignments {

namespace test {

// Tests if alignment corresponds to the string representations in fields.
bool Equals(const Alignment& alignment,
            const std::vector<std::string>& fields,
            const std::vector<int>& identifiers,
            const ScoringSystem& scoring_system,
            const PasteParameters& paste_parameters) {
  assert(fields.size() == 12);
  int sstart{std::stoi(fields.at(2))};
  int send{std::stoi(fields.at(3))};
  int nident{std::stoi(fields.at(4))};
  int mismatch{std::stoi(fields.at(5))};
  int gapopen{std::stoi(fields.at(6))};
  int gaps{std::stoi(fields.at(7))};
  int qlen{std::stoi(fields.at(8))};
  int length{static_cast<int>(fields.at(10).length())};
  float raw_score{scoring_system.RawScore(nident, mismatch, gapopen, gaps)};

  return (alignment.PastedIdentifiers() == identifiers
          && alignment.Qstart() == std::stoi(fields.at(0))
          && alignment.Qend() == std::stoi(fields.at(1))
          && alignment.Sstart() == std::min(sstart, send)
          && alignment.Send() == std::max(sstart, send)
          && alignment.PlusStrand() == (sstart <= send)
          && alignment.Nident() == nident
          && alignment.Mismatch() == mismatch
          && alignment.Gapopen() == gapopen
          && alignment.Gaps() == gaps
          && alignment.Qlen() == qlen
          && alignment.Slen() == std::stoi(fields.at(9))
          && alignment.Qseq() == fields.at(10)
          && alignment.Sseq() == fields.at(11)
          && helpers::FuzzyFloatEquals(alignment.Pident(),
                                       100.0f * std::stof(fields.at(4))
                                           / static_cast<float>(length),
                                       paste_parameters.float_epsilon)
          && helpers::FuzzyFloatEquals(alignment.RawScore(),
                                       raw_score,
                                       paste_parameters.float_epsilon)
          && helpers::FuzzyFloatEquals(alignment.Bitscore(),
                                       scoring_system.Bitscore(
                                           raw_score, paste_parameters))
          && helpers::FuzzyDoubleEquals(alignment.Evalue(),
                                        scoring_system.Evalue(
                                            raw_score, qlen, paste_parameters))
          && !alignment.IncludeInOutput()
          && alignment.UngappedSuffixBegin() == 0
          && alignment.UngappedPrefixEnd() == length);
}

void HandleOffset(
    int query_offset, int subject_offset, bool plus_strand,
    int left_sstart, int left_send,
    int& right_qstart, int& right_qend, int& right_sstart, int& right_send) {
  right_qstart += query_offset;
  right_qend += query_offset;

  if (plus_strand) {
    right_sstart += subject_offset;
    right_send += subject_offset;
  } else {
    // Shift alignment's subject region to the left.
    int right_sregion_length{right_send - right_sstart + 1};
    int left_sregion_length{left_send - left_sstart + 1};
    right_sstart -= (right_sregion_length + left_sregion_length
                     + subject_offset);
    right_send -= (right_sregion_length + left_sregion_length
                   + subject_offset);
  }
}

AlignmentConfiguration GetConfiguration(int query_offset, int subject_offset,
                                        int left_length, int right_length) {
  AlignmentConfiguration config;

  config.query_offset = query_offset;
  config.query_overlap = std::abs(std::min(0, query_offset));
  config.query_distance = std::max(0, query_offset);

  config.subject_offset = subject_offset;
  config.subject_overlap = std::abs(std::min(0, subject_offset));
  config.subject_distance = std::max(0, subject_offset);

  config.shift = std::abs(query_offset - subject_offset);
  config.left_length = left_length;
  config.right_length = right_length;
  config.pasted_length = left_length + right_length
                         + std::max(query_offset, subject_offset);
  return config;
}

namespace {

SCENARIO("Test correctness of Alignment::FromStringFields.",
         "[Alignment][FromStringFields][correctness]") {
  ScoringSystem scoring_system{ScoringSystem::Create(100000l, 1, 2, 1, 1)};
  PasteParameters paste_parameters;

  GIVEN("Valid string representations of fields.") {
    std::vector<std::string_view> a_views, b_views, c_views, d_views;
    std::vector<std::string> a{"101", "110", "1101", "1110",
                               "10", "0", "0", "0",
                               "10000", "100000",
                               "CCCCAAAATT", "CCCCAAAATT"};
    a_views.insert(a_views.begin(), a.cbegin(), a.cend());
    std::vector<std::string> b{"201", "210", "2110", "2101",
                               "10", "0", "0", "0",
                               "10000", "100000",
                               "CCCCAAAATT", "CCCCAAAATT"};
    b_views.insert(b_views.begin(), b.cbegin(), b.cend());
    std::vector<std::string> c{"1", "10", "3101", "3110",
                               "6", "0", "2", "4",
                               "10000", "100000",
                               "CCCCAA--TT", "C--CAAAATT"};
    c_views.insert(c_views.begin(), c.cbegin(), c.cend());
    std::vector<std::string> d{"4101", "4110", "1", "10",
                               "6", "4", "0", "0",
                               "10000", "100000",
                               "CCCCGGAATT", "CCCCAAAAAA"};
    d_views.insert(d_views.begin(), d.cbegin(), d.cend());

    WHEN("Converting the fields.") {
      Alignment alignment_a{Alignment::FromStringFields(1, a_views,
                                                        scoring_system,
                                                        paste_parameters)};
      Alignment alignment_b{Alignment::FromStringFields(2020, b_views,
                                                        scoring_system,
                                                        paste_parameters)};
      Alignment alignment_c{Alignment::FromStringFields(-2020, c_views,
                                                        scoring_system,
                                                        paste_parameters)};
      Alignment alignment_d{Alignment::FromStringFields(0, d_views,
                                                        scoring_system,
                                                        paste_parameters)};

      THEN("Alignments are created as expected.") {
        CHECK(Equals(alignment_a, a, {1}, scoring_system, paste_parameters));
        CHECK(Equals(alignment_b, b, {2020}, scoring_system, paste_parameters));
        CHECK(Equals(alignment_c, c, {-2020}, scoring_system,
                     paste_parameters));
        CHECK(Equals(alignment_d, d, {0}, scoring_system, paste_parameters));
      }
    }
  }
}

SCENARIO("Test exceptions thrown by Alignment::FromStringFields.",
         "[Alignment][FromStringFields][exceptions]") {
  ScoringSystem scoring_system{ScoringSystem::Create(100000l, 1, 2, 1, 1)};
  PasteParameters paste_parameters;

  WHEN("Fewer than 12 fields are provided.") {
    std::vector<std::string_view> test_a{
        "101", "110", "1101", "1110",
        "10", "0", "0", "0",
        "10000", "100000",
        "CCCCAAAATT"};
    std::vector<std::string_view> test_b{
        "110", "1101", "1110",
        "10", "0", "0", "0",
        "10000", "100000",
        "CCCCAAAATT", "CCCCAAAATT"};
    std::vector<std::string_view> test_c{
        "101", "110", "1101", "1110",
        "10", "0",
        "10000", "100000",
        "CCCCAAAATT", "CCCCAAAATT"};
    std::vector<std::string_view> test_d{};

    THEN("`exceptions::ParsingError` is thrown.") {
      CHECK_THROWS_AS(Alignment::FromStringFields(0, test_a, scoring_system,
                                                  paste_parameters),
                      exceptions::ParsingError);
      CHECK_THROWS_AS(Alignment::FromStringFields(0, test_b, scoring_system,
                                                  paste_parameters),
                      exceptions::ParsingError);
      CHECK_THROWS_AS(Alignment::FromStringFields(0, test_c, scoring_system,
                                                  paste_parameters),
                      exceptions::ParsingError);
      CHECK_THROWS_AS(Alignment::FromStringFields(0, test_d, scoring_system,
                                                  paste_parameters),
                      exceptions::ParsingError);
    }
  }

  WHEN("One of the fields, except qseq and sseq cannot be converted to"
       " integer.") {
    std::vector<std::string_view> test_a{
        "a", "110", "abc", "1110",
        "10", "0", "0", "0",
        "10000", "100000",
        "CCCCAAAATT", "CCCCAAAATT"};
    std::vector<std::string_view> test_b{
        "101", "b", "1101", "1110",
        "10", "0", "0", "0",
        "10000", "100000",
        "CCCCAAAATT", "CCCCAAAATT"};
    std::vector<std::string_view> test_c{
        "101", "110", "c", "1110",
        "10", "0", "0", "0",
        "10000", "100000",
        "CCCCAAAATT", "CCCCAAAATT"};
    std::vector<std::string_view> test_d{
        "101", "110", "1101", "d",
        "10", "0", "0", "0",
        "10000", "100000",
        "CCCCAAAATT", "CCCCAAAATT"};
    std::vector<std::string_view> test_e{
        "101", "110", "1101", "1110",
        "e", "0", "0", "0",
        "10000", "100000",
        "CCCCAAAATT", "CCCCAAAATT"};
    std::vector<std::string_view> test_f{
        "101", "110", "1101", "1110",
        "10", "", "0", "0",
        "10000", "100000",
        "CCCCAAAATT", "CCCCAAAATT"};
    std::vector<std::string_view> test_g{
        "101", "110", "1101", "1110",
        "10", "0", "foo", "0",
        "10000", "100000",
        "CCCCAAAATT", "CCCCAAAATT"};
    std::vector<std::string_view> test_h{
        "101", "110", "1101", "1110",
        "10", "0", "0", std::to_string(2 * std::numeric_limits<int>::max()),
        "10000", "100000",
        "CCCCAAAATT", "CCCCAAAATT"};
    std::vector<std::string_view> test_i{
        "101", "110", "1101", "1110",
        "10", "0", "0", "0",
        "\t", "100000",
        "CCCCAAAATT", "CCCCAAAATT"};
    std::vector<std::string_view> test_j{
        "101", "110", "1101", "1110",
        "10", "0", "0", "0",
        "10000", "\n",
        "CCCCAAAATT", "CCCCAAAATT"};

    THEN("`exceptions::ParsingError` is thrown.") {
      CHECK_THROWS_AS(Alignment::FromStringFields(0, test_a, scoring_system,
                                                  paste_parameters),
                      exceptions::ParsingError);
      CHECK_THROWS_AS(Alignment::FromStringFields(0, test_b, scoring_system,
                                                  paste_parameters),
                      exceptions::ParsingError);
      CHECK_THROWS_AS(Alignment::FromStringFields(0, test_c, scoring_system,
                                                  paste_parameters),
                      exceptions::ParsingError);
      CHECK_THROWS_AS(Alignment::FromStringFields(0, test_d, scoring_system,
                                                  paste_parameters),
                      exceptions::ParsingError);
      CHECK_THROWS_AS(Alignment::FromStringFields(0, test_e, scoring_system,
                                                  paste_parameters),
                      exceptions::ParsingError);
      CHECK_THROWS_AS(Alignment::FromStringFields(0, test_f, scoring_system,
                                                  paste_parameters),
                      exceptions::ParsingError);
      CHECK_THROWS_AS(Alignment::FromStringFields(0, test_g, scoring_system,
                                                  paste_parameters),
                      exceptions::ParsingError);
      CHECK_THROWS_AS(Alignment::FromStringFields(0, test_h, scoring_system,
                                                  paste_parameters),
                      exceptions::ParsingError);
      CHECK_THROWS_AS(Alignment::FromStringFields(0, test_i, scoring_system,
                                                  paste_parameters),
                      exceptions::ParsingError);
      CHECK_THROWS_AS(Alignment::FromStringFields(0, test_j, scoring_system,
                                                  paste_parameters),
                      exceptions::ParsingError);
    }
  }

  WHEN("qstart is larger than qend coordinate.") {
    std::vector<std::string_view> test_a{
        "110", "101", "1101", "1110",
        "10", "0", "0", "0",
        "10000", "100000",
        "CCCCAAAATT", "CCCCAAAATT"};
    std::vector<std::string_view> test_b{
        "1", "0", "1101", "1110",
        "10", "0", "0", "0",
        "10000", "100000",
        "CCCCAAAATT", "CCCCAAAATT"};

    THEN("`exceptions::ParsingError` is thrown.") {
      CHECK_THROWS_AS(Alignment::FromStringFields(0, test_a, scoring_system,
                                                  paste_parameters),
                      exceptions::ParsingError);
      CHECK_THROWS_AS(Alignment::FromStringFields(0, test_b, scoring_system,
                                                  paste_parameters),
                      exceptions::ParsingError);
    }
  }

  WHEN("One of qstart, qend, sstart, or send is negative.") {
    std::vector<std::string_view> test_a{
        "-101", "110", "1101", "1110",
        "10", "0", "0", "0",
        "10000", "100000",
        "CCCCAAAATT", "CCCCAAAATT"};
    std::vector<std::string_view> test_b{
        "101", "-110", "1101", "1110",
        "10", "0", "0", "0",
        "10000", "100000",
        "CCCCAAAATT", "CCCCAAAATT"};
    std::vector<std::string_view> test_c{
        "101", "110", "-1101", "1110",
        "10", "0", "0", "0",
        "10000", "100000",
        "CCCCAAAATT", "CCCCAAAATT"};
    std::vector<std::string_view> test_d{
        "101", "110", "1101", "-1110",
        "10", "0", "0", "0",
        "10000", "100000",
        "CCCCAAAATT", "CCCCAAAATT"};

    THEN("`exceptions::ParsingError` is thrown.") {
      CHECK_THROWS_AS(Alignment::FromStringFields(0, test_a, scoring_system,
                                                  paste_parameters),
                      exceptions::ParsingError);
      CHECK_THROWS_AS(Alignment::FromStringFields(0, test_b, scoring_system,
                                                  paste_parameters),
                      exceptions::ParsingError);
      CHECK_THROWS_AS(Alignment::FromStringFields(0, test_c, scoring_system,
                                                  paste_parameters),
                      exceptions::ParsingError);
      CHECK_THROWS_AS(Alignment::FromStringFields(0, test_d, scoring_system,
                                                  paste_parameters),
                      exceptions::ParsingError);
    }
  }

  WHEN("One of nident, mismatch, gapopen, or gaps is negative.") {
    std::vector<std::string_view> test_a{
        "101", "110", "1101", "1110",
        "-10", "10", "10", "10",
        "10000", "100000",
        "CCCCAAAATT", "CCCCAAAATT"};
    std::vector<std::string_view> test_b{
        "101", "110", "1101", "1110",
        "10", "-10", "10", "10",
        "10000", "100000",
        "CCCCAAAATT", "CCCCAAAATT"};
    std::vector<std::string_view> test_c{
        "101", "110", "1101", "1110",
        "10", "10", "-10", "10",
        "10000", "100000",
        "CCCCAAAATT", "CCCCAAAATT"};
    std::vector<std::string_view> test_d{
        "101", "110", "1101", "1110",
        "10", "10", "10", "-10",
        "10000", "100000",
        "CCCCAAAATT", "CCCCAAAATT"};

    THEN("`exceptions::ParsingError` is thrown.") {
      CHECK_THROWS_AS(Alignment::FromStringFields(0, test_a, scoring_system,
                                                  paste_parameters),
                      exceptions::ParsingError);
      CHECK_THROWS_AS(Alignment::FromStringFields(0, test_b, scoring_system,
                                                  paste_parameters),
                      exceptions::ParsingError);
      CHECK_THROWS_AS(Alignment::FromStringFields(0, test_c, scoring_system,
                                                  paste_parameters),
                      exceptions::ParsingError);
      CHECK_THROWS_AS(Alignment::FromStringFields(0, test_d, scoring_system,
                                                  paste_parameters),
                      exceptions::ParsingError);
    }
  }

  WHEN("One of qlen, or slen is non-positive.") {
    std::vector<std::string_view> test_a{
        "101", "110", "1101", "1110",
        "10", "0", "0", "0",
        "0", "100000",
        "CCCCAAAATT", "CCCCAAAATT"};
    std::vector<std::string_view> test_b{
        "101", "110", "1101", "1110",
        "10", "0", "0", "0",
        "10000", "0",
        "CCCCAAAATT", "CCCCAAAATT"};
    std::vector<std::string_view> test_c{
        "101", "110", "1101", "1110",
        "10", "0", "0", "0",
        "-10000", "100000",
        "CCCCAAAATT", "CCCCAAAATT"};
    std::vector<std::string_view> test_d{
        "101", "110", "1101", "1110",
        "10", "0", "0", "0",
        "10000", "-100000",
        "CCCCAAAATT", "CCCCAAAATT"};

    THEN("`exceptions::ParsingError` is thrown.") {
      CHECK_THROWS_AS(Alignment::FromStringFields(0, test_a, scoring_system,
                                                  paste_parameters),
                      exceptions::ParsingError);
      CHECK_THROWS_AS(Alignment::FromStringFields(0, test_b, scoring_system,
                                                  paste_parameters),
                      exceptions::ParsingError);
      CHECK_THROWS_AS(Alignment::FromStringFields(0, test_c, scoring_system,
                                                  paste_parameters),
                      exceptions::ParsingError);
      CHECK_THROWS_AS(Alignment::FromStringFields(0, test_d, scoring_system,
                                                  paste_parameters),
                      exceptions::ParsingError);
    }
  }

  WHEN("One of qseq, or sseq is empty.") {
    std::vector<std::string_view> test_a{
        "101", "110", "1101", "1110",
        "10", "0", "0", "0",
        "10000", "100000",
        "", "CCCCAAAATT"};
    std::vector<std::string_view> test_b{
        "101", "110", "1101", "1110",
        "10", "0", "0", "0",
        "10000", "100000",
        "CCCCAAAATT", ""};
    std::vector<std::string_view> test_c{
        "101", "110", "1101", "1110",
        "10", "0", "0", "0",
        "-10000", "100000",
        "", ""};

    THEN("`exceptions::ParsingError` is thrown.") {
      CHECK_THROWS_AS(Alignment::FromStringFields(0, test_a, scoring_system,
                                                  paste_parameters),
                      exceptions::ParsingError);
      CHECK_THROWS_AS(Alignment::FromStringFields(0, test_b, scoring_system,
                                                  paste_parameters),
                      exceptions::ParsingError);
      CHECK_THROWS_AS(Alignment::FromStringFields(0, test_c, scoring_system,
                                                  paste_parameters),
                      exceptions::ParsingError);
    }
  }

  WHEN("Lengths of qseq and sseq aren't equal.") {
    std::vector<std::string_view> test_a{
        "101", "110", "1101", "1110",
        "10", "0", "0", "0",
        "10000", "100000",
        "CCCCAAAAT", "CCCCAAAATT"};
    std::vector<std::string_view> test_b{
        "101", "110", "1101", "1110",
        "10", "0", "0", "0",
        "-10000", "100000",
        "CCCCAAAATTAA", "CCCCAAAATT"};

    THEN("`exceptions::ParsingError` is thrown.") {
      CHECK_THROWS_AS(Alignment::FromStringFields(0, test_a, scoring_system,
                                                  paste_parameters),
                      exceptions::ParsingError);
      CHECK_THROWS_AS(Alignment::FromStringFields(0, test_b, scoring_system,
                                                  paste_parameters),
                      exceptions::ParsingError);
    }
  }
}

SCENARIO("Test correctness of Alignment::PasteRight <prefix>.",
         "[Alignment][PasteRight][correctness][prefix]") {
  ScoringSystem scoring_system{ScoringSystem::Create(100000l, 1, 2, 1, 1)};
  PasteParameters paste_parameters;

  GIVEN("Left alignment's portion in pasted alignment has a gap.") {
    int left_length{36}, left_sregion_length{34};
    int query_offset = GENERATE(-10, -5, 0, 5, 10);
    int subject_offset = GENERATE(-10, -5, 0, 5, 10);
    Alignment left_plus{Alignment::FromStringFields(0,
        {"101", "134", "1001", "1034",
         "24", "8", "2", "4",
         "10000", "100000",
         "AAAA--AAGGAATTTTCCCCGGGGAAAAGGGGAAAA",
         "AAAAGGAA--AACCCCCCCCGGGGAAAAAAAAAAAA"},
        scoring_system, paste_parameters)};
    left_plus.UngappedPrefixEnd(4);
    left_plus.UngappedSuffixBegin(10);
    Alignment left_minus{Alignment::FromStringFields(0,
        {"101", "134", "1034", "1001",
         "24", "8", "2", "4",
         "10000", "100000",
         "AAAA--AAGGAATTTTCCCCGGGGAAAAGGGGAAAA",
         "AAAAGGAA--AACCCCCCCCGGGGAAAAAAAAAAAA"},
        scoring_system, paste_parameters)};
    left_minus.UngappedPrefixEnd(4);
    left_minus.UngappedSuffixBegin(10);

    WHEN("Right alignment is ungapped.") {
      int right_length{24};
      AlignmentConfiguration config{GetConfiguration(
          query_offset, subject_offset, left_length, right_length)};
      Alignment right_plus{Alignment::FromStringFields(1,
          {std::to_string(135 + query_offset),
           std::to_string(158 + query_offset),
           std::to_string(1035 + subject_offset),
           std::to_string(1058 + subject_offset),
           "20", "4", "0", "0",
           "10000", "100000",
           "AAAAAAAATTTTCCCCAAAAAAAA",
           "AAAAAAAAGGGGCCCCAAAAAAAA"},
          scoring_system, paste_parameters)};
      Alignment right_minus{Alignment::FromStringFields(1,
          {std::to_string(135 + query_offset),
           std::to_string(158 + query_offset),
           std::to_string(1058 - subject_offset - left_sregion_length
                               - right_length),
           std::to_string(1035 - subject_offset - left_sregion_length
                               - right_length),
           "20", "4", "0", "0",
           "10000", "100000",
           "AAAAAAAATTTTCCCCAAAAAAAA",
           "AAAAAAAAGGGGCCCCAAAAAAAA"},
          scoring_system, paste_parameters)};

      THEN("Pasted alignment takes on left's ungapped prefix.") {
        int original_left_ungapped_prefix = left_plus.UngappedPrefixEnd();
        left_plus.PasteRight(right_plus, config, scoring_system,
                             paste_parameters);
        CHECK(original_left_ungapped_prefix == left_plus.UngappedPrefixEnd());

        original_left_ungapped_prefix = left_minus.UngappedPrefixEnd();
        left_minus.PasteRight(right_minus, config, scoring_system,
                              paste_parameters);
        CHECK(original_left_ungapped_prefix == left_minus.UngappedPrefixEnd());
      }
    }

    WHEN("Right alignment has gaps.") {
      int right_length{28}, right_sregion_length{27};
      AlignmentConfiguration config{GetConfiguration(
          query_offset, subject_offset, left_length, right_length)};
      Alignment right_plus{Alignment::FromStringFields(1,
          {std::to_string(135 + query_offset),
           std::to_string(159 + query_offset),
           std::to_string(1035 + subject_offset),
           std::to_string(1061 + subject_offset),
           "20", "4", "2", "4",
           "10000", "100000",
           "AAAAAAAATTTTGCCCC---AAAAAAAA",
           "AAAAAAAAGGGG-CCCCGGGAAAAAAAA"},
          scoring_system, paste_parameters)};
      right_plus.UngappedPrefixEnd(12);
      right_plus.UngappedSuffixBegin(20);
      Alignment right_minus{Alignment::FromStringFields(1,
          {std::to_string(135 + query_offset),
           std::to_string(159 + query_offset),
           std::to_string(1061 - subject_offset - left_sregion_length
                               - right_sregion_length),
           std::to_string(1035 - subject_offset - left_sregion_length
                               - right_sregion_length),
           "20", "4", "2", "4",
           "10000", "100000",
           "AAAAAAAATTTTGCCCC---AAAAAAAA",
           "AAAAAAAAGGGG-CCCCGGGAAAAAAAA"},
          scoring_system, paste_parameters)};
      right_minus.UngappedPrefixEnd(12);
      right_minus.UngappedSuffixBegin(20);

      THEN("Pasted alignment takes on left's ungapped prefix.") {
        int original_left_ungapped_prefix = left_plus.UngappedPrefixEnd();
        left_plus.PasteRight(right_plus, config, scoring_system,
                             paste_parameters);
        CHECK(original_left_ungapped_prefix == left_plus.UngappedPrefixEnd());

        original_left_ungapped_prefix = left_minus.UngappedPrefixEnd();
        left_minus.PasteRight(right_minus, config, scoring_system,
                              paste_parameters);
        CHECK(original_left_ungapped_prefix == left_minus.UngappedPrefixEnd());
      }
    }
  }

  GIVEN("Left alignment's portion in pasted alignment has no gap and 0 shift.") {
    int left_length{36}, left_sregion_length{34};
    int query_offset = GENERATE(-10, -8, -6);
    int subject_offset{query_offset};
    Alignment left_plus{Alignment::FromStringFields(0,
        {"101", "134", "1001", "1034",
         "24", "8", "2", "4",
         "10000", "100000",
         "AAAAAAAATTTTCCCCGGGGAAAAGGGGAAGGA--A",
         "AAAAAAAACCCCCCCCGGGGAAAAAAAAAA--AGGA"},
        scoring_system, paste_parameters)};
    left_plus.UngappedPrefixEnd(30);
    left_plus.UngappedSuffixBegin(35);
    Alignment left_minus{Alignment::FromStringFields(0,
        {"101", "134", "1034", "1001",
         "24", "8", "2", "4",
         "10000", "100000",
         "AAAAAAAATTTTCCCCGGGGAAAAGGGGAAGGA--A",
         "AAAAAAAACCCCCCCCGGGGAAAAAAAAAA--AGGA"},
        scoring_system, paste_parameters)};
    left_minus.UngappedPrefixEnd(30);
    left_minus.UngappedSuffixBegin(35);

    WHEN("Right alignment is ungapped.") {
      int right_length{24};
      AlignmentConfiguration config{GetConfiguration(
          query_offset, subject_offset, left_length, right_length)};
      Alignment right_plus{Alignment::FromStringFields(1,
          {std::to_string(135 + query_offset),
           std::to_string(158 + query_offset),
           std::to_string(1035 + subject_offset),
           std::to_string(1058 + subject_offset),
           "20", "4", "0", "0",
           "10000", "100000",
           "AAAAAAAATTTTCCCCAAAAAAAA",
           "AAAAAAAAGGGGCCCCAAAAAAAA"},
          scoring_system, paste_parameters)};
      Alignment right_minus{Alignment::FromStringFields(1,
          {std::to_string(135 + query_offset),
           std::to_string(158 + query_offset),
           std::to_string(1058 - subject_offset - left_sregion_length
                               - right_length),
           std::to_string(1035 - subject_offset - left_sregion_length
                               - right_length),
           "20", "4", "0", "0",
           "10000", "100000",
           "AAAAAAAATTTTCCCCAAAAAAAA",
           "AAAAAAAAGGGGCCCCAAAAAAAA"},
          scoring_system, paste_parameters)};

      THEN("Pasted alignment is ungapped.") {
        int expected_ungapped_prefix_end = config.pasted_length;
        left_plus.PasteRight(right_plus, config, scoring_system,
                             paste_parameters);
        CHECK(expected_ungapped_prefix_end == left_plus.UngappedPrefixEnd());

        left_minus.PasteRight(right_minus, config, scoring_system,
                              paste_parameters);
        CHECK(expected_ungapped_prefix_end == left_minus.UngappedPrefixEnd());
      }
    }

    WHEN("Right alignment has gaps.") {
      int right_length{28}, right_sregion_length{27};
      AlignmentConfiguration config{GetConfiguration(
          query_offset, subject_offset, left_length, right_length)};
      Alignment right_plus{Alignment::FromStringFields(1,
          {std::to_string(135 + query_offset),
           std::to_string(159 + query_offset),
           std::to_string(1035 + subject_offset),
           std::to_string(1061 + subject_offset),
           "20", "4", "2", "4",
           "10000", "100000",
           "AAAAAAAATTTTGCCCC---AAAAAAAA",
           "AAAAAAAAGGGG-CCCCGGGAAAAAAAA"},
          scoring_system, paste_parameters)};
      right_plus.UngappedPrefixEnd(12);
      right_plus.UngappedSuffixBegin(20);
      Alignment right_minus{Alignment::FromStringFields(1,
          {std::to_string(135 + query_offset),
           std::to_string(159 + query_offset),
           std::to_string(1061 - subject_offset - left_sregion_length
                               - right_sregion_length),
           std::to_string(1035 - subject_offset - left_sregion_length
                               - right_sregion_length),
           "20", "4", "2", "4",
           "10000", "100000",
           "AAAAAAAATTTTGCCCC---AAAAAAAA",
           "AAAAAAAAGGGG-CCCCGGGAAAAAAAA"},
          scoring_system, paste_parameters)};
      right_minus.UngappedPrefixEnd(12);
      right_minus.UngappedSuffixBegin(20);

      THEN("Pasted alignment includes right's ungapped prefix.") {
        int expected_ungapped_prefix_end{config.pasted_length
                                         - right_plus.Length()
                                         + right_plus.UngappedPrefixEnd()};
        left_plus.PasteRight(right_plus, config, scoring_system,
                             paste_parameters);
        CHECK(expected_ungapped_prefix_end == left_plus.UngappedPrefixEnd());

        expected_ungapped_prefix_end = config.pasted_length
                                       - right_plus.Length()
                                       + right_plus.UngappedPrefixEnd();
        left_minus.PasteRight(right_minus, config, scoring_system,
                              paste_parameters);
        CHECK(expected_ungapped_prefix_end == left_minus.UngappedPrefixEnd());
      }
    }
  }

  GIVEN("Left alignment's portion in pasted has no gap and non-0 shift.") {
    int left_length{36}, left_sregion_length{34};
    int query_offset = GENERATE(-10, -8, -6);
    int shift = GENERATE(-5, -2, -1, 1, 2, 5);
    int subject_offset{query_offset + shift};
    Alignment left_plus{Alignment::FromStringFields(0,
        {"101", "134", "1001", "1034",
         "24", "8", "2", "4",
         "10000", "100000",
         "AAAAAAAATTTTCCCCGGGGAAAAGGGGAAGGA--A",
         "AAAAAAAACCCCCCCCGGGGAAAAAAAAAA--AGGA"},
        scoring_system, paste_parameters)};
    left_plus.UngappedPrefixEnd(30);
    left_plus.UngappedSuffixBegin(35);
    Alignment left_minus{Alignment::FromStringFields(0,
        {"101", "134", "1034", "1001",
         "24", "8", "2", "4",
         "10000", "100000",
         "AAAAAAAATTTTCCCCGGGGAAAAGGGGAAGGA--A",
         "AAAAAAAACCCCCCCCGGGGAAAAAAAAAA--AGGA"},
        scoring_system, paste_parameters)};
    left_minus.UngappedPrefixEnd(30);
    left_minus.UngappedSuffixBegin(35);

    WHEN("Right alignment is ungapped.") {
      int right_length{24};
      AlignmentConfiguration config{GetConfiguration(
          query_offset, subject_offset, left_length, right_length)};
      Alignment right_plus{Alignment::FromStringFields(1,
          {std::to_string(135 + query_offset),
           std::to_string(158 + query_offset),
           std::to_string(1035 + subject_offset),
           std::to_string(1058 + subject_offset),
           "20", "4", "0", "0",
           "10000", "100000",
           "AAAAAAAATTTTCCCCAAAAAAAA",
           "AAAAAAAAGGGGCCCCAAAAAAAA"},
          scoring_system, paste_parameters)};
      Alignment right_minus{Alignment::FromStringFields(1,
          {std::to_string(135 + query_offset),
           std::to_string(158 + query_offset),
           std::to_string(1058 - subject_offset - left_sregion_length
                               - right_length),
           std::to_string(1035 - subject_offset - left_sregion_length
                               - right_length),
           "20", "4", "0", "0",
           "10000", "100000",
           "AAAAAAAATTTTCCCCAAAAAAAA",
           "AAAAAAAAGGGGCCCCAAAAAAAA"},
          scoring_system, paste_parameters)};

      THEN("Pasted ungapped prefix stops at introduced gap.") {
        int expected_ungapped_prefix_end{
            left_plus.Length() + std::min(query_offset, subject_offset)};
        left_plus.PasteRight(right_plus, config, scoring_system,
                             paste_parameters);
        CHECK(expected_ungapped_prefix_end == left_plus.UngappedPrefixEnd());

        expected_ungapped_prefix_end =
            left_minus.Length() + std::min(query_offset, subject_offset);
        left_minus.PasteRight(right_minus, config, scoring_system,
                              paste_parameters);
        CHECK(expected_ungapped_prefix_end == left_minus.UngappedPrefixEnd());
      }
    }

    WHEN("Right alignment has gaps.") {
      int right_length{28}, right_sregion_length{27};
      AlignmentConfiguration config{GetConfiguration(
          query_offset, subject_offset, left_length, right_length)};
      Alignment right_plus{Alignment::FromStringFields(1,
          {std::to_string(135 + query_offset),
           std::to_string(159 + query_offset),
           std::to_string(1035 + subject_offset),
           std::to_string(1061 + subject_offset),
           "20", "4", "2", "4",
           "10000", "100000",
           "AAAAAAAATTTTGCCCC---AAAAAAAA",
           "AAAAAAAAGGGG-CCCCGGGAAAAAAAA"},
          scoring_system, paste_parameters)};
      right_plus.UngappedPrefixEnd(12);
      right_plus.UngappedSuffixBegin(20);
      Alignment right_minus{Alignment::FromStringFields(1,
          {std::to_string(135 + query_offset),
           std::to_string(159 + query_offset),
           std::to_string(1061 - subject_offset - left_sregion_length
                               - right_sregion_length),
           std::to_string(1035 - subject_offset - left_sregion_length
                               - right_sregion_length),
           "20", "4", "2", "4",
           "10000", "100000",
           "AAAAAAAATTTTGCCCC---AAAAAAAA",
           "AAAAAAAAGGGG-CCCCGGGAAAAAAAA"},
          scoring_system, paste_parameters)};
      right_minus.UngappedPrefixEnd(12);
      right_minus.UngappedSuffixBegin(20);

      THEN("Pasted ungapped prefix stops at introduced gap.") {
        int expected_ungapped_prefix_end{
            left_plus.Length() + std::min(query_offset, subject_offset)};
        left_plus.PasteRight(right_plus, config, scoring_system,
                             paste_parameters);
        CHECK(expected_ungapped_prefix_end == left_plus.UngappedPrefixEnd());

        expected_ungapped_prefix_end =
            left_minus.Length() + std::min(query_offset, subject_offset);
        left_minus.PasteRight(right_minus, config, scoring_system,
                              paste_parameters);
        CHECK(expected_ungapped_prefix_end == left_minus.UngappedPrefixEnd());
      }
    }
  }

  GIVEN("Left alignment is ungapped and 0 shift.") {
    int left_length{32}, left_sregion_length{32};
    int query_offset = GENERATE(-10, -8, -6);
    int subject_offset{query_offset};
    Alignment left_plus{Alignment::FromStringFields(0,
        {"101", "132", "1001", "1032",
         "24", "8", "0", "0",
         "10000", "100000",
         "AAAAAAAATTTTCCCCGGGGAAAAGGGGAAAA",
         "AAAAAAAACCCCCCCCGGGGAAAAAAAAAAAA"},
        scoring_system, paste_parameters)};
    Alignment left_minus{Alignment::FromStringFields(0,
        {"101", "132", "1032", "1001",
         "24", "8", "0", "0",
         "10000", "100000",
         "AAAAAAAATTTTCCCCGGGGAAAAGGGGAAAA",
         "AAAAAAAACCCCCCCCGGGGAAAAAAAAAAAA"},
        scoring_system, paste_parameters)};

    WHEN("Right alignment is ungapped.") {
      int right_length{24};
      AlignmentConfiguration config{GetConfiguration(
          query_offset, subject_offset, left_length, right_length)};
      Alignment right_plus{Alignment::FromStringFields(1,
          {std::to_string(133 + query_offset),
           std::to_string(156 + query_offset),
           std::to_string(1033 + subject_offset),
           std::to_string(1056 + subject_offset),
           "20", "4", "0", "0",
           "10000", "100000",
           "AAAAAAAATTTTCCCCAAAAAAAA",
           "AAAAAAAAGGGGCCCCAAAAAAAA"},
          scoring_system, paste_parameters)};
      Alignment right_minus{Alignment::FromStringFields(1,
          {std::to_string(133 + query_offset),
           std::to_string(156 + query_offset),
           std::to_string(1056 - subject_offset - left_sregion_length
                               - right_length),
           std::to_string(1033 - subject_offset - left_sregion_length
                               - right_length),
           "20", "4", "0", "0",
           "10000", "100000",
           "AAAAAAAATTTTCCCCAAAAAAAA",
           "AAAAAAAAGGGGCCCCAAAAAAAA"},
          scoring_system, paste_parameters)};

      THEN("Pasted alignment is ungapped.") {
        int expected_ungapped_prefix_end = config.pasted_length;
        left_plus.PasteRight(right_plus, config, scoring_system,
                             paste_parameters);
        CHECK(expected_ungapped_prefix_end == left_plus.UngappedPrefixEnd());

        left_minus.PasteRight(right_minus, config, scoring_system,
                              paste_parameters);
        CHECK(expected_ungapped_prefix_end == left_minus.UngappedPrefixEnd());
      }
    }

    WHEN("Right alignment has gaps.") {
      int right_length{28}, right_sregion_length{27};
      AlignmentConfiguration config{GetConfiguration(
          query_offset, subject_offset, left_length, right_length)};
      Alignment right_plus{Alignment::FromStringFields(1,
          {std::to_string(133 + query_offset),
           std::to_string(157 + query_offset),
           std::to_string(1033 + subject_offset),
           std::to_string(1059 + subject_offset),
           "20", "4", "2", "4",
           "10000", "100000",
           "AAAAAAAATTTTGCCCC---AAAAAAAA",
           "AAAAAAAAGGGG-CCCCGGGAAAAAAAA"},
          scoring_system, paste_parameters)};
      right_plus.UngappedPrefixEnd(12);
      right_plus.UngappedSuffixBegin(20);
      Alignment right_minus{Alignment::FromStringFields(1,
          {std::to_string(133 + query_offset),
           std::to_string(157 + query_offset),
           std::to_string(1059 - subject_offset - left_sregion_length
                               - right_sregion_length),
           std::to_string(1033 - subject_offset - left_sregion_length
                               - right_sregion_length),
           "20", "4", "2", "4",
           "10000", "100000",
           "AAAAAAAATTTTGCCCC---AAAAAAAA",
           "AAAAAAAAGGGG-CCCCGGGAAAAAAAA"},
          scoring_system, paste_parameters)};
      right_minus.UngappedPrefixEnd(12);
      right_minus.UngappedSuffixBegin(20);

      THEN("Pasted alignment includes right's ungapped prefix.") {
        int expected_ungapped_prefix_end{config.pasted_length
                                         - right_plus.Length()
                                         + right_plus.UngappedPrefixEnd()};
        left_plus.PasteRight(right_plus, config, scoring_system,
                             paste_parameters);
        CHECK(expected_ungapped_prefix_end == left_plus.UngappedPrefixEnd());

        expected_ungapped_prefix_end = config.pasted_length
                                       - right_plus.Length()
                                       + right_plus.UngappedPrefixEnd();
        left_minus.PasteRight(right_minus, config, scoring_system,
                              paste_parameters);
        CHECK(expected_ungapped_prefix_end == left_minus.UngappedPrefixEnd());
      }
    }
  }

  GIVEN("Left alignment is ungapped non-0 shift.") {
    int left_length{32}, left_sregion_length{32};
    int query_offset = GENERATE(-10, -8, -6);
    int shift = GENERATE(-5, -2, -1, 1, 2, 5);
    int subject_offset{query_offset + shift};
    Alignment left_plus{Alignment::FromStringFields(0,
        {"101", "132", "1001", "1032",
         "24", "8", "0", "0",
         "10000", "100000",
         "AAAAAAAATTTTCCCCGGGGAAAAGGGGAAAA",
         "AAAAAAAACCCCCCCCGGGGAAAAAAAAAAAA"},
        scoring_system, paste_parameters)};
    Alignment left_minus{Alignment::FromStringFields(0,
        {"101", "132", "1032", "1001",
         "24", "8", "0", "0",
         "10000", "100000",
         "AAAAAAAATTTTCCCCGGGGAAAAGGGGAAAA",
         "AAAAAAAACCCCCCCCGGGGAAAAAAAAAAAA"},
        scoring_system, paste_parameters)};

    WHEN("Right alignment is ungapped.") {
      int right_length{24};
      AlignmentConfiguration config{GetConfiguration(
          query_offset, subject_offset, left_length, right_length)};
      Alignment right_plus{Alignment::FromStringFields(1,
          {std::to_string(133 + query_offset),
           std::to_string(156 + query_offset),
           std::to_string(1033 + subject_offset),
           std::to_string(1056 + subject_offset),
           "20", "4", "0", "0",
           "10000", "100000",
           "AAAAAAAATTTTCCCCAAAAAAAA",
           "AAAAAAAAGGGGCCCCAAAAAAAA"},
          scoring_system, paste_parameters)};
      Alignment right_minus{Alignment::FromStringFields(1,
          {std::to_string(133 + query_offset),
           std::to_string(156 + query_offset),
           std::to_string(1056 - subject_offset - left_sregion_length
                               - right_length),
           std::to_string(1033 - subject_offset - left_sregion_length
                               - right_length),
           "20", "4", "0", "0",
           "10000", "100000",
           "AAAAAAAATTTTCCCCAAAAAAAA",
           "AAAAAAAAGGGGCCCCAAAAAAAA"},
          scoring_system, paste_parameters)};

      THEN("Pasted ungapped prefix stops at introduced gap.") {
        int expected_ungapped_prefix_end{
            left_plus.Length() + std::min(query_offset, subject_offset)};
        left_plus.PasteRight(right_plus, config, scoring_system,
                             paste_parameters);
        CHECK(expected_ungapped_prefix_end == left_plus.UngappedPrefixEnd());

        expected_ungapped_prefix_end =
            left_minus.Length() + std::min(query_offset, subject_offset);
        left_minus.PasteRight(right_minus, config, scoring_system,
                              paste_parameters);
        CHECK(expected_ungapped_prefix_end == left_minus.UngappedPrefixEnd());
      }
    }

    WHEN("Right alignment has gaps.") {
      int right_length{28}, right_sregion_length{27};
      AlignmentConfiguration config{GetConfiguration(
          query_offset, subject_offset, left_length, right_length)};
      Alignment right_plus{Alignment::FromStringFields(1,
          {std::to_string(133 + query_offset),
           std::to_string(157 + query_offset),
           std::to_string(1033 + subject_offset),
           std::to_string(1059 + subject_offset),
           "20", "4", "2", "4",
           "10000", "100000",
           "AAAAAAAATTTTGCCCC---AAAAAAAA",
           "AAAAAAAAGGGG-CCCCGGGAAAAAAAA"},
          scoring_system, paste_parameters)};
      right_plus.UngappedPrefixEnd(12);
      right_plus.UngappedSuffixBegin(20);
      Alignment right_minus{Alignment::FromStringFields(1,
          {std::to_string(133 + query_offset),
           std::to_string(157 + query_offset),
           std::to_string(1059 - subject_offset - left_sregion_length
                               - right_sregion_length),
           std::to_string(1033 - subject_offset - left_sregion_length
                               - right_sregion_length),
           "20", "4", "2", "4",
           "10000", "100000",
           "AAAAAAAATTTTGCCCC---AAAAAAAA",
           "AAAAAAAAGGGG-CCCCGGGAAAAAAAA"},
          scoring_system, paste_parameters)};
      right_minus.UngappedPrefixEnd(12);
      right_minus.UngappedSuffixBegin(20);

      THEN("Pasted ungapped prefix stops at introduced gap.") {
        int expected_ungapped_prefix_end{
            left_plus.Length() + std::min(query_offset, subject_offset)};
        left_plus.PasteRight(right_plus, config, scoring_system,
                             paste_parameters);
        CHECK(expected_ungapped_prefix_end == left_plus.UngappedPrefixEnd());

        expected_ungapped_prefix_end =
            left_minus.Length() + std::min(query_offset, subject_offset);
        left_minus.PasteRight(right_minus, config, scoring_system,
                              paste_parameters);
        CHECK(expected_ungapped_prefix_end == left_minus.UngappedPrefixEnd());
      }
    }
  }
}

SCENARIO("Test correctness of Alignment::PasteRight <suffix>.",
         "[Alignment][PasteRight][correctness][suffix]") {
  ScoringSystem scoring_system{ScoringSystem::Create(100000l, 1, 2, 1, 1)};
  PasteParameters paste_parameters;

  GIVEN("Right alignlefment has gaps.") {
    int right_length{36}, right_sregion_length{34};
    Alignment right_plus{Alignment::FromStringFields(0,
        {"101", "134", "1001", "1034",
         "24", "8", "2", "4",
         "10000", "100000",
         "AAAAAAAATTTTCCCCGGGGAA--AAGGGGAAGGAA",
         "AAAAAAAACCCCCCCCGGGGAAGGAAAAAAAA--AA"},
        scoring_system, paste_parameters)};
    right_plus.UngappedPrefixEnd(22);
    right_plus.UngappedSuffixBegin(34);
    Alignment right_minus{Alignment::FromStringFields(0,
        {"101", "134", "1034", "1001",
         "24", "8", "2", "4",
         "10000", "100000",
         "AAAAAAAATTTTCCCCGGGGAA--AAGGGGAAGGAA",
         "AAAAAAAACCCCCCCCGGGGAAGGAAAAAAAA--AA"},
        scoring_system, paste_parameters)};
    right_minus.UngappedPrefixEnd(22);
    right_minus.UngappedSuffixBegin(34);

    WHEN("Left alignment is ungapped.") {
      int left_length{24};
      int query_offset = GENERATE(-10, -5, 0, 5, 10);
      int subject_offset = GENERATE(-10, -5, 0, 5, 10);
      AlignmentConfiguration config{GetConfiguration(
          query_offset, subject_offset, left_length, right_length)};
      Alignment left_plus{Alignment::FromStringFields(1,
          {std::to_string(77 - query_offset),
           std::to_string(100 - query_offset),
           std::to_string(977 - subject_offset),
           std::to_string(1000 - subject_offset),
           "20", "4", "0", "0",
           "10000", "100000",
           "AAAAAAAATTTTCCCCAAAAAAAA",
           "AAAAAAAAGGGGCCCCAAAAAAAA"},
          scoring_system, paste_parameters)};
      Alignment left_minus{Alignment::FromStringFields(1,
          {std::to_string(77 - query_offset),
           std::to_string(100 - query_offset),
           std::to_string(1000 + subject_offset + right_sregion_length
                               + left_length),
           std::to_string(977 + subject_offset + right_sregion_length
                              + left_length),
           "20", "4", "0", "0",
           "10000", "100000",
           "AAAAAAAATTTTCCCCAAAAAAAA",
           "AAAAAAAAGGGGCCCCAAAAAAAA"},
          scoring_system, paste_parameters)};

      THEN("Pasted alignment takes on right's ungapped suffix.") {
        int expected_ungapped_suffix_begin{config.pasted_length
                                           - right_plus.Length()
                                           + right_plus.UngappedSuffixBegin()};
        left_plus.PasteRight(right_plus, config, scoring_system,
                             paste_parameters);
        CHECK(expected_ungapped_suffix_begin
              == left_plus.UngappedSuffixBegin());

        expected_ungapped_suffix_begin = config.pasted_length
                                         - right_minus.Length()
                                         + right_minus.UngappedSuffixBegin();
        left_minus.PasteRight(right_minus, config, scoring_system,
                              paste_parameters);
        CHECK(expected_ungapped_suffix_begin
              == left_minus.UngappedSuffixBegin());
      }
    }

    WHEN("Left alignment's portion in pasted has no gaps.") {
      int left_length{28}, left_sregion_length{27};
      int query_offset = GENERATE(-10, -8, -6);
      int subject_offset = GENERATE(-10, -8, -6);
      AlignmentConfiguration config{GetConfiguration(
          query_offset, subject_offset, left_length, right_length)};
      Alignment left_plus{Alignment::FromStringFields(1,
          {std::to_string(76 - query_offset),
           std::to_string(100 - query_offset),
           std::to_string(974 - subject_offset),
           std::to_string(1000 - subject_offset),
           "20", "4", "2", "4",
           "10000", "100000",
           "AAAAAAAATTTTCCCCAAAAAAGA---A",
           "AAAAAAAAGGGGCCCCAAAAAA-AGGGA"},
          scoring_system, paste_parameters)};
      left_plus.UngappedPrefixEnd(22);
      left_plus.UngappedSuffixBegin(27);
      Alignment left_minus{Alignment::FromStringFields(1,
          {std::to_string(76 - query_offset),
           std::to_string(100 - query_offset),
           std::to_string(1000 + subject_offset + right_sregion_length
                               + left_sregion_length),
           std::to_string(974 + subject_offset + right_sregion_length
                              + left_sregion_length),
           "20", "4", "2", "4",
           "10000", "100000",
           "AAAAAAAATTTTCCCCAAAAAAGA---A",
           "AAAAAAAAGGGGCCCCAAAAAA-AGGGA"},
          scoring_system, paste_parameters)};
      left_minus.UngappedPrefixEnd(22);
      left_minus.UngappedSuffixBegin(27);

      THEN("Pasted alignment takes on right's ungapped suffix.") {
        int expected_ungapped_suffix_begin{config.pasted_length
                                           - right_plus.Length()
                                           + right_plus.UngappedSuffixBegin()};
        left_plus.PasteRight(right_plus, config, scoring_system,
                             paste_parameters);
        CHECK(expected_ungapped_suffix_begin
              == left_plus.UngappedSuffixBegin());

        expected_ungapped_suffix_begin = config.pasted_length
                                         - right_minus.Length()
                                         + right_minus.UngappedSuffixBegin();
        left_minus.PasteRight(right_minus, config, scoring_system,
                              paste_parameters);
        CHECK(expected_ungapped_suffix_begin
              == left_minus.UngappedSuffixBegin());
      }
    }

    WHEN("Left's portion in pasted has gaps but not right-most.") {
      int left_length{28}, left_sregion_length{27};
      int query_offset = GENERATE(-10, -8, -6);
      int subject_offset = GENERATE(-10, -8, -6);
      AlignmentConfiguration config{GetConfiguration(
          query_offset, subject_offset, left_length, right_length)};
      Alignment left_plus{Alignment::FromStringFields(1,
          {std::to_string(76 - query_offset),
           std::to_string(100 - query_offset),
           std::to_string(974 - subject_offset),
           std::to_string(1000 - subject_offset),
           "20", "4", "2", "4",
           "10000", "100000",
           "AAAGAAAAATTTTCCCCAAAAAAA---A",
           "AAA-AAAAAGGGGCCCCAAAAAAAGGGA"},
          scoring_system, paste_parameters)};
      left_plus.UngappedPrefixEnd(3);
      left_plus.UngappedSuffixBegin(27);
      Alignment left_minus{Alignment::FromStringFields(1,
          {std::to_string(76 - query_offset),
           std::to_string(100 - query_offset),
           std::to_string(1000 + subject_offset + right_sregion_length
                               + left_sregion_length),
           std::to_string(974 + subject_offset + right_sregion_length
                              + left_sregion_length),
           "20", "4", "2", "4",
           "10000", "100000",
           "AAAGAAAAATTTTCCCCAAAAAAA---A",
           "AAA-AAAAAGGGGCCCCAAAAAAAGGGA"},
          scoring_system, paste_parameters)};
      left_minus.UngappedPrefixEnd(3);
      left_minus.UngappedSuffixBegin(27);

      THEN("Pasted alignment takes on right's ungapped suffix.") {
        int expected_ungapped_suffix_begin{config.pasted_length
                                           - right_plus.Length()
                                           + right_plus.UngappedSuffixBegin()};
        left_plus.PasteRight(right_plus, config, scoring_system,
                             paste_parameters);
        CHECK(expected_ungapped_suffix_begin
              == left_plus.UngappedSuffixBegin());

        expected_ungapped_suffix_begin = config.pasted_length
                                         - right_minus.Length()
                                         + right_minus.UngappedSuffixBegin();
        left_minus.PasteRight(right_minus, config, scoring_system,
                              paste_parameters);
        CHECK(expected_ungapped_suffix_begin
              == left_minus.UngappedSuffixBegin());
      }
    }

    WHEN("Left alignment's portion in pasted has right-most gap.") {
      int left_length{28}, left_sregion_length{27};
      int query_offset = GENERATE(-10, -5, 0, 5, 10);
      int subject_offset = GENERATE(-10, -5, 0, 5, 10);
      AlignmentConfiguration config{GetConfiguration(
          query_offset, subject_offset, left_length, right_length)};
      Alignment left_plus{Alignment::FromStringFields(1,
          {std::to_string(76 - query_offset),
           std::to_string(100 - query_offset),
           std::to_string(974 - subject_offset),
           std::to_string(1000 - subject_offset),
           "20", "4", "2", "4",
           "10000", "100000",
           "AAGAAAAA---ATTTTCCCCAAAAAAAA",
           "AA-AAAAAGGGAGGGGCCCCAAAAAAAA"},
          scoring_system, paste_parameters)};
      left_plus.UngappedPrefixEnd(2);
      left_plus.UngappedSuffixBegin(11);
      Alignment left_minus{Alignment::FromStringFields(1,
          {std::to_string(76 - query_offset),
           std::to_string(100 - query_offset),
           std::to_string(1000 + subject_offset + right_sregion_length
                               + left_sregion_length),
           std::to_string(974 + subject_offset + right_sregion_length
                              + left_sregion_length),
           "20", "4", "2", "4",
           "10000", "100000",
           "AAGAAAAA---ATTTTCCCCAAAAAAAA",
           "AA-AAAAAGGGAGGGGCCCCAAAAAAAA"},
          scoring_system, paste_parameters)};
      left_minus.UngappedPrefixEnd(2);
      left_minus.UngappedSuffixBegin(11);

      THEN("Pasted alignment takes on right's ungapped suffix.") {
        int expected_ungapped_suffix_begin{config.pasted_length
                                           - right_plus.Length()
                                           + right_plus.UngappedSuffixBegin()};
        left_plus.PasteRight(right_plus, config, scoring_system,
                             paste_parameters);
        CHECK(expected_ungapped_suffix_begin
              == left_plus.UngappedSuffixBegin());

        expected_ungapped_suffix_begin = config.pasted_length
                                         - right_minus.Length()
                                         + right_minus.UngappedSuffixBegin();
        left_minus.PasteRight(right_minus, config, scoring_system,
                              paste_parameters);
        CHECK(expected_ungapped_suffix_begin
              == left_minus.UngappedSuffixBegin());
      }
    }
  }

  GIVEN("Right alignment is ungapped.") {
    int right_length{32}, right_sregion_length{32};
    Alignment right_plus{Alignment::FromStringFields(0,
        {"101", "132", "1001", "1032",
         "24", "8", "0", "0",
         "10000", "100000",
         "AAAAAAAATTTTCCCCGGGGAAAAGGGGAAAA",
         "AAAAAAAACCCCCCCCGGGGAAAAAAAAAAAA"},
        scoring_system, paste_parameters)};
    Alignment right_minus{Alignment::FromStringFields(0,
        {"101", "132", "1032", "1001",
         "24", "8", "0", "0",
         "10000", "100000",
         "AAAAAAAATTTTCCCCGGGGAAAAGGGGAAAA",
         "AAAAAAAACCCCCCCCGGGGAAAAAAAAAAAA"},
        scoring_system, paste_parameters)};

    WHEN("Left alignment is ungapped and 0-shift.") {
      int left_length{24};
      int query_offset = GENERATE(-10, -8, -6);
      int subject_offset{query_offset};
      AlignmentConfiguration config{GetConfiguration(
          query_offset, subject_offset, left_length, right_length)};
      Alignment left_plus{Alignment::FromStringFields(1,
          {std::to_string(77 - query_offset),
           std::to_string(100 - query_offset),
           std::to_string(977 - subject_offset),
           std::to_string(1000 - subject_offset),
           "20", "4", "0", "0",
           "10000", "100000",
           "AAAAAAAATTTTCCCCAAAAAAAA",
           "AAAAAAAAGGGGCCCCAAAAAAAA"},
          scoring_system, paste_parameters)};
      Alignment left_minus{Alignment::FromStringFields(1,
          {std::to_string(77 - query_offset),
           std::to_string(100 - query_offset),
           std::to_string(1000 + subject_offset + right_sregion_length
                               + left_length),
           std::to_string(977 + subject_offset + right_sregion_length
                              + left_length),
           "20", "4", "0", "0",
           "10000", "100000",
           "AAAAAAAATTTTCCCCAAAAAAAA",
           "AAAAAAAAGGGGCCCCAAAAAAAA"},
          scoring_system, paste_parameters)};

      THEN("Pasted alignment is ungapped.") {
        left_plus.PasteRight(right_plus, config, scoring_system,
                             paste_parameters);
        CHECK(0 == left_plus.UngappedSuffixBegin());

        left_minus.PasteRight(right_minus, config, scoring_system,
                              paste_parameters);
        CHECK(0 == left_minus.UngappedSuffixBegin());
      }
    }

    WHEN("Left alignment's portion in pasted has no gaps and 0-shift.") {
      int left_length{28}, left_sregion_length{27};
      int query_offset = GENERATE(-10, -8, -6);
      int subject_offset{query_offset};
      AlignmentConfiguration config{GetConfiguration(
          query_offset, subject_offset, left_length, right_length)};
      Alignment left_plus{Alignment::FromStringFields(1,
          {std::to_string(76 - query_offset),
           std::to_string(100 - query_offset),
           std::to_string(974 - subject_offset),
           std::to_string(1000 - subject_offset),
           "20", "4", "2", "4",
           "10000", "100000",
           "AAAAAAAATTTTCCCCAAAAAAGA---A",
           "AAAAAAAAGGGGCCCCAAAAAA-AGGGA"},
          scoring_system, paste_parameters)};
      left_plus.UngappedPrefixEnd(22);
      left_plus.UngappedSuffixBegin(27);
      Alignment left_minus{Alignment::FromStringFields(1,
          {std::to_string(76 - query_offset),
           std::to_string(100 - query_offset),
           std::to_string(1000 + subject_offset + right_sregion_length
                               + left_sregion_length),
           std::to_string(974 + subject_offset + right_sregion_length
                              + left_sregion_length),
           "20", "4", "2", "4",
           "10000", "100000",
           "AAAAAAAATTTTCCCCAAAAAAGA---A",
           "AAAAAAAAGGGGCCCCAAAAAA-AGGGA"},
          scoring_system, paste_parameters)};
      left_minus.UngappedPrefixEnd(22);
      left_minus.UngappedSuffixBegin(27);

      THEN("Pasted alignment is ungapped.") {
        left_plus.PasteRight(right_plus, config, scoring_system,
                             paste_parameters);
        CHECK(0 == left_plus.UngappedSuffixBegin());

        left_minus.PasteRight(right_minus, config, scoring_system,
                              paste_parameters);
        CHECK(0 == left_minus.UngappedSuffixBegin());
      }
    }

    WHEN("Left's portion in pasted has gaps but not right-most and 0-shift.") {
      int left_length{28}, left_sregion_length{27};
      int query_offset = GENERATE(-10, -8, -6);
      int subject_offset{query_offset};
      AlignmentConfiguration config{GetConfiguration(
          query_offset, subject_offset, left_length, right_length)};
      Alignment left_plus{Alignment::FromStringFields(1,
          {std::to_string(76 - query_offset),
           std::to_string(100 - query_offset),
           std::to_string(974 - subject_offset),
           std::to_string(1000 - subject_offset),
           "20", "4", "2", "4",
           "10000", "100000",
           "AAAGAAAAATTTTCCCCAAAAAAA---A",
           "AAA-AAAAAGGGGCCCCAAAAAAAGGGA"},
          scoring_system, paste_parameters)};
      left_plus.UngappedPrefixEnd(3);
      left_plus.UngappedSuffixBegin(27);
      Alignment left_minus{Alignment::FromStringFields(1,
          {std::to_string(76 - query_offset),
           std::to_string(100 - query_offset),
           std::to_string(1000 + subject_offset + right_sregion_length
                               + left_sregion_length),
           std::to_string(974 + subject_offset + right_sregion_length
                              + left_sregion_length),
           "20", "4", "2", "4",
           "10000", "100000",
           "AAAGAAAAATTTTCCCCAAAAAAA---A",
           "AAA-AAAAAGGGGCCCCAAAAAAAGGGA"},
          scoring_system, paste_parameters)};
      left_minus.UngappedPrefixEnd(3);
      left_minus.UngappedSuffixBegin(27);

      THEN("Pasted alignment's suffix starts where left's portion ends.") {
        int left_end{left_plus.Length() + std::min({0, query_offset,
                                                    subject_offset})};
        left_plus.PasteRight(right_plus, config, scoring_system,
                             paste_parameters);
        CHECK(left_end == left_plus.UngappedSuffixBegin());

        left_end = left_minus.Length() + std::min({0, query_offset,
                                                   subject_offset});
        left_minus.PasteRight(right_minus, config, scoring_system,
                              paste_parameters);
        CHECK(left_end == left_minus.UngappedSuffixBegin());
      }
    }

    WHEN("Left alignment's portion in pasted has gaps and 0-shift.") {
      int left_length{28}, left_sregion_length{27};
      int query_offset = GENERATE(-10, -5, 0, 5, 10);
      int subject_offset{query_offset};
      AlignmentConfiguration config{GetConfiguration(
          query_offset, subject_offset, left_length, right_length)};
      Alignment left_plus{Alignment::FromStringFields(1,
          {std::to_string(76 - query_offset),
           std::to_string(100 - query_offset),
           std::to_string(974 - subject_offset),
           std::to_string(1000 - subject_offset),
           "20", "4", "2", "4",
           "10000", "100000",
           "AAGAAAAA---ATTTTCCCCAAAAAAAA",
           "AA-AAAAAGGGAGGGGCCCCAAAAAAAA"},
          scoring_system, paste_parameters)};
      left_plus.UngappedPrefixEnd(2);
      left_plus.UngappedSuffixBegin(11);
      Alignment left_minus{Alignment::FromStringFields(1,
          {std::to_string(76 - query_offset),
           std::to_string(100 - query_offset),
           std::to_string(1000 + subject_offset + right_sregion_length
                               + left_sregion_length),
           std::to_string(974 + subject_offset + right_sregion_length
                              + left_sregion_length),
           "20", "4", "2", "4",
           "10000", "100000",
           "AAGAAAAA---ATTTTCCCCAAAAAAAA",
           "AA-AAAAAGGGAGGGGCCCCAAAAAAAA"},
          scoring_system, paste_parameters)};
      left_minus.UngappedPrefixEnd(2);
      left_minus.UngappedSuffixBegin(11);

      THEN("Pasted alignment's sufffix includes left's suffix.") {
        int expected_ungapped_suffix_begin{left_plus.UngappedSuffixBegin()};
        left_plus.PasteRight(right_plus, config, scoring_system,
                             paste_parameters);
        CHECK(expected_ungapped_suffix_begin
              == left_plus.UngappedSuffixBegin());

        expected_ungapped_suffix_begin = left_minus.UngappedSuffixBegin();
        left_minus.PasteRight(right_minus, config, scoring_system,
                              paste_parameters);
        CHECK(expected_ungapped_suffix_begin
              == left_minus.UngappedSuffixBegin());
      }
    }

    WHEN("Left alignment is ungapped and non0-shift.") {
      int left_length{24};
      int query_offset = GENERATE(-10, 0, 10);
      int shift = GENERATE(-5, -2, -1, 1, 2, 5);
      int subject_offset{query_offset + shift};
      AlignmentConfiguration config{GetConfiguration(
          query_offset, subject_offset, left_length, right_length)};
      Alignment left_plus{Alignment::FromStringFields(1,
          {std::to_string(77 - query_offset),
           std::to_string(100 - query_offset),
           std::to_string(977 - subject_offset),
           std::to_string(1000 - subject_offset),
           "20", "4", "0", "0",
           "10000", "100000",
           "AAAAAAAATTTTCCCCAAAAAAAA",
           "AAAAAAAAGGGGCCCCAAAAAAAA"},
          scoring_system, paste_parameters)};
      Alignment left_minus{Alignment::FromStringFields(1,
          {std::to_string(77 - query_offset),
           std::to_string(100 - query_offset),
           std::to_string(1000 + subject_offset + right_sregion_length
                               + left_length),
           std::to_string(977 + subject_offset + right_sregion_length
                              + left_length),
           "20", "4", "0", "0",
           "10000", "100000",
           "AAAAAAAATTTTCCCCAAAAAAAA",
           "AAAAAAAAGGGGCCCCAAAAAAAA"},
          scoring_system, paste_parameters)};

      THEN("Pasted's suffix begins where inserted gap ends.") {
        int expected_ungapped_suffix_begin{
            left_plus.Length() + std::min({0, query_offset, subject_offset})
            + std::abs(shift)};
        left_plus.PasteRight(right_plus, config, scoring_system,
                             paste_parameters);
        CHECK(expected_ungapped_suffix_begin
              == left_plus.UngappedSuffixBegin());

        expected_ungapped_suffix_begin
            = left_minus.Length() + std::min({0, query_offset, subject_offset})
              + std::abs(shift);
        left_minus.PasteRight(right_minus, config, scoring_system,
                              paste_parameters);
        CHECK(expected_ungapped_suffix_begin
              == left_minus.UngappedSuffixBegin());
      }
    }

    WHEN("Left alignment's portion in pasted has no gaps and non0-shift.") {
      int left_length{28}, left_sregion_length{27};
      int query_offset = GENERATE(-10, -8, -6);
      int shift = GENERATE(-5, -2, -1, 1, 2, 5);
      int subject_offset{query_offset + shift};
      AlignmentConfiguration config{GetConfiguration(
          query_offset, subject_offset, left_length, right_length)};
      Alignment left_plus{Alignment::FromStringFields(1,
          {std::to_string(76 - query_offset),
           std::to_string(100 - query_offset),
           std::to_string(974 - subject_offset),
           std::to_string(1000 - subject_offset),
           "20", "4", "2", "4",
           "10000", "100000",
           "AAAAAAAATTTTCCCCAAAAAAGA---A",
           "AAAAAAAAGGGGCCCCAAAAAA-AGGGA"},
          scoring_system, paste_parameters)};
      left_plus.UngappedPrefixEnd(22);
      left_plus.UngappedSuffixBegin(27);
      Alignment left_minus{Alignment::FromStringFields(1,
          {std::to_string(76 - query_offset),
           std::to_string(100 - query_offset),
           std::to_string(1000 + subject_offset + right_sregion_length
                               + left_sregion_length),
           std::to_string(974 + subject_offset + right_sregion_length
                              + left_sregion_length),
           "20", "4", "2", "4",
           "10000", "100000",
           "AAAAAAAATTTTCCCCAAAAAAGA---A",
           "AAAAAAAAGGGGCCCCAAAAAA-AGGGA"},
          scoring_system, paste_parameters)};
      left_minus.UngappedPrefixEnd(22);
      left_minus.UngappedSuffixBegin(27);

      THEN("Pasted's suffix begins where inserted gap ends.") {
        int expected_ungapped_suffix_begin{
            left_plus.Length() + std::min({0, query_offset, subject_offset})
            + std::abs(shift)};
        left_plus.PasteRight(right_plus, config, scoring_system,
                             paste_parameters);
        CHECK(expected_ungapped_suffix_begin
              == left_plus.UngappedSuffixBegin());

        expected_ungapped_suffix_begin
            = left_minus.Length() + std::min({0, query_offset, subject_offset})
              + std::abs(shift);
        left_minus.PasteRight(right_minus, config, scoring_system,
                              paste_parameters);
        CHECK(expected_ungapped_suffix_begin
              == left_minus.UngappedSuffixBegin());
      }
    }

    WHEN("Left's portion in pasted has gaps but not right-most; non0-shift.") {
      int left_length{28}, left_sregion_length{27};
      int query_offset = GENERATE(-10, -8, -6);
      int shift = GENERATE(-5, -2, -1, 1, 2, 5);
      int subject_offset{query_offset + shift};
      AlignmentConfiguration config{GetConfiguration(
          query_offset, subject_offset, left_length, right_length)};
      Alignment left_plus{Alignment::FromStringFields(1,
          {std::to_string(76 - query_offset),
           std::to_string(100 - query_offset),
           std::to_string(974 - subject_offset),
           std::to_string(1000 - subject_offset),
           "20", "4", "2", "4",
           "10000", "100000",
           "AAAGAAAAATTTTCCCCAAAAAAA---A",
           "AAA-AAAAAGGGGCCCCAAAAAAAGGGA"},
          scoring_system, paste_parameters)};
      left_plus.UngappedPrefixEnd(3);
      left_plus.UngappedSuffixBegin(27);
      Alignment left_minus{Alignment::FromStringFields(1,
          {std::to_string(76 - query_offset),
           std::to_string(100 - query_offset),
           std::to_string(1000 + subject_offset + right_sregion_length
                               + left_sregion_length),
           std::to_string(974 + subject_offset + right_sregion_length
                              + left_sregion_length),
           "20", "4", "2", "4",
           "10000", "100000",
           "AAAGAAAAATTTTCCCCAAAAAAA---A",
           "AAA-AAAAAGGGGCCCCAAAAAAAGGGA"},
          scoring_system, paste_parameters)};
      left_minus.UngappedPrefixEnd(3);
      left_minus.UngappedSuffixBegin(27);

      THEN("Pasted's suffix begins where inserted gap ends.") {
        int expected_ungapped_suffix_begin{
            left_plus.Length() + std::min({0, query_offset, subject_offset})
            + std::abs(shift)};
        left_plus.PasteRight(right_plus, config, scoring_system,
                             paste_parameters);
        CHECK(expected_ungapped_suffix_begin
              == left_plus.UngappedSuffixBegin());

        expected_ungapped_suffix_begin
            = left_minus.Length() + std::min({0, query_offset, subject_offset})
              + std::abs(shift);
        left_minus.PasteRight(right_minus, config, scoring_system,
                              paste_parameters);
        CHECK(expected_ungapped_suffix_begin
              == left_minus.UngappedSuffixBegin());
      }
    }

    WHEN("Left alignment's portion in pasted has right-most gap; non0-shift.") {
      int left_length{28}, left_sregion_length{27};
      int query_offset = GENERATE(-10, -5, 0, 5, 10);
      int shift = GENERATE(-5, -2, -1, 1, 2, 5);
      int subject_offset{query_offset + shift};
      AlignmentConfiguration config{GetConfiguration(
          query_offset, subject_offset, left_length, right_length)};
      Alignment left_plus{Alignment::FromStringFields(1,
          {std::to_string(76 - query_offset),
           std::to_string(100 - query_offset),
           std::to_string(974 - subject_offset),
           std::to_string(1000 - subject_offset),
           "20", "4", "2", "4",
           "10000", "100000",
           "AAGAAAAA---ATTTTCCCCAAAAAAAA",
           "AA-AAAAAGGGAGGGGCCCCAAAAAAAA"},
          scoring_system, paste_parameters)};
      left_plus.UngappedPrefixEnd(2);
      left_plus.UngappedSuffixBegin(11);
      Alignment left_minus{Alignment::FromStringFields(1,
          {std::to_string(76 - query_offset),
           std::to_string(100 - query_offset),
           std::to_string(1000 + subject_offset + right_sregion_length
                               + left_sregion_length),
           std::to_string(974 + subject_offset + right_sregion_length
                              + left_sregion_length),
           "20", "4", "2", "4",
           "10000", "100000",
           "AAGAAAAA---ATTTTCCCCAAAAAAAA",
           "AA-AAAAAGGGAGGGGCCCCAAAAAAAA"},
          scoring_system, paste_parameters)};
      left_minus.UngappedPrefixEnd(2);
      left_minus.UngappedSuffixBegin(11);

      THEN("Pasted's suffix begins where inserted gap ends.") {
        int expected_ungapped_suffix_begin{
            left_plus.Length() + std::min({0, query_offset, subject_offset})
            + std::abs(shift)};
        left_plus.PasteRight(right_plus, config, scoring_system,
                             paste_parameters);
        CHECK(expected_ungapped_suffix_begin
              == left_plus.UngappedSuffixBegin());

        expected_ungapped_suffix_begin
            = left_minus.Length() + std::min({0, query_offset, subject_offset})
              + std::abs(shift);
        left_minus.PasteRight(right_minus, config, scoring_system,
                              paste_parameters);
        CHECK(expected_ungapped_suffix_begin
              == left_minus.UngappedSuffixBegin());
      }
    }
  }
}

SCENARIO("Test correctness of Alignment::PasteRight <main>.",
         "[Alignment][PasteRight][correctness][main]") {
  ScoringSystem scoring_system{ScoringSystem::Create(100000l, 1, 2, 1, 1)};
  PasteParameters paste_parameters;

  GIVEN("Data for alignments whose pasting won't alter prefix/suffix length.") {
    int left_qstart{101}, left_qend{134},
        left_sstart{1001}, left_send{1036},
        right_qstart{135}, right_qend{160},
        right_sstart{1037}, right_send{1060},
        left_nident{24}, left_mismatch{8}, left_gapopen{2}, left_gaps{6},
        right_nident{20}, right_mismatch{4}, right_gapopen{1}, right_gaps{2},
        qlen{10000}, slen{100000};
    std::string left_qseq{"AAAAAAAATTTTTTCCCCGGGG----GGGGAAAAAAAA"},
                left_sseq{"AAAAAAAACCCC--CCCCGGGGAAAATTTTAAAAAAAA"},
                right_qseq{"AAAAAAAATTTTCCCCGGAAAAAAAA"},
                right_sseq{"AAAAAAAAGGGGCCCC--AAAAAAAA"};
    int left_prefix_end{12}, left_suffix_begin{26},
        right_prefix_end{16}, right_suffix_begin{18};

    WHEN("query_offset == subject_offset == 0; +strand") {
      // Specify test case parameters.
      int query_offset{0}, subject_offset{0};
      bool plus_strand{true};
      HandleOffset(query_offset, subject_offset, plus_strand,
                   left_sstart, left_send, right_qstart, right_qend,
                   right_sstart, right_send);
      // Specify expected pasted outcome.
      int pasted_qstart{left_qstart}, pasted_qend{right_qend},
          pasted_sstart{left_sstart}, pasted_send{right_send},
          pasted_nident{left_nident + right_nident},
          pasted_mismatch{left_mismatch + right_mismatch},
          pasted_gapopen{left_gapopen + right_gapopen},
          pasted_gaps{left_gaps + right_gaps},
          pasted_prefix_end{left_prefix_end},
          pasted_suffix_begin{right_suffix_begin
                              + static_cast<int>(left_qseq.length())};
      std::string pasted_qseq, pasted_sseq;
      pasted_qseq = left_qseq;
      pasted_qseq.append(right_qseq);
      pasted_sseq = left_sseq;
      pasted_sseq.append(right_sseq);

      // Swapped subject coordinates indicate minus strand.
      if (!plus_strand) {
        std::swap(left_sstart, left_send);
        std::swap(right_sstart, right_send);
      }

      Alignment left{Alignment::FromStringFields(0,
        {std::to_string(left_qstart), std::to_string(left_qend),
         std::to_string(left_sstart), std::to_string(left_send),
         std::to_string(left_nident), std::to_string(left_mismatch),
         std::to_string(left_gapopen), std::to_string(left_gaps),
         std::to_string(qlen), std::to_string(slen),
         left_qseq, left_sseq},
        scoring_system, paste_parameters)};
      left.UngappedPrefixEnd(left_prefix_end);
      left.UngappedSuffixBegin(left_suffix_begin);
      Alignment right{Alignment::FromStringFields(1,
        {std::to_string(right_qstart), std::to_string(right_qend),
         std::to_string(right_sstart), std::to_string(right_send),
         std::to_string(right_nident), std::to_string(right_mismatch),
         std::to_string(right_gapopen), std::to_string(right_gaps),
         std::to_string(qlen), std::to_string(slen),
         right_qseq, right_sseq},
        scoring_system, paste_parameters)};
      right.UngappedPrefixEnd(right_prefix_end);
      right.UngappedSuffixBegin(right_suffix_begin);

      AlignmentConfiguration config{GetConfiguration(
          query_offset, subject_offset,
          left_qseq.length(), right_qseq.length())};

      Alignment original_right{right};
      left.PasteRight(right, config, scoring_system, paste_parameters);

      THEN("Left becomes the pasted alignment.") {
        CHECK(left.Qstart() == pasted_qstart);
        CHECK(left.Qend() == pasted_qend);
        CHECK(left.Sstart() == pasted_sstart);
        CHECK(left.Send() == pasted_send);
        CHECK(left.PlusStrand() == plus_strand);
        CHECK(left.Nident() == pasted_nident);
        CHECK(left.Mismatch() == pasted_mismatch);
        CHECK(left.Gapopen() == pasted_gapopen);
        CHECK(left.Gaps() == pasted_gaps);
        CHECK(left.Qlen() == qlen);
        CHECK(left.Slen() == slen);
        CHECK(left.Qseq() == pasted_qseq);
        CHECK(left.Sseq() == pasted_sseq);
        CHECK(left.IncludeInOutput() == false);
        CHECK(left.UngappedPrefixEnd() == pasted_prefix_end);
        CHECK(left.UngappedSuffixBegin() == pasted_suffix_begin);
      }

      THEN("Right remains unchanged.") {
        CHECK(right == original_right);
      }
    }

    WHEN("query_offset == subject_offset == 0; -strand") {
      // Specify test case parameters.
      int query_offset{0}, subject_offset{0};
      bool plus_strand{false};
      HandleOffset(query_offset, subject_offset, plus_strand,
                   left_sstart, left_send, right_qstart, right_qend,
                   right_sstart, right_send);
      // Specify expected pasted outcome.
      int pasted_qstart{left_qstart}, pasted_qend{right_qend},
          pasted_sstart{right_sstart}, pasted_send{left_send},
          pasted_nident{left_nident + right_nident},
          pasted_mismatch{left_mismatch + right_mismatch},
          pasted_gapopen{left_gapopen + right_gapopen},
          pasted_gaps{left_gaps + right_gaps},
          pasted_prefix_end{left_prefix_end},
          pasted_suffix_begin{right_suffix_begin
                              + static_cast<int>(left_qseq.length())};
      std::string pasted_qseq, pasted_sseq;
      pasted_qseq = left_qseq;
      pasted_qseq.append(right_qseq);
      pasted_sseq = left_sseq;
      pasted_sseq.append(right_sseq);

      // Swapped subject coordinates indicate minus strand.
      if (!plus_strand) {
        std::swap(left_sstart, left_send);
        std::swap(right_sstart, right_send);
      }

      Alignment left{Alignment::FromStringFields(0,
        {std::to_string(left_qstart), std::to_string(left_qend),
         std::to_string(left_sstart), std::to_string(left_send),
         std::to_string(left_nident), std::to_string(left_mismatch),
         std::to_string(left_gapopen), std::to_string(left_gaps),
         std::to_string(qlen), std::to_string(slen),
         left_qseq, left_sseq},
        scoring_system, paste_parameters)};
      left.UngappedPrefixEnd(left_prefix_end);
      left.UngappedSuffixBegin(left_suffix_begin);
      Alignment right{Alignment::FromStringFields(1,
        {std::to_string(right_qstart), std::to_string(right_qend),
         std::to_string(right_sstart), std::to_string(right_send),
         std::to_string(right_nident), std::to_string(right_mismatch),
         std::to_string(right_gapopen), std::to_string(right_gaps),
         std::to_string(qlen), std::to_string(slen),
         right_qseq, right_sseq},
        scoring_system, paste_parameters)};
      right.UngappedPrefixEnd(right_prefix_end);
      right.UngappedSuffixBegin(right_suffix_begin);

      AlignmentConfiguration config{GetConfiguration(
          query_offset, subject_offset,
          left_qseq.length(), right_qseq.length())};

      Alignment original_right{right};
      left.PasteRight(right, config, scoring_system, paste_parameters);

      THEN("Left becomes the pasted alignment.") {
        CHECK(left.Qstart() == pasted_qstart);
        CHECK(left.Qend() == pasted_qend);
        CHECK(left.Sstart() == pasted_sstart);
        CHECK(left.Send() == pasted_send);
        CHECK(left.PlusStrand() == plus_strand);
        CHECK(left.Nident() == pasted_nident);
        CHECK(left.Mismatch() == pasted_mismatch);
        CHECK(left.Gapopen() == pasted_gapopen);
        CHECK(left.Gaps() == pasted_gaps);
        CHECK(left.Qlen() == qlen);
        CHECK(left.Slen() == slen);
        CHECK(left.Qseq() == pasted_qseq);
        CHECK(left.Sseq() == pasted_sseq);
        CHECK(left.IncludeInOutput() == false);
        CHECK(left.UngappedPrefixEnd() == pasted_prefix_end);
        CHECK(left.UngappedSuffixBegin() == pasted_suffix_begin);
      }

      THEN("Right remains unchanged.") {
        CHECK(right == original_right);
      }
    }

    WHEN("query_offset == subject_offset > 0; +strand") {
      // Specify test case parameters.
      int query_offset{10}, subject_offset{10};
      bool plus_strand{true};
      HandleOffset(query_offset, subject_offset, plus_strand,
                   left_sstart, left_send, right_qstart, right_qend,
                   right_sstart, right_send);
      // Specify expected pasted outcome.
      int pasted_qstart{left_qstart}, pasted_qend{right_qend},
          pasted_sstart{left_sstart}, pasted_send{right_send},
          pasted_nident{left_nident + right_nident},
          pasted_mismatch{left_mismatch + right_mismatch + query_offset},
          pasted_gapopen{left_gapopen + right_gapopen},
          pasted_gaps{left_gaps + right_gaps},
          pasted_prefix_end{left_prefix_end},
          pasted_suffix_begin{right_suffix_begin + query_offset
                              + static_cast<int>(left_qseq.length())};

      std::string pasted_qseq, pasted_sseq;
      pasted_qseq = left_qseq;
      pasted_qseq.append(query_offset, 'N');
      pasted_qseq.append(right_qseq);

      pasted_sseq = left_sseq;
      pasted_sseq.append(subject_offset, 'N');
      pasted_sseq.append(right_sseq);

      // Swapped subject coordinates indicate minus strand.
      if (!plus_strand) {
        std::swap(left_sstart, left_send);
        std::swap(right_sstart, right_send);
      }

      Alignment left{Alignment::FromStringFields(0,
        {std::to_string(left_qstart), std::to_string(left_qend),
         std::to_string(left_sstart), std::to_string(left_send),
         std::to_string(left_nident), std::to_string(left_mismatch),
         std::to_string(left_gapopen), std::to_string(left_gaps),
         std::to_string(qlen), std::to_string(slen),
         left_qseq, left_sseq},
        scoring_system, paste_parameters)};
      left.UngappedPrefixEnd(left_prefix_end);
      left.UngappedSuffixBegin(left_suffix_begin);
      Alignment right{Alignment::FromStringFields(1,
        {std::to_string(right_qstart), std::to_string(right_qend),
         std::to_string(right_sstart), std::to_string(right_send),
         std::to_string(right_nident), std::to_string(right_mismatch),
         std::to_string(right_gapopen), std::to_string(right_gaps),
         std::to_string(qlen), std::to_string(slen),
         right_qseq, right_sseq},
        scoring_system, paste_parameters)};
      right.UngappedPrefixEnd(right_prefix_end);
      right.UngappedSuffixBegin(right_suffix_begin);

      AlignmentConfiguration config{GetConfiguration(
          query_offset, subject_offset,
          left_qseq.length(), right_qseq.length())};

      Alignment original_right{right};
      left.PasteRight(right, config, scoring_system, paste_parameters);

      THEN("Left becomes the pasted alignment.") {
        CHECK(left.Qstart() == pasted_qstart);
        CHECK(left.Qend() == pasted_qend);
        CHECK(left.Sstart() == pasted_sstart);
        CHECK(left.Send() == pasted_send);
        CHECK(left.PlusStrand() == plus_strand);
        CHECK(left.Nident() == pasted_nident);
        CHECK(left.Mismatch() == pasted_mismatch);
        CHECK(left.Gapopen() == pasted_gapopen);
        CHECK(left.Gaps() == pasted_gaps);
        CHECK(left.Qlen() == qlen);
        CHECK(left.Slen() == slen);
        CHECK(left.Qseq() == pasted_qseq);
        CHECK(left.Sseq() == pasted_sseq);
        CHECK(left.IncludeInOutput() == false);
        CHECK(left.UngappedPrefixEnd() == pasted_prefix_end);
        CHECK(left.UngappedSuffixBegin() == pasted_suffix_begin);
      }

      THEN("Right remains unchanged.") {
        CHECK(right == original_right);
      }
    }

    WHEN("query_offset == subject_offset > 0; -strand") {
      // Specify test case parameters.
      int query_offset{10}, subject_offset{10};
      bool plus_strand{false};
      HandleOffset(query_offset, subject_offset, plus_strand,
                   left_sstart, left_send, right_qstart, right_qend,
                   right_sstart, right_send);
      // Specify expected pasted outcome.
      int pasted_qstart{left_qstart}, pasted_qend{right_qend},
          pasted_sstart{right_sstart}, pasted_send{left_send},
          pasted_nident{left_nident + right_nident},
          pasted_mismatch{left_mismatch + right_mismatch + query_offset},
          pasted_gapopen{left_gapopen + right_gapopen},
          pasted_gaps{left_gaps + right_gaps},
          pasted_prefix_end{left_prefix_end},
          pasted_suffix_begin{right_suffix_begin + query_offset
                              + static_cast<int>(left_qseq.length())};

      std::string pasted_qseq, pasted_sseq;
      pasted_qseq = left_qseq;
      pasted_qseq.append(query_offset, 'N');
      pasted_qseq.append(right_qseq);

      pasted_sseq = left_sseq;
      pasted_sseq.append(subject_offset, 'N');
      pasted_sseq.append(right_sseq);

      // Swapped subject coordinates indicate minus strand.
      if (!plus_strand) {
        std::swap(left_sstart, left_send);
        std::swap(right_sstart, right_send);
      }

      Alignment left{Alignment::FromStringFields(0,
        {std::to_string(left_qstart), std::to_string(left_qend),
         std::to_string(left_sstart), std::to_string(left_send),
         std::to_string(left_nident), std::to_string(left_mismatch),
         std::to_string(left_gapopen), std::to_string(left_gaps),
         std::to_string(qlen), std::to_string(slen),
         left_qseq, left_sseq},
        scoring_system, paste_parameters)};
      left.UngappedPrefixEnd(left_prefix_end);
      left.UngappedSuffixBegin(left_suffix_begin);
      Alignment right{Alignment::FromStringFields(1,
        {std::to_string(right_qstart), std::to_string(right_qend),
         std::to_string(right_sstart), std::to_string(right_send),
         std::to_string(right_nident), std::to_string(right_mismatch),
         std::to_string(right_gapopen), std::to_string(right_gaps),
         std::to_string(qlen), std::to_string(slen),
         right_qseq, right_sseq},
        scoring_system, paste_parameters)};
      right.UngappedPrefixEnd(right_prefix_end);
      right.UngappedSuffixBegin(right_suffix_begin);

      AlignmentConfiguration config{GetConfiguration(
          query_offset, subject_offset,
          left_qseq.length(), right_qseq.length())};

      Alignment original_right{right};
      left.PasteRight(right, config, scoring_system, paste_parameters);

      THEN("Left becomes the pasted alignment.") {
        CHECK(left.Qstart() == pasted_qstart);
        CHECK(left.Qend() == pasted_qend);
        CHECK(left.Sstart() == pasted_sstart);
        CHECK(left.Send() == pasted_send);
        CHECK(left.PlusStrand() == plus_strand);
        CHECK(left.Nident() == pasted_nident);
        CHECK(left.Mismatch() == pasted_mismatch);
        CHECK(left.Gapopen() == pasted_gapopen);
        CHECK(left.Gaps() == pasted_gaps);
        CHECK(left.Qlen() == qlen);
        CHECK(left.Slen() == slen);
        CHECK(left.Qseq() == pasted_qseq);
        CHECK(left.Sseq() == pasted_sseq);
        CHECK(left.IncludeInOutput() == false);
        CHECK(left.UngappedPrefixEnd() == pasted_prefix_end);
        CHECK(left.UngappedSuffixBegin() == pasted_suffix_begin);
      }

      THEN("Right remains unchanged.") {
        CHECK(right == original_right);
      }
    }

    WHEN("query_offset == subject_offset < 0; +strand") {
      // Specify test case parameters.
      int query_offset{-4}, subject_offset{-4};
      bool plus_strand{true};
      HandleOffset(query_offset, subject_offset, plus_strand,
                   left_sstart, left_send, right_qstart, right_qend,
                   right_sstart, right_send);
      // Specify expected pasted outcome.
      int pasted_qstart{left_qstart}, pasted_qend{right_qend},
          pasted_sstart{left_sstart}, pasted_send{right_send},
          pasted_nident{left_nident + right_nident + query_offset},
          pasted_mismatch{left_mismatch + right_mismatch},
          pasted_gapopen{left_gapopen + right_gapopen},
          pasted_gaps{left_gaps + right_gaps},
          pasted_prefix_end{left_prefix_end},
          pasted_suffix_begin{right_suffix_begin + query_offset
                              + static_cast<int>(left_qseq.length())};

      std::string pasted_qseq, pasted_sseq;
      pasted_qseq = left_qseq.substr(0, left_qseq.length() + query_offset);
      pasted_qseq.append(right_qseq);

      pasted_sseq = left_sseq.substr(0, left_sseq.length() + subject_offset);
      pasted_sseq.append(right_sseq);

      // Swapped subject coordinates indicate minus strand.
      if (!plus_strand) {
        std::swap(left_sstart, left_send);
        std::swap(right_sstart, right_send);
      }

      Alignment left{Alignment::FromStringFields(0,
        {std::to_string(left_qstart), std::to_string(left_qend),
         std::to_string(left_sstart), std::to_string(left_send),
         std::to_string(left_nident), std::to_string(left_mismatch),
         std::to_string(left_gapopen), std::to_string(left_gaps),
         std::to_string(qlen), std::to_string(slen),
         left_qseq, left_sseq},
        scoring_system, paste_parameters)};
      left.UngappedPrefixEnd(left_prefix_end);
      left.UngappedSuffixBegin(left_suffix_begin);
      Alignment right{Alignment::FromStringFields(1,
        {std::to_string(right_qstart), std::to_string(right_qend),
         std::to_string(right_sstart), std::to_string(right_send),
         std::to_string(right_nident), std::to_string(right_mismatch),
         std::to_string(right_gapopen), std::to_string(right_gaps),
         std::to_string(qlen), std::to_string(slen),
         right_qseq, right_sseq},
        scoring_system, paste_parameters)};
      right.UngappedPrefixEnd(right_prefix_end);
      right.UngappedSuffixBegin(right_suffix_begin);

      AlignmentConfiguration config{GetConfiguration(
          query_offset, subject_offset,
          left_qseq.length(), right_qseq.length())};

      Alignment original_right{right};
      left.PasteRight(right, config, scoring_system, paste_parameters);

      THEN("Left becomes the pasted alignment.") {
        CHECK(left.Qstart() == pasted_qstart);
        CHECK(left.Qend() == pasted_qend);
        CHECK(left.Sstart() == pasted_sstart);
        CHECK(left.Send() == pasted_send);
        CHECK(left.PlusStrand() == plus_strand);
        CHECK(left.Nident() == pasted_nident);
        CHECK(left.Mismatch() == pasted_mismatch);
        CHECK(left.Gapopen() == pasted_gapopen);
        CHECK(left.Gaps() == pasted_gaps);
        CHECK(left.Qlen() == qlen);
        CHECK(left.Slen() == slen);
        CHECK(left.Qseq() == pasted_qseq);
        CHECK(left.Sseq() == pasted_sseq);
        CHECK(left.IncludeInOutput() == false);
        CHECK(left.UngappedPrefixEnd() == pasted_prefix_end);
        CHECK(left.UngappedSuffixBegin() == pasted_suffix_begin);
      }

      THEN("Right remains unchanged.") {
        CHECK(right == original_right);
      }
    }

    WHEN("query_offset == subject_offset < 0; -strand") {
      // Specify test case parameters.
      int query_offset{-4}, subject_offset{-4};
      bool plus_strand{false};
      HandleOffset(query_offset, subject_offset, plus_strand,
                   left_sstart, left_send, right_qstart, right_qend,
                   right_sstart, right_send);
      // Specify expected pasted outcome.
      int pasted_qstart{left_qstart}, pasted_qend{right_qend},
          pasted_sstart{right_sstart}, pasted_send{left_send},
          pasted_nident{left_nident + right_nident + query_offset},
          pasted_mismatch{left_mismatch + right_mismatch},
          pasted_gapopen{left_gapopen + right_gapopen},
          pasted_gaps{left_gaps + right_gaps},
          pasted_prefix_end{left_prefix_end},
          pasted_suffix_begin{right_suffix_begin + query_offset
                              + static_cast<int>(left_qseq.length())};

      std::string pasted_qseq, pasted_sseq;
      pasted_qseq = left_qseq.substr(0, left_qseq.length() + query_offset);
      pasted_qseq.append(right_qseq);

      pasted_sseq = left_sseq.substr(0, left_qseq.length() + subject_offset);
      pasted_sseq.append(right_sseq);

      // Swapped subject coordinates indicate minus strand.
      if (!plus_strand) {
        std::swap(left_sstart, left_send);
        std::swap(right_sstart, right_send);
      }

      Alignment left{Alignment::FromStringFields(0,
        {std::to_string(left_qstart), std::to_string(left_qend),
         std::to_string(left_sstart), std::to_string(left_send),
         std::to_string(left_nident), std::to_string(left_mismatch),
         std::to_string(left_gapopen), std::to_string(left_gaps),
         std::to_string(qlen), std::to_string(slen),
         left_qseq, left_sseq},
        scoring_system, paste_parameters)};
      left.UngappedPrefixEnd(left_prefix_end);
      left.UngappedSuffixBegin(left_suffix_begin);
      Alignment right{Alignment::FromStringFields(1,
        {std::to_string(right_qstart), std::to_string(right_qend),
         std::to_string(right_sstart), std::to_string(right_send),
         std::to_string(right_nident), std::to_string(right_mismatch),
         std::to_string(right_gapopen), std::to_string(right_gaps),
         std::to_string(qlen), std::to_string(slen),
         right_qseq, right_sseq},
        scoring_system, paste_parameters)};
      right.UngappedPrefixEnd(right_prefix_end);
      right.UngappedSuffixBegin(right_suffix_begin);

      AlignmentConfiguration config{GetConfiguration(
          query_offset, subject_offset,
          left_qseq.length(), right_qseq.length())};

      Alignment original_right{right};
      left.PasteRight(right, config, scoring_system, paste_parameters);

      THEN("Left becomes the pasted alignment.") {
        CHECK(left.Qstart() == pasted_qstart);
        CHECK(left.Qend() == pasted_qend);
        CHECK(left.Sstart() == pasted_sstart);
        CHECK(left.Send() == pasted_send);
        CHECK(left.PlusStrand() == plus_strand);
        CHECK(left.Nident() == pasted_nident);
        CHECK(left.Mismatch() == pasted_mismatch);
        CHECK(left.Gapopen() == pasted_gapopen);
        CHECK(left.Gaps() == pasted_gaps);
        CHECK(left.Qlen() == qlen);
        CHECK(left.Slen() == slen);
        CHECK(left.Qseq() == pasted_qseq);
        CHECK(left.Sseq() == pasted_sseq);
        CHECK(left.IncludeInOutput() == false);
        CHECK(left.UngappedPrefixEnd() == pasted_prefix_end);
        CHECK(left.UngappedSuffixBegin() == pasted_suffix_begin);
      }

      THEN("Right remains unchanged.") {
        CHECK(right == original_right);
      }
    }

    WHEN("query_offset == 0 > subject_offset; +strand") {
      // Specify test case parameters.
      int query_offset{0}, subject_offset{-4};
      bool plus_strand{true};
      HandleOffset(query_offset, subject_offset, plus_strand,
                   left_sstart, left_send, right_qstart, right_qend,
                   right_sstart, right_send);
      // Specify expected pasted outcome.
      int pasted_qstart{left_qstart}, pasted_qend{right_qend},
          pasted_sstart{left_sstart}, pasted_send{right_send},
          pasted_nident{left_nident + right_nident + subject_offset},
          pasted_mismatch{left_mismatch + right_mismatch},
          pasted_gapopen{left_gapopen + right_gapopen + 1},
          pasted_gaps{left_gaps + right_gaps - subject_offset},
          pasted_prefix_end{left_prefix_end},
          pasted_suffix_begin{right_suffix_begin
                              + static_cast<int>(left_qseq.length())};

      std::string pasted_qseq, pasted_sseq;
      pasted_qseq = left_qseq.substr(0, left_qseq.length() + subject_offset);
      pasted_qseq.append(std::abs(subject_offset), 'N');
      pasted_qseq.append(right_qseq);

      pasted_sseq = left_sseq.substr(0, left_sseq.length() + subject_offset);
      pasted_sseq.append(std::abs(subject_offset), '-');
      pasted_sseq.append(right_sseq);

      // Swapped subject coordinates indicate minus strand.
      if (!plus_strand) {
        std::swap(left_sstart, left_send);
        std::swap(right_sstart, right_send);
      }

      Alignment left{Alignment::FromStringFields(0,
        {std::to_string(left_qstart), std::to_string(left_qend),
         std::to_string(left_sstart), std::to_string(left_send),
         std::to_string(left_nident), std::to_string(left_mismatch),
         std::to_string(left_gapopen), std::to_string(left_gaps),
         std::to_string(qlen), std::to_string(slen),
         left_qseq, left_sseq},
        scoring_system, paste_parameters)};
      left.UngappedPrefixEnd(left_prefix_end);
      left.UngappedSuffixBegin(left_suffix_begin);
      Alignment right{Alignment::FromStringFields(1,
        {std::to_string(right_qstart), std::to_string(right_qend),
         std::to_string(right_sstart), std::to_string(right_send),
         std::to_string(right_nident), std::to_string(right_mismatch),
         std::to_string(right_gapopen), std::to_string(right_gaps),
         std::to_string(qlen), std::to_string(slen),
         right_qseq, right_sseq},
        scoring_system, paste_parameters)};
      right.UngappedPrefixEnd(right_prefix_end);
      right.UngappedSuffixBegin(right_suffix_begin);

      AlignmentConfiguration config{GetConfiguration(
          query_offset, subject_offset,
          left_qseq.length(), right_qseq.length())};

      Alignment original_right{right};
      left.PasteRight(right, config, scoring_system, paste_parameters);

      THEN("Left becomes the pasted alignment.") {
        CHECK(left.Qstart() == pasted_qstart);
        CHECK(left.Qend() == pasted_qend);
        CHECK(left.Sstart() == pasted_sstart);
        CHECK(left.Send() == pasted_send);
        CHECK(left.PlusStrand() == plus_strand);
        CHECK(left.Nident() == pasted_nident);
        CHECK(left.Mismatch() == pasted_mismatch);
        CHECK(left.Gapopen() == pasted_gapopen);
        CHECK(left.Gaps() == pasted_gaps);
        CHECK(left.Qlen() == qlen);
        CHECK(left.Slen() == slen);
        CHECK(left.Qseq() == pasted_qseq);
        CHECK(left.Sseq() == pasted_sseq);
        CHECK(left.IncludeInOutput() == false);
        CHECK(left.UngappedPrefixEnd() == pasted_prefix_end);
        CHECK(left.UngappedSuffixBegin() == pasted_suffix_begin);
      }

      THEN("Right remains unchanged.") {
        CHECK(right == original_right);
      }
    }

    WHEN("query_offset == 0 > subject_offset; -strand") {
      // Specify test case parameters.
      int query_offset{0}, subject_offset{-4};
      bool plus_strand{false};
      HandleOffset(query_offset, subject_offset, plus_strand,
                   left_sstart, left_send, right_qstart, right_qend,
                   right_sstart, right_send);
      // Specify expected pasted outcome.
      int pasted_qstart{left_qstart}, pasted_qend{right_qend},
          pasted_sstart{right_sstart}, pasted_send{left_send},
          pasted_nident{left_nident + right_nident + subject_offset},
          pasted_mismatch{left_mismatch + right_mismatch},
          pasted_gapopen{left_gapopen + right_gapopen + 1},
          pasted_gaps{left_gaps + right_gaps - subject_offset},
          pasted_prefix_end{left_prefix_end},
          pasted_suffix_begin{right_suffix_begin
                              + static_cast<int>(left_qseq.length())};

      std::string pasted_qseq, pasted_sseq;
      pasted_qseq = left_qseq.substr(0, left_qseq.length() + subject_offset);
      pasted_qseq.append(std::abs(subject_offset), 'N');
      pasted_qseq.append(right_qseq);

      pasted_sseq = left_sseq.substr(0, left_qseq.length() + subject_offset);
      pasted_sseq.append(std::abs(subject_offset), '-');
      pasted_sseq.append(right_sseq);

      // Swapped subject coordinates indicate minus strand.
      if (!plus_strand) {
        std::swap(left_sstart, left_send);
        std::swap(right_sstart, right_send);
      }

      Alignment left{Alignment::FromStringFields(0,
        {std::to_string(left_qstart), std::to_string(left_qend),
         std::to_string(left_sstart), std::to_string(left_send),
         std::to_string(left_nident), std::to_string(left_mismatch),
         std::to_string(left_gapopen), std::to_string(left_gaps),
         std::to_string(qlen), std::to_string(slen),
         left_qseq, left_sseq},
        scoring_system, paste_parameters)};
      left.UngappedPrefixEnd(left_prefix_end);
      left.UngappedSuffixBegin(left_suffix_begin);
      Alignment right{Alignment::FromStringFields(1,
        {std::to_string(right_qstart), std::to_string(right_qend),
         std::to_string(right_sstart), std::to_string(right_send),
         std::to_string(right_nident), std::to_string(right_mismatch),
         std::to_string(right_gapopen), std::to_string(right_gaps),
         std::to_string(qlen), std::to_string(slen),
         right_qseq, right_sseq},
        scoring_system, paste_parameters)};
      right.UngappedPrefixEnd(right_prefix_end);
      right.UngappedSuffixBegin(right_suffix_begin);

      AlignmentConfiguration config{GetConfiguration(
          query_offset, subject_offset,
          left_qseq.length(), right_qseq.length())};

      Alignment original_right{right};
      left.PasteRight(right, config, scoring_system, paste_parameters);

      THEN("Left becomes the pasted alignment.") {
        CHECK(left.Qstart() == pasted_qstart);
        CHECK(left.Qend() == pasted_qend);
        CHECK(left.Sstart() == pasted_sstart);
        CHECK(left.Send() == pasted_send);
        CHECK(left.PlusStrand() == plus_strand);
        CHECK(left.Nident() == pasted_nident);
        CHECK(left.Mismatch() == pasted_mismatch);
        CHECK(left.Gapopen() == pasted_gapopen);
        CHECK(left.Gaps() == pasted_gaps);
        CHECK(left.Qlen() == qlen);
        CHECK(left.Slen() == slen);
        CHECK(left.Qseq() == pasted_qseq);
        CHECK(left.Sseq() == pasted_sseq);
        CHECK(left.IncludeInOutput() == false);
        CHECK(left.UngappedPrefixEnd() == pasted_prefix_end);
        CHECK(left.UngappedSuffixBegin() == pasted_suffix_begin);
      }

      THEN("Right remains unchanged.") {
        CHECK(right == original_right);
      }
    }

    WHEN("subject_offset == 0 > query_offset; +strand") {
      // Specify test case parameters.
      int query_offset{-4}, subject_offset{0};
      bool plus_strand{true};
      HandleOffset(query_offset, subject_offset, plus_strand,
                   left_sstart, left_send, right_qstart, right_qend,
                   right_sstart, right_send);
      // Specify expected pasted outcome.
      int pasted_qstart{left_qstart}, pasted_qend{right_qend},
          pasted_sstart{left_sstart}, pasted_send{right_send},
          pasted_nident{left_nident + right_nident + query_offset},
          pasted_mismatch{left_mismatch + right_mismatch},
          pasted_gapopen{left_gapopen + right_gapopen + 1},
          pasted_gaps{left_gaps + right_gaps - query_offset},
          pasted_prefix_end{left_prefix_end},
          pasted_suffix_begin{right_suffix_begin
                              + static_cast<int>(left_qseq.length())};

      std::string pasted_qseq, pasted_sseq;
      pasted_qseq = left_qseq.substr(0, left_qseq.length() + query_offset);
      pasted_qseq.append(std::abs(query_offset), '-');
      pasted_qseq.append(right_qseq);

      pasted_sseq = left_sseq.substr(0, left_sseq.length() + query_offset);
      pasted_sseq.append(std::abs(query_offset), 'N');
      pasted_sseq.append(right_sseq);

      // Swapped subject coordinates indicate minus strand.
      if (!plus_strand) {
        std::swap(left_sstart, left_send);
        std::swap(right_sstart, right_send);
      }

      Alignment left{Alignment::FromStringFields(0,
        {std::to_string(left_qstart), std::to_string(left_qend),
         std::to_string(left_sstart), std::to_string(left_send),
         std::to_string(left_nident), std::to_string(left_mismatch),
         std::to_string(left_gapopen), std::to_string(left_gaps),
         std::to_string(qlen), std::to_string(slen),
         left_qseq, left_sseq},
        scoring_system, paste_parameters)};
      left.UngappedPrefixEnd(left_prefix_end);
      left.UngappedSuffixBegin(left_suffix_begin);
      Alignment right{Alignment::FromStringFields(1,
        {std::to_string(right_qstart), std::to_string(right_qend),
         std::to_string(right_sstart), std::to_string(right_send),
         std::to_string(right_nident), std::to_string(right_mismatch),
         std::to_string(right_gapopen), std::to_string(right_gaps),
         std::to_string(qlen), std::to_string(slen),
         right_qseq, right_sseq},
        scoring_system, paste_parameters)};
      right.UngappedPrefixEnd(right_prefix_end);
      right.UngappedSuffixBegin(right_suffix_begin);

      AlignmentConfiguration config{GetConfiguration(
          query_offset, subject_offset,
          left_qseq.length(), right_qseq.length())};

      Alignment original_right{right};
      left.PasteRight(right, config, scoring_system, paste_parameters);

      THEN("Left becomes the pasted alignment.") {
        CHECK(left.Qstart() == pasted_qstart);
        CHECK(left.Qend() == pasted_qend);
        CHECK(left.Sstart() == pasted_sstart);
        CHECK(left.Send() == pasted_send);
        CHECK(left.PlusStrand() == plus_strand);
        CHECK(left.Nident() == pasted_nident);
        CHECK(left.Mismatch() == pasted_mismatch);
        CHECK(left.Gapopen() == pasted_gapopen);
        CHECK(left.Gaps() == pasted_gaps);
        CHECK(left.Qlen() == qlen);
        CHECK(left.Slen() == slen);
        CHECK(left.Qseq() == pasted_qseq);
        CHECK(left.Sseq() == pasted_sseq);
        CHECK(left.IncludeInOutput() == false);
        CHECK(left.UngappedPrefixEnd() == pasted_prefix_end);
        CHECK(left.UngappedSuffixBegin() == pasted_suffix_begin);
      }

      THEN("Right remains unchanged.") {
        CHECK(right == original_right);
      }
    }

    WHEN("subject_offset == 0 > query_offset; -strand") {
      // Specify test case parameters.
      int query_offset{-4}, subject_offset{0};
      bool plus_strand{false};
      HandleOffset(query_offset, subject_offset, plus_strand,
                   left_sstart, left_send, right_qstart, right_qend,
                   right_sstart, right_send);
      // Specify expected pasted outcome.
      int pasted_qstart{left_qstart}, pasted_qend{right_qend},
          pasted_sstart{right_sstart}, pasted_send{left_send},
          pasted_nident{left_nident + right_nident + query_offset},
          pasted_mismatch{left_mismatch + right_mismatch},
          pasted_gapopen{left_gapopen + right_gapopen + 1},
          pasted_gaps{left_gaps + right_gaps - query_offset},
          pasted_prefix_end{left_prefix_end},
          pasted_suffix_begin{right_suffix_begin
                              + static_cast<int>(left_qseq.length())};

      std::string pasted_qseq, pasted_sseq;
      pasted_qseq = left_qseq.substr(0, left_qseq.length() + query_offset);
      pasted_qseq.append(std::abs(query_offset), '-');
      pasted_qseq.append(right_qseq);

      pasted_sseq = left_sseq.substr(0, left_qseq.length() + query_offset);
      pasted_sseq.append(std::abs(query_offset), 'N');
      pasted_sseq.append(right_sseq);

      // Swapped subject coordinates indicate minus strand.
      if (!plus_strand) {
        std::swap(left_sstart, left_send);
        std::swap(right_sstart, right_send);
      }

      Alignment left{Alignment::FromStringFields(0,
        {std::to_string(left_qstart), std::to_string(left_qend),
         std::to_string(left_sstart), std::to_string(left_send),
         std::to_string(left_nident), std::to_string(left_mismatch),
         std::to_string(left_gapopen), std::to_string(left_gaps),
         std::to_string(qlen), std::to_string(slen),
         left_qseq, left_sseq},
        scoring_system, paste_parameters)};
      left.UngappedPrefixEnd(left_prefix_end);
      left.UngappedSuffixBegin(left_suffix_begin);
      Alignment right{Alignment::FromStringFields(1,
        {std::to_string(right_qstart), std::to_string(right_qend),
         std::to_string(right_sstart), std::to_string(right_send),
         std::to_string(right_nident), std::to_string(right_mismatch),
         std::to_string(right_gapopen), std::to_string(right_gaps),
         std::to_string(qlen), std::to_string(slen),
         right_qseq, right_sseq},
        scoring_system, paste_parameters)};
      right.UngappedPrefixEnd(right_prefix_end);
      right.UngappedSuffixBegin(right_suffix_begin);

      AlignmentConfiguration config{GetConfiguration(
          query_offset, subject_offset,
          left_qseq.length(), right_qseq.length())};

      Alignment original_right{right};
      left.PasteRight(right, config, scoring_system, paste_parameters);

      THEN("Left becomes the pasted alignment.") {
        CHECK(left.Qstart() == pasted_qstart);
        CHECK(left.Qend() == pasted_qend);
        CHECK(left.Sstart() == pasted_sstart);
        CHECK(left.Send() == pasted_send);
        CHECK(left.PlusStrand() == plus_strand);
        CHECK(left.Nident() == pasted_nident);
        CHECK(left.Mismatch() == pasted_mismatch);
        CHECK(left.Gapopen() == pasted_gapopen);
        CHECK(left.Gaps() == pasted_gaps);
        CHECK(left.Qlen() == qlen);
        CHECK(left.Slen() == slen);
        CHECK(left.Qseq() == pasted_qseq);
        CHECK(left.Sseq() == pasted_sseq);
        CHECK(left.IncludeInOutput() == false);
        CHECK(left.UngappedPrefixEnd() == pasted_prefix_end);
        CHECK(left.UngappedSuffixBegin() == pasted_suffix_begin);
      }

      THEN("Right remains unchanged.") {
        CHECK(right == original_right);
      }
    }

    WHEN("subject_offset > query_offset == 0; +strand") {
      // Specify test case parameters.
      int query_offset{0}, subject_offset{10};
      bool plus_strand{true};
      HandleOffset(query_offset, subject_offset, plus_strand,
                   left_sstart, left_send, right_qstart, right_qend,
                   right_sstart, right_send);
      // Specify expected pasted outcome.
      int pasted_qstart{left_qstart}, pasted_qend{right_qend},
          pasted_sstart{left_sstart}, pasted_send{right_send},
          pasted_nident{left_nident + right_nident},
          pasted_mismatch{left_mismatch + right_mismatch},
          pasted_gapopen{left_gapopen + right_gapopen + 1},
          pasted_gaps{left_gaps + right_gaps + subject_offset},
          pasted_prefix_end{left_prefix_end},
          pasted_suffix_begin{right_suffix_begin + subject_offset
                              + static_cast<int>(left_qseq.length())};

      std::string pasted_qseq, pasted_sseq;
      pasted_qseq = left_qseq;
      pasted_qseq.append(subject_offset, '-');
      pasted_qseq.append(right_qseq);

      pasted_sseq = left_sseq;
      pasted_sseq.append(subject_offset, 'N');
      pasted_sseq.append(right_sseq);

      // Swapped subject coordinates indicate minus strand.
      if (!plus_strand) {
        std::swap(left_sstart, left_send);
        std::swap(right_sstart, right_send);
      }

      Alignment left{Alignment::FromStringFields(0,
        {std::to_string(left_qstart), std::to_string(left_qend),
         std::to_string(left_sstart), std::to_string(left_send),
         std::to_string(left_nident), std::to_string(left_mismatch),
         std::to_string(left_gapopen), std::to_string(left_gaps),
         std::to_string(qlen), std::to_string(slen),
         left_qseq, left_sseq},
        scoring_system, paste_parameters)};
      left.UngappedPrefixEnd(left_prefix_end);
      left.UngappedSuffixBegin(left_suffix_begin);
      Alignment right{Alignment::FromStringFields(1,
        {std::to_string(right_qstart), std::to_string(right_qend),
         std::to_string(right_sstart), std::to_string(right_send),
         std::to_string(right_nident), std::to_string(right_mismatch),
         std::to_string(right_gapopen), std::to_string(right_gaps),
         std::to_string(qlen), std::to_string(slen),
         right_qseq, right_sseq},
        scoring_system, paste_parameters)};
      right.UngappedPrefixEnd(right_prefix_end);
      right.UngappedSuffixBegin(right_suffix_begin);

      AlignmentConfiguration config{GetConfiguration(
          query_offset, subject_offset,
          left_qseq.length(), right_qseq.length())};

      Alignment original_right{right};
      left.PasteRight(right, config, scoring_system, paste_parameters);

      THEN("Left becomes the pasted alignment.") {
        CHECK(left.Qstart() == pasted_qstart);
        CHECK(left.Qend() == pasted_qend);
        CHECK(left.Sstart() == pasted_sstart);
        CHECK(left.Send() == pasted_send);
        CHECK(left.PlusStrand() == plus_strand);
        CHECK(left.Nident() == pasted_nident);
        CHECK(left.Mismatch() == pasted_mismatch);
        CHECK(left.Gapopen() == pasted_gapopen);
        CHECK(left.Gaps() == pasted_gaps);
        CHECK(left.Qlen() == qlen);
        CHECK(left.Slen() == slen);
        CHECK(left.Qseq() == pasted_qseq);
        CHECK(left.Sseq() == pasted_sseq);
        CHECK(left.IncludeInOutput() == false);
        CHECK(left.UngappedPrefixEnd() == pasted_prefix_end);
        CHECK(left.UngappedSuffixBegin() == pasted_suffix_begin);
      }

      THEN("Right remains unchanged.") {
        CHECK(right == original_right);
      }
    }

    WHEN("subject_offset > query_offset == 0; -strand") {
      // Specify test case parameters.
      int query_offset{0}, subject_offset{10};
      bool plus_strand{false};
      HandleOffset(query_offset, subject_offset, plus_strand,
                   left_sstart, left_send, right_qstart, right_qend,
                   right_sstart, right_send);
      // Specify expected pasted outcome.
      int pasted_qstart{left_qstart}, pasted_qend{right_qend},
          pasted_sstart{right_sstart}, pasted_send{left_send},
          pasted_nident{left_nident + right_nident},
          pasted_mismatch{left_mismatch + right_mismatch},
          pasted_gapopen{left_gapopen + right_gapopen + 1},
          pasted_gaps{left_gaps + right_gaps + subject_offset},
          pasted_prefix_end{left_prefix_end},
          pasted_suffix_begin{right_suffix_begin + subject_offset
                              + static_cast<int>(left_qseq.length())};

      std::string pasted_qseq, pasted_sseq;
      pasted_qseq = left_qseq;
      pasted_qseq.append(subject_offset, '-');
      pasted_qseq.append(right_qseq);

      pasted_sseq = left_sseq;
      pasted_sseq.append(subject_offset, 'N');
      pasted_sseq.append(right_sseq);

      // Swapped subject coordinates indicate minus strand.
      if (!plus_strand) {
        std::swap(left_sstart, left_send);
        std::swap(right_sstart, right_send);
      }

      Alignment left{Alignment::FromStringFields(0,
        {std::to_string(left_qstart), std::to_string(left_qend),
         std::to_string(left_sstart), std::to_string(left_send),
         std::to_string(left_nident), std::to_string(left_mismatch),
         std::to_string(left_gapopen), std::to_string(left_gaps),
         std::to_string(qlen), std::to_string(slen),
         left_qseq, left_sseq},
        scoring_system, paste_parameters)};
      left.UngappedPrefixEnd(left_prefix_end);
      left.UngappedSuffixBegin(left_suffix_begin);
      Alignment right{Alignment::FromStringFields(1,
        {std::to_string(right_qstart), std::to_string(right_qend),
         std::to_string(right_sstart), std::to_string(right_send),
         std::to_string(right_nident), std::to_string(right_mismatch),
         std::to_string(right_gapopen), std::to_string(right_gaps),
         std::to_string(qlen), std::to_string(slen),
         right_qseq, right_sseq},
        scoring_system, paste_parameters)};
      right.UngappedPrefixEnd(right_prefix_end);
      right.UngappedSuffixBegin(right_suffix_begin);

      AlignmentConfiguration config{GetConfiguration(
          query_offset, subject_offset,
          left_qseq.length(), right_qseq.length())};

      Alignment original_right{right};
      left.PasteRight(right, config, scoring_system, paste_parameters);

      THEN("Left becomes the pasted alignment.") {
        CHECK(left.Qstart() == pasted_qstart);
        CHECK(left.Qend() == pasted_qend);
        CHECK(left.Sstart() == pasted_sstart);
        CHECK(left.Send() == pasted_send);
        CHECK(left.PlusStrand() == plus_strand);
        CHECK(left.Nident() == pasted_nident);
        CHECK(left.Mismatch() == pasted_mismatch);
        CHECK(left.Gapopen() == pasted_gapopen);
        CHECK(left.Gaps() == pasted_gaps);
        CHECK(left.Qlen() == qlen);
        CHECK(left.Slen() == slen);
        CHECK(left.Qseq() == pasted_qseq);
        CHECK(left.Sseq() == pasted_sseq);
        CHECK(left.IncludeInOutput() == false);
        CHECK(left.UngappedPrefixEnd() == pasted_prefix_end);
        CHECK(left.UngappedSuffixBegin() == pasted_suffix_begin);
      }

      THEN("Right remains unchanged.") {
        CHECK(right == original_right);
      }
    }

    WHEN("query_offset > subject_offset == 0; +strand") {
      // Specify test case parameters.
      int query_offset{10}, subject_offset{0};
      bool plus_strand{true};
      HandleOffset(query_offset, subject_offset, plus_strand,
                   left_sstart, left_send, right_qstart, right_qend,
                   right_sstart, right_send);
      // Specify expected pasted outcome.
      int pasted_qstart{left_qstart}, pasted_qend{right_qend},
          pasted_sstart{left_sstart}, pasted_send{right_send},
          pasted_nident{left_nident + right_nident},
          pasted_mismatch{left_mismatch + right_mismatch},
          pasted_gapopen{left_gapopen + right_gapopen + 1},
          pasted_gaps{left_gaps + right_gaps + query_offset},
          pasted_prefix_end{left_prefix_end},
          pasted_suffix_begin{right_suffix_begin + query_offset
                              + static_cast<int>(left_qseq.length())};

      std::string pasted_qseq, pasted_sseq;
      pasted_qseq = left_qseq;
      pasted_qseq.append(query_offset, 'N');
      pasted_qseq.append(right_qseq);

      pasted_sseq = left_sseq;
      pasted_sseq.append(query_offset, '-');
      pasted_sseq.append(right_sseq);

      // Swapped subject coordinates indicate minus strand.
      if (!plus_strand) {
        std::swap(left_sstart, left_send);
        std::swap(right_sstart, right_send);
      }

      Alignment left{Alignment::FromStringFields(0,
        {std::to_string(left_qstart), std::to_string(left_qend),
         std::to_string(left_sstart), std::to_string(left_send),
         std::to_string(left_nident), std::to_string(left_mismatch),
         std::to_string(left_gapopen), std::to_string(left_gaps),
         std::to_string(qlen), std::to_string(slen),
         left_qseq, left_sseq},
        scoring_system, paste_parameters)};
      left.UngappedPrefixEnd(left_prefix_end);
      left.UngappedSuffixBegin(left_suffix_begin);
      Alignment right{Alignment::FromStringFields(1,
        {std::to_string(right_qstart), std::to_string(right_qend),
         std::to_string(right_sstart), std::to_string(right_send),
         std::to_string(right_nident), std::to_string(right_mismatch),
         std::to_string(right_gapopen), std::to_string(right_gaps),
         std::to_string(qlen), std::to_string(slen),
         right_qseq, right_sseq},
        scoring_system, paste_parameters)};
      right.UngappedPrefixEnd(right_prefix_end);
      right.UngappedSuffixBegin(right_suffix_begin);

      AlignmentConfiguration config{GetConfiguration(
          query_offset, subject_offset,
          left_qseq.length(), right_qseq.length())};

      Alignment original_right{right};
      left.PasteRight(right, config, scoring_system, paste_parameters);

      THEN("Left becomes the pasted alignment.") {
        CHECK(left.Qstart() == pasted_qstart);
        CHECK(left.Qend() == pasted_qend);
        CHECK(left.Sstart() == pasted_sstart);
        CHECK(left.Send() == pasted_send);
        CHECK(left.PlusStrand() == plus_strand);
        CHECK(left.Nident() == pasted_nident);
        CHECK(left.Mismatch() == pasted_mismatch);
        CHECK(left.Gapopen() == pasted_gapopen);
        CHECK(left.Gaps() == pasted_gaps);
        CHECK(left.Qlen() == qlen);
        CHECK(left.Slen() == slen);
        CHECK(left.Qseq() == pasted_qseq);
        CHECK(left.Sseq() == pasted_sseq);
        CHECK(left.IncludeInOutput() == false);
        CHECK(left.UngappedPrefixEnd() == pasted_prefix_end);
        CHECK(left.UngappedSuffixBegin() == pasted_suffix_begin);
      }

      THEN("Right remains unchanged.") {
        CHECK(right == original_right);
      }
    }

    WHEN("query_offset > subject_offset == 0; -strand") {
      // Specify test case parameters.
      int query_offset{10}, subject_offset{0};
      bool plus_strand{false};
      HandleOffset(query_offset, subject_offset, plus_strand,
                   left_sstart, left_send, right_qstart, right_qend,
                   right_sstart, right_send);
      // Specify expected pasted outcome.
      int pasted_qstart{left_qstart}, pasted_qend{right_qend},
          pasted_sstart{right_sstart}, pasted_send{left_send},
          pasted_nident{left_nident + right_nident},
          pasted_mismatch{left_mismatch + right_mismatch},
          pasted_gapopen{left_gapopen + right_gapopen + 1},
          pasted_gaps{left_gaps + right_gaps + query_offset},
          pasted_prefix_end{left_prefix_end},
          pasted_suffix_begin{right_suffix_begin + query_offset
                              + static_cast<int>(left_qseq.length())};

      std::string pasted_qseq, pasted_sseq;
      pasted_qseq = left_qseq;
      pasted_qseq.append(query_offset, 'N');
      pasted_qseq.append(right_qseq);

      pasted_sseq = left_sseq;
      pasted_sseq.append(query_offset, '-');
      pasted_sseq.append(right_sseq);

      // Swapped subject coordinates indicate minus strand.
      if (!plus_strand) {
        std::swap(left_sstart, left_send);
        std::swap(right_sstart, right_send);
      }

      Alignment left{Alignment::FromStringFields(0,
        {std::to_string(left_qstart), std::to_string(left_qend),
         std::to_string(left_sstart), std::to_string(left_send),
         std::to_string(left_nident), std::to_string(left_mismatch),
         std::to_string(left_gapopen), std::to_string(left_gaps),
         std::to_string(qlen), std::to_string(slen),
         left_qseq, left_sseq},
        scoring_system, paste_parameters)};
      left.UngappedPrefixEnd(left_prefix_end);
      left.UngappedSuffixBegin(left_suffix_begin);
      Alignment right{Alignment::FromStringFields(1,
        {std::to_string(right_qstart), std::to_string(right_qend),
         std::to_string(right_sstart), std::to_string(right_send),
         std::to_string(right_nident), std::to_string(right_mismatch),
         std::to_string(right_gapopen), std::to_string(right_gaps),
         std::to_string(qlen), std::to_string(slen),
         right_qseq, right_sseq},
        scoring_system, paste_parameters)};
      right.UngappedPrefixEnd(right_prefix_end);
      right.UngappedSuffixBegin(right_suffix_begin);

      AlignmentConfiguration config{GetConfiguration(
          query_offset, subject_offset,
          left_qseq.length(), right_qseq.length())};

      Alignment original_right{right};
      left.PasteRight(right, config, scoring_system, paste_parameters);

      THEN("Left becomes the pasted alignment.") {
        CHECK(left.Qstart() == pasted_qstart);
        CHECK(left.Qend() == pasted_qend);
        CHECK(left.Sstart() == pasted_sstart);
        CHECK(left.Send() == pasted_send);
        CHECK(left.PlusStrand() == plus_strand);
        CHECK(left.Nident() == pasted_nident);
        CHECK(left.Mismatch() == pasted_mismatch);
        CHECK(left.Gapopen() == pasted_gapopen);
        CHECK(left.Gaps() == pasted_gaps);
        CHECK(left.Qlen() == qlen);
        CHECK(left.Slen() == slen);
        CHECK(left.Qseq() == pasted_qseq);
        CHECK(left.Sseq() == pasted_sseq);
        CHECK(left.IncludeInOutput() == false);
        CHECK(left.UngappedPrefixEnd() == pasted_prefix_end);
        CHECK(left.UngappedSuffixBegin() == pasted_suffix_begin);
      }

      THEN("Right remains unchanged.") {
        CHECK(right == original_right);
      }
    }

    WHEN("query_offset > subject_offset > 0; +strand") {
      // Specify test case parameters.
      int query_offset{10}, subject_offset{4};
      bool plus_strand{true};
      HandleOffset(query_offset, subject_offset, plus_strand,
                   left_sstart, left_send, right_qstart, right_qend,
                   right_sstart, right_send);
      // Specify expected pasted outcome.
      int pasted_qstart{left_qstart}, pasted_qend{right_qend},
          pasted_sstart{left_sstart}, pasted_send{right_send},
          pasted_nident{left_nident + right_nident},
          pasted_mismatch{left_mismatch + right_mismatch + subject_offset},
          pasted_gapopen{left_gapopen + right_gapopen + 1},
          pasted_gaps{left_gaps + right_gaps + query_offset - subject_offset},
          pasted_prefix_end{left_prefix_end},
          pasted_suffix_begin{right_suffix_begin + query_offset
                              + static_cast<int>(left_qseq.length())};

      std::string pasted_qseq, pasted_sseq;
      pasted_qseq = left_qseq;
      pasted_qseq.append(query_offset, 'N');
      pasted_qseq.append(right_qseq);

      pasted_sseq = left_sseq;
      pasted_sseq.append(query_offset - subject_offset, '-');
      pasted_sseq.append(subject_offset, 'N');
      pasted_sseq.append(right_sseq);

      // Swapped subject coordinates indicate minus strand.
      if (!plus_strand) {
        std::swap(left_sstart, left_send);
        std::swap(right_sstart, right_send);
      }

      Alignment left{Alignment::FromStringFields(0,
        {std::to_string(left_qstart), std::to_string(left_qend),
         std::to_string(left_sstart), std::to_string(left_send),
         std::to_string(left_nident), std::to_string(left_mismatch),
         std::to_string(left_gapopen), std::to_string(left_gaps),
         std::to_string(qlen), std::to_string(slen),
         left_qseq, left_sseq},
        scoring_system, paste_parameters)};
      left.UngappedPrefixEnd(left_prefix_end);
      left.UngappedSuffixBegin(left_suffix_begin);
      Alignment right{Alignment::FromStringFields(1,
        {std::to_string(right_qstart), std::to_string(right_qend),
         std::to_string(right_sstart), std::to_string(right_send),
         std::to_string(right_nident), std::to_string(right_mismatch),
         std::to_string(right_gapopen), std::to_string(right_gaps),
         std::to_string(qlen), std::to_string(slen),
         right_qseq, right_sseq},
        scoring_system, paste_parameters)};
      right.UngappedPrefixEnd(right_prefix_end);
      right.UngappedSuffixBegin(right_suffix_begin);

      AlignmentConfiguration config{GetConfiguration(
          query_offset, subject_offset,
          left_qseq.length(), right_qseq.length())};

      Alignment original_right{right};
      left.PasteRight(right, config, scoring_system, paste_parameters);

      THEN("Left becomes the pasted alignment.") {
        CHECK(left.Qstart() == pasted_qstart);
        CHECK(left.Qend() == pasted_qend);
        CHECK(left.Sstart() == pasted_sstart);
        CHECK(left.Send() == pasted_send);
        CHECK(left.PlusStrand() == plus_strand);
        CHECK(left.Nident() == pasted_nident);
        CHECK(left.Mismatch() == pasted_mismatch);
        CHECK(left.Gapopen() == pasted_gapopen);
        CHECK(left.Gaps() == pasted_gaps);
        CHECK(left.Qlen() == qlen);
        CHECK(left.Slen() == slen);
        CHECK(left.Qseq() == pasted_qseq);
        CHECK(left.Sseq() == pasted_sseq);
        CHECK(left.IncludeInOutput() == false);
        CHECK(left.UngappedPrefixEnd() == pasted_prefix_end);
        CHECK(left.UngappedSuffixBegin() == pasted_suffix_begin);
      }

      THEN("Right remains unchanged.") {
        CHECK(right == original_right);
      }
    }

    WHEN("query_offset > subject_offset > 0; -strand") {
      // Specify test case parameters.
      int query_offset{10}, subject_offset{4};
      bool plus_strand{false};
      HandleOffset(query_offset, subject_offset, plus_strand,
                   left_sstart, left_send, right_qstart, right_qend,
                   right_sstart, right_send);
      // Specify expected pasted outcome.
      int pasted_qstart{left_qstart}, pasted_qend{right_qend},
          pasted_sstart{right_sstart}, pasted_send{left_send},
          pasted_nident{left_nident + right_nident},
          pasted_mismatch{left_mismatch + right_mismatch + subject_offset},
          pasted_gapopen{left_gapopen + right_gapopen + 1},
          pasted_gaps{left_gaps + right_gaps + query_offset - subject_offset},
          pasted_prefix_end{left_prefix_end},
          pasted_suffix_begin{right_suffix_begin + query_offset
                              + static_cast<int>(left_qseq.length())};

      std::string pasted_qseq, pasted_sseq;
      pasted_qseq = left_qseq;
      pasted_qseq.append(query_offset, 'N');
      pasted_qseq.append(right_qseq);

      pasted_sseq = left_sseq;
      pasted_sseq.append(query_offset - subject_offset, '-');
      pasted_sseq.append(subject_offset, 'N');
      pasted_sseq.append(right_sseq);

      // Swapped subject coordinates indicate minus strand.
      if (!plus_strand) {
        std::swap(left_sstart, left_send);
        std::swap(right_sstart, right_send);
      }

      Alignment left{Alignment::FromStringFields(0,
        {std::to_string(left_qstart), std::to_string(left_qend),
         std::to_string(left_sstart), std::to_string(left_send),
         std::to_string(left_nident), std::to_string(left_mismatch),
         std::to_string(left_gapopen), std::to_string(left_gaps),
         std::to_string(qlen), std::to_string(slen),
         left_qseq, left_sseq},
        scoring_system, paste_parameters)};
      left.UngappedPrefixEnd(left_prefix_end);
      left.UngappedSuffixBegin(left_suffix_begin);
      Alignment right{Alignment::FromStringFields(1,
        {std::to_string(right_qstart), std::to_string(right_qend),
         std::to_string(right_sstart), std::to_string(right_send),
         std::to_string(right_nident), std::to_string(right_mismatch),
         std::to_string(right_gapopen), std::to_string(right_gaps),
         std::to_string(qlen), std::to_string(slen),
         right_qseq, right_sseq},
        scoring_system, paste_parameters)};
      right.UngappedPrefixEnd(right_prefix_end);
      right.UngappedSuffixBegin(right_suffix_begin);

      AlignmentConfiguration config{GetConfiguration(
          query_offset, subject_offset,
          left_qseq.length(), right_qseq.length())};

      Alignment original_right{right};
      left.PasteRight(right, config, scoring_system, paste_parameters);

      THEN("Left becomes the pasted alignment.") {
        CHECK(left.Qstart() == pasted_qstart);
        CHECK(left.Qend() == pasted_qend);
        CHECK(left.Sstart() == pasted_sstart);
        CHECK(left.Send() == pasted_send);
        CHECK(left.PlusStrand() == plus_strand);
        CHECK(left.Nident() == pasted_nident);
        CHECK(left.Mismatch() == pasted_mismatch);
        CHECK(left.Gapopen() == pasted_gapopen);
        CHECK(left.Gaps() == pasted_gaps);
        CHECK(left.Qlen() == qlen);
        CHECK(left.Slen() == slen);
        CHECK(left.Qseq() == pasted_qseq);
        CHECK(left.Sseq() == pasted_sseq);
        CHECK(left.IncludeInOutput() == false);
        CHECK(left.UngappedPrefixEnd() == pasted_prefix_end);
        CHECK(left.UngappedSuffixBegin() == pasted_suffix_begin);
      }

      THEN("Right remains unchanged.") {
        CHECK(right == original_right);
      }
    }

    WHEN("subject_offset > query_offset > 0; +strand") {
      // Specify test case parameters.
      int query_offset{4}, subject_offset{10};
      bool plus_strand{true};
      HandleOffset(query_offset, subject_offset, plus_strand,
                   left_sstart, left_send, right_qstart, right_qend,
                   right_sstart, right_send);
      // Specify expected pasted outcome.
      int pasted_qstart{left_qstart}, pasted_qend{right_qend},
          pasted_sstart{left_sstart}, pasted_send{right_send},
          pasted_nident{left_nident + right_nident},
          pasted_mismatch{left_mismatch + right_mismatch + query_offset},
          pasted_gapopen{left_gapopen + right_gapopen + 1},
          pasted_gaps{left_gaps + right_gaps + subject_offset - query_offset},
          pasted_prefix_end{left_prefix_end},
          pasted_suffix_begin{right_suffix_begin + subject_offset
                              + static_cast<int>(left_qseq.length())};

      std::string pasted_qseq, pasted_sseq;
      pasted_qseq = left_qseq;
      pasted_qseq.append(subject_offset - query_offset, '-');
      pasted_qseq.append(query_offset, 'N');
      pasted_qseq.append(right_qseq);

      pasted_sseq = left_sseq;
      pasted_sseq.append(subject_offset, 'N');
      pasted_sseq.append(right_sseq);

      // Swapped subject coordinates indicate minus strand.
      if (!plus_strand) {
        std::swap(left_sstart, left_send);
        std::swap(right_sstart, right_send);
      }

      Alignment left{Alignment::FromStringFields(0,
        {std::to_string(left_qstart), std::to_string(left_qend),
         std::to_string(left_sstart), std::to_string(left_send),
         std::to_string(left_nident), std::to_string(left_mismatch),
         std::to_string(left_gapopen), std::to_string(left_gaps),
         std::to_string(qlen), std::to_string(slen),
         left_qseq, left_sseq},
        scoring_system, paste_parameters)};
      left.UngappedPrefixEnd(left_prefix_end);
      left.UngappedSuffixBegin(left_suffix_begin);
      Alignment right{Alignment::FromStringFields(1,
        {std::to_string(right_qstart), std::to_string(right_qend),
         std::to_string(right_sstart), std::to_string(right_send),
         std::to_string(right_nident), std::to_string(right_mismatch),
         std::to_string(right_gapopen), std::to_string(right_gaps),
         std::to_string(qlen), std::to_string(slen),
         right_qseq, right_sseq},
        scoring_system, paste_parameters)};
      right.UngappedPrefixEnd(right_prefix_end);
      right.UngappedSuffixBegin(right_suffix_begin);

      AlignmentConfiguration config{GetConfiguration(
          query_offset, subject_offset,
          left_qseq.length(), right_qseq.length())};

      Alignment original_right{right};
      left.PasteRight(right, config, scoring_system, paste_parameters);

      THEN("Left becomes the pasted alignment.") {
        CHECK(left.Qstart() == pasted_qstart);
        CHECK(left.Qend() == pasted_qend);
        CHECK(left.Sstart() == pasted_sstart);
        CHECK(left.Send() == pasted_send);
        CHECK(left.PlusStrand() == plus_strand);
        CHECK(left.Nident() == pasted_nident);
        CHECK(left.Mismatch() == pasted_mismatch);
        CHECK(left.Gapopen() == pasted_gapopen);
        CHECK(left.Gaps() == pasted_gaps);
        CHECK(left.Qlen() == qlen);
        CHECK(left.Slen() == slen);
        CHECK(left.Qseq() == pasted_qseq);
        CHECK(left.Sseq() == pasted_sseq);
        CHECK(left.IncludeInOutput() == false);
        CHECK(left.UngappedPrefixEnd() == pasted_prefix_end);
        CHECK(left.UngappedSuffixBegin() == pasted_suffix_begin);
      }

      THEN("Right remains unchanged.") {
        CHECK(right == original_right);
      }
    }

    WHEN("subject_offset > query_offset > 0; -strand") {
      // Specify test case parameters.
      int query_offset{4}, subject_offset{10};
      bool plus_strand{false};
      HandleOffset(query_offset, subject_offset, plus_strand,
                   left_sstart, left_send, right_qstart, right_qend,
                   right_sstart, right_send);
      // Specify expected pasted outcome.
      int pasted_qstart{left_qstart}, pasted_qend{right_qend},
          pasted_sstart{right_sstart}, pasted_send{left_send},
          pasted_nident{left_nident + right_nident},
          pasted_mismatch{left_mismatch + right_mismatch + query_offset},
          pasted_gapopen{left_gapopen + right_gapopen + 1},
          pasted_gaps{left_gaps + right_gaps + subject_offset - query_offset},
          pasted_prefix_end{left_prefix_end},
          pasted_suffix_begin{right_suffix_begin + subject_offset
                              + static_cast<int>(left_qseq.length())};

      std::string pasted_qseq, pasted_sseq;
      pasted_qseq = left_qseq;
      pasted_qseq.append(subject_offset - query_offset, '-');
      pasted_qseq.append(query_offset, 'N');
      pasted_qseq.append(right_qseq);

      pasted_sseq = left_sseq;
      pasted_sseq.append(subject_offset, 'N');
      pasted_sseq.append(right_sseq);

      // Swapped subject coordinates indicate minus strand.
      if (!plus_strand) {
        std::swap(left_sstart, left_send);
        std::swap(right_sstart, right_send);
      }

      Alignment left{Alignment::FromStringFields(0,
        {std::to_string(left_qstart), std::to_string(left_qend),
         std::to_string(left_sstart), std::to_string(left_send),
         std::to_string(left_nident), std::to_string(left_mismatch),
         std::to_string(left_gapopen), std::to_string(left_gaps),
         std::to_string(qlen), std::to_string(slen),
         left_qseq, left_sseq},
        scoring_system, paste_parameters)};
      left.UngappedPrefixEnd(left_prefix_end);
      left.UngappedSuffixBegin(left_suffix_begin);
      Alignment right{Alignment::FromStringFields(1,
        {std::to_string(right_qstart), std::to_string(right_qend),
         std::to_string(right_sstart), std::to_string(right_send),
         std::to_string(right_nident), std::to_string(right_mismatch),
         std::to_string(right_gapopen), std::to_string(right_gaps),
         std::to_string(qlen), std::to_string(slen),
         right_qseq, right_sseq},
        scoring_system, paste_parameters)};
      right.UngappedPrefixEnd(right_prefix_end);
      right.UngappedSuffixBegin(right_suffix_begin);

      AlignmentConfiguration config{GetConfiguration(
          query_offset, subject_offset,
          left_qseq.length(), right_qseq.length())};

      Alignment original_right{right};
      left.PasteRight(right, config, scoring_system, paste_parameters);

      THEN("Left becomes the pasted alignment.") {
        CHECK(left.Qstart() == pasted_qstart);
        CHECK(left.Qend() == pasted_qend);
        CHECK(left.Sstart() == pasted_sstart);
        CHECK(left.Send() == pasted_send);
        CHECK(left.PlusStrand() == plus_strand);
        CHECK(left.Nident() == pasted_nident);
        CHECK(left.Mismatch() == pasted_mismatch);
        CHECK(left.Gapopen() == pasted_gapopen);
        CHECK(left.Gaps() == pasted_gaps);
        CHECK(left.Qlen() == qlen);
        CHECK(left.Slen() == slen);
        CHECK(left.Qseq() == pasted_qseq);
        CHECK(left.Sseq() == pasted_sseq);
        CHECK(left.IncludeInOutput() == false);
        CHECK(left.UngappedPrefixEnd() == pasted_prefix_end);
        CHECK(left.UngappedSuffixBegin() == pasted_suffix_begin);
      }

      THEN("Right remains unchanged.") {
        CHECK(right == original_right);
      }
    }

    WHEN("0 > subject_offset > query_offset; +strand") {
      // Specify test case parameters.
      int query_offset{-5}, subject_offset{-2};
      bool plus_strand{true};
      HandleOffset(query_offset, subject_offset, plus_strand,
                   left_sstart, left_send, right_qstart, right_qend,
                   right_sstart, right_send);
      // Specify expected pasted outcome.
      int pasted_qstart{left_qstart}, pasted_qend{right_qend},
          pasted_sstart{left_sstart}, pasted_send{right_send},
          pasted_nident{left_nident + right_nident + query_offset},
          pasted_mismatch{left_mismatch + right_mismatch},
          pasted_gapopen{left_gapopen + right_gapopen + 1},
          pasted_gaps{left_gaps + right_gaps + subject_offset - query_offset},
          pasted_prefix_end{left_prefix_end},
          pasted_suffix_begin{right_suffix_begin + subject_offset
                              + static_cast<int>(left_qseq.length())};

      std::string pasted_qseq, pasted_sseq;
      pasted_qseq = left_qseq.substr(0, left_qseq.length() + query_offset);
      pasted_qseq.append(subject_offset - query_offset, '-');
      pasted_qseq.append(right_qseq);

      pasted_sseq = left_sseq.substr(0, left_sseq.length() + query_offset);
      pasted_sseq.append(subject_offset - query_offset, 'N');
      pasted_sseq.append(right_sseq);

      // Swapped subject coordinates indicate minus strand.
      if (!plus_strand) {
        std::swap(left_sstart, left_send);
        std::swap(right_sstart, right_send);
      }

      Alignment left{Alignment::FromStringFields(0,
        {std::to_string(left_qstart), std::to_string(left_qend),
         std::to_string(left_sstart), std::to_string(left_send),
         std::to_string(left_nident), std::to_string(left_mismatch),
         std::to_string(left_gapopen), std::to_string(left_gaps),
         std::to_string(qlen), std::to_string(slen),
         left_qseq, left_sseq},
        scoring_system, paste_parameters)};
      left.UngappedPrefixEnd(left_prefix_end);
      left.UngappedSuffixBegin(left_suffix_begin);
      Alignment right{Alignment::FromStringFields(1,
        {std::to_string(right_qstart), std::to_string(right_qend),
         std::to_string(right_sstart), std::to_string(right_send),
         std::to_string(right_nident), std::to_string(right_mismatch),
         std::to_string(right_gapopen), std::to_string(right_gaps),
         std::to_string(qlen), std::to_string(slen),
         right_qseq, right_sseq},
        scoring_system, paste_parameters)};
      right.UngappedPrefixEnd(right_prefix_end);
      right.UngappedSuffixBegin(right_suffix_begin);

      AlignmentConfiguration config{GetConfiguration(
          query_offset, subject_offset,
          left_qseq.length(), right_qseq.length())};

      Alignment original_right{right};
      left.PasteRight(right, config, scoring_system, paste_parameters);

      THEN("Left becomes the pasted alignment.") {
        CHECK(left.Qstart() == pasted_qstart);
        CHECK(left.Qend() == pasted_qend);
        CHECK(left.Sstart() == pasted_sstart);
        CHECK(left.Send() == pasted_send);
        CHECK(left.PlusStrand() == plus_strand);
        CHECK(left.Nident() == pasted_nident);
        CHECK(left.Mismatch() == pasted_mismatch);
        CHECK(left.Gapopen() == pasted_gapopen);
        CHECK(left.Gaps() == pasted_gaps);
        CHECK(left.Qlen() == qlen);
        CHECK(left.Slen() == slen);
        CHECK(left.Qseq() == pasted_qseq);
        CHECK(left.Sseq() == pasted_sseq);
        CHECK(left.IncludeInOutput() == false);
        CHECK(left.UngappedPrefixEnd() == pasted_prefix_end);
        CHECK(left.UngappedSuffixBegin() == pasted_suffix_begin);
      }

      THEN("Right remains unchanged.") {
        CHECK(right == original_right);
      }
    }

    WHEN("0 > subject_offset > query_offset; -strand") {
      // Specify test case parameters.
      int query_offset{-5}, subject_offset{-2};
      bool plus_strand{false};
      HandleOffset(query_offset, subject_offset, plus_strand,
                   left_sstart, left_send, right_qstart, right_qend,
                   right_sstart, right_send);
      // Specify expected pasted outcome.
      int pasted_qstart{left_qstart}, pasted_qend{right_qend},
          pasted_sstart{right_sstart}, pasted_send{left_send},
          pasted_nident{left_nident + right_nident + query_offset},
          pasted_mismatch{left_mismatch + right_mismatch},
          pasted_gapopen{left_gapopen + right_gapopen + 1},
          pasted_gaps{left_gaps + right_gaps + subject_offset - query_offset},
          pasted_prefix_end{left_prefix_end},
          pasted_suffix_begin{right_suffix_begin + subject_offset
                              + static_cast<int>(left_qseq.length())};

      std::string pasted_qseq, pasted_sseq;
      pasted_qseq = left_qseq.substr(0, left_qseq.length() + query_offset);
      pasted_qseq.append(subject_offset - query_offset, '-');
      pasted_qseq.append(right_qseq);

      pasted_sseq = left_sseq.substr(0, left_qseq.length() + query_offset);
      pasted_sseq.append(subject_offset - query_offset, 'N');
      pasted_sseq.append(right_sseq);

      // Swapped subject coordinates indicate minus strand.
      if (!plus_strand) {
        std::swap(left_sstart, left_send);
        std::swap(right_sstart, right_send);
      }

      Alignment left{Alignment::FromStringFields(0,
        {std::to_string(left_qstart), std::to_string(left_qend),
         std::to_string(left_sstart), std::to_string(left_send),
         std::to_string(left_nident), std::to_string(left_mismatch),
         std::to_string(left_gapopen), std::to_string(left_gaps),
         std::to_string(qlen), std::to_string(slen),
         left_qseq, left_sseq},
        scoring_system, paste_parameters)};
      left.UngappedPrefixEnd(left_prefix_end);
      left.UngappedSuffixBegin(left_suffix_begin);
      Alignment right{Alignment::FromStringFields(1,
        {std::to_string(right_qstart), std::to_string(right_qend),
         std::to_string(right_sstart), std::to_string(right_send),
         std::to_string(right_nident), std::to_string(right_mismatch),
         std::to_string(right_gapopen), std::to_string(right_gaps),
         std::to_string(qlen), std::to_string(slen),
         right_qseq, right_sseq},
        scoring_system, paste_parameters)};
      right.UngappedPrefixEnd(right_prefix_end);
      right.UngappedSuffixBegin(right_suffix_begin);

      AlignmentConfiguration config{GetConfiguration(
          query_offset, subject_offset,
          left_qseq.length(), right_qseq.length())};

      Alignment original_right{right};
      left.PasteRight(right, config, scoring_system, paste_parameters);

      THEN("Left becomes the pasted alignment.") {
        CHECK(left.Qstart() == pasted_qstart);
        CHECK(left.Qend() == pasted_qend);
        CHECK(left.Sstart() == pasted_sstart);
        CHECK(left.Send() == pasted_send);
        CHECK(left.PlusStrand() == plus_strand);
        CHECK(left.Nident() == pasted_nident);
        CHECK(left.Mismatch() == pasted_mismatch);
        CHECK(left.Gapopen() == pasted_gapopen);
        CHECK(left.Gaps() == pasted_gaps);
        CHECK(left.Qlen() == qlen);
        CHECK(left.Slen() == slen);
        CHECK(left.Qseq() == pasted_qseq);
        CHECK(left.Sseq() == pasted_sseq);
        CHECK(left.IncludeInOutput() == false);
        CHECK(left.UngappedPrefixEnd() == pasted_prefix_end);
        CHECK(left.UngappedSuffixBegin() == pasted_suffix_begin);
      }

      THEN("Right remains unchanged.") {
        CHECK(right == original_right);
      }
    }

    WHEN("0 > query_offset > subject_offset; +strand") {
      // Specify test case parameters.
      int query_offset{-2}, subject_offset{-5};
      bool plus_strand{true};
      HandleOffset(query_offset, subject_offset, plus_strand,
                   left_sstart, left_send, right_qstart, right_qend,
                   right_sstart, right_send);
      // Specify expected pasted outcome.
      int pasted_qstart{left_qstart}, pasted_qend{right_qend},
          pasted_sstart{left_sstart}, pasted_send{right_send},
          pasted_nident{left_nident + right_nident + subject_offset},
          pasted_mismatch{left_mismatch + right_mismatch},
          pasted_gapopen{left_gapopen + right_gapopen + 1},
          pasted_gaps{left_gaps + right_gaps + query_offset - subject_offset},
          pasted_prefix_end{left_prefix_end},
          pasted_suffix_begin{right_suffix_begin + query_offset
                              + static_cast<int>(left_qseq.length())};

      std::string pasted_qseq, pasted_sseq;
      pasted_qseq = left_qseq.substr(0, left_qseq.length() + subject_offset);
      pasted_qseq.append(query_offset - subject_offset, 'N');
      pasted_qseq.append(right_qseq);

      pasted_sseq = left_sseq.substr(0, left_sseq.length() + subject_offset);
      pasted_sseq.append(query_offset - subject_offset, '-');
      pasted_sseq.append(right_sseq);

      // Swapped subject coordinates indicate minus strand.
      if (!plus_strand) {
        std::swap(left_sstart, left_send);
        std::swap(right_sstart, right_send);
      }

      Alignment left{Alignment::FromStringFields(0,
        {std::to_string(left_qstart), std::to_string(left_qend),
         std::to_string(left_sstart), std::to_string(left_send),
         std::to_string(left_nident), std::to_string(left_mismatch),
         std::to_string(left_gapopen), std::to_string(left_gaps),
         std::to_string(qlen), std::to_string(slen),
         left_qseq, left_sseq},
        scoring_system, paste_parameters)};
      left.UngappedPrefixEnd(left_prefix_end);
      left.UngappedSuffixBegin(left_suffix_begin);
      Alignment right{Alignment::FromStringFields(1,
        {std::to_string(right_qstart), std::to_string(right_qend),
         std::to_string(right_sstart), std::to_string(right_send),
         std::to_string(right_nident), std::to_string(right_mismatch),
         std::to_string(right_gapopen), std::to_string(right_gaps),
         std::to_string(qlen), std::to_string(slen),
         right_qseq, right_sseq},
        scoring_system, paste_parameters)};
      right.UngappedPrefixEnd(right_prefix_end);
      right.UngappedSuffixBegin(right_suffix_begin);

      AlignmentConfiguration config{GetConfiguration(
          query_offset, subject_offset,
          left_qseq.length(), right_qseq.length())};

      Alignment original_right{right};
      left.PasteRight(right, config, scoring_system, paste_parameters);

      THEN("Left becomes the pasted alignment.") {
        CHECK(left.Qstart() == pasted_qstart);
        CHECK(left.Qend() == pasted_qend);
        CHECK(left.Sstart() == pasted_sstart);
        CHECK(left.Send() == pasted_send);
        CHECK(left.PlusStrand() == plus_strand);
        CHECK(left.Nident() == pasted_nident);
        CHECK(left.Mismatch() == pasted_mismatch);
        CHECK(left.Gapopen() == pasted_gapopen);
        CHECK(left.Gaps() == pasted_gaps);
        CHECK(left.Qlen() == qlen);
        CHECK(left.Slen() == slen);
        CHECK(left.Qseq() == pasted_qseq);
        CHECK(left.Sseq() == pasted_sseq);
        CHECK(left.IncludeInOutput() == false);
        CHECK(left.UngappedPrefixEnd() == pasted_prefix_end);
        CHECK(left.UngappedSuffixBegin() == pasted_suffix_begin);
      }

      THEN("Right remains unchanged.") {
        CHECK(right == original_right);
      }
    }

    WHEN("0 > query_offset > subject_offset; -strand") {
      // Specify test case parameters.
      int query_offset{-2}, subject_offset{-5};
      bool plus_strand{false};
      HandleOffset(query_offset, subject_offset, plus_strand,
                   left_sstart, left_send, right_qstart, right_qend,
                   right_sstart, right_send);
      // Specify expected pasted outcome.
      int pasted_qstart{left_qstart}, pasted_qend{right_qend},
          pasted_sstart{right_sstart}, pasted_send{left_send},
          pasted_nident{left_nident + right_nident + subject_offset},
          pasted_mismatch{left_mismatch + right_mismatch},
          pasted_gapopen{left_gapopen + right_gapopen + 1},
          pasted_gaps{left_gaps + right_gaps + query_offset - subject_offset},
          pasted_prefix_end{left_prefix_end},
          pasted_suffix_begin{right_suffix_begin + query_offset
                              + static_cast<int>(left_qseq.length())};

      std::string pasted_qseq, pasted_sseq;
      pasted_qseq = left_qseq.substr(0, left_qseq.length() + subject_offset);
      pasted_qseq.append(query_offset - subject_offset, 'N');
      pasted_qseq.append(right_qseq);

      pasted_sseq = left_sseq.substr(0, left_qseq.length() + subject_offset);
      pasted_sseq.append(query_offset - subject_offset, '-');
      pasted_sseq.append(right_sseq);

      // Swapped subject coordinates indicate minus strand.
      if (!plus_strand) {
        std::swap(left_sstart, left_send);
        std::swap(right_sstart, right_send);
      }

      Alignment left{Alignment::FromStringFields(0,
        {std::to_string(left_qstart), std::to_string(left_qend),
         std::to_string(left_sstart), std::to_string(left_send),
         std::to_string(left_nident), std::to_string(left_mismatch),
         std::to_string(left_gapopen), std::to_string(left_gaps),
         std::to_string(qlen), std::to_string(slen),
         left_qseq, left_sseq},
        scoring_system, paste_parameters)};
      left.UngappedPrefixEnd(left_prefix_end);
      left.UngappedSuffixBegin(left_suffix_begin);
      Alignment right{Alignment::FromStringFields(1,
        {std::to_string(right_qstart), std::to_string(right_qend),
         std::to_string(right_sstart), std::to_string(right_send),
         std::to_string(right_nident), std::to_string(right_mismatch),
         std::to_string(right_gapopen), std::to_string(right_gaps),
         std::to_string(qlen), std::to_string(slen),
         right_qseq, right_sseq},
        scoring_system, paste_parameters)};
      right.UngappedPrefixEnd(right_prefix_end);
      right.UngappedSuffixBegin(right_suffix_begin);

      AlignmentConfiguration config{GetConfiguration(
          query_offset, subject_offset,
          left_qseq.length(), right_qseq.length())};

      Alignment original_right{right};
      left.PasteRight(right, config, scoring_system, paste_parameters);

      THEN("Left becomes the pasted alignment.") {
        CHECK(left.Qstart() == pasted_qstart);
        CHECK(left.Qend() == pasted_qend);
        CHECK(left.Sstart() == pasted_sstart);
        CHECK(left.Send() == pasted_send);
        CHECK(left.PlusStrand() == plus_strand);
        CHECK(left.Nident() == pasted_nident);
        CHECK(left.Mismatch() == pasted_mismatch);
        CHECK(left.Gapopen() == pasted_gapopen);
        CHECK(left.Gaps() == pasted_gaps);
        CHECK(left.Qlen() == qlen);
        CHECK(left.Slen() == slen);
        CHECK(left.Qseq() == pasted_qseq);
        CHECK(left.Sseq() == pasted_sseq);
        CHECK(left.IncludeInOutput() == false);
        CHECK(left.UngappedPrefixEnd() == pasted_prefix_end);
        CHECK(left.UngappedSuffixBegin() == pasted_suffix_begin);
      }

      THEN("Right remains unchanged.") {
        CHECK(right == original_right);
      }
    }

    WHEN("query_offset > 0 > subject_offset; +strand") {
      // Specify test case parameters.
      int query_offset{10}, subject_offset{-3};
      bool plus_strand{true};
      HandleOffset(query_offset, subject_offset, plus_strand,
                   left_sstart, left_send, right_qstart, right_qend,
                   right_sstart, right_send);
      // Specify expected pasted outcome.
      int pasted_qstart{left_qstart}, pasted_qend{right_qend},
          pasted_sstart{left_sstart}, pasted_send{right_send},
          pasted_nident{left_nident + right_nident + subject_offset},
          pasted_mismatch{left_mismatch + right_mismatch},
          pasted_gapopen{left_gapopen + right_gapopen + 1},
          pasted_gaps{left_gaps + right_gaps + query_offset - subject_offset},
          pasted_prefix_end{left_prefix_end},
          pasted_suffix_begin{right_suffix_begin + query_offset
                              + static_cast<int>(left_qseq.length())};

      std::string pasted_qseq, pasted_sseq;
      pasted_qseq = left_qseq.substr(0, left_qseq.length() + subject_offset);
      pasted_qseq.append(query_offset - subject_offset, 'N');
      pasted_qseq.append(right_qseq);

      pasted_sseq = left_sseq.substr(0, left_sseq.length() + subject_offset);
      pasted_sseq.append(query_offset - subject_offset, '-');
      pasted_sseq.append(right_sseq);

      // Swapped subject coordinates indicate minus strand.
      if (!plus_strand) {
        std::swap(left_sstart, left_send);
        std::swap(right_sstart, right_send);
      }

      Alignment left{Alignment::FromStringFields(0,
        {std::to_string(left_qstart), std::to_string(left_qend),
         std::to_string(left_sstart), std::to_string(left_send),
         std::to_string(left_nident), std::to_string(left_mismatch),
         std::to_string(left_gapopen), std::to_string(left_gaps),
         std::to_string(qlen), std::to_string(slen),
         left_qseq, left_sseq},
        scoring_system, paste_parameters)};
      left.UngappedPrefixEnd(left_prefix_end);
      left.UngappedSuffixBegin(left_suffix_begin);
      Alignment right{Alignment::FromStringFields(1,
        {std::to_string(right_qstart), std::to_string(right_qend),
         std::to_string(right_sstart), std::to_string(right_send),
         std::to_string(right_nident), std::to_string(right_mismatch),
         std::to_string(right_gapopen), std::to_string(right_gaps),
         std::to_string(qlen), std::to_string(slen),
         right_qseq, right_sseq},
        scoring_system, paste_parameters)};
      right.UngappedPrefixEnd(right_prefix_end);
      right.UngappedSuffixBegin(right_suffix_begin);

      AlignmentConfiguration config{GetConfiguration(
          query_offset, subject_offset,
          left_qseq.length(), right_qseq.length())};

      Alignment original_right{right};
      left.PasteRight(right, config, scoring_system, paste_parameters);

      THEN("Left becomes the pasted alignment.") {
        CHECK(left.Qstart() == pasted_qstart);
        CHECK(left.Qend() == pasted_qend);
        CHECK(left.Sstart() == pasted_sstart);
        CHECK(left.Send() == pasted_send);
        CHECK(left.PlusStrand() == plus_strand);
        CHECK(left.Nident() == pasted_nident);
        CHECK(left.Mismatch() == pasted_mismatch);
        CHECK(left.Gapopen() == pasted_gapopen);
        CHECK(left.Gaps() == pasted_gaps);
        CHECK(left.Qlen() == qlen);
        CHECK(left.Slen() == slen);
        CHECK(left.Qseq() == pasted_qseq);
        CHECK(left.Sseq() == pasted_sseq);
        CHECK(left.IncludeInOutput() == false);
        CHECK(left.UngappedPrefixEnd() == pasted_prefix_end);
        CHECK(left.UngappedSuffixBegin() == pasted_suffix_begin);
      }

      THEN("Right remains unchanged.") {
        CHECK(right == original_right);
      }
    }

    WHEN("query_offset > 0 > subject_offset; -strand") {
      // Specify test case parameters.
      int query_offset{10}, subject_offset{-3};
      bool plus_strand{false};
      HandleOffset(query_offset, subject_offset, plus_strand,
                   left_sstart, left_send, right_qstart, right_qend,
                   right_sstart, right_send);
      // Specify expected pasted outcome.
      int pasted_qstart{left_qstart}, pasted_qend{right_qend},
          pasted_sstart{right_sstart}, pasted_send{left_send},
          pasted_nident{left_nident + right_nident + subject_offset},
          pasted_mismatch{left_mismatch + right_mismatch},
          pasted_gapopen{left_gapopen + right_gapopen + 1},
          pasted_gaps{left_gaps + right_gaps + query_offset - subject_offset},
          pasted_prefix_end{left_prefix_end},
          pasted_suffix_begin{right_suffix_begin + query_offset
                              + static_cast<int>(left_qseq.length())};

      std::string pasted_qseq, pasted_sseq;
      pasted_qseq = left_qseq.substr(0, left_qseq.length() + subject_offset);
      pasted_qseq.append(query_offset - subject_offset, 'N');
      pasted_qseq.append(right_qseq);

      pasted_sseq = left_sseq.substr(0, left_qseq.length() + subject_offset);
      pasted_sseq.append(query_offset - subject_offset, '-');
      pasted_sseq.append(right_sseq);

      // Swapped subject coordinates indicate minus strand.
      if (!plus_strand) {
        std::swap(left_sstart, left_send);
        std::swap(right_sstart, right_send);
      }

      Alignment left{Alignment::FromStringFields(0,
        {std::to_string(left_qstart), std::to_string(left_qend),
         std::to_string(left_sstart), std::to_string(left_send),
         std::to_string(left_nident), std::to_string(left_mismatch),
         std::to_string(left_gapopen), std::to_string(left_gaps),
         std::to_string(qlen), std::to_string(slen),
         left_qseq, left_sseq},
        scoring_system, paste_parameters)};
      left.UngappedPrefixEnd(left_prefix_end);
      left.UngappedSuffixBegin(left_suffix_begin);
      Alignment right{Alignment::FromStringFields(1,
        {std::to_string(right_qstart), std::to_string(right_qend),
         std::to_string(right_sstart), std::to_string(right_send),
         std::to_string(right_nident), std::to_string(right_mismatch),
         std::to_string(right_gapopen), std::to_string(right_gaps),
         std::to_string(qlen), std::to_string(slen),
         right_qseq, right_sseq},
        scoring_system, paste_parameters)};
      right.UngappedPrefixEnd(right_prefix_end);
      right.UngappedSuffixBegin(right_suffix_begin);

      AlignmentConfiguration config{GetConfiguration(
          query_offset, subject_offset,
          left_qseq.length(), right_qseq.length())};

      Alignment original_right{right};
      left.PasteRight(right, config, scoring_system, paste_parameters);

      THEN("Left becomes the pasted alignment.") {
        CHECK(left.Qstart() == pasted_qstart);
        CHECK(left.Qend() == pasted_qend);
        CHECK(left.Sstart() == pasted_sstart);
        CHECK(left.Send() == pasted_send);
        CHECK(left.PlusStrand() == plus_strand);
        CHECK(left.Nident() == pasted_nident);
        CHECK(left.Mismatch() == pasted_mismatch);
        CHECK(left.Gapopen() == pasted_gapopen);
        CHECK(left.Gaps() == pasted_gaps);
        CHECK(left.Qlen() == qlen);
        CHECK(left.Slen() == slen);
        CHECK(left.Qseq() == pasted_qseq);
        CHECK(left.Sseq() == pasted_sseq);
        CHECK(left.IncludeInOutput() == false);
        CHECK(left.UngappedPrefixEnd() == pasted_prefix_end);
        CHECK(left.UngappedSuffixBegin() == pasted_suffix_begin);
      }

      THEN("Right remains unchanged.") {
        CHECK(right == original_right);
      }
    }

    WHEN("subject_offset > 0 > query_offset; +strand") {
      // Specify test case parameters.
      int query_offset{-3}, subject_offset{10};
      bool plus_strand{true};
      HandleOffset(query_offset, subject_offset, plus_strand,
                   left_sstart, left_send, right_qstart, right_qend,
                   right_sstart, right_send);
      // Specify expected pasted outcome.
      int pasted_qstart{left_qstart}, pasted_qend{right_qend},
          pasted_sstart{left_sstart}, pasted_send{right_send},
          pasted_nident{left_nident + right_nident + query_offset},
          pasted_mismatch{left_mismatch + right_mismatch},
          pasted_gapopen{left_gapopen + right_gapopen + 1},
          pasted_gaps{left_gaps + right_gaps + subject_offset - query_offset},
          pasted_prefix_end{left_prefix_end},
          pasted_suffix_begin{right_suffix_begin + subject_offset
                              + static_cast<int>(left_qseq.length())};

      std::string pasted_qseq, pasted_sseq;
      pasted_qseq = left_qseq.substr(0, left_qseq.length() + query_offset);
      pasted_qseq.append(subject_offset - query_offset, '-');
      pasted_qseq.append(right_qseq);

      pasted_sseq = left_sseq.substr(0, left_sseq.length() + query_offset);
      pasted_sseq.append(subject_offset - query_offset, 'N');
      pasted_sseq.append(right_sseq);

      // Swapped subject coordinates indicate minus strand.
      if (!plus_strand) {
        std::swap(left_sstart, left_send);
        std::swap(right_sstart, right_send);
      }

      Alignment left{Alignment::FromStringFields(0,
        {std::to_string(left_qstart), std::to_string(left_qend),
         std::to_string(left_sstart), std::to_string(left_send),
         std::to_string(left_nident), std::to_string(left_mismatch),
         std::to_string(left_gapopen), std::to_string(left_gaps),
         std::to_string(qlen), std::to_string(slen),
         left_qseq, left_sseq},
        scoring_system, paste_parameters)};
      left.UngappedPrefixEnd(left_prefix_end);
      left.UngappedSuffixBegin(left_suffix_begin);
      Alignment right{Alignment::FromStringFields(1,
        {std::to_string(right_qstart), std::to_string(right_qend),
         std::to_string(right_sstart), std::to_string(right_send),
         std::to_string(right_nident), std::to_string(right_mismatch),
         std::to_string(right_gapopen), std::to_string(right_gaps),
         std::to_string(qlen), std::to_string(slen),
         right_qseq, right_sseq},
        scoring_system, paste_parameters)};
      right.UngappedPrefixEnd(right_prefix_end);
      right.UngappedSuffixBegin(right_suffix_begin);

      AlignmentConfiguration config{GetConfiguration(
          query_offset, subject_offset,
          left_qseq.length(), right_qseq.length())};

      Alignment original_right{right};
      left.PasteRight(right, config, scoring_system, paste_parameters);

      THEN("Left becomes the pasted alignment.") {
        CHECK(left.Qstart() == pasted_qstart);
        CHECK(left.Qend() == pasted_qend);
        CHECK(left.Sstart() == pasted_sstart);
        CHECK(left.Send() == pasted_send);
        CHECK(left.PlusStrand() == plus_strand);
        CHECK(left.Nident() == pasted_nident);
        CHECK(left.Mismatch() == pasted_mismatch);
        CHECK(left.Gapopen() == pasted_gapopen);
        CHECK(left.Gaps() == pasted_gaps);
        CHECK(left.Qlen() == qlen);
        CHECK(left.Slen() == slen);
        CHECK(left.Qseq() == pasted_qseq);
        CHECK(left.Sseq() == pasted_sseq);
        CHECK(left.IncludeInOutput() == false);
        CHECK(left.UngappedPrefixEnd() == pasted_prefix_end);
        CHECK(left.UngappedSuffixBegin() == pasted_suffix_begin);
      }

      THEN("Right remains unchanged.") {
        CHECK(right == original_right);
      }
    }

    WHEN("subject_offset > 0 > query_offset; -strand") {
      // Specify test case parameters.
      int query_offset{-3}, subject_offset{10};
      bool plus_strand{false};
      HandleOffset(query_offset, subject_offset, plus_strand,
                   left_sstart, left_send, right_qstart, right_qend,
                   right_sstart, right_send);
      // Specify expected pasted outcome.
      int pasted_qstart{left_qstart}, pasted_qend{right_qend},
          pasted_sstart{right_sstart}, pasted_send{left_send},
          pasted_nident{left_nident + right_nident + query_offset},
          pasted_mismatch{left_mismatch + right_mismatch},
          pasted_gapopen{left_gapopen + right_gapopen + 1},
          pasted_gaps{left_gaps + right_gaps + subject_offset - query_offset},
          pasted_prefix_end{left_prefix_end},
          pasted_suffix_begin{right_suffix_begin + subject_offset
                              + static_cast<int>(left_qseq.length())};

      std::string pasted_qseq, pasted_sseq;
      pasted_qseq = left_qseq.substr(0, left_qseq.length() + query_offset);
      pasted_qseq.append(subject_offset - query_offset, '-');
      pasted_qseq.append(right_qseq);

      pasted_sseq = left_sseq.substr(0, left_qseq.length() + query_offset);
      pasted_sseq.append(subject_offset - query_offset, 'N');
      pasted_sseq.append(right_sseq);

      // Swapped subject coordinates indicate minus strand.
      if (!plus_strand) {
        std::swap(left_sstart, left_send);
        std::swap(right_sstart, right_send);
      }

      Alignment left{Alignment::FromStringFields(0,
        {std::to_string(left_qstart), std::to_string(left_qend),
         std::to_string(left_sstart), std::to_string(left_send),
         std::to_string(left_nident), std::to_string(left_mismatch),
         std::to_string(left_gapopen), std::to_string(left_gaps),
         std::to_string(qlen), std::to_string(slen),
         left_qseq, left_sseq},
        scoring_system, paste_parameters)};
      left.UngappedPrefixEnd(left_prefix_end);
      left.UngappedSuffixBegin(left_suffix_begin);
      Alignment right{Alignment::FromStringFields(1,
        {std::to_string(right_qstart), std::to_string(right_qend),
         std::to_string(right_sstart), std::to_string(right_send),
         std::to_string(right_nident), std::to_string(right_mismatch),
         std::to_string(right_gapopen), std::to_string(right_gaps),
         std::to_string(qlen), std::to_string(slen),
         right_qseq, right_sseq},
        scoring_system, paste_parameters)};
      right.UngappedPrefixEnd(right_prefix_end);
      right.UngappedSuffixBegin(right_suffix_begin);

      AlignmentConfiguration config{GetConfiguration(
          query_offset, subject_offset,
          left_qseq.length(), right_qseq.length())};

      Alignment original_right{right};
      left.PasteRight(right, config, scoring_system, paste_parameters);

      THEN("Left becomes the pasted alignment.") {
        CHECK(left.Qstart() == pasted_qstart);
        CHECK(left.Qend() == pasted_qend);
        CHECK(left.Sstart() == pasted_sstart);
        CHECK(left.Send() == pasted_send);
        CHECK(left.PlusStrand() == plus_strand);
        CHECK(left.Nident() == pasted_nident);
        CHECK(left.Mismatch() == pasted_mismatch);
        CHECK(left.Gapopen() == pasted_gapopen);
        CHECK(left.Gaps() == pasted_gaps);
        CHECK(left.Qlen() == qlen);
        CHECK(left.Slen() == slen);
        CHECK(left.Qseq() == pasted_qseq);
        CHECK(left.Sseq() == pasted_sseq);
        CHECK(left.IncludeInOutput() == false);
        CHECK(left.UngappedPrefixEnd() == pasted_prefix_end);
        CHECK(left.UngappedSuffixBegin() == pasted_suffix_begin);
      }

      THEN("Right remains unchanged.") {
        CHECK(right == original_right);
      }
    }
  }
}

SCENARIO("Test invariant preservation by Alignment::PasteRight.",
         "[Alignment][PasteRight][invariants]") {
  ScoringSystem scoring_system{ScoringSystem::Create(100000l, 1, 2, 1, 1)};
  PasteParameters paste_parameters;

  GIVEN("An alignment pair on plus strand.") {
    Alignment left{Alignment::FromStringFields(0,
        {"101", "134", "1001", "1036",
         "24", "8", "2", "6",
         "10000", "100000",
         "AAAAAAAATTTTTTCCCCGGGG----GGGGAAAAAAAA",
         "AAAAAAAACCCC--CCCCGGGGAAAATTTTAAAAAAAA"},
        scoring_system, paste_parameters)};

    WHEN("query_offset == subject_offset == 0") {
      Alignment right{Alignment::FromStringFields(1,
          {"135", "160", "1037", "1060",
           "20", "4", "1", "2",
           "10000", "100000",
           "AAAAAAAATTTTCCCCGGAAAAAAAA",
           "AAAAAAAAGGGGCCCC--AAAAAAAA"},
          scoring_system, paste_parameters)};
      AlignmentConfiguration config{GetConfiguration(0, 0, 38, 26)};
      left.PasteRight(right, config, scoring_system, paste_parameters);

      THEN("All invariants are satisfied.") {
        CHECK(left.Qstart() <= left.Qend());
        CHECK(left.Sstart() <= left.Send());
        CHECK(left.Qlen() > 0);
        CHECK(left.Slen() > 0);
        CHECK_FALSE(left.Qseq().empty());
        CHECK_FALSE(left.Sseq().empty());
        CHECK(left.Qseq().length() == left.Sseq().length());
        CHECK(std::find(left.PastedIdentifiers().begin(),
                        left.PastedIdentifiers().end(),
                        left.Id())
              != left.PastedIdentifiers().end());
      }
    }

    WHEN("query_offset > subject_offset == 0") {
      Alignment right{Alignment::FromStringFields(1,
          {"145", "170", "1037", "1060",
           "20", "4", "1", "2",
           "10000", "100000",
           "AAAAAAAATTTTCCCCGGAAAAAAAA",
           "AAAAAAAAGGGGCCCC--AAAAAAAA"},
          scoring_system, paste_parameters)};
      AlignmentConfiguration config{GetConfiguration(10, 0, 38, 26)};
      left.PasteRight(right, config, scoring_system, paste_parameters);

      THEN("All invariants are satisfied.") {
        CHECK(left.Qstart() <= left.Qend());
        CHECK(left.Sstart() <= left.Send());
        CHECK(left.Qlen() > 0);
        CHECK(left.Slen() > 0);
        CHECK_FALSE(left.Qseq().empty());
        CHECK_FALSE(left.Sseq().empty());
        CHECK(left.Qseq().length() == left.Sseq().length());
        CHECK(std::find(left.PastedIdentifiers().begin(),
                        left.PastedIdentifiers().end(),
                        left.Id())
              != left.PastedIdentifiers().end());
      }
    }

    WHEN("query_offset < subject_offset == 0") {
      Alignment right{Alignment::FromStringFields(1,
          {"125", "150", "1037", "1060",
           "20", "4", "1", "2",
           "10000", "100000",
           "AAAAAAAATTTTCCCCGGAAAAAAAA",
           "AAAAAAAAGGGGCCCC--AAAAAAAA"},
          scoring_system, paste_parameters)};
      AlignmentConfiguration config{GetConfiguration(-10, 0, 38, 26)};
      left.PasteRight(right, config, scoring_system, paste_parameters);

      THEN("All invariants are satisfied.") {
        CHECK(left.Qstart() <= left.Qend());
        CHECK(left.Sstart() <= left.Send());
        CHECK(left.Qlen() > 0);
        CHECK(left.Slen() > 0);
        CHECK_FALSE(left.Qseq().empty());
        CHECK_FALSE(left.Sseq().empty());
        CHECK(left.Qseq().length() == left.Sseq().length());
        CHECK(std::find(left.PastedIdentifiers().begin(),
                        left.PastedIdentifiers().end(),
                        left.Id())
              != left.PastedIdentifiers().end());
      }
    }

    WHEN("query_offset == subject_offset < 0") {
      Alignment right{Alignment::FromStringFields(1,
          {"125", "150", "1027", "1040",
           "20", "4", "1", "2",
           "10000", "100000",
           "AAAAAAAATTTTCCCCGGAAAAAAAA",
           "AAAAAAAAGGGGCCCC--AAAAAAAA"},
          scoring_system, paste_parameters)};
      AlignmentConfiguration config{GetConfiguration(-10, -10, 38, 26)};
      left.PasteRight(right, config, scoring_system, paste_parameters);

      THEN("All invariants are satisfied.") {
        CHECK(left.Qstart() <= left.Qend());
        CHECK(left.Sstart() <= left.Send());
        CHECK(left.Qlen() > 0);
        CHECK(left.Slen() > 0);
        CHECK_FALSE(left.Qseq().empty());
        CHECK_FALSE(left.Sseq().empty());
        CHECK(left.Qseq().length() == left.Sseq().length());
        CHECK(std::find(left.PastedIdentifiers().begin(),
                        left.PastedIdentifiers().end(),
                        left.Id())
              != left.PastedIdentifiers().end());
      }
    }

    WHEN("query_offset == subject_offset > 0") {
      Alignment right{Alignment::FromStringFields(1,
          {"145", "170", "1047", "1070",
           "20", "4", "1", "2",
           "10000", "100000",
           "AAAAAAAATTTTCCCCGGAAAAAAAA",
           "AAAAAAAAGGGGCCCC--AAAAAAAA"},
          scoring_system, paste_parameters)};
      AlignmentConfiguration config{GetConfiguration(10, 10, 38, 26)};
      left.PasteRight(right, config, scoring_system, paste_parameters);

      THEN("All invariants are satisfied.") {
        CHECK(left.Qstart() <= left.Qend());
        CHECK(left.Sstart() <= left.Send());
        CHECK(left.Qlen() > 0);
        CHECK(left.Slen() > 0);
        CHECK_FALSE(left.Qseq().empty());
        CHECK_FALSE(left.Sseq().empty());
        CHECK(left.Qseq().length() == left.Sseq().length());
        CHECK(std::find(left.PastedIdentifiers().begin(),
                        left.PastedIdentifiers().end(),
                        left.Id())
              != left.PastedIdentifiers().end());
      }
    }

    WHEN("query_offset > subject_offset > 0") {
      Alignment right{Alignment::FromStringFields(1,
          {"145", "170", "1042", "1065",
           "20", "4", "1", "2",
           "10000", "100000",
           "AAAAAAAATTTTCCCCGGAAAAAAAA",
           "AAAAAAAAGGGGCCCC--AAAAAAAA"},
          scoring_system, paste_parameters)};
      AlignmentConfiguration config{GetConfiguration(10, 5, 38, 26)};
      left.PasteRight(right, config, scoring_system, paste_parameters);

      THEN("All invariants are satisfied.") {
        CHECK(left.Qstart() <= left.Qend());
        CHECK(left.Sstart() <= left.Send());
        CHECK(left.Qlen() > 0);
        CHECK(left.Slen() > 0);
        CHECK_FALSE(left.Qseq().empty());
        CHECK_FALSE(left.Sseq().empty());
        CHECK(left.Qseq().length() == left.Sseq().length());
        CHECK(std::find(left.PastedIdentifiers().begin(),
                        left.PastedIdentifiers().end(),
                        left.Id())
              != left.PastedIdentifiers().end());
      }
    }

    WHEN("query_offset < subject_offset < 0") {
      Alignment right{Alignment::FromStringFields(1,
          {"125", "150", "1032", "1055",
           "20", "4", "1", "2",
           "10000", "100000",
           "AAAAAAAATTTTCCCCGGAAAAAAAA",
           "AAAAAAAAGGGGCCCC--AAAAAAAA"},
          scoring_system, paste_parameters)};
      AlignmentConfiguration config{GetConfiguration(-10, -5, 38, 26)};
      left.PasteRight(right, config, scoring_system, paste_parameters);

      THEN("All invariants are satisfied.") {
        CHECK(left.Qstart() <= left.Qend());
        CHECK(left.Sstart() <= left.Send());
        CHECK(left.Qlen() > 0);
        CHECK(left.Slen() > 0);
        CHECK_FALSE(left.Qseq().empty());
        CHECK_FALSE(left.Sseq().empty());
        CHECK(left.Qseq().length() == left.Sseq().length());
        CHECK(std::find(left.PastedIdentifiers().begin(),
                        left.PastedIdentifiers().end(),
                        left.Id())
              != left.PastedIdentifiers().end());
      }
    }

    WHEN("query_offset < 0 < subject_offset") {
      Alignment right{Alignment::FromStringFields(1,
          {"125", "150", "1047", "1070",
           "20", "4", "1", "2",
           "10000", "100000",
           "AAAAAAAATTTTCCCCGGAAAAAAAA",
           "AAAAAAAAGGGGCCCC--AAAAAAAA"},
          scoring_system, paste_parameters)};
      AlignmentConfiguration config{GetConfiguration(-10, 10, 38, 26)};
      left.PasteRight(right, config, scoring_system, paste_parameters);

      THEN("All invariants are satisfied.") {
        CHECK(left.Qstart() <= left.Qend());
        CHECK(left.Sstart() <= left.Send());
        CHECK(left.Qlen() > 0);
        CHECK(left.Slen() > 0);
        CHECK_FALSE(left.Qseq().empty());
        CHECK_FALSE(left.Sseq().empty());
        CHECK(left.Qseq().length() == left.Sseq().length());
        CHECK(std::find(left.PastedIdentifiers().begin(),
                        left.PastedIdentifiers().end(),
                        left.Id())
              != left.PastedIdentifiers().end());
      }
    }

    WHEN("query_offset > 0 > subject_offset") {
      Alignment right{Alignment::FromStringFields(1,
          {"145", "170", "1027", "1050",
           "20", "4", "1", "2",
           "10000", "100000",
           "AAAAAAAATTTTCCCCGGAAAAAAAA",
           "AAAAAAAAGGGGCCCC--AAAAAAAA"},
          scoring_system, paste_parameters)};
      AlignmentConfiguration config{GetConfiguration(10, -10, 38, 26)};
      left.PasteRight(right, config, scoring_system, paste_parameters);

      THEN("All invariants are satisfied.") {
        CHECK(left.Qstart() <= left.Qend());
        CHECK(left.Sstart() <= left.Send());
        CHECK(left.Qlen() > 0);
        CHECK(left.Slen() > 0);
        CHECK_FALSE(left.Qseq().empty());
        CHECK_FALSE(left.Sseq().empty());
        CHECK(left.Qseq().length() == left.Sseq().length());
        CHECK(std::find(left.PastedIdentifiers().begin(),
                        left.PastedIdentifiers().end(),
                        left.Id())
              != left.PastedIdentifiers().end());
      }
    }

    WHEN("query_offset == 0 < subject_offset") {
      Alignment right{Alignment::FromStringFields(1,
          {"135", "160", "1047", "1070",
           "20", "4", "1", "2",
           "10000", "100000",
           "AAAAAAAATTTTCCCCGGAAAAAAAA",
           "AAAAAAAAGGGGCCCC--AAAAAAAA"},
          scoring_system, paste_parameters)};
      AlignmentConfiguration config{GetConfiguration(0, 10, 38, 26)};
      left.PasteRight(right, config, scoring_system, paste_parameters);

      THEN("All invariants are satisfied.") {
        CHECK(left.Qstart() <= left.Qend());
        CHECK(left.Sstart() <= left.Send());
        CHECK(left.Qlen() > 0);
        CHECK(left.Slen() > 0);
        CHECK_FALSE(left.Qseq().empty());
        CHECK_FALSE(left.Sseq().empty());
        CHECK(left.Qseq().length() == left.Sseq().length());
        CHECK(std::find(left.PastedIdentifiers().begin(),
                        left.PastedIdentifiers().end(),
                        left.Id())
              != left.PastedIdentifiers().end());
      }
    }

    WHEN("query_offset == 0 > subject_offset") {
      Alignment right{Alignment::FromStringFields(1,
          {"135", "160", "1027", "1050",
           "20", "4", "1", "2",
           "10000", "100000",
           "AAAAAAAATTTTCCCCGGAAAAAAAA",
           "AAAAAAAAGGGGCCCC--AAAAAAAA"},
          scoring_system, paste_parameters)};
      AlignmentConfiguration config{GetConfiguration(0, -10, 38, 26)};
      left.PasteRight(right, config, scoring_system, paste_parameters);

      THEN("All invariants are satisfied.") {
        CHECK(left.Qstart() <= left.Qend());
        CHECK(left.Sstart() <= left.Send());
        CHECK(left.Qlen() > 0);
        CHECK(left.Slen() > 0);
        CHECK_FALSE(left.Qseq().empty());
        CHECK_FALSE(left.Sseq().empty());
        CHECK(left.Qseq().length() == left.Sseq().length());
        CHECK(std::find(left.PastedIdentifiers().begin(),
                        left.PastedIdentifiers().end(),
                        left.Id())
              != left.PastedIdentifiers().end());
      }
    }

    WHEN("subject_offset > query_offset > 0") {
      Alignment right{Alignment::FromStringFields(1,
          {"140", "165", "1047", "1070",
           "20", "4", "1", "2",
           "10000", "100000",
           "AAAAAAAATTTTCCCCGGAAAAAAAA",
           "AAAAAAAAGGGGCCCC--AAAAAAAA"},
          scoring_system, paste_parameters)};
      AlignmentConfiguration config{GetConfiguration(5, 10, 38, 26)};
      left.PasteRight(right, config, scoring_system, paste_parameters);

      THEN("All invariants are satisfied.") {
        CHECK(left.Qstart() <= left.Qend());
        CHECK(left.Sstart() <= left.Send());
        CHECK(left.Qlen() > 0);
        CHECK(left.Slen() > 0);
        CHECK_FALSE(left.Qseq().empty());
        CHECK_FALSE(left.Sseq().empty());
        CHECK(left.Qseq().length() == left.Sseq().length());
        CHECK(std::find(left.PastedIdentifiers().begin(),
                        left.PastedIdentifiers().end(),
                        left.Id())
              != left.PastedIdentifiers().end());
      }
    }

    WHEN("subject_offset < query_offset < 0") {
      Alignment right{Alignment::FromStringFields(1,
          {"130", "155", "1027", "1050",
           "20", "4", "1", "2",
           "10000", "100000",
           "AAAAAAAATTTTCCCCGGAAAAAAAA",
           "AAAAAAAAGGGGCCCC--AAAAAAAA"},
          scoring_system, paste_parameters)};
      AlignmentConfiguration config{GetConfiguration(-5, -10, 38, 26)};
      left.PasteRight(right, config, scoring_system, paste_parameters);

      THEN("All invariants are satisfied.") {
        CHECK(left.Qstart() <= left.Qend());
        CHECK(left.Sstart() <= left.Send());
        CHECK(left.Qlen() > 0);
        CHECK(left.Slen() > 0);
        CHECK_FALSE(left.Qseq().empty());
        CHECK_FALSE(left.Sseq().empty());
        CHECK(left.Qseq().length() == left.Sseq().length());
        CHECK(std::find(left.PastedIdentifiers().begin(),
                        left.PastedIdentifiers().end(),
                        left.Id())
              != left.PastedIdentifiers().end());
      }
    }
  }
}

SCENARIO("Test exceptions thrown by Alignment::PasteRight.",
         "[Alignment][PasteRight][exceptions]") {
  ScoringSystem scoring_system{ScoringSystem::Create(100000l, 1, 2, 1, 1)};
  PasteParameters paste_parameters;

  WHEN("When two alignments are given.") {
    int query_offset = GENERATE(-4, -2, 0, 5, 10);
    int subject_offset = GENERATE(-4, -2, 0, 5, 10);
    Alignment left_plus{Alignment::FromStringFields(0,
        {"101", "134", "1001", "1036",
         "24", "8", "2", "6",
         "10000", "100000",
         "AAAAAAAATTTTTTCCCCGGGG----GGGGAAAAAAAA",
         "AAAAAAAACCCC--CCCCGGGGAAAATTTTAAAAAAAA"},
        scoring_system, paste_parameters)};
    Alignment right_plus{Alignment::FromStringFields(1,
        {"135", "160", "1037", "1060",
         "20", "4", "1", "2",
         "10000", "100000",
         "AAAAAAAATTTTCCCCGGAAAAAAAA",
         "AAAAAAAAGGGGCCCC--AAAAAAAA"},
        scoring_system, paste_parameters)};
    Alignment left_minus{Alignment::FromStringFields(1,
        {"101", "134", "1036", "1001",
         "24", "8", "2", "6",
         "10000", "100000",
         "AAAAAAAATTTTTTCCCCGGGG----GGGGAAAAAAAA",
         "AAAAAAAACCCC--CCCCGGGGAAAATTTTAAAAAAAA"},
        scoring_system, paste_parameters)};
    Alignment right_minus{Alignment::FromStringFields(1,
        {"135", "160", "1000", "977",
         "20", "4", "1", "2",
         "10000", "100000",
         "AAAAAAAATTTTCCCCGGAAAAAAAA",
         "AAAAAAAAGGGGCCCC--AAAAAAAA"},
        scoring_system, paste_parameters)};
    AlignmentConfiguration config{GetConfiguration(
        query_offset, subject_offset,
        left_plus.Length(), right_plus.Length())};

    THEN("If on opposite strands, exception is thrown.") {
      CHECK_THROWS_AS(left_plus.PasteRight(right_minus, config, scoring_system,
                                           paste_parameters),
                      exceptions::PastingError);
      CHECK_THROWS_AS(left_minus.PasteRight(right_plus, config, scoring_system,
                                            paste_parameters),
                      exceptions::PastingError);
    }

    THEN("If on same strands, no exception is thrown.") {
      CHECK_NOTHROW(left_plus.PasteRight(right_plus, config, scoring_system,
                                         paste_parameters));
      CHECK_NOTHROW(left_minus.PasteRight(right_minus, config, scoring_system,
                                          paste_parameters));
    }
  }

  THEN("Query/subject region of one contained in other causes exception.") {
    Alignment left{Alignment::FromStringFields(0,
        {"101", "134", "1001", "1036",
         "24", "8", "2", "6",
         "10000", "100000",
         "AAAAAAAATTTTTTCCCCGGGG----GGGGAAAAAAAA",
         "AAAAAAAACCCC--CCCCGGGGAAAATTTTAAAAAAAA"},
        scoring_system, paste_parameters)};
    Alignment right_inside_query{Alignment::FromStringFields(1,
        {"109", "126", "2009", "2028",
         "8", "8", "2", "6",
         "10000", "100000",
         "TTTTTTCCCCGGGG----GGGG",
         "CCCC--CCCCGGGGAAAATTTT"},
        scoring_system, paste_parameters)};
    Alignment right_contains_query{Alignment::FromStringFields(1,
        {"95", "136", "1997", "2038",
         "30", "8", "2", "6",
         "10000", "100000",
         "CCCCAAAAAAAATTTTTTCCCCGGGG----GGGGAAAAAAAATT",
         "CCCCAAAAAAAACCCC--CCCCGGGGAAAATTTTAAAAAAAATT"},
        scoring_system, paste_parameters)};
    Alignment right_inside_subject{Alignment::FromStringFields(1,
        {"309", "326", "1009", "1028",
         "8", "8", "2", "6",
         "10000", "100000",
         "TTTTTTCCCCGGGG----GGGG",
         "CCCC--CCCCGGGGAAAATTTT"},
        scoring_system, paste_parameters)};
    Alignment right_contains_subject{Alignment::FromStringFields(1,
        {"295", "336", "997", "1038",
         "30", "8", "2", "6",
         "10000", "100000",
         "CCCCAAAAAAAATTTTTTCCCCGGGG----GGGGAAAAAAAATT",
         "CCCCAAAAAAAACCCC--CCCCGGGGAAAATTTTAAAAAAAATT"},
        scoring_system, paste_parameters)};
    // Offset is disregarded here.
    AlignmentConfiguration config_short{GetConfiguration(
        0, 0, left.Length(), right_inside_query.Length())};
    AlignmentConfiguration config_long{GetConfiguration(
        0, 0, left.Length(), right_contains_query.Length())};
    AlignmentConfiguration config_self{GetConfiguration(
        0, 0, left.Length(), left.Length())};

    THEN("If on opposite strands, exception is thrown.") {
      CHECK_THROWS_AS(left.PasteRight(right_inside_query, config_short,
                                      scoring_system, paste_parameters),
                      exceptions::PastingError);
      CHECK_THROWS_AS(left.PasteRight(right_contains_query, config_long,
                                      scoring_system, paste_parameters),
                      exceptions::PastingError);
      CHECK_THROWS_AS(left.PasteRight(right_inside_subject, config_short,
                                      scoring_system, paste_parameters),
                      exceptions::PastingError);
      CHECK_THROWS_AS(left.PasteRight(right_contains_subject, config_long,
                                      scoring_system, paste_parameters),
                      exceptions::PastingError);
      CHECK_THROWS_AS(left.PasteRight(left, config_self,
                                      scoring_system, paste_parameters),
                      exceptions::PastingError);
    }
  }

  THEN("Right query region not starting/ending after left causes exception.") {
    Alignment left{Alignment::FromStringFields(0,
        {"101", "134", "1001", "1036",
         "24", "8", "2", "6",
         "10000", "100000",
         "AAAAAAAATTTTTTCCCCGGGG----GGGGAAAAAAAA",
         "AAAAAAAACCCC--CCCCGGGGAAAATTTTAAAAAAAA"},
        scoring_system, paste_parameters)};
    Alignment false_right_nooverlap{Alignment::FromStringFields(1,
        {"35", "60", "1037", "1060",
         "20", "4", "1", "2",
         "10000", "100000",
         "AAAAAAAATTTTCCCCGGAAAAAAAA",
         "AAAAAAAAGGGGCCCC--AAAAAAAA"},
        scoring_system, paste_parameters)};
    Alignment false_right_overlap{Alignment::FromStringFields(1,
        {"95", "120", "1037", "1060",
         "20", "4", "1", "2",
         "10000", "100000",
         "AAAAAAAATTTTCCCCGGAAAAAAAA",
         "AAAAAAAAGGGGCCCC--AAAAAAAA"},
        scoring_system, paste_parameters)};
    Alignment false_right_equal_start{Alignment::FromStringFields(1,
        {"101", "126", "1037", "1060",
         "20", "4", "1", "2",
         "10000", "100000",
         "AAAAAAAATTTTCCCCGGAAAAAAAA",
         "AAAAAAAAGGGGCCCC--AAAAAAAA"},
        scoring_system, paste_parameters)};
    // Offset is disregarded here.
    AlignmentConfiguration config{GetConfiguration(
        0, 0, left.Length(), false_right_nooverlap.Length())};


    CHECK_THROWS_AS(left.PasteRight(false_right_nooverlap, config,
                                    scoring_system, paste_parameters),
                    exceptions::PastingError);
    CHECK_THROWS_AS(left.PasteRight(false_right_overlap, config,
                                    scoring_system, paste_parameters),
                    exceptions::PastingError);
    CHECK_THROWS_AS(left.PasteRight(false_right_equal_start, config,
                                    scoring_system, paste_parameters),
                    exceptions::PastingError);
  }

  THEN("Right subject not starting/ending after left causes exception (+).") {
    Alignment left{Alignment::FromStringFields(0,
        {"101", "134", "1001", "1036",
         "24", "8", "2", "6",
         "10000", "100000",
         "AAAAAAAATTTTTTCCCCGGGG----GGGGAAAAAAAA",
         "AAAAAAAACCCC--CCCCGGGGAAAATTTTAAAAAAAA"},
        scoring_system, paste_parameters)};
    Alignment false_right_nooverlap{Alignment::FromStringFields(1,
        {"135", "160", "937", "960",
         "20", "4", "1", "2",
         "10000", "100000",
         "AAAAAAAATTTTCCCCGGAAAAAAAA",
         "AAAAAAAAGGGGCCCC--AAAAAAAA"},
        scoring_system, paste_parameters)};
    Alignment false_right_overlap{Alignment::FromStringFields(1,
        {"135", "160", "997", "1020",
         "20", "4", "1", "2",
         "10000", "100000",
         "AAAAAAAATTTTCCCCGGAAAAAAAA",
         "AAAAAAAAGGGGCCCC--AAAAAAAA"},
        scoring_system, paste_parameters)};
    Alignment false_right_equal_start{Alignment::FromStringFields(1,
        {"135", "160", "1001", "1024",
         "20", "4", "1", "2",
         "10000", "100000",
         "AAAAAAAATTTTCCCCGGAAAAAAAA",
         "AAAAAAAAGGGGCCCC--AAAAAAAA"},
        scoring_system, paste_parameters)};
    // Offset is disregarded here.
    AlignmentConfiguration config{GetConfiguration(
        0, 0, left.Length(), false_right_nooverlap.Length())};


    CHECK_THROWS_AS(left.PasteRight(false_right_nooverlap, config,
                                    scoring_system, paste_parameters),
                    exceptions::PastingError);
    CHECK_THROWS_AS(left.PasteRight(false_right_overlap, config,
                                    scoring_system, paste_parameters),
                    exceptions::PastingError);
    CHECK_THROWS_AS(left.PasteRight(false_right_equal_start, config,
                                    scoring_system, paste_parameters),
                    exceptions::PastingError);
  }

  THEN("Right subject not starting/ending after left causes exception (-).") {
    Alignment left{Alignment::FromStringFields(0,
        {"101", "134", "1036", "1001",
         "24", "8", "2", "6",
         "10000", "100000",
         "AAAAAAAATTTTTTCCCCGGGG----GGGGAAAAAAAA",
         "AAAAAAAACCCC--CCCCGGGGAAAATTTTAAAAAAAA"},
        scoring_system, paste_parameters)};
    Alignment false_right_nooverlap{Alignment::FromStringFields(1,
        {"135", "160", "1060", "1037",
         "20", "4", "1", "2",
         "10000", "100000",
         "AAAAAAAATTTTCCCCGGAAAAAAAA",
         "AAAAAAAAGGGGCCCC--AAAAAAAA"},
        scoring_system, paste_parameters)};
    Alignment false_right_overlap{Alignment::FromStringFields(1,
        {"135", "160", "1050", "1027",
         "20", "4", "1", "2",
         "10000", "100000",
         "AAAAAAAATTTTCCCCGGAAAAAAAA",
         "AAAAAAAAGGGGCCCC--AAAAAAAA"},
        scoring_system, paste_parameters)};
    Alignment false_right_equal_start{Alignment::FromStringFields(1,
        {"135", "160", "1024", "1001",
         "20", "4", "1", "2",
         "10000", "100000",
         "AAAAAAAATTTTCCCCGGAAAAAAAA",
         "AAAAAAAAGGGGCCCC--AAAAAAAA"},
        scoring_system, paste_parameters)};
    // Offset is disregarded here.
    AlignmentConfiguration config{GetConfiguration(
        0, 0, left.Length(), false_right_nooverlap.Length())};

    CHECK_THROWS_AS(left.PasteRight(false_right_nooverlap, config,
                                    scoring_system, paste_parameters),
                    exceptions::PastingError);
    CHECK_THROWS_AS(left.PasteRight(false_right_overlap, config,
                                    scoring_system, paste_parameters),
                    exceptions::PastingError);
    CHECK_THROWS_AS(left.PasteRight(false_right_equal_start, config,
                                    scoring_system, paste_parameters),
                    exceptions::PastingError);
  }
}

SCENARIO("Test correctness of Alignment::PasteLeft <prefix>.",
         "[Alignment][PasteLeft][correctness][prefix]") {
  ScoringSystem scoring_system{ScoringSystem::Create(100000l, 1, 2, 1, 1)};
  PasteParameters paste_parameters;

  GIVEN("Left alignment has gaps.") {
    int left_length{36}, left_sregion_length{34};
    Alignment left_plus{Alignment::FromStringFields(0,
        {"101", "134", "1001", "1034",
         "24", "8", "2", "4",
         "10000", "100000",
         "AAAA--AAGGAATTTTCCCCGGGGAAAAGGGGAAAA",
         "AAAAGGAA--AACCCCCCCCGGGGAAAAAAAAAAAA"},
        scoring_system, paste_parameters)};
    left_plus.UngappedPrefixEnd(4);
    left_plus.UngappedSuffixBegin(10);
    Alignment left_minus{Alignment::FromStringFields(0,
        {"101", "134", "1034", "1001",
         "24", "8", "2", "4",
         "10000", "100000",
         "AAAA--AAGGAATTTTCCCCGGGGAAAAGGGGAAAA",
         "AAAAGGAA--AACCCCCCCCGGGGAAAAAAAAAAAA"},
        scoring_system, paste_parameters)};
    left_minus.UngappedPrefixEnd(4);
    left_minus.UngappedSuffixBegin(10);

    WHEN("Right alignment is ungapped.") {
      int right_length{24};
      int query_offset = GENERATE(-10, -5, 0, 5, 10);
      int subject_offset = GENERATE(-10, -5, 0, 5, 10);
      AlignmentConfiguration config{GetConfiguration(
          query_offset, subject_offset, left_length, right_length)};
      Alignment right_plus{Alignment::FromStringFields(1,
          {std::to_string(135 + query_offset),
           std::to_string(158 + query_offset),
           std::to_string(1035 + subject_offset),
           std::to_string(1058 + subject_offset),
           "20", "4", "0", "0",
           "10000", "100000",
           "AAAAAAAATTTTCCCCAAAAAAAA",
           "AAAAAAAAGGGGCCCCAAAAAAAA"},
          scoring_system, paste_parameters)};
      Alignment right_minus{Alignment::FromStringFields(1,
          {std::to_string(135 + query_offset),
           std::to_string(158 + query_offset),
           std::to_string(1058 - subject_offset - left_sregion_length
                               - right_length),
           std::to_string(1035 - subject_offset - left_sregion_length
                               - right_length),
           "20", "4", "0", "0",
           "10000", "100000",
           "AAAAAAAATTTTCCCCAAAAAAAA",
           "AAAAAAAAGGGGCCCCAAAAAAAA"},
          scoring_system, paste_parameters)};

      THEN("Pasted alignment takes on left's ungapped prefix.") {
        int original_left_ungapped_prefix = left_plus.UngappedPrefixEnd();
        right_plus.PasteLeft(left_plus, config, scoring_system,
                             paste_parameters);
        CHECK(original_left_ungapped_prefix == right_plus.UngappedPrefixEnd());

        original_left_ungapped_prefix = left_minus.UngappedPrefixEnd();
        right_minus.PasteLeft(left_minus, config, scoring_system,
                              paste_parameters);
        CHECK(original_left_ungapped_prefix == right_minus.UngappedPrefixEnd());
      }
    }

    WHEN("Right alignment's portion in pasted has no gap.") {
      int right_length{28}, right_sregion_length{27};
      int query_offset = GENERATE(-10, -8, -6);
      int subject_offset = GENERATE(-10, -8, -6);
      AlignmentConfiguration config{GetConfiguration(
          query_offset, subject_offset, left_length, right_length)};
      Alignment right_plus{Alignment::FromStringFields(1,
          {std::to_string(135 + query_offset),
           std::to_string(159 + query_offset),
           std::to_string(1035 + subject_offset),
           std::to_string(1061 + subject_offset),
           "20", "4", "2", "4",
           "10000", "100000",
           "A---AGAAAAAATTTTCCCCAAAAAAAA",
           "AGGGA-AAAAAAGGGGCCCCAAAAAAAA"},
          scoring_system, paste_parameters)};
      right_plus.UngappedPrefixEnd(1);
      right_plus.UngappedSuffixBegin(6);
      Alignment right_minus{Alignment::FromStringFields(1,
          {std::to_string(135 + query_offset),
           std::to_string(159 + query_offset),
           std::to_string(1061 - subject_offset - left_sregion_length
                               - right_sregion_length),
           std::to_string(1035 - subject_offset - left_sregion_length
                               - right_sregion_length),
           "20", "4", "2", "4",
           "10000", "100000",
           "A---AGAAAAAATTTTCCCCAAAAAAAA",
           "AGGGA-AAAAAAGGGGCCCCAAAAAAAA"},
          scoring_system, paste_parameters)};
      right_minus.UngappedPrefixEnd(1);
      right_minus.UngappedSuffixBegin(6);

      THEN("Pasted alignment takes on left's ungapped prefix.") {
        int original_left_ungapped_prefix = left_plus.UngappedPrefixEnd();
        right_plus.PasteLeft(left_plus, config, scoring_system,
                             paste_parameters);
        CHECK(original_left_ungapped_prefix == right_plus.UngappedPrefixEnd());

        original_left_ungapped_prefix = left_minus.UngappedPrefixEnd();
        right_minus.PasteLeft(left_minus, config, scoring_system,
                              paste_parameters);
        CHECK(original_left_ungapped_prefix == right_minus.UngappedPrefixEnd());
      }
    }

    WHEN("Right alignment's portion in pasted has left-most gap.") {
      int right_length{28}, right_sregion_length{27};
      int query_offset = GENERATE(-10, -5, 0, 5, 10);
      int subject_offset = GENERATE(-10, -5, 0, 5, 10);
      AlignmentConfiguration config{GetConfiguration(
          query_offset, subject_offset, left_length, right_length)};
      Alignment right_plus{Alignment::FromStringFields(1,
          {std::to_string(135 + query_offset),
           std::to_string(159 + query_offset),
           std::to_string(1035 + subject_offset),
           std::to_string(1061 + subject_offset),
           "20", "4", "2", "4",
           "10000", "100000",
           "AAAAAAAATTTTGCCCC---AAAAAAAA",
           "AAAAAAAAGGGG-CCCCGGGAAAAAAAA"},
          scoring_system, paste_parameters)};
      right_plus.UngappedPrefixEnd(12);
      right_plus.UngappedSuffixBegin(20);
      Alignment right_minus{Alignment::FromStringFields(1,
          {std::to_string(135 + query_offset),
           std::to_string(159 + query_offset),
           std::to_string(1061 - subject_offset - left_sregion_length
                               - right_sregion_length),
           std::to_string(1035 - subject_offset - left_sregion_length
                               - right_sregion_length),
           "20", "4", "2", "4",
           "10000", "100000",
           "AAAAAAAATTTTGCCCC---AAAAAAAA",
           "AAAAAAAAGGGG-CCCCGGGAAAAAAAA"},
          scoring_system, paste_parameters)};
      right_minus.UngappedPrefixEnd(12);
      right_minus.UngappedSuffixBegin(20);

      THEN("Pasted alignment takes on left's ungapped prefix.") {
        int original_left_ungapped_prefix = left_plus.UngappedPrefixEnd();
        right_plus.PasteLeft(left_plus, config, scoring_system,
                             paste_parameters);
        CHECK(original_left_ungapped_prefix == right_plus.UngappedPrefixEnd());

        original_left_ungapped_prefix = left_minus.UngappedPrefixEnd();
        right_minus.PasteLeft(left_minus, config, scoring_system,
                              paste_parameters);
        CHECK(original_left_ungapped_prefix == right_minus.UngappedPrefixEnd());
      }
    }

    WHEN("Right alignment's portion in pasted has gap but not left-most.") {
      int right_length{28}, right_sregion_length{27};
      int query_offset = GENERATE(-10, -8, -5);
      int subject_offset = GENERATE(-10, -8, -5);
      AlignmentConfiguration config{GetConfiguration(
          query_offset, subject_offset, left_length, right_length)};
      Alignment right_plus{Alignment::FromStringFields(1,
          {std::to_string(135 + query_offset),
           std::to_string(159 + query_offset),
           std::to_string(1035 + subject_offset),
           std::to_string(1061 + subject_offset),
           "20", "4", "2", "4",
           "10000", "100000",
           "AAGAAAAAATTTTCCCC---AAAAAAAA",
           "AA-AAAAAAGGGGCCCCGGGAAAAAAAA"},
          scoring_system, paste_parameters)};
      right_plus.UngappedPrefixEnd(2);
      right_plus.UngappedSuffixBegin(20);
      Alignment right_minus{Alignment::FromStringFields(1,
          {std::to_string(135 + query_offset),
           std::to_string(159 + query_offset),
           std::to_string(1061 - subject_offset - left_sregion_length
                               - right_sregion_length),
           std::to_string(1035 - subject_offset - left_sregion_length
                               - right_sregion_length),
           "20", "4", "2", "4",
           "10000", "100000",
           "AAGAAAAAATTTTCCCC---AAAAAAAA",
           "AA-AAAAAAGGGGCCCCGGGAAAAAAAA"},
          scoring_system, paste_parameters)};
      right_minus.UngappedPrefixEnd(2);
      right_minus.UngappedSuffixBegin(20);

      THEN("Pasted alignment takes on left's ungapped prefix.") {
        int original_left_ungapped_prefix = left_plus.UngappedPrefixEnd();
        right_plus.PasteLeft(left_plus, config, scoring_system,
                             paste_parameters);
        CHECK(original_left_ungapped_prefix == right_plus.UngappedPrefixEnd());

        original_left_ungapped_prefix = left_minus.UngappedPrefixEnd();
        right_minus.PasteLeft(left_minus, config, scoring_system,
                              paste_parameters);
        CHECK(original_left_ungapped_prefix == right_minus.UngappedPrefixEnd());
      }
    }
  }

  GIVEN("Left alignment is ungapped.") {
    int left_length{32}, left_sregion_length{32};
    Alignment left_plus{Alignment::FromStringFields(0,
        {"101", "132", "1001", "1032",
         "24", "8", "0", "0",
         "10000", "100000",
         "AAAAAAAATTTTCCCCGGGGAAAAGGGGAAAA",
         "AAAAAAAACCCCCCCCGGGGAAAAAAAAAAAA"},
        scoring_system, paste_parameters)};
    Alignment left_minus{Alignment::FromStringFields(0,
        {"101", "132", "1032", "1001",
         "24", "8", "0", "0",
         "10000", "100000",
         "AAAAAAAATTTTCCCCGGGGAAAAGGGGAAAA",
         "AAAAAAAACCCCCCCCGGGGAAAAAAAAAAAA"},
        scoring_system, paste_parameters)};

    WHEN("Right alignment is ungapped and 0-shift.") {
      int right_length{24};
      int query_offset = GENERATE(-10, -5, 0, 5, 10);
      int subject_offset{query_offset};
      AlignmentConfiguration config{GetConfiguration(
          query_offset, subject_offset, left_length, right_length)};
      Alignment right_plus{Alignment::FromStringFields(1,
          {std::to_string(133 + query_offset),
           std::to_string(156 + query_offset),
           std::to_string(1033 + subject_offset),
           std::to_string(1056 + subject_offset),
           "20", "4", "0", "0",
           "10000", "100000",
           "AAAAAAAATTTTCCCCAAAAAAAA",
           "AAAAAAAAGGGGCCCCAAAAAAAA"},
          scoring_system, paste_parameters)};
      Alignment right_minus{Alignment::FromStringFields(1,
          {std::to_string(133 + query_offset),
           std::to_string(156 + query_offset),
           std::to_string(1056 - subject_offset - left_sregion_length
                               - right_length),
           std::to_string(1033 - subject_offset - left_sregion_length
                               - right_length),
           "20", "4", "0", "0",
           "10000", "100000",
           "AAAAAAAATTTTCCCCAAAAAAAA",
           "AAAAAAAAGGGGCCCCAAAAAAAA"},
          scoring_system, paste_parameters)};

      THEN("Pasted is ungapped.") {
        int expected_ungapped_prefix_end = config.pasted_length;
        right_plus.PasteLeft(left_plus, config, scoring_system,
                             paste_parameters);
        CHECK(expected_ungapped_prefix_end == right_plus.UngappedPrefixEnd());

        right_minus.PasteLeft(left_minus, config, scoring_system,
                              paste_parameters);
        CHECK(expected_ungapped_prefix_end == right_minus.UngappedPrefixEnd());
      }
    }

    WHEN("Right alignment's portion in pasted has no gap and 0-shift.") {
      int right_length{28}, right_sregion_length{27};
      int query_offset = GENERATE(-10, -8, -6);
      int subject_offset{query_offset};
      AlignmentConfiguration config{GetConfiguration(
          query_offset, subject_offset, left_length, right_length)};
      Alignment right_plus{Alignment::FromStringFields(1,
          {std::to_string(133 + query_offset),
           std::to_string(157 + query_offset),
           std::to_string(1033 + subject_offset),
           std::to_string(1059 + subject_offset),
           "20", "4", "2", "4",
           "10000", "100000",
           "A---AGAAAAAATTTTCCCCAAAAAAAA",
           "AGGGA-AAAAAAGGGGCCCCAAAAAAAA"},
          scoring_system, paste_parameters)};
      right_plus.UngappedPrefixEnd(1);
      right_plus.UngappedSuffixBegin(6);
      Alignment right_minus{Alignment::FromStringFields(1,
          {std::to_string(133 + query_offset),
           std::to_string(157 + query_offset),
           std::to_string(1059 - subject_offset - left_sregion_length
                               - right_sregion_length),
           std::to_string(1033 - subject_offset - left_sregion_length
                               - right_sregion_length),
           "20", "4", "2", "4",
           "10000", "100000",
           "A---AGAAAAAATTTTCCCCAAAAAAAA",
           "AGGGA-AAAAAAGGGGCCCCAAAAAAAA"},
          scoring_system, paste_parameters)};
      right_minus.UngappedPrefixEnd(1);
      right_minus.UngappedSuffixBegin(6);

      THEN("Pasted is ungapped.") {
        int expected_ungapped_prefix_end = config.pasted_length;
        right_plus.PasteLeft(left_plus, config, scoring_system,
                             paste_parameters);
        CHECK(expected_ungapped_prefix_end == right_plus.UngappedPrefixEnd());

        right_minus.PasteLeft(left_minus, config, scoring_system,
                              paste_parameters);
        CHECK(expected_ungapped_prefix_end == right_minus.UngappedPrefixEnd());
      }
    }

    WHEN("Right alignment's portion in pasted has left-most gap and 0-shift.") {
      int right_length{28}, right_sregion_length{27};
      int query_offset = GENERATE(-10, -5, 0, 5, 10);
      int subject_offset = {query_offset};
      AlignmentConfiguration config{GetConfiguration(
          query_offset, subject_offset, left_length, right_length)};
      Alignment right_plus{Alignment::FromStringFields(1,
          {std::to_string(133 + query_offset),
           std::to_string(157 + query_offset),
           std::to_string(1033 + subject_offset),
           std::to_string(1059 + subject_offset),
           "20", "4", "2", "4",
           "10000", "100000",
           "AAAAAAAATTTTGCCCC---AAAAAAAA",
           "AAAAAAAAGGGG-CCCCGGGAAAAAAAA"},
          scoring_system, paste_parameters)};
      right_plus.UngappedPrefixEnd(12);
      right_plus.UngappedSuffixBegin(20);
      Alignment right_minus{Alignment::FromStringFields(1,
          {std::to_string(133 + query_offset),
           std::to_string(157 + query_offset),
           std::to_string(1059 - subject_offset - left_sregion_length
                               - right_sregion_length),
           std::to_string(1033 - subject_offset - left_sregion_length
                               - right_sregion_length),
           "20", "4", "2", "4",
           "10000", "100000",
           "AAAAAAAATTTTGCCCC---AAAAAAAA",
           "AAAAAAAAGGGG-CCCCGGGAAAAAAAA"},
          scoring_system, paste_parameters)};
      right_minus.UngappedPrefixEnd(12);
      right_minus.UngappedSuffixBegin(20);

      THEN("Pasted alignment takes on right's ungapped prefix.") {
        int expected_ungapped_prefix_end{config.pasted_length
                                         - right_plus.Length()
                                         + right_plus.UngappedPrefixEnd()};
        right_plus.PasteLeft(left_plus, config, scoring_system,
                             paste_parameters);
        CHECK(expected_ungapped_prefix_end == right_plus.UngappedPrefixEnd());

        expected_ungapped_prefix_end = config.pasted_length
                                       - right_minus.Length()
                                       + right_minus.UngappedPrefixEnd();
        right_minus.PasteLeft(left_minus, config, scoring_system,
                              paste_parameters);
        CHECK(expected_ungapped_prefix_end == right_minus.UngappedPrefixEnd());
      }
    }

    WHEN("Right's portion in pasted has gap but not left-most and 0-shift.") {
      int right_length{28}, right_sregion_length{27};
      int query_offset = GENERATE(-10, -8, -5);
      int subject_offset{query_offset};
      AlignmentConfiguration config{GetConfiguration(
          query_offset, subject_offset, left_length, right_length)};
      Alignment right_plus{Alignment::FromStringFields(1,
          {std::to_string(133 + query_offset),
           std::to_string(157 + query_offset),
           std::to_string(1033 + subject_offset),
           std::to_string(1059 + subject_offset),
           "20", "4", "2", "4",
           "10000", "100000",
           "AAGAAAAAATTTTCCCC---AAAAAAAA",
           "AA-AAAAAAGGGGCCCCGGGAAAAAAAA"},
          scoring_system, paste_parameters)};
      right_plus.UngappedPrefixEnd(2);
      right_plus.UngappedSuffixBegin(20);
      Alignment right_minus{Alignment::FromStringFields(1,
          {std::to_string(133 + query_offset),
           std::to_string(157 + query_offset),
           std::to_string(1059 - subject_offset - left_sregion_length
                               - right_sregion_length),
           std::to_string(1033 - subject_offset - left_sregion_length
                               - right_sregion_length),
           "20", "4", "2", "4",
           "10000", "100000",
           "AAGAAAAAATTTTCCCC---AAAAAAAA",
           "AA-AAAAAAGGGGCCCCGGGAAAAAAAA"},
          scoring_system, paste_parameters)};
      right_minus.UngappedPrefixEnd(2);
      right_minus.UngappedSuffixBegin(20);

      THEN("Pasted's suffix begins where right's portion begins.") {
        int expected_ungapped_prefix_end{left_plus.Length()
                                         + std::max(0, query_offset)};
        right_plus.PasteLeft(left_plus, config, scoring_system,
                             paste_parameters);
        CHECK(expected_ungapped_prefix_end == right_plus.UngappedPrefixEnd());

        expected_ungapped_prefix_end = left_minus.Length()
                                       + std::max(0, query_offset);
        right_minus.PasteLeft(left_minus, config, scoring_system,
                              paste_parameters);
        CHECK(expected_ungapped_prefix_end == right_minus.UngappedPrefixEnd());
      }
    }

    WHEN("Right alignment is ungapped and non0-shift.") {
      int right_length{24};
      int query_offset = GENERATE(-10, -5, 0, 5, 10);
      int shift = GENERATE(-5, -2, -1, 1, 2, 5);
      int subject_offset{query_offset + shift};
      AlignmentConfiguration config{GetConfiguration(
          query_offset, subject_offset, left_length, right_length)};
      Alignment right_plus{Alignment::FromStringFields(1,
          {std::to_string(133 + query_offset),
           std::to_string(156 + query_offset),
           std::to_string(1033 + subject_offset),
           std::to_string(1056 + subject_offset),
           "20", "4", "0", "0",
           "10000", "100000",
           "AAAAAAAATTTTCCCCAAAAAAAA",
           "AAAAAAAAGGGGCCCCAAAAAAAA"},
          scoring_system, paste_parameters)};
      Alignment right_minus{Alignment::FromStringFields(1,
          {std::to_string(133 + query_offset),
           std::to_string(156 + query_offset),
           std::to_string(1056 - subject_offset - left_sregion_length
                               - right_length),
           std::to_string(1033 - subject_offset - left_sregion_length
                               - right_length),
           "20", "4", "0", "0",
           "10000", "100000",
           "AAAAAAAATTTTCCCCAAAAAAAA",
           "AAAAAAAAGGGGCCCCAAAAAAAA"},
          scoring_system, paste_parameters)};

      THEN("Pasted's prefix ends where inserted gap begins.") {
        int expected_ungapped_prefix_end{left_plus.Length()
                                         + std::min(config.query_distance,
                                                    config.subject_distance)};
        right_plus.PasteLeft(left_plus, config, scoring_system,
                             paste_parameters);
        CHECK(expected_ungapped_prefix_end == right_plus.UngappedPrefixEnd());

        expected_ungapped_prefix_end = left_minus.Length()
                                       + std::min(config.query_distance,
                                                  config.subject_distance);
        right_minus.PasteLeft(left_minus, config, scoring_system,
                              paste_parameters);
        CHECK(expected_ungapped_prefix_end == right_minus.UngappedPrefixEnd());
      }
    }

    WHEN("Right alignment's portion in pasted has no gap and non0-shift.") {
      int right_length{28}, right_sregion_length{27};
      int query_offset = GENERATE(-10, -8, -6);
      int shift = GENERATE(-5, -2, -1, 1, 2, 5);
      int subject_offset{query_offset + shift};
      AlignmentConfiguration config{GetConfiguration(
          query_offset, subject_offset, left_length, right_length)};
      Alignment right_plus{Alignment::FromStringFields(1,
          {std::to_string(133 + query_offset),
           std::to_string(157 + query_offset),
           std::to_string(1033 + subject_offset),
           std::to_string(1059 + subject_offset),
           "20", "4", "2", "4",
           "10000", "100000",
           "A---AGAAAAAATTTTCCCCAAAAAAAA",
           "AGGGA-AAAAAAGGGGCCCCAAAAAAAA"},
          scoring_system, paste_parameters)};
      right_plus.UngappedPrefixEnd(1);
      right_plus.UngappedSuffixBegin(6);
      Alignment right_minus{Alignment::FromStringFields(1,
          {std::to_string(133 + query_offset),
           std::to_string(157 + query_offset),
           std::to_string(1059 - subject_offset - left_sregion_length
                               - right_sregion_length),
           std::to_string(1033 - subject_offset - left_sregion_length
                               - right_sregion_length),
           "20", "4", "2", "4",
           "10000", "100000",
           "A---AGAAAAAATTTTCCCCAAAAAAAA",
           "AGGGA-AAAAAAGGGGCCCCAAAAAAAA"},
          scoring_system, paste_parameters)};
      right_minus.UngappedPrefixEnd(1);
      right_minus.UngappedSuffixBegin(6);

      THEN("Pasted's prefix ends where inserted gap begins.") {
        int expected_ungapped_prefix_end{left_plus.Length()
                                         + std::min(config.query_distance,
                                                    config.subject_distance)};
        right_plus.PasteLeft(left_plus, config, scoring_system,
                             paste_parameters);
        CHECK(expected_ungapped_prefix_end == right_plus.UngappedPrefixEnd());

        expected_ungapped_prefix_end = left_minus.Length()
                                       + std::min(config.query_distance,
                                                  config.subject_distance);
        right_minus.PasteLeft(left_minus, config, scoring_system,
                              paste_parameters);
        CHECK(expected_ungapped_prefix_end == right_minus.UngappedPrefixEnd());
      }
    }

    WHEN("Right's portion in pasted has left-most gap and non0-shift.") {
      int right_length{28}, right_sregion_length{27};
      int query_offset = GENERATE(-10, -5, 0, 5, 10);
      int shift = GENERATE(-2, -1, 1, 2, 5);
      int subject_offset{query_offset + shift};
      AlignmentConfiguration config{GetConfiguration(
          query_offset, subject_offset, left_length, right_length)};
      Alignment right_plus{Alignment::FromStringFields(1,
          {std::to_string(133 + query_offset),
           std::to_string(157 + query_offset),
           std::to_string(1033 + subject_offset),
           std::to_string(1059 + subject_offset),
           "20", "4", "2", "4",
           "10000", "100000",
           "AAAAAAAATTTTGCCCC---AAAAAAAA",
           "AAAAAAAAGGGG-CCCCGGGAAAAAAAA"},
          scoring_system, paste_parameters)};
      right_plus.UngappedPrefixEnd(12);
      right_plus.UngappedSuffixBegin(20);
      Alignment right_minus{Alignment::FromStringFields(1,
          {std::to_string(133 + query_offset),
           std::to_string(157 + query_offset),
           std::to_string(1059 - subject_offset - left_sregion_length
                               - right_sregion_length),
           std::to_string(1033 - subject_offset - left_sregion_length
                               - right_sregion_length),
           "20", "4", "2", "4",
           "10000", "100000",
           "AAAAAAAATTTTGCCCC---AAAAAAAA",
           "AAAAAAAAGGGG-CCCCGGGAAAAAAAA"},
          scoring_system, paste_parameters)};
      right_minus.UngappedPrefixEnd(12);
      right_minus.UngappedSuffixBegin(20);

      THEN("Pasted's prefix ends where inserted gap begins.") {
        int expected_ungapped_prefix_end{left_plus.Length()
                                         + std::min(config.query_distance,
                                                    config.subject_distance)};
        right_plus.PasteLeft(left_plus, config, scoring_system,
                             paste_parameters);
        CHECK(expected_ungapped_prefix_end == right_plus.UngappedPrefixEnd());

        expected_ungapped_prefix_end = left_minus.Length()
                                       + std::min(config.query_distance,
                                                  config.subject_distance);
        right_minus.PasteLeft(left_minus, config, scoring_system,
                              paste_parameters);
        CHECK(expected_ungapped_prefix_end == right_minus.UngappedPrefixEnd());
      }
    }

    WHEN("Right's portion in pasted has gap but not left-most; non0-shift.") {
      int right_length{28}, right_sregion_length{27};
      int query_offset = GENERATE(-10, -8, -5);
      int shift = GENERATE(-5, -2, -1, 1, 2, 5);
      int subject_offset{query_offset + shift};
      AlignmentConfiguration config{GetConfiguration(
          query_offset, subject_offset, left_length, right_length)};
      Alignment right_plus{Alignment::FromStringFields(1,
          {std::to_string(133 + query_offset),
           std::to_string(157 + query_offset),
           std::to_string(1033 + subject_offset),
           std::to_string(1059 + subject_offset),
           "20", "4", "2", "4",
           "10000", "100000",
           "AAGAAAAAATTTTCCCC---AAAAAAAA",
           "AA-AAAAAAGGGGCCCCGGGAAAAAAAA"},
          scoring_system, paste_parameters)};
      right_plus.UngappedPrefixEnd(2);
      right_plus.UngappedSuffixBegin(20);
      Alignment right_minus{Alignment::FromStringFields(1,
          {std::to_string(133 + query_offset),
           std::to_string(157 + query_offset),
           std::to_string(1059 - subject_offset - left_sregion_length
                               - right_sregion_length),
           std::to_string(1033 - subject_offset - left_sregion_length
                               - right_sregion_length),
           "20", "4", "2", "4",
           "10000", "100000",
           "AAGAAAAAATTTTCCCC---AAAAAAAA",
           "AA-AAAAAAGGGGCCCCGGGAAAAAAAA"},
          scoring_system, paste_parameters)};
      right_minus.UngappedPrefixEnd(2);
      right_minus.UngappedSuffixBegin(20);

      THEN("Pasted's prefix ends where inserted gap begins.") {
        int expected_ungapped_prefix_end{left_plus.Length()
                                         + std::min(config.query_distance,
                                                    config.subject_distance)};
        right_plus.PasteLeft(left_plus, config, scoring_system,
                             paste_parameters);
        CHECK(expected_ungapped_prefix_end == right_plus.UngappedPrefixEnd());

        expected_ungapped_prefix_end = left_minus.Length()
                                       + std::min(config.query_distance,
                                                  config.subject_distance);
        right_minus.PasteLeft(left_minus, config, scoring_system,
                              paste_parameters);
        CHECK(expected_ungapped_prefix_end == right_minus.UngappedPrefixEnd());
      }
    }
  }
}

SCENARIO("Test correctness of Alignment::PasteLeft <suffix>.",
         "[Alignment][PasteLeft][correctness][suffix]") {
  ScoringSystem scoring_system{ScoringSystem::Create(100000l, 1, 2, 1, 1)};
  PasteParameters paste_parameters;

  GIVEN("Right alignment's portion in pasted has gaps.") {
    int right_length{36}, right_sregion_length{34};
    int query_offset = GENERATE(-10, -5, 0, 5, 10);
    int subject_offset = GENERATE(-10, -5, 0, 5, 10);
    Alignment right_plus{Alignment::FromStringFields(0,
        {"101", "134", "1001", "1034",
         "24", "8", "2", "4",
         "10000", "100000",
         "AAAAAAAATTTTCCCCGGGGAA--AAGGGGAAGGAA",
         "AAAAAAAACCCCCCCCGGGGAAGGAAAAAAAA--AA"},
        scoring_system, paste_parameters)};
    right_plus.UngappedPrefixEnd(22);
    right_plus.UngappedSuffixBegin(34);
    Alignment right_minus{Alignment::FromStringFields(0,
        {"101", "134", "1034", "1001",
         "24", "8", "2", "4",
         "10000", "100000",
         "AAAAAAAATTTTCCCCGGGGAA--AAGGGGAAGGAA",
         "AAAAAAAACCCCCCCCGGGGAAGGAAAAAAAA--AA"},
        scoring_system, paste_parameters)};
    right_minus.UngappedPrefixEnd(22);
    right_minus.UngappedSuffixBegin(34);

    WHEN("Left alignment is ungapped.") {
      int left_length{24};
      AlignmentConfiguration config{GetConfiguration(
          query_offset, subject_offset, left_length, right_length)};
      Alignment left_plus{Alignment::FromStringFields(1,
          {std::to_string(77 - query_offset),
           std::to_string(100 - query_offset),
           std::to_string(977 - subject_offset),
           std::to_string(1000 - subject_offset),
           "20", "4", "0", "0",
           "10000", "100000",
           "AAAAAAAATTTTCCCCAAAAAAAA",
           "AAAAAAAAGGGGCCCCAAAAAAAA"},
          scoring_system, paste_parameters)};
      Alignment left_minus{Alignment::FromStringFields(1,
          {std::to_string(77 - query_offset),
           std::to_string(100 - query_offset),
           std::to_string(1000 + subject_offset + right_sregion_length
                               + left_length),
           std::to_string(977 + subject_offset + right_sregion_length
                              + left_length),
           "20", "4", "0", "0",
           "10000", "100000",
           "AAAAAAAATTTTCCCCAAAAAAAA",
           "AAAAAAAAGGGGCCCCAAAAAAAA"},
          scoring_system, paste_parameters)};

      THEN("Pasted alignment takes on right's ungapped suffix.") {
        int expected_ungapped_suffix_begin{config.pasted_length
                                           - right_plus.Length()
                                           + right_plus.UngappedSuffixBegin()};
        right_plus.PasteLeft(left_plus, config, scoring_system,
                             paste_parameters);
        CHECK(expected_ungapped_suffix_begin
              == right_plus.UngappedSuffixBegin());

        expected_ungapped_suffix_begin = config.pasted_length
                                         - right_minus.Length()
                                         + right_minus.UngappedSuffixBegin();
        right_minus.PasteLeft(left_minus, config, scoring_system,
                              paste_parameters);
        CHECK(expected_ungapped_suffix_begin
              == right_minus.UngappedSuffixBegin());
      }
    }

    WHEN("Left alignment has gaps.") {
      int left_length{28}, left_sregion_length{27};
      AlignmentConfiguration config{GetConfiguration(
          query_offset, subject_offset, left_length, right_length)};
      Alignment left_plus{Alignment::FromStringFields(1,
          {std::to_string(76 - query_offset),
           std::to_string(100 - query_offset),
           std::to_string(974 - subject_offset),
           std::to_string(1000 - subject_offset),
           "20", "4", "2", "4",
           "10000", "100000",
           "AAGAAAAA---ATTTTCCCCAAAAAAAA",
           "AA-AAAAAGGGAGGGGCCCCAAAAAAAA"},
          scoring_system, paste_parameters)};
      left_plus.UngappedPrefixEnd(2);
      left_plus.UngappedSuffixBegin(11);
      Alignment left_minus{Alignment::FromStringFields(1,
          {std::to_string(76 - query_offset),
           std::to_string(100 - query_offset),
           std::to_string(1000 + subject_offset + right_sregion_length
                               + left_sregion_length),
           std::to_string(974 + subject_offset + right_sregion_length
                              + left_sregion_length),
           "20", "4", "2", "4",
           "10000", "100000",
           "AAGAAAAA---ATTTTCCCCAAAAAAAA",
           "AA-AAAAAGGGAGGGGCCCCAAAAAAAA"},
          scoring_system, paste_parameters)};
      left_minus.UngappedPrefixEnd(2);
      left_minus.UngappedSuffixBegin(11);

      THEN("Pasted alignment takes on right's ungapped suffix.") {
        int expected_ungapped_suffix_begin{config.pasted_length
                                           - right_plus.Length()
                                           + right_plus.UngappedSuffixBegin()};
        right_plus.PasteLeft(left_plus, config, scoring_system,
                             paste_parameters);
        CHECK(expected_ungapped_suffix_begin
              == right_plus.UngappedSuffixBegin());

        expected_ungapped_suffix_begin = config.pasted_length
                                         - right_minus.Length()
                                         + right_minus.UngappedSuffixBegin();
        right_minus.PasteLeft(left_minus, config, scoring_system,
                              paste_parameters);
        CHECK(expected_ungapped_suffix_begin
              == right_minus.UngappedSuffixBegin());
      }
    }
  }

  GIVEN("Right alignment is ungapped and 0-shift.") {
    int right_length{32}, right_sregion_length{32};
    int query_offset = GENERATE(-10, -5, 0, 5, 10);
    int subject_offset{query_offset};
    Alignment right_plus{Alignment::FromStringFields(0,
        {"101", "132", "1001", "1032",
         "24", "8", "0", "0",
         "10000", "100000",
         "AAAAAAAATTTTCCCCGGGGAAAAGGGGAAAA",
         "AAAAAAAACCCCCCCCGGGGAAAAAAAAAAAA"},
        scoring_system, paste_parameters)};
    Alignment right_minus{Alignment::FromStringFields(0,
        {"101", "132", "1032", "1001",
         "24", "8", "0", "0",
         "10000", "100000",
         "AAAAAAAATTTTCCCCGGGGAAAAGGGGAAAA",
         "AAAAAAAACCCCCCCCGGGGAAAAAAAAAAAA"},
        scoring_system, paste_parameters)};

    WHEN("Left alignment is ungapped.") {
      int left_length{24};
      AlignmentConfiguration config{GetConfiguration(
          query_offset, subject_offset, left_length, right_length)};
      Alignment left_plus{Alignment::FromStringFields(1,
          {std::to_string(77 - query_offset),
           std::to_string(100 - query_offset),
           std::to_string(977 - subject_offset),
           std::to_string(1000 - subject_offset),
           "20", "4", "0", "0",
           "10000", "100000",
           "AAAAAAAATTTTCCCCAAAAAAAA",
           "AAAAAAAAGGGGCCCCAAAAAAAA"},
          scoring_system, paste_parameters)};
      Alignment left_minus{Alignment::FromStringFields(1,
          {std::to_string(77 - query_offset),
           std::to_string(100 - query_offset),
           std::to_string(1000 + subject_offset + right_sregion_length
                               + left_length),
           std::to_string(977 + subject_offset + right_sregion_length
                              + left_length),
           "20", "4", "0", "0",
           "10000", "100000",
           "AAAAAAAATTTTCCCCAAAAAAAA",
           "AAAAAAAAGGGGCCCCAAAAAAAA"},
          scoring_system, paste_parameters)};

      THEN("Pasted alignment is ungapped.") {
        right_plus.PasteLeft(left_plus, config, scoring_system,
                             paste_parameters);
        CHECK(0 == right_plus.UngappedSuffixBegin());

        right_minus.PasteLeft(left_minus, config, scoring_system,
                              paste_parameters);
        CHECK(0 == right_minus.UngappedSuffixBegin());
      }
    }

    WHEN("Left alignment has gaps.") {
      int left_length{28}, left_sregion_length{27};
      AlignmentConfiguration config{GetConfiguration(
          query_offset, subject_offset, left_length, right_length)};
      Alignment left_plus{Alignment::FromStringFields(1,
          {std::to_string(76 - query_offset),
           std::to_string(100 - query_offset),
           std::to_string(974 - subject_offset),
           std::to_string(1000 - subject_offset),
           "20", "4", "2", "4",
           "10000", "100000",
           "AAAAAAAATTTTCCCCAAAAAAGA---A",
           "AAAAAAAAGGGGCCCCAAAAAA-AGGGA"},
          scoring_system, paste_parameters)};
      left_plus.UngappedPrefixEnd(22);
      left_plus.UngappedSuffixBegin(27);
      Alignment left_minus{Alignment::FromStringFields(1,
          {std::to_string(76 - query_offset),
           std::to_string(100 - query_offset),
           std::to_string(1000 + subject_offset + right_sregion_length
                               + left_sregion_length),
           std::to_string(974 + subject_offset + right_sregion_length
                              + left_sregion_length),
           "20", "4", "2", "4",
           "10000", "100000",
           "AAAAAAAATTTTCCCCAAAAAAGA---A",
           "AAAAAAAAGGGGCCCCAAAAAA-AGGGA"},
          scoring_system, paste_parameters)};
      left_minus.UngappedPrefixEnd(22);
      left_minus.UngappedSuffixBegin(27);

      THEN("Pasted's suffix includes left's suffix.") {
        int expected_ungapped_suffix_begin{left_plus.UngappedSuffixBegin()};
        right_plus.PasteLeft(left_plus, config, scoring_system,
                             paste_parameters);
        CHECK(expected_ungapped_suffix_begin
              == right_plus.UngappedSuffixBegin());

        expected_ungapped_suffix_begin = left_minus.UngappedSuffixBegin();
        right_minus.PasteLeft(left_minus, config, scoring_system,
                               paste_parameters);
        CHECK(expected_ungapped_suffix_begin
              == right_minus.UngappedSuffixBegin());
      }
    }
  }

  GIVEN("Right alignment is ungapped and non0-shift.") {
    int right_length{32}, right_sregion_length{32};
    int query_offset = GENERATE(-10, -5, 0, 5, 10);
    int shift = GENERATE(-5, -2, -1, 1, 2, 5);
    int subject_offset{query_offset + shift};
    Alignment right_plus{Alignment::FromStringFields(0,
        {"101", "132", "1001", "1032",
         "24", "8", "0", "0",
         "10000", "100000",
         "AAAAAAAATTTTCCCCGGGGAAAAGGGGAAAA",
         "AAAAAAAACCCCCCCCGGGGAAAAAAAAAAAA"},
        scoring_system, paste_parameters)};
    Alignment right_minus{Alignment::FromStringFields(0,
        {"101", "132", "1032", "1001",
         "24", "8", "0", "0",
         "10000", "100000",
         "AAAAAAAATTTTCCCCGGGGAAAAGGGGAAAA",
         "AAAAAAAACCCCCCCCGGGGAAAAAAAAAAAA"},
        scoring_system, paste_parameters)};

    WHEN("Left alignment is ungapped.") {
      int left_length{24};
      AlignmentConfiguration config{GetConfiguration(
          query_offset, subject_offset, left_length, right_length)};
      Alignment left_plus{Alignment::FromStringFields(1,
          {std::to_string(77 - query_offset),
           std::to_string(100 - query_offset),
           std::to_string(977 - subject_offset),
           std::to_string(1000 - subject_offset),
           "20", "4", "0", "0",
           "10000", "100000",
           "AAAAAAAATTTTCCCCAAAAAAAA",
           "AAAAAAAAGGGGCCCCAAAAAAAA"},
          scoring_system, paste_parameters)};
      Alignment left_minus{Alignment::FromStringFields(1,
          {std::to_string(77 - query_offset),
           std::to_string(100 - query_offset),
           std::to_string(1000 + subject_offset + right_sregion_length
                               + left_length),
           std::to_string(977 + subject_offset + right_sregion_length
                              + left_length),
           "20", "4", "0", "0",
           "10000", "100000",
           "AAAAAAAATTTTCCCCAAAAAAAA",
           "AAAAAAAAGGGGCCCCAAAAAAAA"},
          scoring_system, paste_parameters)};

      THEN("Pasted ungapped suffix begins at end of introduced gap.") {
        int expected_ungapped_suffix_begin{left_plus.Length()
                                           + std::abs(shift)
                                           + std::min(config.query_distance,
                                                      config.subject_distance)};
        right_plus.PasteLeft(left_plus, config, scoring_system,
                             paste_parameters);
        CHECK(expected_ungapped_suffix_begin
              == right_plus.UngappedSuffixBegin());

        expected_ungapped_suffix_begin = left_minus.Length()
                                         + std::abs(shift)
                                         + std::min(config.query_distance,
                                                    config.subject_distance);
        right_minus.PasteLeft(left_minus, config, scoring_system,
                              paste_parameters);
        CHECK(expected_ungapped_suffix_begin
              == right_minus.UngappedSuffixBegin());
      }
    }

    WHEN("Left alignment has gaps.") {
      int left_length{28}, left_sregion_length{27};
      AlignmentConfiguration config{GetConfiguration(
          query_offset, subject_offset, left_length, right_length)};
      Alignment left_plus{Alignment::FromStringFields(1,
          {std::to_string(76 - query_offset),
           std::to_string(100 - query_offset),
           std::to_string(974 - subject_offset),
           std::to_string(1000 - subject_offset),
           "20", "4", "2", "4",
           "10000", "100000",
           "AAAAAAAATTTTCCCCAAAAAAGA---A",
           "AAAAAAAAGGGGCCCCAAAAAA-AGGGA"},
          scoring_system, paste_parameters)};
      left_plus.UngappedPrefixEnd(22);
      left_plus.UngappedSuffixBegin(27);
      Alignment left_minus{Alignment::FromStringFields(1,
          {std::to_string(76 - query_offset),
           std::to_string(100 - query_offset),
           std::to_string(1000 + subject_offset + right_sregion_length
                               + left_sregion_length),
           std::to_string(974 + subject_offset + right_sregion_length
                              + left_sregion_length),
           "20", "4", "2", "4",
           "10000", "100000",
           "AAAAAAAATTTTCCCCAAAAAAGA---A",
           "AAAAAAAAGGGGCCCCAAAAAA-AGGGA"},
          scoring_system, paste_parameters)};
      left_minus.UngappedPrefixEnd(22);
      left_minus.UngappedSuffixBegin(27);

      THEN("Pasted ungapped suffix begins at end of introduced gap.") {
        int expected_ungapped_suffix_begin{left_plus.Length()
                                           + std::abs(shift)
                                           + std::min(config.query_distance,
                                                      config.subject_distance)};
        right_plus.PasteLeft(left_plus, config, scoring_system,
                             paste_parameters);
        CHECK(expected_ungapped_suffix_begin
              == right_plus.UngappedSuffixBegin());

        expected_ungapped_suffix_begin = left_minus.Length()
                                         + std::abs(shift)
                                         + std::min(config.query_distance,
                                                    config.subject_distance);
        right_minus.PasteLeft(left_minus, config, scoring_system,
                              paste_parameters);
        CHECK(expected_ungapped_suffix_begin
              == right_minus.UngappedSuffixBegin());
      }
    }
  }

  GIVEN("Right alignment's portion in pasted is ungapped and 0-shift.") {
    int right_length{36}, right_sregion_length{33};
    int query_offset = GENERATE(-10, -8, -6);
    int subject_offset{query_offset};
    Alignment right_plus{Alignment::FromStringFields(0,
        {"101", "135", "1001", "1033",
         "24", "8", "2", "4",
         "10000", "100000",
         "A-AGGGAAAAAATTTTCCCCGGGGAAAAGGGGAAAA",
         "AGA---AAAAAACCCCCCCCGGGGAAAAAAAAAAAA"},
        scoring_system, paste_parameters)};
    right_plus.UngappedPrefixEnd(1);
    right_plus.UngappedSuffixBegin(6);
    Alignment right_minus{Alignment::FromStringFields(0,
        {"101", "135", "1033", "1001",
         "24", "8", "2", "4",
         "10000", "100000",
         "A-AGGGAAAAAATTTTCCCCGGGGAAAAGGGGAAAA",
         "AGA---AAAAAACCCCCCCCGGGGAAAAAAAAAAAA"},
        scoring_system, paste_parameters)};
    right_minus.UngappedPrefixEnd(1);
    right_minus.UngappedSuffixBegin(6);

    WHEN("Left alignment is ungapped.") {
      int left_length{24};
      AlignmentConfiguration config{GetConfiguration(
          query_offset, subject_offset, left_length, right_length)};
      Alignment left_plus{Alignment::FromStringFields(1,
          {std::to_string(77 - query_offset),
           std::to_string(100 - query_offset),
           std::to_string(977 - subject_offset),
           std::to_string(1000 - subject_offset),
           "20", "4", "0", "0",
           "10000", "100000",
           "AAAAAAAATTTTCCCCAAAAAAAA",
           "AAAAAAAAGGGGCCCCAAAAAAAA"},
          scoring_system, paste_parameters)};
      Alignment left_minus{Alignment::FromStringFields(1,
          {std::to_string(77 - query_offset),
           std::to_string(100 - query_offset),
           std::to_string(1000 + subject_offset + right_sregion_length
                               + left_length),
           std::to_string(977 + subject_offset + right_sregion_length
                              + left_length),
           "20", "4", "0", "0",
           "10000", "100000",
           "AAAAAAAATTTTCCCCAAAAAAAA",
           "AAAAAAAAGGGGCCCCAAAAAAAA"},
          scoring_system, paste_parameters)};

      THEN("Pasted alignment is ungapped.") {
        right_plus.PasteLeft(left_plus, config, scoring_system,
                             paste_parameters);
        CHECK(0 == right_plus.UngappedSuffixBegin());

        right_minus.PasteLeft(left_minus, config, scoring_system,
                              paste_parameters);
        CHECK(0 == right_minus.UngappedSuffixBegin());
      }
    }

    WHEN("Left alignment has gaps.") {
      int left_length{28}, left_sregion_length{27};
      AlignmentConfiguration config{GetConfiguration(
          query_offset, subject_offset, left_length, right_length)};
      Alignment left_plus{Alignment::FromStringFields(1,
          {std::to_string(76 - query_offset),
           std::to_string(100 - query_offset),
           std::to_string(974 - subject_offset),
           std::to_string(1000 - subject_offset),
           "20", "4", "2", "4",
           "10000", "100000",
           "AAAAAAAATTTTCCCCAAAAAAGA---A",
           "AAAAAAAAGGGGCCCCAAAAAA-AGGGA"},
          scoring_system, paste_parameters)};
      left_plus.UngappedPrefixEnd(22);
      left_plus.UngappedSuffixBegin(27);
      Alignment left_minus{Alignment::FromStringFields(1,
          {std::to_string(76 - query_offset),
           std::to_string(100 - query_offset),
           std::to_string(1000 + subject_offset + right_sregion_length
                               + left_sregion_length),
           std::to_string(974 + subject_offset + right_sregion_length
                              + left_sregion_length),
           "20", "4", "2", "4",
           "10000", "100000",
           "AAAAAAAATTTTCCCCAAAAAAGA---A",
           "AAAAAAAAGGGGCCCCAAAAAA-AGGGA"},
          scoring_system, paste_parameters)};
      left_minus.UngappedPrefixEnd(22);
      left_minus.UngappedSuffixBegin(27);

      THEN("Pasted's suffix includes left's suffix.") {
        int expected_ungapped_suffix_begin{left_plus.UngappedSuffixBegin()};
        right_plus.PasteLeft(left_plus, config, scoring_system,
                             paste_parameters);
        CHECK(expected_ungapped_suffix_begin
              == right_plus.UngappedSuffixBegin());

        expected_ungapped_suffix_begin = left_minus.UngappedSuffixBegin();
        right_minus.PasteLeft(left_minus, config, scoring_system,
                              paste_parameters);
        CHECK(expected_ungapped_suffix_begin
              == right_minus.UngappedSuffixBegin());
      }
    }
  }

  GIVEN("Right alignment's portion in pasted is ungapped and non0-shift.") {
    int right_length{36}, right_sregion_length{33};
    int query_offset = GENERATE(-10, -8, -6);
    int shift = GENERATE(-5, -2, -1, 1, 2, 5);
    int subject_offset{query_offset + shift};
    Alignment right_plus{Alignment::FromStringFields(0,
        {"101", "135", "1001", "1033",
         "24", "8", "2", "4",
         "10000", "100000",
         "A-AGGGAAAAAATTTTCCCCGGGGAAAAGGGGAAAA",
         "AGA---AAAAAACCCCCCCCGGGGAAAAAAAAAAAA"},
        scoring_system, paste_parameters)};
    right_plus.UngappedPrefixEnd(1);
    right_plus.UngappedSuffixBegin(6);
    Alignment right_minus{Alignment::FromStringFields(0,
        {"101", "135", "1033", "1001",
         "24", "8", "2", "4",
         "10000", "100000",
         "A-AGGGAAAAAATTTTCCCCGGGGAAAAGGGGAAAA",
         "AGA---AAAAAACCCCCCCCGGGGAAAAAAAAAAAA"},
        scoring_system, paste_parameters)};
    right_minus.UngappedPrefixEnd(1);
    right_minus.UngappedSuffixBegin(6);

    WHEN("Left alignment is ungapped.") {
      int left_length{24};
      AlignmentConfiguration config{GetConfiguration(
          query_offset, subject_offset, left_length, right_length)};
      Alignment left_plus{Alignment::FromStringFields(1,
          {std::to_string(77 - query_offset),
           std::to_string(100 - query_offset),
           std::to_string(977 - subject_offset),
           std::to_string(1000 - subject_offset),
           "20", "4", "0", "0",
           "10000", "100000",
           "AAAAAAAATTTTCCCCAAAAAAAA",
           "AAAAAAAAGGGGCCCCAAAAAAAA"},
          scoring_system, paste_parameters)};
      Alignment left_minus{Alignment::FromStringFields(1,
          {std::to_string(77 - query_offset),
           std::to_string(100 - query_offset),
           std::to_string(1000 + subject_offset + right_sregion_length
                               + left_length),
           std::to_string(977 + subject_offset + right_sregion_length
                              + left_length),
           "20", "4", "0", "0",
           "10000", "100000",
           "AAAAAAAATTTTCCCCAAAAAAAA",
           "AAAAAAAAGGGGCCCCAAAAAAAA"},
          scoring_system, paste_parameters)};

      THEN("Pasted ungapped suffix begins at end of introduced gap.") {
        int expected_ungapped_suffix_begin{left_plus.Length()
                                           + std::abs(shift)
                                           + std::min(config.query_distance,
                                                      config.subject_distance)};
        right_plus.PasteLeft(left_plus, config, scoring_system,
                             paste_parameters);
        CHECK(expected_ungapped_suffix_begin
              == right_plus.UngappedSuffixBegin());

        expected_ungapped_suffix_begin = left_minus.Length()
                                         + std::abs(shift)
                                         + std::min(config.query_distance,
                                                    config.subject_distance);
        right_minus.PasteLeft(left_minus, config, scoring_system,
                              paste_parameters);
        CHECK(expected_ungapped_suffix_begin
              == right_minus.UngappedSuffixBegin());
      }
    }

    WHEN("Left alignment has gaps.") {
      int left_length{28}, left_sregion_length{27};
      AlignmentConfiguration config{GetConfiguration(
          query_offset, subject_offset, left_length, right_length)};
      Alignment left_plus{Alignment::FromStringFields(1,
          {std::to_string(76 - query_offset),
           std::to_string(100 - query_offset),
           std::to_string(974 - subject_offset),
           std::to_string(1000 - subject_offset),
           "20", "4", "2", "4",
           "10000", "100000",
           "AAAAAAAATTTTCCCCAAAAAAGA---A",
           "AAAAAAAAGGGGCCCCAAAAAA-AGGGA"},
          scoring_system, paste_parameters)};
      left_plus.UngappedPrefixEnd(22);
      left_plus.UngappedSuffixBegin(27);
      Alignment left_minus{Alignment::FromStringFields(1,
          {std::to_string(76 - query_offset),
           std::to_string(100 - query_offset),
           std::to_string(1000 + subject_offset + right_sregion_length
                               + left_sregion_length),
           std::to_string(974 + subject_offset + right_sregion_length
                              + left_sregion_length),
           "20", "4", "2", "4",
           "10000", "100000",
           "AAAAAAAATTTTCCCCAAAAAAGA---A",
           "AAAAAAAAGGGGCCCCAAAAAA-AGGGA"},
          scoring_system, paste_parameters)};
      left_minus.UngappedPrefixEnd(22);
      left_minus.UngappedSuffixBegin(27);

      THEN("Pasted ungapped suffix begins at end of introduced gap.") {
        int expected_ungapped_suffix_begin{left_plus.Length()
                                           + std::abs(shift)
                                           + std::min(config.query_distance,
                                                      config.subject_distance)};
        right_plus.PasteLeft(left_plus, config, scoring_system,
                             paste_parameters);
        CHECK(expected_ungapped_suffix_begin
              == right_plus.UngappedSuffixBegin());

        expected_ungapped_suffix_begin = left_minus.Length()
                                         + std::abs(shift)
                                         + std::min(config.query_distance,
                                                    config.subject_distance);
        right_minus.PasteLeft(left_minus, config, scoring_system,
                              paste_parameters);
        CHECK(expected_ungapped_suffix_begin
              == right_minus.UngappedSuffixBegin());
      }
    }
  }
}

SCENARIO("Test correctness of Alignment::PasteLeft <main>.",
         "[Alignment][PasteLeft][correctness][main]") {
  ScoringSystem scoring_system{ScoringSystem::Create(100000l, 1, 2, 1, 1)};
  PasteParameters paste_parameters;

  GIVEN("Data for alignments whose pasting won't alter prefix/suffix length.") {
    int left_qstart{101}, left_qend{134},
        left_sstart{1001}, left_send{1036},
        right_qstart{135}, right_qend{160},
        right_sstart{1037}, right_send{1060},
        left_nident{24}, left_mismatch{8}, left_gapopen{2}, left_gaps{6},
        right_nident{20}, right_mismatch{4}, right_gapopen{1}, right_gaps{2},
        qlen{10000}, slen{100000};
    std::string left_qseq{"AAAAAAAATTTTTTCCCCGGGG----GGGGAAAAAAAA"},
                left_sseq{"AAAAAAAACCCC--CCCCGGGGAAAATTTTAAAAAAAA"},
                right_qseq{"AAAAAAAATTTTCCCCGGAAAAAAAA"},
                right_sseq{"AAAAAAAAGGGGCCCC--AAAAAAAA"};
    int left_prefix_end{12}, left_suffix_begin{26},
        right_prefix_end{16}, right_suffix_begin{18};

    WHEN("query_offset == subject_offset == 0; +strand") {
      // Specify test case parameters.
      int query_offset{0}, subject_offset{0};
      bool plus_strand{true};
      HandleOffset(query_offset, subject_offset, plus_strand,
                   left_sstart, left_send, right_qstart, right_qend,
                   right_sstart, right_send);
      // Specify expected pasted outcome.
      int pasted_qstart{left_qstart}, pasted_qend{right_qend},
          pasted_sstart{left_sstart}, pasted_send{right_send},
          pasted_nident{left_nident + right_nident},
          pasted_mismatch{left_mismatch + right_mismatch},
          pasted_gapopen{left_gapopen + right_gapopen},
          pasted_gaps{left_gaps + right_gaps},
          pasted_prefix_end{left_prefix_end},
          pasted_suffix_begin{right_suffix_begin
                              + static_cast<int>(left_qseq.length())};
      std::string pasted_qseq, pasted_sseq;
      pasted_qseq = left_qseq;
      pasted_qseq.append(right_qseq);
      pasted_sseq = left_sseq;
      pasted_sseq.append(right_sseq);

      // Swapped subject coordinates indicate minus strand.
      if (!plus_strand) {
        std::swap(left_sstart, left_send);
        std::swap(right_sstart, right_send);
      }

      Alignment left{Alignment::FromStringFields(0,
        {std::to_string(left_qstart), std::to_string(left_qend),
         std::to_string(left_sstart), std::to_string(left_send),
         std::to_string(left_nident), std::to_string(left_mismatch),
         std::to_string(left_gapopen), std::to_string(left_gaps),
         std::to_string(qlen), std::to_string(slen),
         left_qseq, left_sseq},
        scoring_system, paste_parameters)};
      left.UngappedPrefixEnd(left_prefix_end);
      left.UngappedSuffixBegin(left_suffix_begin);
      Alignment right{Alignment::FromStringFields(1,
        {std::to_string(right_qstart), std::to_string(right_qend),
         std::to_string(right_sstart), std::to_string(right_send),
         std::to_string(right_nident), std::to_string(right_mismatch),
         std::to_string(right_gapopen), std::to_string(right_gaps),
         std::to_string(qlen), std::to_string(slen),
         right_qseq, right_sseq},
        scoring_system, paste_parameters)};
      right.UngappedPrefixEnd(right_prefix_end);
      right.UngappedSuffixBegin(right_suffix_begin);

      AlignmentConfiguration config{GetConfiguration(
          query_offset, subject_offset,
          left_qseq.length(), right_qseq.length())};

      Alignment original_left{left};
      right.PasteLeft(left, config, scoring_system, paste_parameters);

      THEN("Right becomes the pasted alignment.") {
        CHECK(right.Qstart() == pasted_qstart);
        CHECK(right.Qend() == pasted_qend);
        CHECK(right.Sstart() == pasted_sstart);
        CHECK(right.Send() == pasted_send);
        CHECK(right.PlusStrand() == plus_strand);
        CHECK(right.Nident() == pasted_nident);
        CHECK(right.Mismatch() == pasted_mismatch);
        CHECK(right.Gapopen() == pasted_gapopen);
        CHECK(right.Gaps() == pasted_gaps);
        CHECK(right.Qlen() == qlen);
        CHECK(right.Slen() == slen);
        CHECK(right.Qseq() == pasted_qseq);
        CHECK(right.Sseq() == pasted_sseq);
        CHECK(right.IncludeInOutput() == false);
        CHECK(right.UngappedPrefixEnd() == pasted_prefix_end);
        CHECK(right.UngappedSuffixBegin() == pasted_suffix_begin);
      }

      THEN("Left remains unchanged.") {
        CHECK(left == original_left);
      }
    }

    WHEN("query_offset == subject_offset == 0; -strand") {
      // Specify test case parameters.
      int query_offset{0}, subject_offset{0};
      bool plus_strand{false};
      HandleOffset(query_offset, subject_offset, plus_strand,
                   left_sstart, left_send, right_qstart, right_qend,
                   right_sstart, right_send);
      // Specify expected pasted outcome.
      int pasted_qstart{left_qstart}, pasted_qend{right_qend},
          pasted_sstart{right_sstart}, pasted_send{left_send},
          pasted_nident{left_nident + right_nident},
          pasted_mismatch{left_mismatch + right_mismatch},
          pasted_gapopen{left_gapopen + right_gapopen},
          pasted_gaps{left_gaps + right_gaps},
          pasted_prefix_end{left_prefix_end},
          pasted_suffix_begin{right_suffix_begin
                              + static_cast<int>(left_qseq.length())};
      std::string pasted_qseq, pasted_sseq;
      pasted_qseq = left_qseq;
      pasted_qseq.append(right_qseq);
      pasted_sseq = left_sseq;
      pasted_sseq.append(right_sseq);

      // Swapped subject coordinates indicate minus strand.
      if (!plus_strand) {
        std::swap(left_sstart, left_send);
        std::swap(right_sstart, right_send);
      }

      Alignment left{Alignment::FromStringFields(0,
        {std::to_string(left_qstart), std::to_string(left_qend),
         std::to_string(left_sstart), std::to_string(left_send),
         std::to_string(left_nident), std::to_string(left_mismatch),
         std::to_string(left_gapopen), std::to_string(left_gaps),
         std::to_string(qlen), std::to_string(slen),
         left_qseq, left_sseq},
        scoring_system, paste_parameters)};
      left.UngappedPrefixEnd(left_prefix_end);
      left.UngappedSuffixBegin(left_suffix_begin);
      Alignment right{Alignment::FromStringFields(1,
        {std::to_string(right_qstart), std::to_string(right_qend),
         std::to_string(right_sstart), std::to_string(right_send),
         std::to_string(right_nident), std::to_string(right_mismatch),
         std::to_string(right_gapopen), std::to_string(right_gaps),
         std::to_string(qlen), std::to_string(slen),
         right_qseq, right_sseq},
        scoring_system, paste_parameters)};
      right.UngappedPrefixEnd(right_prefix_end);
      right.UngappedSuffixBegin(right_suffix_begin);

      AlignmentConfiguration config{GetConfiguration(
          query_offset, subject_offset,
          left_qseq.length(), right_qseq.length())};

      Alignment original_left{left};
      right.PasteLeft(left, config, scoring_system, paste_parameters);

      THEN("Right becomes the pasted alignment.") {
        CHECK(right.Qstart() == pasted_qstart);
        CHECK(right.Qend() == pasted_qend);
        CHECK(right.Sstart() == pasted_sstart);
        CHECK(right.Send() == pasted_send);
        CHECK(right.PlusStrand() == plus_strand);
        CHECK(right.Nident() == pasted_nident);
        CHECK(right.Mismatch() == pasted_mismatch);
        CHECK(right.Gapopen() == pasted_gapopen);
        CHECK(right.Gaps() == pasted_gaps);
        CHECK(right.Qlen() == qlen);
        CHECK(right.Slen() == slen);
        CHECK(right.Qseq() == pasted_qseq);
        CHECK(right.Sseq() == pasted_sseq);
        CHECK(right.IncludeInOutput() == false);
        CHECK(right.UngappedPrefixEnd() == pasted_prefix_end);
        CHECK(right.UngappedSuffixBegin() == pasted_suffix_begin);
      }

      THEN("Left remains unchanged.") {
        CHECK(left == original_left);
      }
    }

    WHEN("query_offset == subject_offset > 0; +strand") {
      // Specify test case parameters.
      int query_offset{10}, subject_offset{10};
      bool plus_strand{true};
      HandleOffset(query_offset, subject_offset, plus_strand,
                   left_sstart, left_send, right_qstart, right_qend,
                   right_sstart, right_send);
      // Specify expected pasted outcome.
      int pasted_qstart{left_qstart}, pasted_qend{right_qend},
          pasted_sstart{left_sstart}, pasted_send{right_send},
          pasted_nident{left_nident + right_nident},
          pasted_mismatch{left_mismatch + right_mismatch + query_offset},
          pasted_gapopen{left_gapopen + right_gapopen},
          pasted_gaps{left_gaps + right_gaps},
          pasted_prefix_end{left_prefix_end},
          pasted_suffix_begin{right_suffix_begin + query_offset
                              + static_cast<int>(left_qseq.length())};

      std::string pasted_qseq, pasted_sseq;
      pasted_qseq = left_qseq;
      pasted_qseq.append(query_offset, 'N');
      pasted_qseq.append(right_qseq);

      pasted_sseq = left_sseq;
      pasted_sseq.append(subject_offset, 'N');
      pasted_sseq.append(right_sseq);

      // Swapped subject coordinates indicate minus strand.
      if (!plus_strand) {
        std::swap(left_sstart, left_send);
        std::swap(right_sstart, right_send);
      }

      Alignment left{Alignment::FromStringFields(0,
        {std::to_string(left_qstart), std::to_string(left_qend),
         std::to_string(left_sstart), std::to_string(left_send),
         std::to_string(left_nident), std::to_string(left_mismatch),
         std::to_string(left_gapopen), std::to_string(left_gaps),
         std::to_string(qlen), std::to_string(slen),
         left_qseq, left_sseq},
        scoring_system, paste_parameters)};
      left.UngappedPrefixEnd(left_prefix_end);
      left.UngappedSuffixBegin(left_suffix_begin);
      Alignment right{Alignment::FromStringFields(1,
        {std::to_string(right_qstart), std::to_string(right_qend),
         std::to_string(right_sstart), std::to_string(right_send),
         std::to_string(right_nident), std::to_string(right_mismatch),
         std::to_string(right_gapopen), std::to_string(right_gaps),
         std::to_string(qlen), std::to_string(slen),
         right_qseq, right_sseq},
        scoring_system, paste_parameters)};
      right.UngappedPrefixEnd(right_prefix_end);
      right.UngappedSuffixBegin(right_suffix_begin);

      AlignmentConfiguration config{GetConfiguration(
          query_offset, subject_offset,
          left_qseq.length(), right_qseq.length())};

      Alignment original_left{left};
      right.PasteLeft(left, config, scoring_system, paste_parameters);

      THEN("Right becomes the pasted alignment.") {
        CHECK(right.Qstart() == pasted_qstart);
        CHECK(right.Qend() == pasted_qend);
        CHECK(right.Sstart() == pasted_sstart);
        CHECK(right.Send() == pasted_send);
        CHECK(right.PlusStrand() == plus_strand);
        CHECK(right.Nident() == pasted_nident);
        CHECK(right.Mismatch() == pasted_mismatch);
        CHECK(right.Gapopen() == pasted_gapopen);
        CHECK(right.Gaps() == pasted_gaps);
        CHECK(right.Qlen() == qlen);
        CHECK(right.Slen() == slen);
        CHECK(right.Qseq() == pasted_qseq);
        CHECK(right.Sseq() == pasted_sseq);
        CHECK(right.IncludeInOutput() == false);
        CHECK(right.UngappedPrefixEnd() == pasted_prefix_end);
        CHECK(right.UngappedSuffixBegin() == pasted_suffix_begin);
      }

      THEN("Left remains unchanged.") {
        CHECK(left == original_left);
      }
    }

    WHEN("query_offset == subject_offset > 0; -strand") {
      // Specify test case parameters.
      int query_offset{10}, subject_offset{10};
      bool plus_strand{false};
      HandleOffset(query_offset, subject_offset, plus_strand,
                   left_sstart, left_send, right_qstart, right_qend,
                   right_sstart, right_send);
      // Specify expected pasted outcome.
      int pasted_qstart{left_qstart}, pasted_qend{right_qend},
          pasted_sstart{right_sstart}, pasted_send{left_send},
          pasted_nident{left_nident + right_nident},
          pasted_mismatch{left_mismatch + right_mismatch + query_offset},
          pasted_gapopen{left_gapopen + right_gapopen},
          pasted_gaps{left_gaps + right_gaps},
          pasted_prefix_end{left_prefix_end},
          pasted_suffix_begin{right_suffix_begin + query_offset
                              + static_cast<int>(left_qseq.length())};

      std::string pasted_qseq, pasted_sseq;
      pasted_qseq = left_qseq;
      pasted_qseq.append(query_offset, 'N');
      pasted_qseq.append(right_qseq);

      pasted_sseq = left_sseq;
      pasted_sseq.append(subject_offset, 'N');
      pasted_sseq.append(right_sseq);

      // Swapped subject coordinates indicate minus strand.
      if (!plus_strand) {
        std::swap(left_sstart, left_send);
        std::swap(right_sstart, right_send);
      }

      Alignment left{Alignment::FromStringFields(0,
        {std::to_string(left_qstart), std::to_string(left_qend),
         std::to_string(left_sstart), std::to_string(left_send),
         std::to_string(left_nident), std::to_string(left_mismatch),
         std::to_string(left_gapopen), std::to_string(left_gaps),
         std::to_string(qlen), std::to_string(slen),
         left_qseq, left_sseq},
        scoring_system, paste_parameters)};
      left.UngappedPrefixEnd(left_prefix_end);
      left.UngappedSuffixBegin(left_suffix_begin);
      Alignment right{Alignment::FromStringFields(1,
        {std::to_string(right_qstart), std::to_string(right_qend),
         std::to_string(right_sstart), std::to_string(right_send),
         std::to_string(right_nident), std::to_string(right_mismatch),
         std::to_string(right_gapopen), std::to_string(right_gaps),
         std::to_string(qlen), std::to_string(slen),
         right_qseq, right_sseq},
        scoring_system, paste_parameters)};
      right.UngappedPrefixEnd(right_prefix_end);
      right.UngappedSuffixBegin(right_suffix_begin);

      AlignmentConfiguration config{GetConfiguration(
          query_offset, subject_offset,
          left_qseq.length(), right_qseq.length())};

      Alignment original_left{left};
      right.PasteLeft(left, config, scoring_system, paste_parameters);

      THEN("Right becomes the pasted alignment.") {
        CHECK(right.Qstart() == pasted_qstart);
        CHECK(right.Qend() == pasted_qend);
        CHECK(right.Sstart() == pasted_sstart);
        CHECK(right.Send() == pasted_send);
        CHECK(right.PlusStrand() == plus_strand);
        CHECK(right.Nident() == pasted_nident);
        CHECK(right.Mismatch() == pasted_mismatch);
        CHECK(right.Gapopen() == pasted_gapopen);
        CHECK(right.Gaps() == pasted_gaps);
        CHECK(right.Qlen() == qlen);
        CHECK(right.Slen() == slen);
        CHECK(right.Qseq() == pasted_qseq);
        CHECK(right.Sseq() == pasted_sseq);
        CHECK(right.IncludeInOutput() == false);
        CHECK(right.UngappedPrefixEnd() == pasted_prefix_end);
        CHECK(right.UngappedSuffixBegin() == pasted_suffix_begin);
      }

      THEN("Left remains unchanged.") {
        CHECK(left == original_left);
      }
    }

    WHEN("query_offset == subject_offset < 0; +strand") {
      // Specify test case parameters.
      int query_offset{-4}, subject_offset{-4};
      bool plus_strand{true};
      HandleOffset(query_offset, subject_offset, plus_strand,
                   left_sstart, left_send, right_qstart, right_qend,
                   right_sstart, right_send);
      // Specify expected pasted outcome.
      int pasted_qstart{left_qstart}, pasted_qend{right_qend},
          pasted_sstart{left_sstart}, pasted_send{right_send},
          pasted_nident{left_nident + right_nident + query_offset},
          pasted_mismatch{left_mismatch + right_mismatch},
          pasted_gapopen{left_gapopen + right_gapopen},
          pasted_gaps{left_gaps + right_gaps},
          pasted_prefix_end{left_prefix_end},
          pasted_suffix_begin{right_suffix_begin + query_offset
                              + static_cast<int>(left_qseq.length())};

      std::string pasted_qseq, pasted_sseq;
      pasted_qseq = left_qseq;
      pasted_qseq.append(right_qseq.substr(std::abs(query_offset)));

      pasted_sseq = left_sseq;
      pasted_sseq.append(right_sseq.substr(std::abs(query_offset)));

      // Swapped subject coordinates indicate minus strand.
      if (!plus_strand) {
        std::swap(left_sstart, left_send);
        std::swap(right_sstart, right_send);
      }

      Alignment left{Alignment::FromStringFields(0,
        {std::to_string(left_qstart), std::to_string(left_qend),
         std::to_string(left_sstart), std::to_string(left_send),
         std::to_string(left_nident), std::to_string(left_mismatch),
         std::to_string(left_gapopen), std::to_string(left_gaps),
         std::to_string(qlen), std::to_string(slen),
         left_qseq, left_sseq},
        scoring_system, paste_parameters)};
      left.UngappedPrefixEnd(left_prefix_end);
      left.UngappedSuffixBegin(left_suffix_begin);
      Alignment right{Alignment::FromStringFields(1,
        {std::to_string(right_qstart), std::to_string(right_qend),
         std::to_string(right_sstart), std::to_string(right_send),
         std::to_string(right_nident), std::to_string(right_mismatch),
         std::to_string(right_gapopen), std::to_string(right_gaps),
         std::to_string(qlen), std::to_string(slen),
         right_qseq, right_sseq},
        scoring_system, paste_parameters)};
      right.UngappedPrefixEnd(right_prefix_end);
      right.UngappedSuffixBegin(right_suffix_begin);

      AlignmentConfiguration config{GetConfiguration(
          query_offset, subject_offset,
          left_qseq.length(), right_qseq.length())};

      Alignment original_left{left};
      right.PasteLeft(left, config, scoring_system, paste_parameters);

      THEN("Right becomes the pasted alignment.") {
        CHECK(right.Qstart() == pasted_qstart);
        CHECK(right.Qend() == pasted_qend);
        CHECK(right.Sstart() == pasted_sstart);
        CHECK(right.Send() == pasted_send);
        CHECK(right.PlusStrand() == plus_strand);
        CHECK(right.Nident() == pasted_nident);
        CHECK(right.Mismatch() == pasted_mismatch);
        CHECK(right.Gapopen() == pasted_gapopen);
        CHECK(right.Gaps() == pasted_gaps);
        CHECK(right.Qlen() == qlen);
        CHECK(right.Slen() == slen);
        CHECK(right.Qseq() == pasted_qseq);
        CHECK(right.Sseq() == pasted_sseq);
        CHECK(right.IncludeInOutput() == false);
        CHECK(right.UngappedPrefixEnd() == pasted_prefix_end);
        CHECK(right.UngappedSuffixBegin() == pasted_suffix_begin);
      }

      THEN("Left remains unchanged.") {
        CHECK(left == original_left);
      }
    }

    WHEN("query_offset == subject_offset < 0; -strand") {
      // Specify test case parameters.
      int query_offset{-4}, subject_offset{-4};
      bool plus_strand{false};
      HandleOffset(query_offset, subject_offset, plus_strand,
                   left_sstart, left_send, right_qstart, right_qend,
                   right_sstart, right_send);
      // Specify expected pasted outcome.
      int pasted_qstart{left_qstart}, pasted_qend{right_qend},
          pasted_sstart{right_sstart}, pasted_send{left_send},
          pasted_nident{left_nident + right_nident + query_offset},
          pasted_mismatch{left_mismatch + right_mismatch},
          pasted_gapopen{left_gapopen + right_gapopen},
          pasted_gaps{left_gaps + right_gaps},
          pasted_prefix_end{left_prefix_end},
          pasted_suffix_begin{right_suffix_begin + query_offset
                              + static_cast<int>(left_qseq.length())};

      std::string pasted_qseq, pasted_sseq;
      pasted_qseq = left_qseq;
      pasted_qseq.append(right_qseq.substr(std::abs(query_offset)));

      pasted_sseq = left_sseq;
      pasted_sseq.append(right_sseq.substr(std::abs(query_offset)));

      // Swapped subject coordinates indicate minus strand.
      if (!plus_strand) {
        std::swap(left_sstart, left_send);
        std::swap(right_sstart, right_send);
      }

      Alignment left{Alignment::FromStringFields(0,
        {std::to_string(left_qstart), std::to_string(left_qend),
         std::to_string(left_sstart), std::to_string(left_send),
         std::to_string(left_nident), std::to_string(left_mismatch),
         std::to_string(left_gapopen), std::to_string(left_gaps),
         std::to_string(qlen), std::to_string(slen),
         left_qseq, left_sseq},
        scoring_system, paste_parameters)};
      left.UngappedPrefixEnd(left_prefix_end);
      left.UngappedSuffixBegin(left_suffix_begin);
      Alignment right{Alignment::FromStringFields(1,
        {std::to_string(right_qstart), std::to_string(right_qend),
         std::to_string(right_sstart), std::to_string(right_send),
         std::to_string(right_nident), std::to_string(right_mismatch),
         std::to_string(right_gapopen), std::to_string(right_gaps),
         std::to_string(qlen), std::to_string(slen),
         right_qseq, right_sseq},
        scoring_system, paste_parameters)};
      right.UngappedPrefixEnd(right_prefix_end);
      right.UngappedSuffixBegin(right_suffix_begin);

      AlignmentConfiguration config{GetConfiguration(
          query_offset, subject_offset,
          left_qseq.length(), right_qseq.length())};

      Alignment original_left{left};
      right.PasteLeft(left, config, scoring_system, paste_parameters);

      THEN("Right becomes the pasted alignment.") {
        CHECK(right.Qstart() == pasted_qstart);
        CHECK(right.Qend() == pasted_qend);
        CHECK(right.Sstart() == pasted_sstart);
        CHECK(right.Send() == pasted_send);
        CHECK(right.PlusStrand() == plus_strand);
        CHECK(right.Nident() == pasted_nident);
        CHECK(right.Mismatch() == pasted_mismatch);
        CHECK(right.Gapopen() == pasted_gapopen);
        CHECK(right.Gaps() == pasted_gaps);
        CHECK(right.Qlen() == qlen);
        CHECK(right.Slen() == slen);
        CHECK(right.Qseq() == pasted_qseq);
        CHECK(right.Sseq() == pasted_sseq);
        CHECK(right.IncludeInOutput() == false);
        CHECK(right.UngappedPrefixEnd() == pasted_prefix_end);
        CHECK(right.UngappedSuffixBegin() == pasted_suffix_begin);
      }

      THEN("Left remains unchanged.") {
        CHECK(left == original_left);
      }
    }

    WHEN("query_offset == 0 > subject_offset; +strand") {
      // Specify test case parameters.
      int query_offset{0}, subject_offset{-4};
      bool plus_strand{true};
      HandleOffset(query_offset, subject_offset, plus_strand,
                   left_sstart, left_send, right_qstart, right_qend,
                   right_sstart, right_send);
      // Specify expected pasted outcome.
      int pasted_qstart{left_qstart}, pasted_qend{right_qend},
          pasted_sstart{left_sstart}, pasted_send{right_send},
          pasted_nident{left_nident + right_nident + subject_offset},
          pasted_mismatch{left_mismatch + right_mismatch},
          pasted_gapopen{left_gapopen + right_gapopen + 1},
          pasted_gaps{left_gaps + right_gaps - subject_offset},
          pasted_prefix_end{left_prefix_end},
          pasted_suffix_begin{right_suffix_begin
                              + static_cast<int>(left_qseq.length())};

      std::string pasted_qseq, pasted_sseq;
      pasted_qseq = left_qseq;
      pasted_qseq.append(std::abs(subject_offset), 'N');
      pasted_qseq.append(right_qseq.substr(std::abs(subject_offset)));

      pasted_sseq = left_sseq;
      pasted_sseq.append(std::abs(subject_offset), '-');
      pasted_sseq.append(right_sseq.substr(std::abs(subject_offset)));

      // Swapped subject coordinates indicate minus strand.
      if (!plus_strand) {
        std::swap(left_sstart, left_send);
        std::swap(right_sstart, right_send);
      }

      Alignment left{Alignment::FromStringFields(0,
        {std::to_string(left_qstart), std::to_string(left_qend),
         std::to_string(left_sstart), std::to_string(left_send),
         std::to_string(left_nident), std::to_string(left_mismatch),
         std::to_string(left_gapopen), std::to_string(left_gaps),
         std::to_string(qlen), std::to_string(slen),
         left_qseq, left_sseq},
        scoring_system, paste_parameters)};
      left.UngappedPrefixEnd(left_prefix_end);
      left.UngappedSuffixBegin(left_suffix_begin);
      Alignment right{Alignment::FromStringFields(1,
        {std::to_string(right_qstart), std::to_string(right_qend),
         std::to_string(right_sstart), std::to_string(right_send),
         std::to_string(right_nident), std::to_string(right_mismatch),
         std::to_string(right_gapopen), std::to_string(right_gaps),
         std::to_string(qlen), std::to_string(slen),
         right_qseq, right_sseq},
        scoring_system, paste_parameters)};
      right.UngappedPrefixEnd(right_prefix_end);
      right.UngappedSuffixBegin(right_suffix_begin);

      AlignmentConfiguration config{GetConfiguration(
          query_offset, subject_offset,
          left_qseq.length(), right_qseq.length())};

      Alignment original_left{left};
      right.PasteLeft(left, config, scoring_system, paste_parameters);

      THEN("Right becomes the pasted alignment.") {
        CHECK(right.Qstart() == pasted_qstart);
        CHECK(right.Qend() == pasted_qend);
        CHECK(right.Sstart() == pasted_sstart);
        CHECK(right.Send() == pasted_send);
        CHECK(right.PlusStrand() == plus_strand);
        CHECK(right.Nident() == pasted_nident);
        CHECK(right.Mismatch() == pasted_mismatch);
        CHECK(right.Gapopen() == pasted_gapopen);
        CHECK(right.Gaps() == pasted_gaps);
        CHECK(right.Qlen() == qlen);
        CHECK(right.Slen() == slen);
        CHECK(right.Qseq() == pasted_qseq);
        CHECK(right.Sseq() == pasted_sseq);
        CHECK(right.IncludeInOutput() == false);
        CHECK(right.UngappedPrefixEnd() == pasted_prefix_end);
        CHECK(right.UngappedSuffixBegin() == pasted_suffix_begin);
      }

      THEN("Left remains unchanged.") {
        CHECK(left == original_left);
      }
    }

    WHEN("query_offset == 0 > subject_offset; -strand") {
      // Specify test case parameters.
      int query_offset{0}, subject_offset{-4};
      bool plus_strand{false};
      HandleOffset(query_offset, subject_offset, plus_strand,
                   left_sstart, left_send, right_qstart, right_qend,
                   right_sstart, right_send);
      // Specify expected pasted outcome.
      int pasted_qstart{left_qstart}, pasted_qend{right_qend},
          pasted_sstart{right_sstart}, pasted_send{left_send},
          pasted_nident{left_nident + right_nident + subject_offset},
          pasted_mismatch{left_mismatch + right_mismatch},
          pasted_gapopen{left_gapopen + right_gapopen + 1},
          pasted_gaps{left_gaps + right_gaps - subject_offset},
          pasted_prefix_end{left_prefix_end},
          pasted_suffix_begin{right_suffix_begin
                              + static_cast<int>(left_qseq.length())};

      std::string pasted_qseq, pasted_sseq;
      pasted_qseq = left_qseq;
      pasted_qseq.append(std::abs(subject_offset), 'N');
      pasted_qseq.append(right_qseq.substr(std::abs(subject_offset)));

      pasted_sseq = left_sseq;
      pasted_sseq.append(std::abs(subject_offset), '-');
      pasted_sseq.append(right_sseq.substr(std::abs(subject_offset)));

      // Swapped subject coordinates indicate minus strand.
      if (!plus_strand) {
        std::swap(left_sstart, left_send);
        std::swap(right_sstart, right_send);
      }

      Alignment left{Alignment::FromStringFields(0,
        {std::to_string(left_qstart), std::to_string(left_qend),
         std::to_string(left_sstart), std::to_string(left_send),
         std::to_string(left_nident), std::to_string(left_mismatch),
         std::to_string(left_gapopen), std::to_string(left_gaps),
         std::to_string(qlen), std::to_string(slen),
         left_qseq, left_sseq},
        scoring_system, paste_parameters)};
      left.UngappedPrefixEnd(left_prefix_end);
      left.UngappedSuffixBegin(left_suffix_begin);
      Alignment right{Alignment::FromStringFields(1,
        {std::to_string(right_qstart), std::to_string(right_qend),
         std::to_string(right_sstart), std::to_string(right_send),
         std::to_string(right_nident), std::to_string(right_mismatch),
         std::to_string(right_gapopen), std::to_string(right_gaps),
         std::to_string(qlen), std::to_string(slen),
         right_qseq, right_sseq},
        scoring_system, paste_parameters)};
      right.UngappedPrefixEnd(right_prefix_end);
      right.UngappedSuffixBegin(right_suffix_begin);

      AlignmentConfiguration config{GetConfiguration(
          query_offset, subject_offset,
          left_qseq.length(), right_qseq.length())};

      Alignment original_left{left};
      right.PasteLeft(left, config, scoring_system, paste_parameters);

      THEN("Right becomes the pasted alignment.") {
        CHECK(right.Qstart() == pasted_qstart);
        CHECK(right.Qend() == pasted_qend);
        CHECK(right.Sstart() == pasted_sstart);
        CHECK(right.Send() == pasted_send);
        CHECK(right.PlusStrand() == plus_strand);
        CHECK(right.Nident() == pasted_nident);
        CHECK(right.Mismatch() == pasted_mismatch);
        CHECK(right.Gapopen() == pasted_gapopen);
        CHECK(right.Gaps() == pasted_gaps);
        CHECK(right.Qlen() == qlen);
        CHECK(right.Slen() == slen);
        CHECK(right.Qseq() == pasted_qseq);
        CHECK(right.Sseq() == pasted_sseq);
        CHECK(right.IncludeInOutput() == false);
        CHECK(right.UngappedPrefixEnd() == pasted_prefix_end);
        CHECK(right.UngappedSuffixBegin() == pasted_suffix_begin);
      }

      THEN("Left remains unchanged.") {
        CHECK(left == original_left);
      }
    }

    WHEN("subject_offset == 0 > query_offset; +strand") {
      // Specify test case parameters.
      int query_offset{-4}, subject_offset{0};
      bool plus_strand{true};
      HandleOffset(query_offset, subject_offset, plus_strand,
                   left_sstart, left_send, right_qstart, right_qend,
                   right_sstart, right_send);
      // Specify expected pasted outcome.
      int pasted_qstart{left_qstart}, pasted_qend{right_qend},
          pasted_sstart{left_sstart}, pasted_send{right_send},
          pasted_nident{left_nident + right_nident + query_offset},
          pasted_mismatch{left_mismatch + right_mismatch},
          pasted_gapopen{left_gapopen + right_gapopen + 1},
          pasted_gaps{left_gaps + right_gaps - query_offset},
          pasted_prefix_end{left_prefix_end},
          pasted_suffix_begin{right_suffix_begin
                              + static_cast<int>(left_qseq.length())};

      std::string pasted_qseq, pasted_sseq;
      pasted_qseq = left_qseq;
      pasted_qseq.append(std::abs(query_offset), '-');
      pasted_qseq.append(right_qseq.substr(std::abs(query_offset)));

      pasted_sseq = left_sseq;
      pasted_sseq.append(std::abs(query_offset), 'N');
      pasted_sseq.append(right_sseq.substr(std::abs(query_offset)));

      // Swapped subject coordinates indicate minus strand.
      if (!plus_strand) {
        std::swap(left_sstart, left_send);
        std::swap(right_sstart, right_send);
      }

      Alignment left{Alignment::FromStringFields(0,
        {std::to_string(left_qstart), std::to_string(left_qend),
         std::to_string(left_sstart), std::to_string(left_send),
         std::to_string(left_nident), std::to_string(left_mismatch),
         std::to_string(left_gapopen), std::to_string(left_gaps),
         std::to_string(qlen), std::to_string(slen),
         left_qseq, left_sseq},
        scoring_system, paste_parameters)};
      left.UngappedPrefixEnd(left_prefix_end);
      left.UngappedSuffixBegin(left_suffix_begin);
      Alignment right{Alignment::FromStringFields(1,
        {std::to_string(right_qstart), std::to_string(right_qend),
         std::to_string(right_sstart), std::to_string(right_send),
         std::to_string(right_nident), std::to_string(right_mismatch),
         std::to_string(right_gapopen), std::to_string(right_gaps),
         std::to_string(qlen), std::to_string(slen),
         right_qseq, right_sseq},
        scoring_system, paste_parameters)};
      right.UngappedPrefixEnd(right_prefix_end);
      right.UngappedSuffixBegin(right_suffix_begin);

      AlignmentConfiguration config{GetConfiguration(
          query_offset, subject_offset,
          left_qseq.length(), right_qseq.length())};

      Alignment original_left{left};
      right.PasteLeft(left, config, scoring_system, paste_parameters);

      THEN("Right becomes the pasted alignment.") {
        CHECK(right.Qstart() == pasted_qstart);
        CHECK(right.Qend() == pasted_qend);
        CHECK(right.Sstart() == pasted_sstart);
        CHECK(right.Send() == pasted_send);
        CHECK(right.PlusStrand() == plus_strand);
        CHECK(right.Nident() == pasted_nident);
        CHECK(right.Mismatch() == pasted_mismatch);
        CHECK(right.Gapopen() == pasted_gapopen);
        CHECK(right.Gaps() == pasted_gaps);
        CHECK(right.Qlen() == qlen);
        CHECK(right.Slen() == slen);
        CHECK(right.Qseq() == pasted_qseq);
        CHECK(right.Sseq() == pasted_sseq);
        CHECK(right.IncludeInOutput() == false);
        CHECK(right.UngappedPrefixEnd() == pasted_prefix_end);
        CHECK(right.UngappedSuffixBegin() == pasted_suffix_begin);
      }

      THEN("Left remains unchanged.") {
        CHECK(left == original_left);
      }
    }

    WHEN("subject_offset == 0 > query_offset; -strand") {
      // Specify test case parameters.
      int query_offset{-4}, subject_offset{0};
      bool plus_strand{false};
      HandleOffset(query_offset, subject_offset, plus_strand,
                   left_sstart, left_send, right_qstart, right_qend,
                   right_sstart, right_send);
      // Specify expected pasted outcome.
      int pasted_qstart{left_qstart}, pasted_qend{right_qend},
          pasted_sstart{right_sstart}, pasted_send{left_send},
          pasted_nident{left_nident + right_nident + query_offset},
          pasted_mismatch{left_mismatch + right_mismatch},
          pasted_gapopen{left_gapopen + right_gapopen + 1},
          pasted_gaps{left_gaps + right_gaps - query_offset},
          pasted_prefix_end{left_prefix_end},
          pasted_suffix_begin{right_suffix_begin
                              + static_cast<int>(left_qseq.length())};

      std::string pasted_qseq, pasted_sseq;
      pasted_qseq = left_qseq;
      pasted_qseq.append(std::abs(query_offset), '-');
      pasted_qseq.append(right_qseq.substr(std::abs(query_offset)));

      pasted_sseq = left_sseq;
      pasted_sseq.append(std::abs(query_offset), 'N');
      pasted_sseq.append(right_sseq.substr(std::abs(query_offset)));

      // Swapped subject coordinates indicate minus strand.
      if (!plus_strand) {
        std::swap(left_sstart, left_send);
        std::swap(right_sstart, right_send);
      }

      Alignment left{Alignment::FromStringFields(0,
        {std::to_string(left_qstart), std::to_string(left_qend),
         std::to_string(left_sstart), std::to_string(left_send),
         std::to_string(left_nident), std::to_string(left_mismatch),
         std::to_string(left_gapopen), std::to_string(left_gaps),
         std::to_string(qlen), std::to_string(slen),
         left_qseq, left_sseq},
        scoring_system, paste_parameters)};
      left.UngappedPrefixEnd(left_prefix_end);
      left.UngappedSuffixBegin(left_suffix_begin);
      Alignment right{Alignment::FromStringFields(1,
        {std::to_string(right_qstart), std::to_string(right_qend),
         std::to_string(right_sstart), std::to_string(right_send),
         std::to_string(right_nident), std::to_string(right_mismatch),
         std::to_string(right_gapopen), std::to_string(right_gaps),
         std::to_string(qlen), std::to_string(slen),
         right_qseq, right_sseq},
        scoring_system, paste_parameters)};
      right.UngappedPrefixEnd(right_prefix_end);
      right.UngappedSuffixBegin(right_suffix_begin);

      AlignmentConfiguration config{GetConfiguration(
          query_offset, subject_offset,
          left_qseq.length(), right_qseq.length())};

      Alignment original_left{left};
      right.PasteLeft(left, config, scoring_system, paste_parameters);

      THEN("Right becomes the pasted alignment.") {
        CHECK(right.Qstart() == pasted_qstart);
        CHECK(right.Qend() == pasted_qend);
        CHECK(right.Sstart() == pasted_sstart);
        CHECK(right.Send() == pasted_send);
        CHECK(right.PlusStrand() == plus_strand);
        CHECK(right.Nident() == pasted_nident);
        CHECK(right.Mismatch() == pasted_mismatch);
        CHECK(right.Gapopen() == pasted_gapopen);
        CHECK(right.Gaps() == pasted_gaps);
        CHECK(right.Qlen() == qlen);
        CHECK(right.Slen() == slen);
        CHECK(right.Qseq() == pasted_qseq);
        CHECK(right.Sseq() == pasted_sseq);
        CHECK(right.IncludeInOutput() == false);
        CHECK(right.UngappedPrefixEnd() == pasted_prefix_end);
        CHECK(right.UngappedSuffixBegin() == pasted_suffix_begin);
      }

      THEN("Left remains unchanged.") {
        CHECK(left == original_left);
      }
    }

    WHEN("subject_offset > query_offset == 0; +strand") {
      // Specify test case parameters.
      int query_offset{0}, subject_offset{10};
      bool plus_strand{true};
      HandleOffset(query_offset, subject_offset, plus_strand,
                   left_sstart, left_send, right_qstart, right_qend,
                   right_sstart, right_send);
      // Specify expected pasted outcome.
      int pasted_qstart{left_qstart}, pasted_qend{right_qend},
          pasted_sstart{left_sstart}, pasted_send{right_send},
          pasted_nident{left_nident + right_nident},
          pasted_mismatch{left_mismatch + right_mismatch},
          pasted_gapopen{left_gapopen + right_gapopen + 1},
          pasted_gaps{left_gaps + right_gaps + subject_offset},
          pasted_prefix_end{left_prefix_end},
          pasted_suffix_begin{right_suffix_begin + subject_offset
                              + static_cast<int>(left_qseq.length())};

      std::string pasted_qseq, pasted_sseq;
      pasted_qseq = left_qseq;
      pasted_qseq.append(subject_offset, '-');
      pasted_qseq.append(right_qseq);

      pasted_sseq = left_sseq;
      pasted_sseq.append(subject_offset, 'N');
      pasted_sseq.append(right_sseq);

      // Swapped subject coordinates indicate minus strand.
      if (!plus_strand) {
        std::swap(left_sstart, left_send);
        std::swap(right_sstart, right_send);
      }

      Alignment left{Alignment::FromStringFields(0,
        {std::to_string(left_qstart), std::to_string(left_qend),
         std::to_string(left_sstart), std::to_string(left_send),
         std::to_string(left_nident), std::to_string(left_mismatch),
         std::to_string(left_gapopen), std::to_string(left_gaps),
         std::to_string(qlen), std::to_string(slen),
         left_qseq, left_sseq},
        scoring_system, paste_parameters)};
      left.UngappedPrefixEnd(left_prefix_end);
      left.UngappedSuffixBegin(left_suffix_begin);
      Alignment right{Alignment::FromStringFields(1,
        {std::to_string(right_qstart), std::to_string(right_qend),
         std::to_string(right_sstart), std::to_string(right_send),
         std::to_string(right_nident), std::to_string(right_mismatch),
         std::to_string(right_gapopen), std::to_string(right_gaps),
         std::to_string(qlen), std::to_string(slen),
         right_qseq, right_sseq},
        scoring_system, paste_parameters)};
      right.UngappedPrefixEnd(right_prefix_end);
      right.UngappedSuffixBegin(right_suffix_begin);

      AlignmentConfiguration config{GetConfiguration(
          query_offset, subject_offset,
          left_qseq.length(), right_qseq.length())};

      Alignment original_left{left};
      right.PasteLeft(left, config, scoring_system, paste_parameters);

      THEN("Right becomes the pasted alignment.") {
        CHECK(right.Qstart() == pasted_qstart);
        CHECK(right.Qend() == pasted_qend);
        CHECK(right.Sstart() == pasted_sstart);
        CHECK(right.Send() == pasted_send);
        CHECK(right.PlusStrand() == plus_strand);
        CHECK(right.Nident() == pasted_nident);
        CHECK(right.Mismatch() == pasted_mismatch);
        CHECK(right.Gapopen() == pasted_gapopen);
        CHECK(right.Gaps() == pasted_gaps);
        CHECK(right.Qlen() == qlen);
        CHECK(right.Slen() == slen);
        CHECK(right.Qseq() == pasted_qseq);
        CHECK(right.Sseq() == pasted_sseq);
        CHECK(right.IncludeInOutput() == false);
        CHECK(right.UngappedPrefixEnd() == pasted_prefix_end);
        CHECK(right.UngappedSuffixBegin() == pasted_suffix_begin);
      }

      THEN("Left remains unchanged.") {
        CHECK(left == original_left);
      }
    }

    WHEN("subject_offset > query_offset == 0; -strand") {
      // Specify test case parameters.
      int query_offset{0}, subject_offset{10};
      bool plus_strand{false};
      HandleOffset(query_offset, subject_offset, plus_strand,
                   left_sstart, left_send, right_qstart, right_qend,
                   right_sstart, right_send);
      // Specify expected pasted outcome.
      int pasted_qstart{left_qstart}, pasted_qend{right_qend},
          pasted_sstart{right_sstart}, pasted_send{left_send},
          pasted_nident{left_nident + right_nident},
          pasted_mismatch{left_mismatch + right_mismatch},
          pasted_gapopen{left_gapopen + right_gapopen + 1},
          pasted_gaps{left_gaps + right_gaps + subject_offset},
          pasted_prefix_end{left_prefix_end},
          pasted_suffix_begin{right_suffix_begin + subject_offset
                              + static_cast<int>(left_qseq.length())};

      std::string pasted_qseq, pasted_sseq;
      pasted_qseq = left_qseq;
      pasted_qseq.append(subject_offset, '-');
      pasted_qseq.append(right_qseq);

      pasted_sseq = left_sseq;
      pasted_sseq.append(subject_offset, 'N');
      pasted_sseq.append(right_sseq);

      // Swapped subject coordinates indicate minus strand.
      if (!plus_strand) {
        std::swap(left_sstart, left_send);
        std::swap(right_sstart, right_send);
      }

      Alignment left{Alignment::FromStringFields(0,
        {std::to_string(left_qstart), std::to_string(left_qend),
         std::to_string(left_sstart), std::to_string(left_send),
         std::to_string(left_nident), std::to_string(left_mismatch),
         std::to_string(left_gapopen), std::to_string(left_gaps),
         std::to_string(qlen), std::to_string(slen),
         left_qseq, left_sseq},
        scoring_system, paste_parameters)};
      left.UngappedPrefixEnd(left_prefix_end);
      left.UngappedSuffixBegin(left_suffix_begin);
      Alignment right{Alignment::FromStringFields(1,
        {std::to_string(right_qstart), std::to_string(right_qend),
         std::to_string(right_sstart), std::to_string(right_send),
         std::to_string(right_nident), std::to_string(right_mismatch),
         std::to_string(right_gapopen), std::to_string(right_gaps),
         std::to_string(qlen), std::to_string(slen),
         right_qseq, right_sseq},
        scoring_system, paste_parameters)};
      right.UngappedPrefixEnd(right_prefix_end);
      right.UngappedSuffixBegin(right_suffix_begin);

      AlignmentConfiguration config{GetConfiguration(
          query_offset, subject_offset,
          left_qseq.length(), right_qseq.length())};

      Alignment original_left{left};
      right.PasteLeft(left, config, scoring_system, paste_parameters);

      THEN("Right becomes the pasted alignment.") {
        CHECK(right.Qstart() == pasted_qstart);
        CHECK(right.Qend() == pasted_qend);
        CHECK(right.Sstart() == pasted_sstart);
        CHECK(right.Send() == pasted_send);
        CHECK(right.PlusStrand() == plus_strand);
        CHECK(right.Nident() == pasted_nident);
        CHECK(right.Mismatch() == pasted_mismatch);
        CHECK(right.Gapopen() == pasted_gapopen);
        CHECK(right.Gaps() == pasted_gaps);
        CHECK(right.Qlen() == qlen);
        CHECK(right.Slen() == slen);
        CHECK(right.Qseq() == pasted_qseq);
        CHECK(right.Sseq() == pasted_sseq);
        CHECK(right.IncludeInOutput() == false);
        CHECK(right.UngappedPrefixEnd() == pasted_prefix_end);
        CHECK(right.UngappedSuffixBegin() == pasted_suffix_begin);
      }

      THEN("Left remains unchanged.") {
        CHECK(left == original_left);
      }
    }

    WHEN("query_offset > subject_offset == 0; +strand") {
      // Specify test case parameters.
      int query_offset{10}, subject_offset{0};
      bool plus_strand{true};
      HandleOffset(query_offset, subject_offset, plus_strand,
                   left_sstart, left_send, right_qstart, right_qend,
                   right_sstart, right_send);
      // Specify expected pasted outcome.
      int pasted_qstart{left_qstart}, pasted_qend{right_qend},
          pasted_sstart{left_sstart}, pasted_send{right_send},
          pasted_nident{left_nident + right_nident},
          pasted_mismatch{left_mismatch + right_mismatch},
          pasted_gapopen{left_gapopen + right_gapopen + 1},
          pasted_gaps{left_gaps + right_gaps + query_offset},
          pasted_prefix_end{left_prefix_end},
          pasted_suffix_begin{right_suffix_begin + query_offset
                              + static_cast<int>(left_qseq.length())};

      std::string pasted_qseq, pasted_sseq;
      pasted_qseq = left_qseq;
      pasted_qseq.append(query_offset, 'N');
      pasted_qseq.append(right_qseq);

      pasted_sseq = left_sseq;
      pasted_sseq.append(query_offset, '-');
      pasted_sseq.append(right_sseq);

      // Swapped subject coordinates indicate minus strand.
      if (!plus_strand) {
        std::swap(left_sstart, left_send);
        std::swap(right_sstart, right_send);
      }

      Alignment left{Alignment::FromStringFields(0,
        {std::to_string(left_qstart), std::to_string(left_qend),
         std::to_string(left_sstart), std::to_string(left_send),
         std::to_string(left_nident), std::to_string(left_mismatch),
         std::to_string(left_gapopen), std::to_string(left_gaps),
         std::to_string(qlen), std::to_string(slen),
         left_qseq, left_sseq},
        scoring_system, paste_parameters)};
      left.UngappedPrefixEnd(left_prefix_end);
      left.UngappedSuffixBegin(left_suffix_begin);
      Alignment right{Alignment::FromStringFields(1,
        {std::to_string(right_qstart), std::to_string(right_qend),
         std::to_string(right_sstart), std::to_string(right_send),
         std::to_string(right_nident), std::to_string(right_mismatch),
         std::to_string(right_gapopen), std::to_string(right_gaps),
         std::to_string(qlen), std::to_string(slen),
         right_qseq, right_sseq},
        scoring_system, paste_parameters)};
      right.UngappedPrefixEnd(right_prefix_end);
      right.UngappedSuffixBegin(right_suffix_begin);

      AlignmentConfiguration config{GetConfiguration(
          query_offset, subject_offset,
          left_qseq.length(), right_qseq.length())};

      Alignment original_left{left};
      right.PasteLeft(left, config, scoring_system, paste_parameters);

      THEN("Right becomes the pasted alignment.") {
        CHECK(right.Qstart() == pasted_qstart);
        CHECK(right.Qend() == pasted_qend);
        CHECK(right.Sstart() == pasted_sstart);
        CHECK(right.Send() == pasted_send);
        CHECK(right.PlusStrand() == plus_strand);
        CHECK(right.Nident() == pasted_nident);
        CHECK(right.Mismatch() == pasted_mismatch);
        CHECK(right.Gapopen() == pasted_gapopen);
        CHECK(right.Gaps() == pasted_gaps);
        CHECK(right.Qlen() == qlen);
        CHECK(right.Slen() == slen);
        CHECK(right.Qseq() == pasted_qseq);
        CHECK(right.Sseq() == pasted_sseq);
        CHECK(right.IncludeInOutput() == false);
        CHECK(right.UngappedPrefixEnd() == pasted_prefix_end);
        CHECK(right.UngappedSuffixBegin() == pasted_suffix_begin);
      }

      THEN("Left remains unchanged.") {
        CHECK(left == original_left);
      }
    }

    WHEN("query_offset > subject_offset == 0; -strand") {
      // Specify test case parameters.
      int query_offset{10}, subject_offset{0};
      bool plus_strand{false};
      HandleOffset(query_offset, subject_offset, plus_strand,
                   left_sstart, left_send, right_qstart, right_qend,
                   right_sstart, right_send);
      // Specify expected pasted outcome.
      int pasted_qstart{left_qstart}, pasted_qend{right_qend},
          pasted_sstart{right_sstart}, pasted_send{left_send},
          pasted_nident{left_nident + right_nident},
          pasted_mismatch{left_mismatch + right_mismatch},
          pasted_gapopen{left_gapopen + right_gapopen + 1},
          pasted_gaps{left_gaps + right_gaps + query_offset},
          pasted_prefix_end{left_prefix_end},
          pasted_suffix_begin{right_suffix_begin + query_offset
                              + static_cast<int>(left_qseq.length())};

      std::string pasted_qseq, pasted_sseq;
      pasted_qseq = left_qseq;
      pasted_qseq.append(query_offset, 'N');
      pasted_qseq.append(right_qseq);

      pasted_sseq = left_sseq;
      pasted_sseq.append(query_offset, '-');
      pasted_sseq.append(right_sseq);

      // Swapped subject coordinates indicate minus strand.
      if (!plus_strand) {
        std::swap(left_sstart, left_send);
        std::swap(right_sstart, right_send);
      }

      Alignment left{Alignment::FromStringFields(0,
        {std::to_string(left_qstart), std::to_string(left_qend),
         std::to_string(left_sstart), std::to_string(left_send),
         std::to_string(left_nident), std::to_string(left_mismatch),
         std::to_string(left_gapopen), std::to_string(left_gaps),
         std::to_string(qlen), std::to_string(slen),
         left_qseq, left_sseq},
        scoring_system, paste_parameters)};
      left.UngappedPrefixEnd(left_prefix_end);
      left.UngappedSuffixBegin(left_suffix_begin);
      Alignment right{Alignment::FromStringFields(1,
        {std::to_string(right_qstart), std::to_string(right_qend),
         std::to_string(right_sstart), std::to_string(right_send),
         std::to_string(right_nident), std::to_string(right_mismatch),
         std::to_string(right_gapopen), std::to_string(right_gaps),
         std::to_string(qlen), std::to_string(slen),
         right_qseq, right_sseq},
        scoring_system, paste_parameters)};
      right.UngappedPrefixEnd(right_prefix_end);
      right.UngappedSuffixBegin(right_suffix_begin);

      AlignmentConfiguration config{GetConfiguration(
          query_offset, subject_offset,
          left_qseq.length(), right_qseq.length())};

      Alignment original_left{left};
      right.PasteLeft(left, config, scoring_system, paste_parameters);

      THEN("Right becomes the pasted alignment.") {
        CHECK(right.Qstart() == pasted_qstart);
        CHECK(right.Qend() == pasted_qend);
        CHECK(right.Sstart() == pasted_sstart);
        CHECK(right.Send() == pasted_send);
        CHECK(right.PlusStrand() == plus_strand);
        CHECK(right.Nident() == pasted_nident);
        CHECK(right.Mismatch() == pasted_mismatch);
        CHECK(right.Gapopen() == pasted_gapopen);
        CHECK(right.Gaps() == pasted_gaps);
        CHECK(right.Qlen() == qlen);
        CHECK(right.Slen() == slen);
        CHECK(right.Qseq() == pasted_qseq);
        CHECK(right.Sseq() == pasted_sseq);
        CHECK(right.IncludeInOutput() == false);
        CHECK(right.UngappedPrefixEnd() == pasted_prefix_end);
        CHECK(right.UngappedSuffixBegin() == pasted_suffix_begin);
      }

      THEN("Left remains unchanged.") {
        CHECK(left == original_left);
      }
    }

    WHEN("query_offset > subject_offset > 0; +strand") {
      // Specify test case parameters.
      int query_offset{10}, subject_offset{4};
      bool plus_strand{true};
      HandleOffset(query_offset, subject_offset, plus_strand,
                   left_sstart, left_send, right_qstart, right_qend,
                   right_sstart, right_send);
      // Specify expected pasted outcome.
      int pasted_qstart{left_qstart}, pasted_qend{right_qend},
          pasted_sstart{left_sstart}, pasted_send{right_send},
          pasted_nident{left_nident + right_nident},
          pasted_mismatch{left_mismatch + right_mismatch + subject_offset},
          pasted_gapopen{left_gapopen + right_gapopen + 1},
          pasted_gaps{left_gaps + right_gaps + query_offset - subject_offset},
          pasted_prefix_end{left_prefix_end},
          pasted_suffix_begin{right_suffix_begin + query_offset
                              + static_cast<int>(left_qseq.length())};

      std::string pasted_qseq, pasted_sseq;
      pasted_qseq = left_qseq;
      pasted_qseq.append(query_offset, 'N');
      pasted_qseq.append(right_qseq);

      pasted_sseq = left_sseq;
      pasted_sseq.append(subject_offset, 'N');
      pasted_sseq.append(query_offset - subject_offset, '-');
      pasted_sseq.append(right_sseq);

      // Swapped subject coordinates indicate minus strand.
      if (!plus_strand) {
        std::swap(left_sstart, left_send);
        std::swap(right_sstart, right_send);
      }

      Alignment left{Alignment::FromStringFields(0,
        {std::to_string(left_qstart), std::to_string(left_qend),
         std::to_string(left_sstart), std::to_string(left_send),
         std::to_string(left_nident), std::to_string(left_mismatch),
         std::to_string(left_gapopen), std::to_string(left_gaps),
         std::to_string(qlen), std::to_string(slen),
         left_qseq, left_sseq},
        scoring_system, paste_parameters)};
      left.UngappedPrefixEnd(left_prefix_end);
      left.UngappedSuffixBegin(left_suffix_begin);
      Alignment right{Alignment::FromStringFields(1,
        {std::to_string(right_qstart), std::to_string(right_qend),
         std::to_string(right_sstart), std::to_string(right_send),
         std::to_string(right_nident), std::to_string(right_mismatch),
         std::to_string(right_gapopen), std::to_string(right_gaps),
         std::to_string(qlen), std::to_string(slen),
         right_qseq, right_sseq},
        scoring_system, paste_parameters)};
      right.UngappedPrefixEnd(right_prefix_end);
      right.UngappedSuffixBegin(right_suffix_begin);

      AlignmentConfiguration config{GetConfiguration(
          query_offset, subject_offset,
          left_qseq.length(), right_qseq.length())};

      Alignment original_left{left};
      right.PasteLeft(left, config, scoring_system, paste_parameters);

      THEN("Right becomes the pasted alignment.") {
        CHECK(right.Qstart() == pasted_qstart);
        CHECK(right.Qend() == pasted_qend);
        CHECK(right.Sstart() == pasted_sstart);
        CHECK(right.Send() == pasted_send);
        CHECK(right.PlusStrand() == plus_strand);
        CHECK(right.Nident() == pasted_nident);
        CHECK(right.Mismatch() == pasted_mismatch);
        CHECK(right.Gapopen() == pasted_gapopen);
        CHECK(right.Gaps() == pasted_gaps);
        CHECK(right.Qlen() == qlen);
        CHECK(right.Slen() == slen);
        CHECK(right.Qseq() == pasted_qseq);
        CHECK(right.Sseq() == pasted_sseq);
        CHECK(right.IncludeInOutput() == false);
        CHECK(right.UngappedPrefixEnd() == pasted_prefix_end);
        CHECK(right.UngappedSuffixBegin() == pasted_suffix_begin);
      }

      THEN("Left remains unchanged.") {
        CHECK(left == original_left);
      }
    }

    WHEN("query_offset > subject_offset > 0; -strand") {
      // Specify test case parameters.
      int query_offset{10}, subject_offset{4};
      bool plus_strand{false};
      HandleOffset(query_offset, subject_offset, plus_strand,
                   left_sstart, left_send, right_qstart, right_qend,
                   right_sstart, right_send);
      // Specify expected pasted outcome.
      int pasted_qstart{left_qstart}, pasted_qend{right_qend},
          pasted_sstart{right_sstart}, pasted_send{left_send},
          pasted_nident{left_nident + right_nident},
          pasted_mismatch{left_mismatch + right_mismatch + subject_offset},
          pasted_gapopen{left_gapopen + right_gapopen + 1},
          pasted_gaps{left_gaps + right_gaps + query_offset - subject_offset},
          pasted_prefix_end{left_prefix_end},
          pasted_suffix_begin{right_suffix_begin + query_offset
                              + static_cast<int>(left_qseq.length())};

      std::string pasted_qseq, pasted_sseq;
      pasted_qseq = left_qseq;
      pasted_qseq.append(query_offset, 'N');
      pasted_qseq.append(right_qseq);

      pasted_sseq = left_sseq;
      pasted_sseq.append(subject_offset, 'N');
      pasted_sseq.append(query_offset - subject_offset, '-');
      pasted_sseq.append(right_sseq);

      // Swapped subject coordinates indicate minus strand.
      if (!plus_strand) {
        std::swap(left_sstart, left_send);
        std::swap(right_sstart, right_send);
      }

      Alignment left{Alignment::FromStringFields(0,
        {std::to_string(left_qstart), std::to_string(left_qend),
         std::to_string(left_sstart), std::to_string(left_send),
         std::to_string(left_nident), std::to_string(left_mismatch),
         std::to_string(left_gapopen), std::to_string(left_gaps),
         std::to_string(qlen), std::to_string(slen),
         left_qseq, left_sseq},
        scoring_system, paste_parameters)};
      left.UngappedPrefixEnd(left_prefix_end);
      left.UngappedSuffixBegin(left_suffix_begin);
      Alignment right{Alignment::FromStringFields(1,
        {std::to_string(right_qstart), std::to_string(right_qend),
         std::to_string(right_sstart), std::to_string(right_send),
         std::to_string(right_nident), std::to_string(right_mismatch),
         std::to_string(right_gapopen), std::to_string(right_gaps),
         std::to_string(qlen), std::to_string(slen),
         right_qseq, right_sseq},
        scoring_system, paste_parameters)};
      right.UngappedPrefixEnd(right_prefix_end);
      right.UngappedSuffixBegin(right_suffix_begin);

      AlignmentConfiguration config{GetConfiguration(
          query_offset, subject_offset,
          left_qseq.length(), right_qseq.length())};

      Alignment original_left{left};
      right.PasteLeft(left, config, scoring_system, paste_parameters);

      THEN("Right becomes the pasted alignment.") {
        CHECK(right.Qstart() == pasted_qstart);
        CHECK(right.Qend() == pasted_qend);
        CHECK(right.Sstart() == pasted_sstart);
        CHECK(right.Send() == pasted_send);
        CHECK(right.PlusStrand() == plus_strand);
        CHECK(right.Nident() == pasted_nident);
        CHECK(right.Mismatch() == pasted_mismatch);
        CHECK(right.Gapopen() == pasted_gapopen);
        CHECK(right.Gaps() == pasted_gaps);
        CHECK(right.Qlen() == qlen);
        CHECK(right.Slen() == slen);
        CHECK(right.Qseq() == pasted_qseq);
        CHECK(right.Sseq() == pasted_sseq);
        CHECK(right.IncludeInOutput() == false);
        CHECK(right.UngappedPrefixEnd() == pasted_prefix_end);
        CHECK(right.UngappedSuffixBegin() == pasted_suffix_begin);
      }

      THEN("Left remains unchanged.") {
        CHECK(left == original_left);
      }
    }

    WHEN("subject_offset > query_offset > 0; +strand") {
      // Specify test case parameters.
      int query_offset{4}, subject_offset{10};
      bool plus_strand{true};
      HandleOffset(query_offset, subject_offset, plus_strand,
                   left_sstart, left_send, right_qstart, right_qend,
                   right_sstart, right_send);
      // Specify expected pasted outcome.
      int pasted_qstart{left_qstart}, pasted_qend{right_qend},
          pasted_sstart{left_sstart}, pasted_send{right_send},
          pasted_nident{left_nident + right_nident},
          pasted_mismatch{left_mismatch + right_mismatch + query_offset},
          pasted_gapopen{left_gapopen + right_gapopen + 1},
          pasted_gaps{left_gaps + right_gaps + subject_offset - query_offset},
          pasted_prefix_end{left_prefix_end},
          pasted_suffix_begin{right_suffix_begin + subject_offset
                              + static_cast<int>(left_qseq.length())};

      std::string pasted_qseq, pasted_sseq;
      pasted_qseq = left_qseq;
      pasted_qseq.append(query_offset, 'N');
      pasted_qseq.append(subject_offset - query_offset, '-');
      pasted_qseq.append(right_qseq);

      pasted_sseq = left_sseq;
      pasted_sseq.append(subject_offset, 'N');
      pasted_sseq.append(right_sseq);

      // Swapped subject coordinates indicate minus strand.
      if (!plus_strand) {
        std::swap(left_sstart, left_send);
        std::swap(right_sstart, right_send);
      }

      Alignment left{Alignment::FromStringFields(0,
        {std::to_string(left_qstart), std::to_string(left_qend),
         std::to_string(left_sstart), std::to_string(left_send),
         std::to_string(left_nident), std::to_string(left_mismatch),
         std::to_string(left_gapopen), std::to_string(left_gaps),
         std::to_string(qlen), std::to_string(slen),
         left_qseq, left_sseq},
        scoring_system, paste_parameters)};
      left.UngappedPrefixEnd(left_prefix_end);
      left.UngappedSuffixBegin(left_suffix_begin);
      Alignment right{Alignment::FromStringFields(1,
        {std::to_string(right_qstart), std::to_string(right_qend),
         std::to_string(right_sstart), std::to_string(right_send),
         std::to_string(right_nident), std::to_string(right_mismatch),
         std::to_string(right_gapopen), std::to_string(right_gaps),
         std::to_string(qlen), std::to_string(slen),
         right_qseq, right_sseq},
        scoring_system, paste_parameters)};
      right.UngappedPrefixEnd(right_prefix_end);
      right.UngappedSuffixBegin(right_suffix_begin);

      AlignmentConfiguration config{GetConfiguration(
          query_offset, subject_offset,
          left_qseq.length(), right_qseq.length())};

      Alignment original_left{left};
      right.PasteLeft(left, config, scoring_system, paste_parameters);

      THEN("Right becomes the pasted alignment.") {
        CHECK(right.Qstart() == pasted_qstart);
        CHECK(right.Qend() == pasted_qend);
        CHECK(right.Sstart() == pasted_sstart);
        CHECK(right.Send() == pasted_send);
        CHECK(right.PlusStrand() == plus_strand);
        CHECK(right.Nident() == pasted_nident);
        CHECK(right.Mismatch() == pasted_mismatch);
        CHECK(right.Gapopen() == pasted_gapopen);
        CHECK(right.Gaps() == pasted_gaps);
        CHECK(right.Qlen() == qlen);
        CHECK(right.Slen() == slen);
        CHECK(right.Qseq() == pasted_qseq);
        CHECK(right.Sseq() == pasted_sseq);
        CHECK(right.IncludeInOutput() == false);
        CHECK(right.UngappedPrefixEnd() == pasted_prefix_end);
        CHECK(right.UngappedSuffixBegin() == pasted_suffix_begin);
      }

      THEN("Left remains unchanged.") {
        CHECK(left == original_left);
      }
    }

    WHEN("subject_offset > query_offset > 0; -strand") {
      // Specify test case parameters.
      int query_offset{4}, subject_offset{10};
      bool plus_strand{false};
      HandleOffset(query_offset, subject_offset, plus_strand,
                   left_sstart, left_send, right_qstart, right_qend,
                   right_sstart, right_send);
      // Specify expected pasted outcome.
      int pasted_qstart{left_qstart}, pasted_qend{right_qend},
          pasted_sstart{right_sstart}, pasted_send{left_send},
          pasted_nident{left_nident + right_nident},
          pasted_mismatch{left_mismatch + right_mismatch + query_offset},
          pasted_gapopen{left_gapopen + right_gapopen + 1},
          pasted_gaps{left_gaps + right_gaps + subject_offset - query_offset},
          pasted_prefix_end{left_prefix_end},
          pasted_suffix_begin{right_suffix_begin + subject_offset
                              + static_cast<int>(left_qseq.length())};

      std::string pasted_qseq, pasted_sseq;
      pasted_qseq = left_qseq;
      pasted_qseq.append(query_offset, 'N');
      pasted_qseq.append(subject_offset - query_offset, '-');
      pasted_qseq.append(right_qseq);

      pasted_sseq = left_sseq;
      pasted_sseq.append(subject_offset, 'N');
      pasted_sseq.append(right_sseq);

      // Swapped subject coordinates indicate minus strand.
      if (!plus_strand) {
        std::swap(left_sstart, left_send);
        std::swap(right_sstart, right_send);
      }

      Alignment left{Alignment::FromStringFields(0,
        {std::to_string(left_qstart), std::to_string(left_qend),
         std::to_string(left_sstart), std::to_string(left_send),
         std::to_string(left_nident), std::to_string(left_mismatch),
         std::to_string(left_gapopen), std::to_string(left_gaps),
         std::to_string(qlen), std::to_string(slen),
         left_qseq, left_sseq},
        scoring_system, paste_parameters)};
      left.UngappedPrefixEnd(left_prefix_end);
      left.UngappedSuffixBegin(left_suffix_begin);
      Alignment right{Alignment::FromStringFields(1,
        {std::to_string(right_qstart), std::to_string(right_qend),
         std::to_string(right_sstart), std::to_string(right_send),
         std::to_string(right_nident), std::to_string(right_mismatch),
         std::to_string(right_gapopen), std::to_string(right_gaps),
         std::to_string(qlen), std::to_string(slen),
         right_qseq, right_sseq},
        scoring_system, paste_parameters)};
      right.UngappedPrefixEnd(right_prefix_end);
      right.UngappedSuffixBegin(right_suffix_begin);

      AlignmentConfiguration config{GetConfiguration(
          query_offset, subject_offset,
          left_qseq.length(), right_qseq.length())};

      Alignment original_left{left};
      right.PasteLeft(left, config, scoring_system, paste_parameters);

      THEN("Right becomes the pasted alignment.") {
        CHECK(right.Qstart() == pasted_qstart);
        CHECK(right.Qend() == pasted_qend);
        CHECK(right.Sstart() == pasted_sstart);
        CHECK(right.Send() == pasted_send);
        CHECK(right.PlusStrand() == plus_strand);
        CHECK(right.Nident() == pasted_nident);
        CHECK(right.Mismatch() == pasted_mismatch);
        CHECK(right.Gapopen() == pasted_gapopen);
        CHECK(right.Gaps() == pasted_gaps);
        CHECK(right.Qlen() == qlen);
        CHECK(right.Slen() == slen);
        CHECK(right.Qseq() == pasted_qseq);
        CHECK(right.Sseq() == pasted_sseq);
        CHECK(right.IncludeInOutput() == false);
        CHECK(right.UngappedPrefixEnd() == pasted_prefix_end);
        CHECK(right.UngappedSuffixBegin() == pasted_suffix_begin);
      }

      THEN("Left remains unchanged.") {
        CHECK(left == original_left);
      }
    }

    WHEN("0 > subject_offset > query_offset; +strand") {
      // Specify test case parameters.
      int query_offset{-5}, subject_offset{-2};
      bool plus_strand{true};
      HandleOffset(query_offset, subject_offset, plus_strand,
                   left_sstart, left_send, right_qstart, right_qend,
                   right_sstart, right_send);
      // Specify expected pasted outcome.
      int pasted_qstart{left_qstart}, pasted_qend{right_qend},
          pasted_sstart{left_sstart}, pasted_send{right_send},
          pasted_nident{left_nident + right_nident + query_offset},
          pasted_mismatch{left_mismatch + right_mismatch},
          pasted_gapopen{left_gapopen + right_gapopen + 1},
          pasted_gaps{left_gaps + right_gaps + subject_offset - query_offset},
          pasted_prefix_end{left_prefix_end},
          pasted_suffix_begin{right_suffix_begin + subject_offset
                              + static_cast<int>(left_qseq.length())};

      std::string pasted_qseq, pasted_sseq;
      pasted_qseq = left_qseq;
      pasted_qseq.append(subject_offset - query_offset, '-');
      pasted_qseq.append(right_qseq.substr(std::abs(query_offset)));

      pasted_sseq = left_sseq;
      pasted_sseq.append(subject_offset - query_offset, 'N');
      pasted_sseq.append(right_sseq.substr(std::abs(query_offset)));

      // Swapped subject coordinates indicate minus strand.
      if (!plus_strand) {
        std::swap(left_sstart, left_send);
        std::swap(right_sstart, right_send);
      }

      Alignment left{Alignment::FromStringFields(0,
        {std::to_string(left_qstart), std::to_string(left_qend),
         std::to_string(left_sstart), std::to_string(left_send),
         std::to_string(left_nident), std::to_string(left_mismatch),
         std::to_string(left_gapopen), std::to_string(left_gaps),
         std::to_string(qlen), std::to_string(slen),
         left_qseq, left_sseq},
        scoring_system, paste_parameters)};
      left.UngappedPrefixEnd(left_prefix_end);
      left.UngappedSuffixBegin(left_suffix_begin);
      Alignment right{Alignment::FromStringFields(1,
        {std::to_string(right_qstart), std::to_string(right_qend),
         std::to_string(right_sstart), std::to_string(right_send),
         std::to_string(right_nident), std::to_string(right_mismatch),
         std::to_string(right_gapopen), std::to_string(right_gaps),
         std::to_string(qlen), std::to_string(slen),
         right_qseq, right_sseq},
        scoring_system, paste_parameters)};
      right.UngappedPrefixEnd(right_prefix_end);
      right.UngappedSuffixBegin(right_suffix_begin);

      AlignmentConfiguration config{GetConfiguration(
          query_offset, subject_offset,
          left_qseq.length(), right_qseq.length())};

      Alignment original_left{left};
      right.PasteLeft(left, config, scoring_system, paste_parameters);

      THEN("Right becomes the pasted alignment.") {
        CHECK(right.Qstart() == pasted_qstart);
        CHECK(right.Qend() == pasted_qend);
        CHECK(right.Sstart() == pasted_sstart);
        CHECK(right.Send() == pasted_send);
        CHECK(right.PlusStrand() == plus_strand);
        CHECK(right.Nident() == pasted_nident);
        CHECK(right.Mismatch() == pasted_mismatch);
        CHECK(right.Gapopen() == pasted_gapopen);
        CHECK(right.Gaps() == pasted_gaps);
        CHECK(right.Qlen() == qlen);
        CHECK(right.Slen() == slen);
        CHECK(right.Qseq() == pasted_qseq);
        CHECK(right.Sseq() == pasted_sseq);
        CHECK(right.IncludeInOutput() == false);
        CHECK(right.UngappedPrefixEnd() == pasted_prefix_end);
        CHECK(right.UngappedSuffixBegin() == pasted_suffix_begin);
      }

      THEN("Left remains unchanged.") {
        CHECK(left == original_left);
      }
    }

    WHEN("0 > subject_offset > query_offset; -strand") {
      // Specify test case parameters.
      int query_offset{-5}, subject_offset{-2};
      bool plus_strand{false};
      HandleOffset(query_offset, subject_offset, plus_strand,
                   left_sstart, left_send, right_qstart, right_qend,
                   right_sstart, right_send);
      // Specify expected pasted outcome.
      int pasted_qstart{left_qstart}, pasted_qend{right_qend},
          pasted_sstart{right_sstart}, pasted_send{left_send},
          pasted_nident{left_nident + right_nident + query_offset},
          pasted_mismatch{left_mismatch + right_mismatch},
          pasted_gapopen{left_gapopen + right_gapopen + 1},
          pasted_gaps{left_gaps + right_gaps + subject_offset - query_offset},
          pasted_prefix_end{left_prefix_end},
          pasted_suffix_begin{right_suffix_begin + subject_offset
                              + static_cast<int>(left_qseq.length())};

      std::string pasted_qseq, pasted_sseq;
      pasted_qseq = left_qseq;
      pasted_qseq.append(subject_offset - query_offset, '-');
      pasted_qseq.append(right_qseq.substr(std::abs(query_offset)));

      pasted_sseq = left_sseq;
      pasted_sseq.append(subject_offset - query_offset, 'N');
      pasted_sseq.append(right_sseq.substr(std::abs(query_offset)));

      // Swapped subject coordinates indicate minus strand.
      if (!plus_strand) {
        std::swap(left_sstart, left_send);
        std::swap(right_sstart, right_send);
      }

      Alignment left{Alignment::FromStringFields(0,
        {std::to_string(left_qstart), std::to_string(left_qend),
         std::to_string(left_sstart), std::to_string(left_send),
         std::to_string(left_nident), std::to_string(left_mismatch),
         std::to_string(left_gapopen), std::to_string(left_gaps),
         std::to_string(qlen), std::to_string(slen),
         left_qseq, left_sseq},
        scoring_system, paste_parameters)};
      left.UngappedPrefixEnd(left_prefix_end);
      left.UngappedSuffixBegin(left_suffix_begin);
      Alignment right{Alignment::FromStringFields(1,
        {std::to_string(right_qstart), std::to_string(right_qend),
         std::to_string(right_sstart), std::to_string(right_send),
         std::to_string(right_nident), std::to_string(right_mismatch),
         std::to_string(right_gapopen), std::to_string(right_gaps),
         std::to_string(qlen), std::to_string(slen),
         right_qseq, right_sseq},
        scoring_system, paste_parameters)};
      right.UngappedPrefixEnd(right_prefix_end);
      right.UngappedSuffixBegin(right_suffix_begin);

      AlignmentConfiguration config{GetConfiguration(
          query_offset, subject_offset,
          left_qseq.length(), right_qseq.length())};

      Alignment original_left{left};
      right.PasteLeft(left, config, scoring_system, paste_parameters);

      THEN("Right becomes the pasted alignment.") {
        CHECK(right.Qstart() == pasted_qstart);
        CHECK(right.Qend() == pasted_qend);
        CHECK(right.Sstart() == pasted_sstart);
        CHECK(right.Send() == pasted_send);
        CHECK(right.PlusStrand() == plus_strand);
        CHECK(right.Nident() == pasted_nident);
        CHECK(right.Mismatch() == pasted_mismatch);
        CHECK(right.Gapopen() == pasted_gapopen);
        CHECK(right.Gaps() == pasted_gaps);
        CHECK(right.Qlen() == qlen);
        CHECK(right.Slen() == slen);
        CHECK(right.Qseq() == pasted_qseq);
        CHECK(right.Sseq() == pasted_sseq);
        CHECK(right.IncludeInOutput() == false);
        CHECK(right.UngappedPrefixEnd() == pasted_prefix_end);
        CHECK(right.UngappedSuffixBegin() == pasted_suffix_begin);
      }

      THEN("Left remains unchanged.") {
        CHECK(left == original_left);
      }
    }

    WHEN("0 > query_offset > subject_offset; +strand") {
      // Specify test case parameters.
      int query_offset{-2}, subject_offset{-5};
      bool plus_strand{true};
      HandleOffset(query_offset, subject_offset, plus_strand,
                   left_sstart, left_send, right_qstart, right_qend,
                   right_sstart, right_send);
      // Specify expected pasted outcome.
      int pasted_qstart{left_qstart}, pasted_qend{right_qend},
          pasted_sstart{left_sstart}, pasted_send{right_send},
          pasted_nident{left_nident + right_nident + subject_offset},
          pasted_mismatch{left_mismatch + right_mismatch},
          pasted_gapopen{left_gapopen + right_gapopen + 1},
          pasted_gaps{left_gaps + right_gaps + query_offset - subject_offset},
          pasted_prefix_end{left_prefix_end},
          pasted_suffix_begin{right_suffix_begin + query_offset
                              + static_cast<int>(left_qseq.length())};

      std::string pasted_qseq, pasted_sseq;
      pasted_qseq = left_qseq;
      pasted_qseq.append(query_offset - subject_offset, 'N');
      pasted_qseq.append(right_qseq.substr(std::abs(subject_offset)));

      pasted_sseq = left_sseq;
      pasted_sseq.append(query_offset - subject_offset, '-');
      pasted_sseq.append(right_sseq.substr(std::abs(subject_offset)));

      // Swapped subject coordinates indicate minus strand.
      if (!plus_strand) {
        std::swap(left_sstart, left_send);
        std::swap(right_sstart, right_send);
      }

      Alignment left{Alignment::FromStringFields(0,
        {std::to_string(left_qstart), std::to_string(left_qend),
         std::to_string(left_sstart), std::to_string(left_send),
         std::to_string(left_nident), std::to_string(left_mismatch),
         std::to_string(left_gapopen), std::to_string(left_gaps),
         std::to_string(qlen), std::to_string(slen),
         left_qseq, left_sseq},
        scoring_system, paste_parameters)};
      left.UngappedPrefixEnd(left_prefix_end);
      left.UngappedSuffixBegin(left_suffix_begin);
      Alignment right{Alignment::FromStringFields(1,
        {std::to_string(right_qstart), std::to_string(right_qend),
         std::to_string(right_sstart), std::to_string(right_send),
         std::to_string(right_nident), std::to_string(right_mismatch),
         std::to_string(right_gapopen), std::to_string(right_gaps),
         std::to_string(qlen), std::to_string(slen),
         right_qseq, right_sseq},
        scoring_system, paste_parameters)};
      right.UngappedPrefixEnd(right_prefix_end);
      right.UngappedSuffixBegin(right_suffix_begin);

      AlignmentConfiguration config{GetConfiguration(
          query_offset, subject_offset,
          left_qseq.length(), right_qseq.length())};

      Alignment original_left{left};
      right.PasteLeft(left, config, scoring_system, paste_parameters);

      THEN("Right becomes the pasted alignment.") {
        CHECK(right.Qstart() == pasted_qstart);
        CHECK(right.Qend() == pasted_qend);
        CHECK(right.Sstart() == pasted_sstart);
        CHECK(right.Send() == pasted_send);
        CHECK(right.PlusStrand() == plus_strand);
        CHECK(right.Nident() == pasted_nident);
        CHECK(right.Mismatch() == pasted_mismatch);
        CHECK(right.Gapopen() == pasted_gapopen);
        CHECK(right.Gaps() == pasted_gaps);
        CHECK(right.Qlen() == qlen);
        CHECK(right.Slen() == slen);
        CHECK(right.Qseq() == pasted_qseq);
        CHECK(right.Sseq() == pasted_sseq);
        CHECK(right.IncludeInOutput() == false);
        CHECK(right.UngappedPrefixEnd() == pasted_prefix_end);
        CHECK(right.UngappedSuffixBegin() == pasted_suffix_begin);
      }

      THEN("Left remains unchanged.") {
        CHECK(left == original_left);
      }
    }

    WHEN("0 > query_offset > subject_offset; -strand") {
      // Specify test case parameters.
      int query_offset{-2}, subject_offset{-5};
      bool plus_strand{false};
      HandleOffset(query_offset, subject_offset, plus_strand,
                   left_sstart, left_send, right_qstart, right_qend,
                   right_sstart, right_send);
      // Specify expected pasted outcome.
      int pasted_qstart{left_qstart}, pasted_qend{right_qend},
          pasted_sstart{right_sstart}, pasted_send{left_send},
          pasted_nident{left_nident + right_nident + subject_offset},
          pasted_mismatch{left_mismatch + right_mismatch},
          pasted_gapopen{left_gapopen + right_gapopen + 1},
          pasted_gaps{left_gaps + right_gaps + query_offset - subject_offset},
          pasted_prefix_end{left_prefix_end},
          pasted_suffix_begin{right_suffix_begin + query_offset
                              + static_cast<int>(left_qseq.length())};

      std::string pasted_qseq, pasted_sseq;
      pasted_qseq = left_qseq;
      pasted_qseq.append(query_offset - subject_offset, 'N');
      pasted_qseq.append(right_qseq.substr(std::abs(subject_offset)));

      pasted_sseq = left_sseq;
      pasted_sseq.append(query_offset - subject_offset, '-');
      pasted_sseq.append(right_sseq.substr(std::abs(subject_offset)));

      // Swapped subject coordinates indicate minus strand.
      if (!plus_strand) {
        std::swap(left_sstart, left_send);
        std::swap(right_sstart, right_send);
      }

      Alignment left{Alignment::FromStringFields(0,
        {std::to_string(left_qstart), std::to_string(left_qend),
         std::to_string(left_sstart), std::to_string(left_send),
         std::to_string(left_nident), std::to_string(left_mismatch),
         std::to_string(left_gapopen), std::to_string(left_gaps),
         std::to_string(qlen), std::to_string(slen),
         left_qseq, left_sseq},
        scoring_system, paste_parameters)};
      left.UngappedPrefixEnd(left_prefix_end);
      left.UngappedSuffixBegin(left_suffix_begin);
      Alignment right{Alignment::FromStringFields(1,
        {std::to_string(right_qstart), std::to_string(right_qend),
         std::to_string(right_sstart), std::to_string(right_send),
         std::to_string(right_nident), std::to_string(right_mismatch),
         std::to_string(right_gapopen), std::to_string(right_gaps),
         std::to_string(qlen), std::to_string(slen),
         right_qseq, right_sseq},
        scoring_system, paste_parameters)};
      right.UngappedPrefixEnd(right_prefix_end);
      right.UngappedSuffixBegin(right_suffix_begin);

      AlignmentConfiguration config{GetConfiguration(
          query_offset, subject_offset,
          left_qseq.length(), right_qseq.length())};

      Alignment original_left{left};
      right.PasteLeft(left, config, scoring_system, paste_parameters);

      THEN("Right becomes the pasted alignment.") {
        CHECK(right.Qstart() == pasted_qstart);
        CHECK(right.Qend() == pasted_qend);
        CHECK(right.Sstart() == pasted_sstart);
        CHECK(right.Send() == pasted_send);
        CHECK(right.PlusStrand() == plus_strand);
        CHECK(right.Nident() == pasted_nident);
        CHECK(right.Mismatch() == pasted_mismatch);
        CHECK(right.Gapopen() == pasted_gapopen);
        CHECK(right.Gaps() == pasted_gaps);
        CHECK(right.Qlen() == qlen);
        CHECK(right.Slen() == slen);
        CHECK(right.Qseq() == pasted_qseq);
        CHECK(right.Sseq() == pasted_sseq);
        CHECK(right.IncludeInOutput() == false);
        CHECK(right.UngappedPrefixEnd() == pasted_prefix_end);
        CHECK(right.UngappedSuffixBegin() == pasted_suffix_begin);
      }

      THEN("Left remains unchanged.") {
        CHECK(left == original_left);
      }
    }

    WHEN("query_offset > 0 > subject_offset; +strand") {
      // Specify test case parameters.
      int query_offset{10}, subject_offset{-3};
      bool plus_strand{true};
      HandleOffset(query_offset, subject_offset, plus_strand,
                   left_sstart, left_send, right_qstart, right_qend,
                   right_sstart, right_send);
      // Specify expected pasted outcome.
      int pasted_qstart{left_qstart}, pasted_qend{right_qend},
          pasted_sstart{left_sstart}, pasted_send{right_send},
          pasted_nident{left_nident + right_nident + subject_offset},
          pasted_mismatch{left_mismatch + right_mismatch},
          pasted_gapopen{left_gapopen + right_gapopen + 1},
          pasted_gaps{left_gaps + right_gaps + query_offset - subject_offset},
          pasted_prefix_end{left_prefix_end},
          pasted_suffix_begin{right_suffix_begin + query_offset
                              + static_cast<int>(left_qseq.length())};

      std::string pasted_qseq, pasted_sseq;
      pasted_qseq = left_qseq;
      pasted_qseq.append(query_offset - subject_offset, 'N');
      pasted_qseq.append(right_qseq.substr(std::abs(subject_offset)));

      pasted_sseq = left_sseq;
      pasted_sseq.append(query_offset - subject_offset, '-');
      pasted_sseq.append(right_sseq.substr(std::abs(subject_offset)));

      // Swapped subject coordinates indicate minus strand.
      if (!plus_strand) {
        std::swap(left_sstart, left_send);
        std::swap(right_sstart, right_send);
      }

      Alignment left{Alignment::FromStringFields(0,
        {std::to_string(left_qstart), std::to_string(left_qend),
         std::to_string(left_sstart), std::to_string(left_send),
         std::to_string(left_nident), std::to_string(left_mismatch),
         std::to_string(left_gapopen), std::to_string(left_gaps),
         std::to_string(qlen), std::to_string(slen),
         left_qseq, left_sseq},
        scoring_system, paste_parameters)};
      left.UngappedPrefixEnd(left_prefix_end);
      left.UngappedSuffixBegin(left_suffix_begin);
      Alignment right{Alignment::FromStringFields(1,
        {std::to_string(right_qstart), std::to_string(right_qend),
         std::to_string(right_sstart), std::to_string(right_send),
         std::to_string(right_nident), std::to_string(right_mismatch),
         std::to_string(right_gapopen), std::to_string(right_gaps),
         std::to_string(qlen), std::to_string(slen),
         right_qseq, right_sseq},
        scoring_system, paste_parameters)};
      right.UngappedPrefixEnd(right_prefix_end);
      right.UngappedSuffixBegin(right_suffix_begin);

      AlignmentConfiguration config{GetConfiguration(
          query_offset, subject_offset,
          left_qseq.length(), right_qseq.length())};

      Alignment original_left{left};
      right.PasteLeft(left, config, scoring_system, paste_parameters);

      THEN("Right becomes the pasted alignment.") {
        CHECK(right.Qstart() == pasted_qstart);
        CHECK(right.Qend() == pasted_qend);
        CHECK(right.Sstart() == pasted_sstart);
        CHECK(right.Send() == pasted_send);
        CHECK(right.PlusStrand() == plus_strand);
        CHECK(right.Nident() == pasted_nident);
        CHECK(right.Mismatch() == pasted_mismatch);
        CHECK(right.Gapopen() == pasted_gapopen);
        CHECK(right.Gaps() == pasted_gaps);
        CHECK(right.Qlen() == qlen);
        CHECK(right.Slen() == slen);
        CHECK(right.Qseq() == pasted_qseq);
        CHECK(right.Sseq() == pasted_sseq);
        CHECK(right.IncludeInOutput() == false);
        CHECK(right.UngappedPrefixEnd() == pasted_prefix_end);
        CHECK(right.UngappedSuffixBegin() == pasted_suffix_begin);
      }

      THEN("Left remains unchanged.") {
        CHECK(left == original_left);
      }
    }

    WHEN("query_offset > 0 > subject_offset; -strand") {
      // Specify test case parameters.
      int query_offset{10}, subject_offset{-3};
      bool plus_strand{false};
      HandleOffset(query_offset, subject_offset, plus_strand,
                   left_sstart, left_send, right_qstart, right_qend,
                   right_sstart, right_send);
      // Specify expected pasted outcome.
      int pasted_qstart{left_qstart}, pasted_qend{right_qend},
          pasted_sstart{right_sstart}, pasted_send{left_send},
          pasted_nident{left_nident + right_nident + subject_offset},
          pasted_mismatch{left_mismatch + right_mismatch},
          pasted_gapopen{left_gapopen + right_gapopen + 1},
          pasted_gaps{left_gaps + right_gaps + query_offset - subject_offset},
          pasted_prefix_end{left_prefix_end},
          pasted_suffix_begin{right_suffix_begin + query_offset
                              + static_cast<int>(left_qseq.length())};

      std::string pasted_qseq, pasted_sseq;
      pasted_qseq = left_qseq;
      pasted_qseq.append(query_offset - subject_offset, 'N');
      pasted_qseq.append(right_qseq.substr(std::abs(subject_offset)));

      pasted_sseq = left_sseq;
      pasted_sseq.append(query_offset - subject_offset, '-');
      pasted_sseq.append(right_sseq.substr(std::abs(subject_offset)));

      // Swapped subject coordinates indicate minus strand.
      if (!plus_strand) {
        std::swap(left_sstart, left_send);
        std::swap(right_sstart, right_send);
      }

      Alignment left{Alignment::FromStringFields(0,
        {std::to_string(left_qstart), std::to_string(left_qend),
         std::to_string(left_sstart), std::to_string(left_send),
         std::to_string(left_nident), std::to_string(left_mismatch),
         std::to_string(left_gapopen), std::to_string(left_gaps),
         std::to_string(qlen), std::to_string(slen),
         left_qseq, left_sseq},
        scoring_system, paste_parameters)};
      left.UngappedPrefixEnd(left_prefix_end);
      left.UngappedSuffixBegin(left_suffix_begin);
      Alignment right{Alignment::FromStringFields(1,
        {std::to_string(right_qstart), std::to_string(right_qend),
         std::to_string(right_sstart), std::to_string(right_send),
         std::to_string(right_nident), std::to_string(right_mismatch),
         std::to_string(right_gapopen), std::to_string(right_gaps),
         std::to_string(qlen), std::to_string(slen),
         right_qseq, right_sseq},
        scoring_system, paste_parameters)};
      right.UngappedPrefixEnd(right_prefix_end);
      right.UngappedSuffixBegin(right_suffix_begin);

      AlignmentConfiguration config{GetConfiguration(
          query_offset, subject_offset,
          left_qseq.length(), right_qseq.length())};

      Alignment original_left{left};
      right.PasteLeft(left, config, scoring_system, paste_parameters);

      THEN("Right becomes the pasted alignment.") {
        CHECK(right.Qstart() == pasted_qstart);
        CHECK(right.Qend() == pasted_qend);
        CHECK(right.Sstart() == pasted_sstart);
        CHECK(right.Send() == pasted_send);
        CHECK(right.PlusStrand() == plus_strand);
        CHECK(right.Nident() == pasted_nident);
        CHECK(right.Mismatch() == pasted_mismatch);
        CHECK(right.Gapopen() == pasted_gapopen);
        CHECK(right.Gaps() == pasted_gaps);
        CHECK(right.Qlen() == qlen);
        CHECK(right.Slen() == slen);
        CHECK(right.Qseq() == pasted_qseq);
        CHECK(right.Sseq() == pasted_sseq);
        CHECK(right.IncludeInOutput() == false);
        CHECK(right.UngappedPrefixEnd() == pasted_prefix_end);
        CHECK(right.UngappedSuffixBegin() == pasted_suffix_begin);
      }

      THEN("Left remains unchanged.") {
        CHECK(left == original_left);
      }
    }

    WHEN("subject_offset > 0 > query_offset; +strand") {
      // Specify test case parameters.
      int query_offset{-3}, subject_offset{10};
      bool plus_strand{true};
      HandleOffset(query_offset, subject_offset, plus_strand,
                   left_sstart, left_send, right_qstart, right_qend,
                   right_sstart, right_send);
      // Specify expected pasted outcome.
      int pasted_qstart{left_qstart}, pasted_qend{right_qend},
          pasted_sstart{left_sstart}, pasted_send{right_send},
          pasted_nident{left_nident + right_nident + query_offset},
          pasted_mismatch{left_mismatch + right_mismatch},
          pasted_gapopen{left_gapopen + right_gapopen + 1},
          pasted_gaps{left_gaps + right_gaps + subject_offset - query_offset},
          pasted_prefix_end{left_prefix_end},
          pasted_suffix_begin{right_suffix_begin + subject_offset
                              + static_cast<int>(left_qseq.length())};

      std::string pasted_qseq, pasted_sseq;
      pasted_qseq = left_qseq;
      pasted_qseq.append(subject_offset - query_offset, '-');
      pasted_qseq.append(right_qseq.substr(std::abs(query_offset)));

      pasted_sseq = left_sseq;
      pasted_sseq.append(subject_offset - query_offset, 'N');
      pasted_sseq.append(right_sseq.substr(std::abs(query_offset)));

      // Swapped subject coordinates indicate minus strand.
      if (!plus_strand) {
        std::swap(left_sstart, left_send);
        std::swap(right_sstart, right_send);
      }

      Alignment left{Alignment::FromStringFields(0,
        {std::to_string(left_qstart), std::to_string(left_qend),
         std::to_string(left_sstart), std::to_string(left_send),
         std::to_string(left_nident), std::to_string(left_mismatch),
         std::to_string(left_gapopen), std::to_string(left_gaps),
         std::to_string(qlen), std::to_string(slen),
         left_qseq, left_sseq},
        scoring_system, paste_parameters)};
      left.UngappedPrefixEnd(left_prefix_end);
      left.UngappedSuffixBegin(left_suffix_begin);
      Alignment right{Alignment::FromStringFields(1,
        {std::to_string(right_qstart), std::to_string(right_qend),
         std::to_string(right_sstart), std::to_string(right_send),
         std::to_string(right_nident), std::to_string(right_mismatch),
         std::to_string(right_gapopen), std::to_string(right_gaps),
         std::to_string(qlen), std::to_string(slen),
         right_qseq, right_sseq},
        scoring_system, paste_parameters)};
      right.UngappedPrefixEnd(right_prefix_end);
      right.UngappedSuffixBegin(right_suffix_begin);

      AlignmentConfiguration config{GetConfiguration(
          query_offset, subject_offset,
          left_qseq.length(), right_qseq.length())};

      Alignment original_left{left};
      right.PasteLeft(left, config, scoring_system, paste_parameters);

      THEN("Right becomes the pasted alignment.") {
        CHECK(right.Qstart() == pasted_qstart);
        CHECK(right.Qend() == pasted_qend);
        CHECK(right.Sstart() == pasted_sstart);
        CHECK(right.Send() == pasted_send);
        CHECK(right.PlusStrand() == plus_strand);
        CHECK(right.Nident() == pasted_nident);
        CHECK(right.Mismatch() == pasted_mismatch);
        CHECK(right.Gapopen() == pasted_gapopen);
        CHECK(right.Gaps() == pasted_gaps);
        CHECK(right.Qlen() == qlen);
        CHECK(right.Slen() == slen);
        CHECK(right.Qseq() == pasted_qseq);
        CHECK(right.Sseq() == pasted_sseq);
        CHECK(right.IncludeInOutput() == false);
        CHECK(right.UngappedPrefixEnd() == pasted_prefix_end);
        CHECK(right.UngappedSuffixBegin() == pasted_suffix_begin);
      }

      THEN("Left remains unchanged.") {
        CHECK(left == original_left);
      }
    }

    WHEN("subject_offset > 0 > query_offset; -strand") {
      // Specify test case parameters.
      int query_offset{-3}, subject_offset{10};
      bool plus_strand{false};
      HandleOffset(query_offset, subject_offset, plus_strand,
                   left_sstart, left_send, right_qstart, right_qend,
                   right_sstart, right_send);
      // Specify expected pasted outcome.
      int pasted_qstart{left_qstart}, pasted_qend{right_qend},
          pasted_sstart{right_sstart}, pasted_send{left_send},
          pasted_nident{left_nident + right_nident + query_offset},
          pasted_mismatch{left_mismatch + right_mismatch},
          pasted_gapopen{left_gapopen + right_gapopen + 1},
          pasted_gaps{left_gaps + right_gaps + subject_offset - query_offset},
          pasted_prefix_end{left_prefix_end},
          pasted_suffix_begin{right_suffix_begin + subject_offset
                              + static_cast<int>(left_qseq.length())};

      std::string pasted_qseq, pasted_sseq;
      pasted_qseq = left_qseq;
      pasted_qseq.append(subject_offset - query_offset, '-');
      pasted_qseq.append(right_qseq.substr(std::abs(query_offset)));

      pasted_sseq = left_sseq;
      pasted_sseq.append(subject_offset - query_offset, 'N');
      pasted_sseq.append(right_sseq.substr(std::abs(query_offset)));

      // Swapped subject coordinates indicate minus strand.
      if (!plus_strand) {
        std::swap(left_sstart, left_send);
        std::swap(right_sstart, right_send);
      }

      Alignment left{Alignment::FromStringFields(0,
        {std::to_string(left_qstart), std::to_string(left_qend),
         std::to_string(left_sstart), std::to_string(left_send),
         std::to_string(left_nident), std::to_string(left_mismatch),
         std::to_string(left_gapopen), std::to_string(left_gaps),
         std::to_string(qlen), std::to_string(slen),
         left_qseq, left_sseq},
        scoring_system, paste_parameters)};
      left.UngappedPrefixEnd(left_prefix_end);
      left.UngappedSuffixBegin(left_suffix_begin);
      Alignment right{Alignment::FromStringFields(1,
        {std::to_string(right_qstart), std::to_string(right_qend),
         std::to_string(right_sstart), std::to_string(right_send),
         std::to_string(right_nident), std::to_string(right_mismatch),
         std::to_string(right_gapopen), std::to_string(right_gaps),
         std::to_string(qlen), std::to_string(slen),
         right_qseq, right_sseq},
        scoring_system, paste_parameters)};
      right.UngappedPrefixEnd(right_prefix_end);
      right.UngappedSuffixBegin(right_suffix_begin);

      AlignmentConfiguration config{GetConfiguration(
          query_offset, subject_offset,
          left_qseq.length(), right_qseq.length())};

      Alignment original_left{left};
      right.PasteLeft(left, config, scoring_system, paste_parameters);

      THEN("Right becomes the pasted alignment.") {
        CHECK(right.Qstart() == pasted_qstart);
        CHECK(right.Qend() == pasted_qend);
        CHECK(right.Sstart() == pasted_sstart);
        CHECK(right.Send() == pasted_send);
        CHECK(right.PlusStrand() == plus_strand);
        CHECK(right.Nident() == pasted_nident);
        CHECK(right.Mismatch() == pasted_mismatch);
        CHECK(right.Gapopen() == pasted_gapopen);
        CHECK(right.Gaps() == pasted_gaps);
        CHECK(right.Qlen() == qlen);
        CHECK(right.Slen() == slen);
        CHECK(right.Qseq() == pasted_qseq);
        CHECK(right.Sseq() == pasted_sseq);
        CHECK(right.IncludeInOutput() == false);
        CHECK(right.UngappedPrefixEnd() == pasted_prefix_end);
        CHECK(right.UngappedSuffixBegin() == pasted_suffix_begin);
      }

      THEN("Left remains unchanged.") {
        CHECK(left == original_left);
      }
    }
  }
}

SCENARIO("Test invariant preservation by Alignment::PasteLeft.",
         "[Alignment][PasteLeft][invariants]") {
  ScoringSystem scoring_system{ScoringSystem::Create(100000l, 1, 2, 1, 1)};
  PasteParameters paste_parameters;

  GIVEN("An alignment pair on plus strand.") {
    Alignment left{Alignment::FromStringFields(0,
        {"101", "134", "1001", "1036",
         "24", "8", "2", "6",
         "10000", "100000",
         "AAAAAAAATTTTTTCCCCGGGG----GGGGAAAAAAAA",
         "AAAAAAAACCCC--CCCCGGGGAAAATTTTAAAAAAAA"},
        scoring_system, paste_parameters)};

    WHEN("query_offset == subject_offset == 0") {
      Alignment right{Alignment::FromStringFields(1,
          {"135", "160", "1037", "1060",
           "20", "4", "1", "2",
           "10000", "100000",
           "AAAAAAAATTTTCCCCGGAAAAAAAA",
           "AAAAAAAAGGGGCCCC--AAAAAAAA"},
          scoring_system, paste_parameters)};
      AlignmentConfiguration config{GetConfiguration(0, 0, 38, 26)};
      right.PasteLeft(left, config, scoring_system, paste_parameters);

      THEN("All invariants are satisfied.") {
        CHECK(right.Qstart() <= right.Qend());
        CHECK(right.Sstart() <= right.Send());
        CHECK(right.Qlen() > 0);
        CHECK(right.Slen() > 0);
        CHECK_FALSE(right.Qseq().empty());
        CHECK_FALSE(right.Sseq().empty());
        CHECK(right.Qseq().length() == right.Sseq().length());
        CHECK(std::find(right.PastedIdentifiers().begin(),
                        right.PastedIdentifiers().end(),
                        right.Id())
              != right.PastedIdentifiers().end());
      }
    }

    WHEN("query_offset > subject_offset == 0") {
      Alignment right{Alignment::FromStringFields(1,
          {"145", "170", "1037", "1060",
           "20", "4", "1", "2",
           "10000", "100000",
           "AAAAAAAATTTTCCCCGGAAAAAAAA",
           "AAAAAAAAGGGGCCCC--AAAAAAAA"},
          scoring_system, paste_parameters)};
      AlignmentConfiguration config{GetConfiguration(10, 0, 38, 26)};
      right.PasteLeft(left, config, scoring_system, paste_parameters);

      THEN("All invariants are satisfied.") {
        CHECK(right.Qstart() <= right.Qend());
        CHECK(right.Sstart() <= right.Send());
        CHECK(right.Qlen() > 0);
        CHECK(right.Slen() > 0);
        CHECK_FALSE(right.Qseq().empty());
        CHECK_FALSE(right.Sseq().empty());
        CHECK(right.Qseq().length() == right.Sseq().length());
        CHECK(std::find(right.PastedIdentifiers().begin(),
                        right.PastedIdentifiers().end(),
                        right.Id())
              != right.PastedIdentifiers().end());
      }
    }

    WHEN("query_offset < subject_offset == 0") {
      Alignment right{Alignment::FromStringFields(1,
          {"125", "150", "1037", "1060",
           "20", "4", "1", "2",
           "10000", "100000",
           "AAAAAAAATTTTCCCCGGAAAAAAAA",
           "AAAAAAAAGGGGCCCC--AAAAAAAA"},
          scoring_system, paste_parameters)};
      AlignmentConfiguration config{GetConfiguration(-10, 0, 38, 26)};
      right.PasteLeft(left, config, scoring_system, paste_parameters);

      THEN("All invariants are satisfied.") {
        CHECK(right.Qstart() <= right.Qend());
        CHECK(right.Sstart() <= right.Send());
        CHECK(right.Qlen() > 0);
        CHECK(right.Slen() > 0);
        CHECK_FALSE(right.Qseq().empty());
        CHECK_FALSE(right.Sseq().empty());
        CHECK(right.Qseq().length() == right.Sseq().length());
        CHECK(std::find(right.PastedIdentifiers().begin(),
                        right.PastedIdentifiers().end(),
                        right.Id())
              != right.PastedIdentifiers().end());
      }
    }

    WHEN("query_offset == subject_offset < 0") {
      Alignment right{Alignment::FromStringFields(1,
          {"125", "150", "1027", "1040",
           "20", "4", "1", "2",
           "10000", "100000",
           "AAAAAAAATTTTCCCCGGAAAAAAAA",
           "AAAAAAAAGGGGCCCC--AAAAAAAA"},
          scoring_system, paste_parameters)};
      AlignmentConfiguration config{GetConfiguration(-10, -10, 38, 26)};
      right.PasteLeft(left, config, scoring_system, paste_parameters);

      THEN("All invariants are satisfied.") {
        CHECK(right.Qstart() <= right.Qend());
        CHECK(right.Sstart() <= right.Send());
        CHECK(right.Qlen() > 0);
        CHECK(right.Slen() > 0);
        CHECK_FALSE(right.Qseq().empty());
        CHECK_FALSE(right.Sseq().empty());
        CHECK(right.Qseq().length() == right.Sseq().length());
        CHECK(std::find(right.PastedIdentifiers().begin(),
                        right.PastedIdentifiers().end(),
                        right.Id())
              != right.PastedIdentifiers().end());
      }
    }

    WHEN("query_offset == subject_offset > 0") {
      Alignment right{Alignment::FromStringFields(1,
          {"145", "170", "1047", "1070",
           "20", "4", "1", "2",
           "10000", "100000",
           "AAAAAAAATTTTCCCCGGAAAAAAAA",
           "AAAAAAAAGGGGCCCC--AAAAAAAA"},
          scoring_system, paste_parameters)};
      AlignmentConfiguration config{GetConfiguration(10, 10, 38, 26)};
      right.PasteLeft(left, config, scoring_system, paste_parameters);

      THEN("All invariants are satisfied.") {
        CHECK(right.Qstart() <= right.Qend());
        CHECK(right.Sstart() <= right.Send());
        CHECK(right.Qlen() > 0);
        CHECK(right.Slen() > 0);
        CHECK_FALSE(right.Qseq().empty());
        CHECK_FALSE(right.Sseq().empty());
        CHECK(right.Qseq().length() == right.Sseq().length());
        CHECK(std::find(right.PastedIdentifiers().begin(),
                        right.PastedIdentifiers().end(),
                        right.Id())
              != right.PastedIdentifiers().end());
      }
    }

    WHEN("query_offset > subject_offset > 0") {
      Alignment right{Alignment::FromStringFields(1,
          {"145", "170", "1042", "1065",
           "20", "4", "1", "2",
           "10000", "100000",
           "AAAAAAAATTTTCCCCGGAAAAAAAA",
           "AAAAAAAAGGGGCCCC--AAAAAAAA"},
          scoring_system, paste_parameters)};
      AlignmentConfiguration config{GetConfiguration(10, 5, 38, 26)};
      right.PasteLeft(left, config, scoring_system, paste_parameters);

      THEN("All invariants are satisfied.") {
        CHECK(right.Qstart() <= right.Qend());
        CHECK(right.Sstart() <= right.Send());
        CHECK(right.Qlen() > 0);
        CHECK(right.Slen() > 0);
        CHECK_FALSE(right.Qseq().empty());
        CHECK_FALSE(right.Sseq().empty());
        CHECK(right.Qseq().length() == right.Sseq().length());
        CHECK(std::find(right.PastedIdentifiers().begin(),
                        right.PastedIdentifiers().end(),
                        right.Id())
              != right.PastedIdentifiers().end());
      }
    }

    WHEN("query_offset < subject_offset < 0") {
      Alignment right{Alignment::FromStringFields(1,
          {"125", "150", "1032", "1055",
           "20", "4", "1", "2",
           "10000", "100000",
           "AAAAAAAATTTTCCCCGGAAAAAAAA",
           "AAAAAAAAGGGGCCCC--AAAAAAAA"},
          scoring_system, paste_parameters)};
      AlignmentConfiguration config{GetConfiguration(-10, -5, 38, 26)};
      right.PasteLeft(left, config, scoring_system, paste_parameters);

      THEN("All invariants are satisfied.") {
        CHECK(right.Qstart() <= right.Qend());
        CHECK(right.Sstart() <= right.Send());
        CHECK(right.Qlen() > 0);
        CHECK(right.Slen() > 0);
        CHECK_FALSE(right.Qseq().empty());
        CHECK_FALSE(right.Sseq().empty());
        CHECK(right.Qseq().length() == right.Sseq().length());
        CHECK(std::find(right.PastedIdentifiers().begin(),
                        right.PastedIdentifiers().end(),
                        right.Id())
              != right.PastedIdentifiers().end());
      }
    }

    WHEN("query_offset < 0 < subject_offset") {
      Alignment right{Alignment::FromStringFields(1,
          {"125", "150", "1047", "1070",
           "20", "4", "1", "2",
           "10000", "100000",
           "AAAAAAAATTTTCCCCGGAAAAAAAA",
           "AAAAAAAAGGGGCCCC--AAAAAAAA"},
          scoring_system, paste_parameters)};
      AlignmentConfiguration config{GetConfiguration(-10, 10, 38, 26)};
      right.PasteLeft(left, config, scoring_system, paste_parameters);

      THEN("All invariants are satisfied.") {
        CHECK(right.Qstart() <= right.Qend());
        CHECK(right.Sstart() <= right.Send());
        CHECK(right.Qlen() > 0);
        CHECK(right.Slen() > 0);
        CHECK_FALSE(right.Qseq().empty());
        CHECK_FALSE(right.Sseq().empty());
        CHECK(right.Qseq().length() == right.Sseq().length());
        CHECK(std::find(right.PastedIdentifiers().begin(),
                        right.PastedIdentifiers().end(),
                        right.Id())
              != right.PastedIdentifiers().end());
      }
    }

    WHEN("query_offset > 0 > subject_offset") {
      Alignment right{Alignment::FromStringFields(1,
          {"145", "170", "1027", "1050",
           "20", "4", "1", "2",
           "10000", "100000",
           "AAAAAAAATTTTCCCCGGAAAAAAAA",
           "AAAAAAAAGGGGCCCC--AAAAAAAA"},
          scoring_system, paste_parameters)};
      AlignmentConfiguration config{GetConfiguration(10, -10, 38, 26)};
      right.PasteLeft(left, config, scoring_system, paste_parameters);

      THEN("All invariants are satisfied.") {
        CHECK(right.Qstart() <= right.Qend());
        CHECK(right.Sstart() <= right.Send());
        CHECK(right.Qlen() > 0);
        CHECK(right.Slen() > 0);
        CHECK_FALSE(right.Qseq().empty());
        CHECK_FALSE(right.Sseq().empty());
        CHECK(right.Qseq().length() == right.Sseq().length());
        CHECK(std::find(right.PastedIdentifiers().begin(),
                        right.PastedIdentifiers().end(),
                        right.Id())
              != right.PastedIdentifiers().end());
      }
    }

    WHEN("query_offset == 0 < subject_offset") {
      Alignment right{Alignment::FromStringFields(1,
          {"135", "160", "1047", "1070",
           "20", "4", "1", "2",
           "10000", "100000",
           "AAAAAAAATTTTCCCCGGAAAAAAAA",
           "AAAAAAAAGGGGCCCC--AAAAAAAA"},
          scoring_system, paste_parameters)};
      AlignmentConfiguration config{GetConfiguration(0, 10, 38, 26)};
      right.PasteLeft(left, config, scoring_system, paste_parameters);

      THEN("All invariants are satisfied.") {
        CHECK(right.Qstart() <= right.Qend());
        CHECK(right.Sstart() <= right.Send());
        CHECK(right.Qlen() > 0);
        CHECK(right.Slen() > 0);
        CHECK_FALSE(right.Qseq().empty());
        CHECK_FALSE(right.Sseq().empty());
        CHECK(right.Qseq().length() == right.Sseq().length());
        CHECK(std::find(right.PastedIdentifiers().begin(),
                        right.PastedIdentifiers().end(),
                        right.Id())
              != right.PastedIdentifiers().end());
      }
    }

    WHEN("query_offset == 0 > subject_offset") {
      Alignment right{Alignment::FromStringFields(1,
          {"135", "160", "1027", "1050",
           "20", "4", "1", "2",
           "10000", "100000",
           "AAAAAAAATTTTCCCCGGAAAAAAAA",
           "AAAAAAAAGGGGCCCC--AAAAAAAA"},
          scoring_system, paste_parameters)};
      AlignmentConfiguration config{GetConfiguration(0, -10, 38, 26)};
      right.PasteLeft(left, config, scoring_system, paste_parameters);

      THEN("All invariants are satisfied.") {
        CHECK(right.Qstart() <= right.Qend());
        CHECK(right.Sstart() <= right.Send());
        CHECK(right.Qlen() > 0);
        CHECK(right.Slen() > 0);
        CHECK_FALSE(right.Qseq().empty());
        CHECK_FALSE(right.Sseq().empty());
        CHECK(right.Qseq().length() == right.Sseq().length());
        CHECK(std::find(right.PastedIdentifiers().begin(),
                        right.PastedIdentifiers().end(),
                        right.Id())
              != right.PastedIdentifiers().end());
      }
    }

    WHEN("subject_offset > query_offset > 0") {
      Alignment right{Alignment::FromStringFields(1,
          {"140", "165", "1047", "1070",
           "20", "4", "1", "2",
           "10000", "100000",
           "AAAAAAAATTTTCCCCGGAAAAAAAA",
           "AAAAAAAAGGGGCCCC--AAAAAAAA"},
          scoring_system, paste_parameters)};
      AlignmentConfiguration config{GetConfiguration(5, 10, 38, 26)};
      right.PasteLeft(left, config, scoring_system, paste_parameters);

      THEN("All invariants are satisfied.") {
        CHECK(right.Qstart() <= right.Qend());
        CHECK(right.Sstart() <= right.Send());
        CHECK(right.Qlen() > 0);
        CHECK(right.Slen() > 0);
        CHECK_FALSE(right.Qseq().empty());
        CHECK_FALSE(right.Sseq().empty());
        CHECK(right.Qseq().length() == right.Sseq().length());
        CHECK(std::find(right.PastedIdentifiers().begin(),
                        right.PastedIdentifiers().end(),
                        right.Id())
              != right.PastedIdentifiers().end());
      }
    }

    WHEN("subject_offset < query_offset < 0") {
      Alignment right{Alignment::FromStringFields(1,
          {"130", "155", "1027", "1050",
           "20", "4", "1", "2",
           "10000", "100000",
           "AAAAAAAATTTTCCCCGGAAAAAAAA",
           "AAAAAAAAGGGGCCCC--AAAAAAAA"},
          scoring_system, paste_parameters)};
      AlignmentConfiguration config{GetConfiguration(-5, -10, 38, 26)};
      right.PasteLeft(left, config, scoring_system, paste_parameters);

      THEN("All invariants are satisfied.") {
        CHECK(right.Qstart() <= right.Qend());
        CHECK(right.Sstart() <= right.Send());
        CHECK(right.Qlen() > 0);
        CHECK(right.Slen() > 0);
        CHECK_FALSE(right.Qseq().empty());
        CHECK_FALSE(right.Sseq().empty());
        CHECK(right.Qseq().length() == right.Sseq().length());
        CHECK(std::find(right.PastedIdentifiers().begin(),
                        right.PastedIdentifiers().end(),
                        right.Id())
              != right.PastedIdentifiers().end());
      }
    }
  }
}

SCENARIO("Test exceptions thrown by Alignment::PasteLeft.",
         "[Alignment][PasteLeft][exceptions]") {
  ScoringSystem scoring_system{ScoringSystem::Create(100000l, 1, 2, 1, 1)};
  PasteParameters paste_parameters;

  WHEN("When two alignments are given.") {
    int query_offset = GENERATE(-4, -2, 0, 5, 10);
    int subject_offset = GENERATE(-4, -2, 0, 5, 10);
    Alignment left_plus{Alignment::FromStringFields(0,
        {"101", "134", "1001", "1036",
         "24", "8", "2", "6",
         "10000", "100000",
         "AAAAAAAATTTTTTCCCCGGGG----GGGGAAAAAAAA",
         "AAAAAAAACCCC--CCCCGGGGAAAATTTTAAAAAAAA"},
        scoring_system, paste_parameters)};
    Alignment right_plus{Alignment::FromStringFields(1,
        {"135", "160", "1037", "1060",
         "20", "4", "1", "2",
         "10000", "100000",
         "AAAAAAAATTTTCCCCGGAAAAAAAA",
         "AAAAAAAAGGGGCCCC--AAAAAAAA"},
        scoring_system, paste_parameters)};
    Alignment left_minus{Alignment::FromStringFields(1,
        {"101", "134", "1036", "1001",
         "24", "8", "2", "6",
         "10000", "100000",
         "AAAAAAAATTTTTTCCCCGGGG----GGGGAAAAAAAA",
         "AAAAAAAACCCC--CCCCGGGGAAAATTTTAAAAAAAA"},
        scoring_system, paste_parameters)};
    Alignment right_minus{Alignment::FromStringFields(1,
        {"135", "160", "1000", "977",
         "20", "4", "1", "2",
         "10000", "100000",
         "AAAAAAAATTTTCCCCGGAAAAAAAA",
         "AAAAAAAAGGGGCCCC--AAAAAAAA"},
        scoring_system, paste_parameters)};
    AlignmentConfiguration config{GetConfiguration(
        query_offset, subject_offset,
        left_plus.Length(), right_plus.Length())};

    THEN("If on opposite strands, exception is thrown.") {
      CHECK_THROWS_AS(right_plus.PasteLeft(left_minus, config, scoring_system,
                                           paste_parameters),
                      exceptions::PastingError);
      CHECK_THROWS_AS(right_minus.PasteLeft(left_plus, config, scoring_system,
                                            paste_parameters),
                      exceptions::PastingError);
    }

    THEN("If on same strands, no exception is thrown.") {
      CHECK_NOTHROW(right_plus.PasteLeft(left_plus, config, scoring_system,
                                         paste_parameters));
      CHECK_NOTHROW(right_minus.PasteLeft(left_minus, config, scoring_system,
                                          paste_parameters));
    }
  }

  THEN("Query/subject region of one contained in other causes exception.") {
    Alignment left{Alignment::FromStringFields(0,
        {"101", "134", "1001", "1036",
         "24", "8", "2", "6",
         "10000", "100000",
         "AAAAAAAATTTTTTCCCCGGGG----GGGGAAAAAAAA",
         "AAAAAAAACCCC--CCCCGGGGAAAATTTTAAAAAAAA"},
        scoring_system, paste_parameters)};
    Alignment right_inside_query{Alignment::FromStringFields(1,
        {"109", "126", "2009", "2028",
         "8", "8", "2", "6",
         "10000", "100000",
         "TTTTTTCCCCGGGG----GGGG",
         "CCCC--CCCCGGGGAAAATTTT"},
        scoring_system, paste_parameters)};
    Alignment right_contains_query{Alignment::FromStringFields(1,
        {"95", "136", "1997", "2038",
         "30", "8", "2", "6",
         "10000", "100000",
         "CCCCAAAAAAAATTTTTTCCCCGGGG----GGGGAAAAAAAATT",
         "CCCCAAAAAAAACCCC--CCCCGGGGAAAATTTTAAAAAAAATT"},
        scoring_system, paste_parameters)};
    Alignment right_inside_subject{Alignment::FromStringFields(1,
        {"309", "326", "1009", "1028",
         "8", "8", "2", "6",
         "10000", "100000",
         "TTTTTTCCCCGGGG----GGGG",
         "CCCC--CCCCGGGGAAAATTTT"},
        scoring_system, paste_parameters)};
    Alignment right_contains_subject{Alignment::FromStringFields(1,
        {"295", "336", "997", "1038",
         "30", "8", "2", "6",
         "10000", "100000",
         "CCCCAAAAAAAATTTTTTCCCCGGGG----GGGGAAAAAAAATT",
         "CCCCAAAAAAAACCCC--CCCCGGGGAAAATTTTAAAAAAAATT"},
        scoring_system, paste_parameters)};
    // Offset is disregarded here.
    AlignmentConfiguration config_short{GetConfiguration(
        0, 0, left.Length(), right_inside_query.Length())};
    AlignmentConfiguration config_long{GetConfiguration(
        0, 0, left.Length(), right_contains_query.Length())};
    AlignmentConfiguration config_self{GetConfiguration(
        0, 0, left.Length(), left.Length())};

    THEN("If on opposite strands, exception is thrown.") {
      CHECK_THROWS_AS(right_inside_query.PasteLeft(left, config_short,
                                                   scoring_system,
                                                   paste_parameters),
                      exceptions::PastingError);
      CHECK_THROWS_AS(right_contains_query.PasteLeft(left, config_long,
                                                     scoring_system,
                                                     paste_parameters),
                      exceptions::PastingError);
      CHECK_THROWS_AS(right_inside_subject.PasteLeft(left, config_short,
                                                     scoring_system,
                                                     paste_parameters),
                      exceptions::PastingError);
      CHECK_THROWS_AS(right_contains_subject.PasteLeft(left, config_long,
                                                       scoring_system,
                                                       paste_parameters),
                      exceptions::PastingError);
      CHECK_THROWS_AS(left.PasteLeft(left, config_self,
                                     scoring_system, paste_parameters),
                      exceptions::PastingError);
    }
  }

  THEN("Right query region not starting/ending after left causes exception.") {
    Alignment left{Alignment::FromStringFields(0,
        {"101", "134", "1001", "1036",
         "24", "8", "2", "6",
         "10000", "100000",
         "AAAAAAAATTTTTTCCCCGGGG----GGGGAAAAAAAA",
         "AAAAAAAACCCC--CCCCGGGGAAAATTTTAAAAAAAA"},
        scoring_system, paste_parameters)};
    Alignment false_right_nooverlap{Alignment::FromStringFields(1,
        {"35", "60", "1037", "1060",
         "20", "4", "1", "2",
         "10000", "100000",
         "AAAAAAAATTTTCCCCGGAAAAAAAA",
         "AAAAAAAAGGGGCCCC--AAAAAAAA"},
        scoring_system, paste_parameters)};
    Alignment false_right_overlap{Alignment::FromStringFields(1,
        {"95", "120", "1037", "1060",
         "20", "4", "1", "2",
         "10000", "100000",
         "AAAAAAAATTTTCCCCGGAAAAAAAA",
         "AAAAAAAAGGGGCCCC--AAAAAAAA"},
        scoring_system, paste_parameters)};
    Alignment false_right_equal_start{Alignment::FromStringFields(1,
        {"101", "126", "1037", "1060",
         "20", "4", "1", "2",
         "10000", "100000",
         "AAAAAAAATTTTCCCCGGAAAAAAAA",
         "AAAAAAAAGGGGCCCC--AAAAAAAA"},
        scoring_system, paste_parameters)};
    // Offset is disregarded here.
    AlignmentConfiguration config{GetConfiguration(
        0, 0, left.Length(), false_right_nooverlap.Length())};


    CHECK_THROWS_AS(false_right_nooverlap.PasteLeft(left, config,
                                                    scoring_system,
                                                    paste_parameters),
                    exceptions::PastingError);
    CHECK_THROWS_AS(false_right_overlap.PasteLeft(left, config,
                                                  scoring_system,
                                                  paste_parameters),
                    exceptions::PastingError);
    CHECK_THROWS_AS(false_right_equal_start.PasteLeft(left, config,
                                                      scoring_system,
                                                      paste_parameters),
                    exceptions::PastingError);
  }

  THEN("Right subject not starting/ending after left causes exception (+).") {
    Alignment left{Alignment::FromStringFields(0,
        {"101", "134", "1001", "1036",
         "24", "8", "2", "6",
         "10000", "100000",
         "AAAAAAAATTTTTTCCCCGGGG----GGGGAAAAAAAA",
         "AAAAAAAACCCC--CCCCGGGGAAAATTTTAAAAAAAA"},
        scoring_system, paste_parameters)};
    Alignment false_right_nooverlap{Alignment::FromStringFields(1,
        {"135", "160", "937", "960",
         "20", "4", "1", "2",
         "10000", "100000",
         "AAAAAAAATTTTCCCCGGAAAAAAAA",
         "AAAAAAAAGGGGCCCC--AAAAAAAA"},
        scoring_system, paste_parameters)};
    Alignment false_right_overlap{Alignment::FromStringFields(1,
        {"135", "160", "997", "1020",
         "20", "4", "1", "2",
         "10000", "100000",
         "AAAAAAAATTTTCCCCGGAAAAAAAA",
         "AAAAAAAAGGGGCCCC--AAAAAAAA"},
        scoring_system, paste_parameters)};
    Alignment false_right_equal_start{Alignment::FromStringFields(1,
        {"135", "160", "1001", "1024",
         "20", "4", "1", "2",
         "10000", "100000",
         "AAAAAAAATTTTCCCCGGAAAAAAAA",
         "AAAAAAAAGGGGCCCC--AAAAAAAA"},
        scoring_system, paste_parameters)};
    // Offset is disregarded here.
    AlignmentConfiguration config{GetConfiguration(
        0, 0, left.Length(), false_right_nooverlap.Length())};


    CHECK_THROWS_AS(false_right_nooverlap.PasteLeft(left, config,
                                                    scoring_system,
                                                    paste_parameters),
                    exceptions::PastingError);
    CHECK_THROWS_AS(false_right_overlap.PasteLeft(left, config,
                                                  scoring_system,
                                                  paste_parameters),
                    exceptions::PastingError);
    CHECK_THROWS_AS(false_right_equal_start.PasteLeft(left, config,
                                                      scoring_system,
                                                      paste_parameters),
                    exceptions::PastingError);
  }

  THEN("Right subject not starting/ending after left causes exception (-).") {
    Alignment left{Alignment::FromStringFields(0,
        {"101", "134", "1036", "1001",
         "24", "8", "2", "6",
         "10000", "100000",
         "AAAAAAAATTTTTTCCCCGGGG----GGGGAAAAAAAA",
         "AAAAAAAACCCC--CCCCGGGGAAAATTTTAAAAAAAA"},
        scoring_system, paste_parameters)};
    Alignment false_right_nooverlap{Alignment::FromStringFields(1,
        {"135", "160", "1060", "1037",
         "20", "4", "1", "2",
         "10000", "100000",
         "AAAAAAAATTTTCCCCGGAAAAAAAA",
         "AAAAAAAAGGGGCCCC--AAAAAAAA"},
        scoring_system, paste_parameters)};
    Alignment false_right_overlap{Alignment::FromStringFields(1,
        {"135", "160", "1050", "1027",
         "20", "4", "1", "2",
         "10000", "100000",
         "AAAAAAAATTTTCCCCGGAAAAAAAA",
         "AAAAAAAAGGGGCCCC--AAAAAAAA"},
        scoring_system, paste_parameters)};
    Alignment false_right_equal_start{Alignment::FromStringFields(1,
        {"135", "160", "1024", "1001",
         "20", "4", "1", "2",
         "10000", "100000",
         "AAAAAAAATTTTCCCCGGAAAAAAAA",
         "AAAAAAAAGGGGCCCC--AAAAAAAA"},
        scoring_system, paste_parameters)};
    // Offset is disregarded here.
    AlignmentConfiguration config{GetConfiguration(
        0, 0, left.Length(), false_right_nooverlap.Length())};

    CHECK_THROWS_AS(false_right_nooverlap.PasteLeft(left, config,
                                                    scoring_system,
                                                    paste_parameters),
                    exceptions::PastingError);
    CHECK_THROWS_AS(false_right_overlap.PasteLeft(left, config,
                                                  scoring_system,
                                                  paste_parameters),
                    exceptions::PastingError);
    CHECK_THROWS_AS(false_right_equal_start.PasteLeft(left, config,
                                                      scoring_system,
                                                      paste_parameters),
                    exceptions::PastingError);
  }
}

} // namespace

} // namespace test

} // namespace paste_alignments