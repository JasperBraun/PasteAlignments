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

#include "scoring_system.h"

#define CATCH_CONFIG_MAIN
#define CATCH_CONFIG_COLOUR_NONE
#include "catch.h"

#include "string_conversions.h" // include after catch.h

#include <limits>

#include "alignment.h"
#include "exceptions.h"
#include "helpers.h"

// ScoringSystem tests
//
// Test correctness for:
// * Create
// * SetScoringParameters
// * DatabaseSize(long)
// * RawScore
// * Bitscore
// * Evalue
// 
// Test invariants for:
// * Create
// * SetScoringParameters
// * DatabaseSize(long)
//
// Test exceptions for:
// * Create
// * SetScoringParameters
// * DatabaseSize(long)

namespace paste_alignments {

namespace test {

namespace {

SCENARIO("Test correctness of ScoringSystem::Create.",
         "[ScoringSystem][Create][correctness]") {

  GIVEN("Valid set of scoring parameters and positive database size.") {
    int reward, penalty, open_cost, extend_cost;
    long original, database_size;
    database_size = GENERATE(take(3,
        random(1l, std::numeric_limits<long>::max())));
    original = database_size;
    ScoringParameters parameters = GENERATE(from_range(
        ScoringSystem::kBLASTSupportedScoringParameters.cbegin(),
        ScoringSystem::kBLASTSupportedScoringParameters.cend()));
    reward = parameters.reward;
    penalty = parameters.penalty;
    open_cost = parameters.open_cost;
    extend_cost = parameters.extend_cost;

    ScoringSystem scoring_system{ScoringSystem::Create(
        database_size, reward, penalty, open_cost, extend_cost)};

    THEN("Function sets the scoring parameter values correctly.") {
      scoring_system.SetScoringParameters(reward, penalty, open_cost,
                                          extend_cost);
      CHECK(scoring_system.Reward() == parameters.reward);
      CHECK(scoring_system.Penalty() == parameters.penalty);
      CHECK(scoring_system.OpenCost() == parameters.open_cost);
      if (parameters.open_cost == 0 && parameters.extend_cost == 0) {
        CHECK(helpers::FuzzyFloatEquals(
            scoring_system.ExtendCost(),
            helpers::MegablastExtendCost(parameters.reward,
                                         parameters.penalty)));
      } else {
        CHECK(helpers::FuzzyFloatEquals(
            scoring_system.ExtendCost(),
            static_cast<float>(parameters.extend_cost)));
      }
      CHECK(helpers::FuzzyFloatEquals(scoring_system.Lambda(),
                                      parameters.lambda));
      CHECK(helpers::FuzzyFloatEquals(scoring_system.K(), parameters.k));
    }

    THEN("Database size is set correctly.") {
      CHECK(scoring_system.DatabaseSize() == original);
    }
  }
}

SCENARIO("Test invariant preservation by ScoringSystem::Create.",
         "[ScoringSystem][Create][invariants]") {

  GIVEN("Valid set of scoring parameters.") {
    int reward, penalty, open_cost, extend_cost;
    long database_size = GENERATE(take(3,
        random(1l, std::numeric_limits<long>::max())));
    ScoringParameters parameters = GENERATE(from_range(
        ScoringSystem::kBLASTSupportedScoringParameters.cbegin(),
        ScoringSystem::kBLASTSupportedScoringParameters.cend()));
    reward = parameters.reward;
    penalty = parameters.penalty;
    open_cost = parameters.open_cost;
    extend_cost = parameters.extend_cost;

    ScoringSystem scoring_system{ScoringSystem::Create(
        database_size, reward, penalty, open_cost, extend_cost)};

    THEN("Database size is still positive.") {
      CHECK(scoring_system.DatabaseSize() > 0);
    }

    THEN("Object's scoring parameter values are among the supported values.") {
      bool found{false};
      float megablast_gap_extend;
      for (const ScoringParameters& p
           : ScoringSystem::kBLASTSupportedScoringParameters) {
        if (p.reward == scoring_system.Reward()
            && p.penalty == scoring_system.Penalty()
            && p.open_cost == scoring_system.OpenCost()
            && helpers::FuzzyFloatEquals(p.lambda, scoring_system.Lambda())
            && helpers::FuzzyFloatEquals(p.k, scoring_system.K())) {
          if (p.open_cost == 0 && p.extend_cost == 0) { // Megablast value set
            megablast_gap_extend = helpers::MegablastExtendCost(p.reward,
                                                                p.penalty);
            if (helpers::FuzzyFloatEquals(scoring_system.ExtendCost(),
                                          megablast_gap_extend)) {
              found = true;
              break;
            }
          } else {
            if (helpers::FuzzyFloatEquals(scoring_system.ExtendCost(),
                                          static_cast<float>(p.extend_cost))) {
              found = true;
              break;
            }
          }
        }
      }
      CHECK(found);
    }
  }
}

SCENARIO("Test exceptions thrown by ScoringSystem::Create.",
         "[ScoringSystem][Create][exceptions]") {

  GIVEN("Invalid set of scoring parameters.") {
    int reward, penalty, open_cost, extend_cost;
    ScoringParameters valid_parameters{
        ScoringSystem::kBLASTSupportedScoringParameters.at(0)};
    reward = GENERATE(take(2, random(100, 1000)));
    penalty = GENERATE(take(2, random(100, 1000)));
    open_cost = GENERATE(take(2, random(100, 1000)));
    extend_cost = GENERATE(take(2, random(100, 1000)));

    THEN("Function sets the scoring parameter values correctly.") {
      CHECK_THROWS_AS(ScoringSystem::Create(1l,
          reward, valid_parameters.penalty, valid_parameters.open_cost,
          valid_parameters.extend_cost),
          exceptions::ScoringError);
      CHECK_THROWS_AS(ScoringSystem::Create(1l,
          valid_parameters.reward, penalty, valid_parameters.open_cost,
          valid_parameters.extend_cost),
          exceptions::ScoringError);
      CHECK_THROWS_AS(ScoringSystem::Create(1l,
          valid_parameters.reward, valid_parameters.penalty, open_cost,
          valid_parameters.extend_cost),
          exceptions::ScoringError);
      CHECK_THROWS_AS(ScoringSystem::Create(1l,
          valid_parameters.reward, valid_parameters.penalty,
          valid_parameters.open_cost, extend_cost),
          exceptions::ScoringError);
    }
  }

  THEN("Creating with database size 0 causes exception.") {
    CHECK_THROWS_AS(ScoringSystem::Create(0l), exceptions::OutOfRange);
  }

  GIVEN("Negative integer.") {
    long size = GENERATE(take(10, random(std::numeric_limits<long>::min(), -1l)));

    THEN("Exception is thrown.") {
      CHECK_THROWS_AS(ScoringSystem::Create(size), exceptions::OutOfRange);
    }
  }
}

SCENARIO("Test correctness of ScoringSystem::SetScoringParameters.",
         "[ScoringSystem][SetScoringParameters][correctness]") {

  GIVEN("Valid set of scoring parameters.") {
    int reward, penalty, open_cost, extend_cost;
    ScoringParameters parameters = GENERATE(from_range(
        ScoringSystem::kBLASTSupportedScoringParameters.cbegin(),
        ScoringSystem::kBLASTSupportedScoringParameters.cend()));
    reward = parameters.reward;
    penalty = parameters.penalty;
    open_cost = parameters.open_cost;
    extend_cost = parameters.extend_cost;

    ScoringSystem scoring_system{ScoringSystem::Create(1l)};

    THEN("Function sets the scoring parameter values correctly.") {
      scoring_system.SetScoringParameters(reward, penalty, open_cost,
                                          extend_cost);
      CHECK(scoring_system.Reward() == parameters.reward);
      CHECK(scoring_system.Penalty() == parameters.penalty);
      CHECK(scoring_system.OpenCost() == parameters.open_cost);
      if (parameters.open_cost == 0 && parameters.extend_cost == 0) {
        CHECK(helpers::FuzzyFloatEquals(
            scoring_system.ExtendCost(),
            helpers::MegablastExtendCost(parameters.reward,
                                         parameters.penalty)));
      } else {
        CHECK(helpers::FuzzyFloatEquals(
            scoring_system.ExtendCost(),
            static_cast<float>(parameters.extend_cost)));
      }
      CHECK(helpers::FuzzyFloatEquals(scoring_system.Lambda(),
                                      parameters.lambda));
      CHECK(helpers::FuzzyFloatEquals(scoring_system.K(), parameters.k));
    }
  }
}

SCENARIO("Test invariant preservation by ScoringSystem::SetScoringParameters.",
         "[ScoringSystem][SetScoringParameters][invariants]") {

  GIVEN("Valid set of scoring parameters.") {
    int reward, penalty, open_cost, extend_cost;
    ScoringParameters parameters = GENERATE(from_range(
        ScoringSystem::kBLASTSupportedScoringParameters.cbegin(),
        ScoringSystem::kBLASTSupportedScoringParameters.cend()));
    reward = parameters.reward;
    penalty = parameters.penalty;
    open_cost = parameters.open_cost;
    extend_cost = parameters.extend_cost;

    ScoringSystem scoring_system{ScoringSystem::Create(1l)};

    THEN("Database size is still positive.") {
      CHECK(scoring_system.DatabaseSize() > 0);
    }

    THEN("Object's scoring parameter values are among the supported values.") {
      bool found{false};
      float megablast_gap_extend;
      for (const ScoringParameters& p
           : ScoringSystem::kBLASTSupportedScoringParameters) {
        if (p.reward == scoring_system.Reward()
            && p.penalty == scoring_system.Penalty()
            && p.open_cost == scoring_system.OpenCost()
            && helpers::FuzzyFloatEquals(p.lambda, scoring_system.Lambda())
            && helpers::FuzzyFloatEquals(p.k, scoring_system.K())) {
          if (p.open_cost == 0 && p.extend_cost == 0) { // Megablast value set
            megablast_gap_extend = helpers::MegablastExtendCost(p.reward,
                                                                p.penalty);
            if (helpers::FuzzyFloatEquals(scoring_system.ExtendCost(),
                                          megablast_gap_extend)) {
              found = true;
              break;
            }
          } else {
            if (helpers::FuzzyFloatEquals(scoring_system.ExtendCost(),
                                          static_cast<float>(p.extend_cost))) {
              found = true;
              break;
            }
          }
        }
      }
      CHECK(found);
    }
  }
}

SCENARIO("Test exceptions thrown by ScoringSystem::SetScoringParameters.",
         "[ScoringSystem][SetScoringParameters][exceptions]") {

  GIVEN("Invalid set of scoring parameters.") {
    int reward, penalty, open_cost, extend_cost;
    ScoringParameters valid_parameters{
        ScoringSystem::kBLASTSupportedScoringParameters.at(0)};
    reward = GENERATE(take(2, random(100, 1000)));
    penalty = GENERATE(take(2, random(100, 1000)));
    open_cost = GENERATE(take(2, random(100, 1000)));
    extend_cost = GENERATE(take(2, random(100, 1000)));

    ScoringSystem scoring_system{ScoringSystem::Create(1l)};

    THEN("Function sets the scoring parameter values correctly.") {
      CHECK_THROWS_AS(scoring_system.SetScoringParameters(
          reward, valid_parameters.penalty, valid_parameters.open_cost,
          valid_parameters.extend_cost),
          exceptions::ScoringError);
      CHECK_THROWS_AS(scoring_system.SetScoringParameters(
          valid_parameters.reward, penalty, valid_parameters.open_cost,
          valid_parameters.extend_cost),
          exceptions::ScoringError);
      CHECK_THROWS_AS(scoring_system.SetScoringParameters(
          valid_parameters.reward, valid_parameters.penalty, open_cost,
          valid_parameters.extend_cost),
          exceptions::ScoringError);
      CHECK_THROWS_AS(scoring_system.SetScoringParameters(
          valid_parameters.reward, valid_parameters.penalty,
          valid_parameters.open_cost, extend_cost),
          exceptions::ScoringError);
    }
  }
}

SCENARIO("Test correctness of ScoringSystem::DatabaseSize(long).",
         "[ScoringSystem][DatabaseSize(long)][correctness]") {

  GIVEN("Positive integer.") {
    long original, size;
    size = GENERATE(take(10, random(1l, std::numeric_limits<long>::max())));
    original = size;
    ScoringSystem scoring_system{ScoringSystem::Create(1l)};
    scoring_system.DatabaseSize(size);

    THEN("Database size is set correctly.") {
      CHECK(scoring_system.DatabaseSize() == original);
    }
  }
}

SCENARIO("Test invariant preservation by ScoringSystem::DatabaseSize(long).",
         "[ScoringSystem][DatabaseSize(long)][invariants]") {

  GIVEN("Positive integer.") {
    long size = GENERATE(take(10, random(1l, std::numeric_limits<long>::max())));
    ScoringSystem scoring_system{ScoringSystem::Create(1l)};
    scoring_system.DatabaseSize(size);

    THEN("Database size is still positive.") {
      CHECK(scoring_system.DatabaseSize() > 0);
    }
  }
}

SCENARIO("Test exceptions thrown by ScoringSystem::DatabaseSize(long).",
         "[ScoringSystem][DatabaseSize(long)][exceptions]") {

  THEN("Setting database size to 0 causes exception.") {
    ScoringSystem scoring_system{ScoringSystem::Create(1l)};

    CHECK_THROWS_AS(scoring_system.DatabaseSize(0), exceptions::OutOfRange);
  }

  GIVEN("Negative integer.") {
    long size = GENERATE(take(10, random(std::numeric_limits<long>::min(), -1l)));
    ScoringSystem scoring_system{ScoringSystem::Create(1l)};

    THEN("Exception is thrown.") {
      CHECK_THROWS_AS(scoring_system.DatabaseSize(size),
                      exceptions::OutOfRange);
    }
  }
}

SCENARIO("Test correctness of ScoringSystem::Bitscore.",
         "[ScoringSystem][Bitscore][correctness]") {

  // Bitscore = (lambda * score - ln(k)) / ln(2)
  GIVEN("Reward 1, Penalty 5, Megablast parameters.") {
    ScoringSystem scoring_system{ScoringSystem::Create(1l, 1, 5, 0, 0)};

    WHEN("Alignment has score 100.0") {
      Alignment a{Alignment::FromStringFields(0, {
          "101", "110", "1101", "1110",
          "100", "0", "0", "0",
          "10000", "100000",
          "CCCCAAAATT", "CCCCAAAATT"})};
      REQUIRE(helpers::FuzzyFloatEquals(scoring_system.RawScore(a), 100.0f));

      THEN("Bitscore is 200.9554305") {
        CHECK(helpers::FuzzyFloatEquals(scoring_system.Bitscore(a),
                                        200.9554305f));
      }
    }

    WHEN("Alignment has score 200.0") {
      Alignment a{Alignment::FromStringFields(0, {
          "101", "110", "1101", "1110",
          "200", "0", "0", "0",
          "10000", "100000",
          "CCCCAAAATT", "CCCCAAAATT"})};
      REQUIRE(helpers::FuzzyFloatEquals(scoring_system.RawScore(a), 200.0f));

      THEN("Bitscore is 401.4900412") {
        CHECK(helpers::FuzzyFloatEquals(scoring_system.Bitscore(a),
                                        401.4900412f));
      }
    }
  }

  // Bitscore = (lambda * score - ln(k)) / ln(2)
  GIVEN("Reward 1, Penalty 5, OpenCost 3, ExtendCost 3.") {
    ScoringSystem scoring_system{ScoringSystem::Create(1l, 1, 5, 3, 3)};

    WHEN("Alignment has score 100.0") {
      Alignment a{Alignment::FromStringFields(0, {
          "101", "110", "1101", "1110",
          "100", "0", "0", "0",
          "10000", "100000",
          "CCCCAAAATT", "CCCCAAAATT"})};
      REQUIRE(helpers::FuzzyFloatEquals(scoring_system.RawScore(a), 100.0f));

      THEN("Bitscore is 200.9554305") {
        CHECK(helpers::FuzzyFloatEquals(scoring_system.Bitscore(a),
                                        200.9554305f));
      }
    }

    WHEN("Alignment has score 200.0") {
      Alignment a{Alignment::FromStringFields(0, {
          "101", "110", "1101", "1110",
          "200", "0", "0", "0",
          "10000", "100000",
          "CCCCAAAATT", "CCCCAAAATT"})};
      REQUIRE(helpers::FuzzyFloatEquals(scoring_system.RawScore(a), 200.0f));

      THEN("Bitscore is 401.4900412") {
        CHECK(helpers::FuzzyFloatEquals(scoring_system.Bitscore(a),
                                        401.4900412f));
      }
    }
  }

  // Bitscore = (lambda * score - ln(k)) / ln(2)
  GIVEN("Reward 4, Penalty 5, Megablast parameters.") {
    ScoringSystem scoring_system{ScoringSystem::Create(1l, 4, 5, 0, 0)};

    WHEN("Alignment has score 51.0") {
      Alignment a{Alignment::FromStringFields(0, {
          "101", "110", "1101", "1110",
          "20", "3", "1", "2",
          "10000", "100000",
          "CCCCAAAATT", "CCCCAAAATT"})};
      REQUIRE(helpers::FuzzyFloatEquals(scoring_system.RawScore(a), 51.0f));

      THEN("Bitscore is 20.22208531") {
        CHECK(helpers::FuzzyFloatEquals(scoring_system.Bitscore(a),
                                        20.22208531f));
      }
    }

    WHEN("Alignment has score -51.0") {
      Alignment a{Alignment::FromStringFields(0, {
          "101", "110", "1101", "1110",
          "10", "14", "1", "3",
          "10000", "100000",
          "CCCCAAAATT", "CCCCAAAATT"})};
      REQUIRE(helpers::FuzzyFloatEquals(scoring_system.RawScore(a), -51.0f));

      THEN("Bitscore is -12.15199141") {
        CHECK(helpers::FuzzyFloatEquals(scoring_system.Bitscore(a),
                                        -12.15199141f));
      }
    }
  }

  // Bitscore = (lambda * score - ln(k)) / ln(2)
  GIVEN("Reward 4, Penalty 5, OpenCost 4, ExtendCost 5.") {
    ScoringSystem scoring_system{ScoringSystem::Create(1l, 4, 5, 4, 5)};

    WHEN("Alignment has score 151.0") {
      Alignment a{Alignment::FromStringFields(0, {
          "101", "110", "1101", "1110",
          "40", "0", "1", "1",
          "10000", "100000",
          "CCCCAAAATT", "CCCCAAAATT"})};
      REQUIRE(helpers::FuzzyFloatEquals(scoring_system.RawScore(a), 151.0f));

      THEN("Bitscore is 57.78366589") {
        CHECK(helpers::FuzzyFloatEquals(scoring_system.Bitscore(a),
                                        57.78366589f));
      }
    }

    WHEN("Alignment has score -151.0") {
      Alignment a{Alignment::FromStringFields(0, {
          "101", "110", "1101", "1110",
          "17", "42", "1", "1",
          "10000", "100000",
          "CCCCAAAATT", "CCCCAAAATT"})};
      REQUIRE(helpers::FuzzyFloatEquals(scoring_system.RawScore(a), -151.0f));

      THEN("Bitscore is -51.1398097") {
        CHECK(helpers::FuzzyFloatEquals(scoring_system.Bitscore(a),
                                        -51.1398097f));
      }
    }
  }

  // Bitscore = (lambda * score - ln(k)) / ln(2)
  // This parameter value set requires score to be rounded down to nearest even.
  GIVEN("Reward 2, Penalty 3, OpenCost 0, ExtendCost 4.") {
    ScoringSystem scoring_system{ScoringSystem::Create(1l, 2, 3, 0, 4)};

    WHEN("Alignment has score 53.0") {
      Alignment a{Alignment::FromStringFields(0, {
          "101", "110", "1101", "1110",
          "28", "1", "0", "0",
          "10000", "100000",
          "CCCCAAAATT", "CCCCAAAATT"})};
      REQUIRE(helpers::FuzzyFloatEquals(scoring_system.RawScore(a), 53.0f));

      THEN("Bitscore is 42.51261694") {
        CHECK(helpers::FuzzyFloatEquals(scoring_system.Bitscore(a),
                                        43.51261694f));
      }
    }

    WHEN("Alignment has score 52.0") {
      Alignment a{Alignment::FromStringFields(0, {
          "101", "110", "1101", "1110",
          "29", "2", "0", "0",
          "10000", "100000",
          "CCCCAAAATT", "CCCCAAAATT"})};
      REQUIRE(helpers::FuzzyFloatEquals(scoring_system.RawScore(a), 52.0f));

      THEN("Bitscore is 42.51261694") {
        CHECK(helpers::FuzzyFloatEquals(scoring_system.Bitscore(a),
                                        43.51261694f));
      }
    }

    WHEN("Alignment has score 51.0") {
      Alignment a{Alignment::FromStringFields(0, {
          "101", "110", "1101", "1110",
          "27", "1", "0", "0",
          "10000", "100000",
          "CCCCAAAATT", "CCCCAAAATT"})};
      REQUIRE(helpers::FuzzyFloatEquals(scoring_system.RawScore(a), 51.0f));

      THEN("Bitscore is 41.92565239") {
        CHECK(helpers::FuzzyFloatEquals(scoring_system.Bitscore(a),
                                        41.92565239f));
      }
    }

    WHEN("Alignment has score -50.0") {
      Alignment a{Alignment::FromStringFields(0, {
          "101", "110", "1101", "1110",
          "5", "20", "0", "0",
          "10000", "100000",
          "CCCCAAAATT", "CCCCAAAATT"})};
      REQUIRE(helpers::FuzzyFloatEquals(scoring_system.RawScore(a), -50.0f));

      THEN("Bitscore is -37.42257486") {
        CHECK(helpers::FuzzyFloatEquals(scoring_system.Bitscore(a),
                                        -37.42257486f));
      }
    }

    WHEN("Alignment has score -51.0") {
      Alignment a{Alignment::FromStringFields(0, {
          "101", "110", "1101", "1110",
          "6", "21", "0", "0",
          "10000", "100000",
          "CCCCAAAATT", "CCCCAAAATT"})};
      REQUIRE(helpers::FuzzyFloatEquals(scoring_system.RawScore(a), -51.0f));

      THEN("Bitscore is -39.0095394") {
        CHECK(helpers::FuzzyFloatEquals(scoring_system.Bitscore(a),
                                        -39.0095394f));
      }
    }

    WHEN("Alignment has score -52.0") {
      Alignment a{Alignment::FromStringFields(0, {
          "101", "110", "1101", "1110",
          "4", "20", "0", "0",
          "10000", "100000",
          "CCCCAAAATT", "CCCCAAAATT"})};
      REQUIRE(helpers::FuzzyFloatEquals(scoring_system.RawScore(a), -52.0f));

      THEN("Bitscore is -39.0095394") {
        CHECK(helpers::FuzzyFloatEquals(scoring_system.Bitscore(a),
                                        -39.0095394f));
      }
    }
  }

  // Bitscore = (lambda * score - ln(k)) / ln(2)
  // This parameter value set requires score to be rounded down to nearest even.
  GIVEN("Reward 2, Penalty 5, OpenCost 2, ExtendCost 4.") {
    ScoringSystem scoring_system{ScoringSystem::Create(1l, 2, 5, 2, 4)};

    WHEN("Alignment has score 2.0") {
      Alignment a{Alignment::FromStringFields(0, {
          "101", "110", "1101", "1110",
          "6", "2", "0", "0",
          "10000", "100000",
          "CCCCAAAATT", "CCCCAAAATT"})};
      REQUIRE(helpers::FuzzyFloatEquals(scoring_system.RawScore(a), 2.0f));

      THEN("Bitscore is 2.694424495") {
        CHECK(helpers::FuzzyFloatEquals(scoring_system.Bitscore(a),
                                        2.694424495f));
      }
    }

    WHEN("Alignment has score 1.0") {
      Alignment a{Alignment::FromStringFields(0, {
          "101", "110", "1101", "1110",
          "3", "1", "0", "0",
          "10000", "100000",
          "CCCCAAAATT", "CCCCAAAATT"})};
      REQUIRE(helpers::FuzzyFloatEquals(scoring_system.RawScore(a), 1.0f));

      THEN("Bitscore is 0.7612131404") {
        CHECK(helpers::FuzzyFloatEquals(scoring_system.Bitscore(a),
                                        0.7612131404f));
      }
    }

    WHEN("Alignment has score 0.0") {
      Alignment a{Alignment::FromStringFields(0, {
          "101", "110", "1101", "1110",
          "10", "4", "0", "0",
          "10000", "100000",
          "CCCCAAAATT", "CCCCAAAATT"})};
      REQUIRE(helpers::FuzzyFloatEquals(scoring_system.RawScore(a), 0.0f));

      THEN("Bitscore is 0.7612131404") {
        CHECK(helpers::FuzzyFloatEquals(scoring_system.Bitscore(a),
                                        0.7612131404f));
      }
    }

    WHEN("Alignment has score -1.0") {
      Alignment a{Alignment::FromStringFields(0, {
          "101", "110", "1101", "1110",
          "2", "1", "0", "0",
          "10000", "100000",
          "CCCCAAAATT", "CCCCAAAATT"})};
      REQUIRE(helpers::FuzzyFloatEquals(scoring_system.RawScore(a), -1.0f));

      THEN("Bitscore is -1.171998214") {
        CHECK(helpers::FuzzyFloatEquals(scoring_system.Bitscore(a),
                                        -1.171998214f));
      }
    }

    WHEN("Alignment has score -2.0") {
      Alignment a{Alignment::FromStringFields(0, {
          "101", "110", "1101", "1110",
          "4", "2", "0", "0",
          "10000", "100000",
          "CCCCAAAATT", "CCCCAAAATT"})};
      REQUIRE(helpers::FuzzyFloatEquals(scoring_system.RawScore(a), -2.0f));

      THEN("Bitscore is -1.171998214") {
        CHECK(helpers::FuzzyFloatEquals(scoring_system.Bitscore(a),
                                        -1.171998214f));
      }
    }
  }
}

SCENARIO("Testing correctness of ScoringSystem::Evalue.",
         "[ScoringSystem][Evalue][correctness]") {

  // evalue = K x qlen x database_size x (e ^ (-lambda x score))
  GIVEN("DatabaseSize 10,000, Reward 1, Penalty 5, Megablast.") {
    ScoringSystem scoring_system{ScoringSystem::Create(10000l, 1, 5, 0, 0)};

    WHEN("Alignment has score 100.0 and qlen is 80.") {
      Alignment a{Alignment::FromStringFields(0, {
          "101", "110", "1101", "1110",
          "100", "0", "0", "0",
          "80", "10000",
          "CCCCAAAATT", "CCCCAAAATT"})};
      REQUIRE(helpers::FuzzyFloatEquals(scoring_system.RawScore(a), 100.0f));
      REQUIRE(a.qlen() == 80);

      THEN("Evalue is 2.567e-55.") {
        CHECK(helpers::FuzzyDoubleEquals(scoring_system.Evalue(a),
                                         2.567305814e-55));
      }
    }

    WHEN("Alignment has score 50.0 and qlen is 80.") {
      Alignment a{Alignment::FromStringFields(0, {
          "101", "110", "1101", "1110",
          "50", "0", "0", "0",
          "80", "10000",
          "CCCCAAAATT", "CCCCAAAATT"})};
      REQUIRE(helpers::FuzzyFloatEquals(scoring_system.RawScore(a), 50.0f));
      REQUIRE(a.qlen() == 80);

      THEN("Evalue is 3.916914544e-25.") {
        CHECK(helpers::FuzzyDoubleEquals(scoring_system.Evalue(a),
                                         3.916914544e-25));
      }
    }
  }

  // evalue = K x qlen x database_size x (e ^ (-lambda x score))
  GIVEN("DatabaseSize 10,000, Reward 1, Penalty 5, OpenCost 3, ExtendCost 3.") {
    ScoringSystem scoring_system{ScoringSystem::Create(10000l, 1, 5, 3, 3)};

    WHEN("Alignment has score 100.0 and qlen is 160.") {
      Alignment a{Alignment::FromStringFields(0, {
          "101", "110", "1101", "1110",
          "100", "0", "0", "0",
          "160", "10000",
          "CCCCAAAATT", "CCCCAAAATT"})};
      REQUIRE(helpers::FuzzyFloatEquals(scoring_system.RawScore(a), 100.0f));
      REQUIRE(a.qlen() == 160);

      THEN("Evalue is 5.134611627-55.") {
        CHECK(helpers::FuzzyDoubleEquals(scoring_system.Evalue(a),
                                         5.134611627e-55));
      }
    }

    WHEN("Alignment has score 100.0 and qlen is 80.") {
      Alignment a{Alignment::FromStringFields(0, {
          "101", "110", "1101", "1110",
          "100", "0", "0", "0",
          "80", "10000",
          "CCCCAAAATT", "CCCCAAAATT"})};
      REQUIRE(helpers::FuzzyFloatEquals(scoring_system.RawScore(a), 100.0f));
      REQUIRE(a.qlen() == 80);

      THEN("Evalue is 2.567305814e-55.") {
        CHECK(helpers::FuzzyDoubleEquals(scoring_system.Evalue(a),
                                         2.567305814e-55));
      }
    }
  }

  // evalue = K x qlen x database_size x (e ^ (-lambda x score))
  GIVEN("DatabaseSize 10,000, Reward 4, Penalty 5, Megablast.") {
    ScoringSystem scoring_system{ScoringSystem::Create(10000l, 4, 5, 0, 0)};

    WHEN("Alignment has score 30,000.0 and qlen is 80.") {
      Alignment a{Alignment::FromStringFields(0, {
          "101", "110", "1101", "1110",
          "7500", "0", "0", "0",
          "80", "10000",
          "CCCCAAAATT", "CCCCAAAATT"})};
      REQUIRE(helpers::FuzzyFloatEquals(scoring_system.RawScore(a), 30000.0f));
      REQUIRE(a.qlen() == 80);

      THEN("Evalue is 0.0.") {
        CHECK(helpers::FuzzyDoubleEquals(scoring_system.Evalue(a),
                                         0.0));
      }
    }

    WHEN("Alignment has score -105.0 and qlen is 10,000.") {
      Alignment a{Alignment::FromStringFields(0, {
          "101", "110", "1101", "1110",
          "0", "21", "0", "0",
          "10000", "10000",
          "CCCCAAAATT", "CCCCAAAATT"})};
      REQUIRE(helpers::FuzzyFloatEquals(scoring_system.RawScore(a), -105.0f));
      REQUIRE(a.qlen() == 10000);

      THEN("Evalue is 6.569500756e16.") {
        CHECK(helpers::FuzzyDoubleEquals(scoring_system.Evalue(a),
                                         6.569500756e16));
      }
    }
  }

  // evalue = K x qlen x database_size x (e ^ (-lambda x score))
  // This parameter value set requires score to be rounded down to nearest even.
  GIVEN("DatabaseSize 10,000, Reward 2, Penalty 3, OpenCost 0, ExtendCost 4.") {
    ScoringSystem scoring_system{ScoringSystem::Create(10000l, 2, 3, 0, 4)};

    WHEN("Alignment has score 155.0 and qlen is 10000.") {
      Alignment a{Alignment::FromStringFields(0, {
          "101", "110", "1101", "1110",
          "79", "1", "0", "0",
          "10000", "10000",
          "CCCCAAAATT", "CCCCAAAATT"})};
      REQUIRE(helpers::FuzzyFloatEquals(scoring_system.RawScore(a), 155.0f));
      REQUIRE(a.qlen() == 10000);

      THEN("Evalue is 3.447280935e-30.") {
        CHECK(helpers::FuzzyDoubleEquals(scoring_system.Evalue(a),
                                         3.447280935e-30));
      }
    }

    WHEN("Alignment has score 154.0 and qlen is 10000.") {
      Alignment a{Alignment::FromStringFields(0, {
          "101", "110", "1101", "1110",
          "77", "0", "0", "0",
          "10000", "10000",
          "CCCCAAAATT", "CCCCAAAATT"})};
      REQUIRE(helpers::FuzzyFloatEquals(scoring_system.RawScore(a), 154.0f));
      REQUIRE(a.qlen() == 10000);

      THEN("Evalue is 3.447280935e-30.") {
        CHECK(helpers::FuzzyDoubleEquals(scoring_system.Evalue(a),
                                         3.447280935e-30));
      }
    }

    WHEN("Alignment has score -155.0 and qlen is 10000.") {
      Alignment a{Alignment::FromStringFields(0, {
          "101", "110", "1101", "1110",
          "2", "53", "0", "0",
          "10000", "10000",
          "CCCCAAAATT", "CCCCAAAATT"})};
      REQUIRE(helpers::FuzzyFloatEquals(scoring_system.RawScore(a), -155.0f));
      REQUIRE(a.qlen() == 10000);

      THEN("Evalue is 3.843136784e44.") {
        CHECK(helpers::FuzzyDoubleEquals(scoring_system.Evalue(a),
                                         3.843136784e44));
      }
    }

    WHEN("Alignment has score -154.0 and qlen is 10000.") {
      Alignment a{Alignment::FromStringFields(0, {
          "101", "110", "1101", "1110",
          "1", "52", "0", "0",
          "10000", "10000",
          "CCCCAAAATT", "CCCCAAAATT"})};
      REQUIRE(helpers::FuzzyFloatEquals(scoring_system.RawScore(a), -154.0f));
      REQUIRE(a.qlen() == 10000);

      THEN("Evalue is 1.279269106e44.") {
        CHECK(helpers::FuzzyDoubleEquals(scoring_system.Evalue(a),
                                         1.279269106e44));
      }
    }
  }

  // evalue = K x qlen x database_size x (e ^ (-lambda x score))
  // This parameter value set requires score to be rounded down to nearest even.
  GIVEN("DatabaseSize 10,000, Reward 2, Penalty 5, OpenCost 2, ExtendCost 4.") {
    ScoringSystem scoring_system{ScoringSystem::Create(10000l, 2, 5, 2, 4)};

    WHEN("Alignment has score 3.0 and qlen is 10000.") {
      Alignment a{Alignment::FromStringFields(0, {
          "101", "110", "1101", "1110",
          "4", "1", "0", "0",
          "10000", "10000",
          "CCCCAAAATT", "CCCCAAAATT"})};
      REQUIRE(helpers::FuzzyFloatEquals(scoring_system.RawScore(a), 3.0f));
      REQUIRE(a.qlen() == 10000);

      THEN("Evalue is 1.544889445e7.") {
        CHECK(helpers::FuzzyDoubleEquals(scoring_system.Evalue(a),
                                         1.544889445e7));
      }
    }

    WHEN("Alignment has score 2.0 and qlen is 10000.") {
      Alignment a{Alignment::FromStringFields(0, {
          "101", "110", "1101", "1110",
          "1", "0", "0", "0",
          "10000", "10000",
          "CCCCAAAATT", "CCCCAAAATT"})};
      REQUIRE(helpers::FuzzyFloatEquals(scoring_system.RawScore(a), 2.0f));
      REQUIRE(a.qlen() == 10000);

      THEN("Evalue is 1.544889445e7.") {
        CHECK(helpers::FuzzyDoubleEquals(scoring_system.Evalue(a),
                                         1.544889445e7));
      }
    }

    WHEN("Alignment has score 1.0 and qlen is 10000.") {
      Alignment a{Alignment::FromStringFields(0, {
          "101", "110", "1101", "1110",
          "3", "1", "0", "0",
          "10000", "10000",
          "CCCCAAAATT", "CCCCAAAATT"})};
      REQUIRE(helpers::FuzzyFloatEquals(scoring_system.RawScore(a), 1.0f));
      REQUIRE(a.qlen() == 10000);

      THEN("Evalue is 5.9e7.") {
        CHECK(helpers::FuzzyDoubleEquals(scoring_system.Evalue(a),
                                         5.9e7));
      }
    }

    WHEN("Alignment has score 0.0 and qlen is 10000.") {
      Alignment a{Alignment::FromStringFields(0, {
          "101", "110", "1101", "1110",
          "5", "2", "0", "0",
          "10000", "10000",
          "CCCCAAAATT", "CCCCAAAATT"})};
      REQUIRE(helpers::FuzzyFloatEquals(scoring_system.RawScore(a), 0.0f));
      REQUIRE(a.qlen() == 10000);

      THEN("Evalue is 5.9e7.") {
        CHECK(helpers::FuzzyDoubleEquals(scoring_system.Evalue(a),
                                         5.9e7));
      }
    }

    WHEN("Alignment has score -1.0 and qlen is 10000.") {
      Alignment a{Alignment::FromStringFields(0, {
          "101", "110", "1101", "1110",
          "2", "1", "0", "0",
          "10000", "10000",
          "CCCCAAAATT", "CCCCAAAATT"})};
      REQUIRE(helpers::FuzzyFloatEquals(scoring_system.RawScore(a), -1.0f));
      REQUIRE(a.qlen() == 10000);

      THEN("Evalue is 2.253235668e8.") {
        CHECK(helpers::FuzzyDoubleEquals(scoring_system.Evalue(a),
                                         2.253235668e8));
      }
    }
  }
}

SCENARIO("Testing correctness of ScoringSystem::RawScore.",
         "[ScoringSystem][RawScore][correctness]") {

  GIVEN("Supported scoring parameter values.") {
    ScoringParameters parameters = GENERATE(from_range(
        ScoringSystem::kBLASTSupportedScoringParameters.cbegin(),
        ScoringSystem::kBLASTSupportedScoringParameters.cend()));
    int reward{parameters.reward},
        penalty{parameters.penalty},
        open_cost{parameters.open_cost},
        extend_cost{parameters.extend_cost};
    ScoringSystem scoring_system{ScoringSystem::Create(
        1l, reward, penalty, open_cost, extend_cost)};

    WHEN("Various alignments' raw scores are calculated.") {
      int nident, mismatch, gapopen, gaps;
      nident = GENERATE(take(2, random(0, 10000)));
      mismatch = GENERATE(take(2, random(0, 10000)));
      gapopen = GENERATE(take(2, random(0, 10000)));
      gaps = GENERATE(take(2, random(0, 10000)));
      if (nident + mismatch + gapopen + gaps == 0) {
        nident += 1;
      }
      Alignment a{Alignment::FromStringFields(0, {
          "101", "110", "1101", "1110",
          std::to_string(nident), std::to_string(mismatch),
          std::to_string(gapopen), std::to_string(gaps),
          "10000", "10000",
          "CCCCAAAATT", "CCCCAAAATT"
      })};

      THEN("The scores are calculated according to the formula.") {
        // reward * nident - penalty * mismatch
        //   - open_cost * gapopen - extend_cost * gaps
        float expected_raw_score = nident * scoring_system.Reward()
                                   - mismatch * scoring_system.Penalty()
                                   - gapopen * scoring_system.OpenCost()
                                   - gaps * scoring_system.ExtendCost();
        CHECK(helpers::FuzzyFloatEquals(scoring_system.RawScore(a),
                                        expected_raw_score));
      }
    }
  }
}

} // namespace

} // namespace test

} // namespace paste_alignments