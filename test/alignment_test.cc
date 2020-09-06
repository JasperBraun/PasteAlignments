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

#include "exceptions.h"
#include "paste_parameters.h"
#include "scoring_system.h"

// Alignment tests
//
// Test correctness for:
// * FromStringFields
//
// Test exceptions for:
// * FromStringFields // ensures invariants

// string fields for alignments are (in this order):
//
// qstart qend sstart send
// nident mismatch gapopen gaps
// qlen slen
// qseq sseq
//
// default scoring (megablast):
//
// reward: 1 penalty: 2 gapopen: 0 gapextend: 2.5

namespace paste_alignments {

namespace test {

// Tests if alignment corresponds to the string representations in fields.
bool Equals(const Alignment& alignment,
            const std::vector<std::string>& fields) {
  assert(fields.size() == 12);
  int sstart{std::stoi(fields.at(2))};
  int send{std::stoi(fields.at(3))};
  return (alignment.Qstart() == std::stoi(fields.at(0))
          && alignment.Qend() == std::stoi(fields.at(1))
          && alignment.Sstart() == std::min(sstart, send)
          && alignment.Send() == std::max(sstart, send)
          && alignment.PlusStrand() == (sstart <= send)
          && alignment.Nident() == std::stoi(fields.at(4))
          && alignment.Mismatch() == std::stoi(fields.at(5))
          && alignment.Gapopen() == std::stoi(fields.at(6))
          && alignment.Gaps() == std::stoi(fields.at(7))
          && alignment.Qlen() == std::stoi(fields.at(8))
          && alignment.Slen() == std::stoi(fields.at(9))
          && alignment.Qseq() == fields.at(10)
          && alignment.Sseq() == fields.at(11));
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
        CHECK(Equals(alignment_a, a));
        CHECK(alignment_a.Id() == 1);
        CHECK(alignment_a.PastedIdentifiers() == std::vector<int>{1});
        CHECK(Equals(alignment_b, b));
        CHECK(alignment_b.Id() == 2020);
        CHECK(alignment_b.PastedIdentifiers() == std::vector<int>{2020});
        CHECK(Equals(alignment_c, c));
        CHECK(alignment_c.Id() == -2020);
        CHECK(alignment_c.PastedIdentifiers() == std::vector<int>{-2020});
        CHECK(Equals(alignment_d, d));
        CHECK(alignment_d.Id() == 0);
        CHECK(alignment_d.PastedIdentifiers() == std::vector<int>{0});
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
}

} // namespace

} // namespace test

} // namespace paste_alignments