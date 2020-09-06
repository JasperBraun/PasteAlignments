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

#include <cmath>
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
    long size
        = GENERATE(take(10, random(std::numeric_limits<long>::min(), -1l)));
    ScoringSystem scoring_system{ScoringSystem::Create(1l)};

    THEN("Exception is thrown.") {
      CHECK_THROWS_AS(scoring_system.DatabaseSize(size),
                      exceptions::OutOfRange);
    }
  }
}

SCENARIO("Testing correctness of ScoringSystem::RawScore.",
         "[ScoringSystem][RawScore][correctness]") {

  GIVEN("Supported scoring parameter values.") {
    PasteParameters paste_parameters;
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

      THEN("The scores are calculated according to the formula.") {
        // reward * nident - penalty * mismatch
        //   - open_cost * gapopen - extend_cost * gaps
        float expected_raw_score{nident * scoring_system.Reward()
                                 - mismatch * scoring_system.Penalty()
                                 - gapopen * scoring_system.OpenCost()
                                 - gaps * scoring_system.ExtendCost()};
        float actual_raw_score{scoring_system.RawScore(nident, mismatch,
                                                       gapopen, gaps)};
        CHECK(helpers::FuzzyFloatEquals(actual_raw_score, expected_raw_score,
                                        paste_parameters.float_epsilon));
      }
    }
  }
}

SCENARIO("Test correctness of ScoringSystem::Bitscore.",
         "[ScoringSystem][Bitscore][correctness]") {
  PasteParameters paste_parameters;
  
  GIVEN("Reward 1, Penalty 5, Megablast parameters.") {
    ScoringSystem scoring_system{ScoringSystem::Create(1l, 1, 5, 0, 0)};

    WHEN("Score 100.0.") {
      float raw_score{100.0f};

      THEN("Bitscore is 200.9554305") {
        float bitscore{scoring_system.Bitscore(raw_score, paste_parameters)};
        CHECK(helpers::FuzzyFloatEquals(bitscore, 200.9554305f));
      }
    }

    WHEN("Score 200.0.") {
      float raw_score{200.0f};

      THEN("Bitscore is 401.4900412") {
        float bitscore{scoring_system.Bitscore(raw_score, paste_parameters)};
        CHECK(helpers::FuzzyFloatEquals(bitscore, 401.4900412f));
      }
    }
  }

  // Bitscore = (lambda * score - ln(k)) / ln(2)
  GIVEN("Reward 1, Penalty 5, OpenCost 3, ExtendCost 3.") {
    ScoringSystem scoring_system{ScoringSystem::Create(1l, 1, 5, 3, 3)};

    WHEN("Score 100.0.") {
      float raw_score{100.0f};

      THEN("Bitscore is 200.9554305") {
        float bitscore{scoring_system.Bitscore(raw_score, paste_parameters)};
        CHECK(helpers::FuzzyFloatEquals(bitscore, 200.9554305f));
      }
    }

    WHEN("Score 200.0.") {
      float raw_score{200.0f};

      THEN("Bitscore is 401.4900412") {
        float bitscore{scoring_system.Bitscore(raw_score, paste_parameters)};
        CHECK(helpers::FuzzyFloatEquals(bitscore, 401.4900412f));
      }
    }
  }

  // Bitscore = (lambda * score - ln(k)) / ln(2)
  GIVEN("Reward 4, Penalty 5, Megablast parameters.") {
    ScoringSystem scoring_system{ScoringSystem::Create(1l, 4, 5, 0, 0)};

    WHEN("Score 51.0.") {
      float raw_score{51.0f};

      THEN("Bitscore is 20.22208531") {
        float bitscore{scoring_system.Bitscore(raw_score, paste_parameters)};
        CHECK(helpers::FuzzyFloatEquals(bitscore, 20.22208531f));
      }
    }

    WHEN("Score -51.0.") {
      float raw_score{-51.0f};

      THEN("Bitscore is -12.15199141") {
        float bitscore{scoring_system.Bitscore(raw_score, paste_parameters)};
        CHECK(helpers::FuzzyFloatEquals(bitscore, -12.15199141f));
      }
    }
  }

  // Bitscore = (lambda * score - ln(k)) / ln(2)
  GIVEN("Reward 4, Penalty 5, OpenCost 4, ExtendCost 5.") {
    ScoringSystem scoring_system{ScoringSystem::Create(1l, 4, 5, 4, 5)};

    WHEN("Score 151.0.") {
      float raw_score{151.0f};

      THEN("Bitscore is 57.78366589") {
        float bitscore{scoring_system.Bitscore(raw_score, paste_parameters)};
        CHECK(helpers::FuzzyFloatEquals(bitscore, 57.78366589f));
      }
    }

    WHEN("Score -151.0.") {
      float raw_score{-151.0f};

      THEN("Bitscore is -51.1398097") {
        float bitscore{scoring_system.Bitscore(raw_score, paste_parameters)};
        CHECK(helpers::FuzzyFloatEquals(bitscore, -51.1398097f));
      }
    }
  }

  // Bitscore = (lambda * score - ln(k)) / ln(2)
  // This parameter value set requires score to be rounded down to nearest even.
  GIVEN("Reward 2, Penalty 3, OpenCost 0, ExtendCost 4.") {
    ScoringSystem scoring_system{ScoringSystem::Create(1l, 2, 3, 0, 4)};

    WHEN("Score 53.0.") {
      float raw_score{53.0f};

      THEN("Bitscore is 42.51261694") {
        float bitscore{scoring_system.Bitscore(raw_score, paste_parameters)};
        CHECK(helpers::FuzzyFloatEquals(bitscore, 43.51261694f));
      }
    }

    WHEN("Score 52.0.") {
      float raw_score{52.0f};

      THEN("Bitscore is 42.51261694") {
        float bitscore{scoring_system.Bitscore(raw_score, paste_parameters)};
        CHECK(helpers::FuzzyFloatEquals(bitscore, 43.51261694f));
      }
    }

    WHEN("Score 51.0.") {
      float raw_score{51.0f};

      THEN("Bitscore is 41.92565239") {
        float bitscore{scoring_system.Bitscore(raw_score, paste_parameters)};
        CHECK(helpers::FuzzyFloatEquals(bitscore, 41.92565239f));
      }
    }

    WHEN("Score -50.0.") {
      float raw_score{-50.0f};

      THEN("Bitscore is -37.42257486") {
        float bitscore{scoring_system.Bitscore(raw_score, paste_parameters)};
        CHECK(helpers::FuzzyFloatEquals(bitscore, -37.42257486f));
      }
    }

    WHEN("Score -51.0.") {
      float raw_score{-51.0f};

      THEN("Bitscore is -39.0095394") {
        float bitscore{scoring_system.Bitscore(raw_score, paste_parameters)};
        CHECK(helpers::FuzzyFloatEquals(bitscore, -39.0095394f));
      }
    }

    WHEN("Score -52.0.") {
      float raw_score{-52.0f};

      THEN("Bitscore is -39.0095394") {
        float bitscore{scoring_system.Bitscore(raw_score, paste_parameters)};
        CHECK(helpers::FuzzyFloatEquals(bitscore, -39.0095394f));
      }
    }
  }

  // Bitscore = (lambda * score - ln(k)) / ln(2)
  // This parameter value set requires score to be rounded down to nearest even.
  GIVEN("Reward 2, Penalty 5, OpenCost 2, ExtendCost 4.") {
    ScoringSystem scoring_system{ScoringSystem::Create(1l, 2, 5, 2, 4)};

    WHEN("Score 2.0.") {
      float raw_score{2.0f};

      THEN("Bitscore is 2.694424495") {
        float bitscore{scoring_system.Bitscore(raw_score, paste_parameters)};
        CHECK(helpers::FuzzyFloatEquals(bitscore, 2.694424495f));
      }
    }

    WHEN("Score 1.0.") {
      float raw_score{1.0f};

      THEN("Bitscore is 0.7612131404") {
        float bitscore{scoring_system.Bitscore(raw_score, paste_parameters)};
        CHECK(helpers::FuzzyFloatEquals(bitscore, 0.7612131404f));
      }
    }

    WHEN("Score 0.0.") {
      float raw_score{0.0f};

      THEN("Bitscore is 0.7612131404") {
        float bitscore{scoring_system.Bitscore(raw_score, paste_parameters)};
        CHECK(helpers::FuzzyFloatEquals(bitscore, 0.7612131404f));
      }
    }

    WHEN("Score -1.0.") {
      float raw_score{-1.0f};

      THEN("Bitscore is -1.171998214") {
        float bitscore{scoring_system.Bitscore(raw_score, paste_parameters)};
        CHECK(helpers::FuzzyFloatEquals(bitscore, -1.171998214f));
      }
    }

    WHEN("Score -2.0.") {
      float raw_score{-2.0f};

      THEN("Bitscore is -1.171998214") {
        float bitscore{scoring_system.Bitscore(raw_score, paste_parameters)};
        CHECK(helpers::FuzzyFloatEquals(bitscore, -1.171998214f));
      }
    }
  }
}

SCENARIO("Testing correctness of ScoringSystem::Evalue.",
         "[ScoringSystem][Evalue][correctness]") {
  PasteParameters paste_parameters;

  // evalue = K x qlen x database_size x (e ^ (-lambda x score))
  GIVEN("DatabaseSize 10,000, Reward 1, Penalty 5, Megablast.") {
    ScoringSystem scoring_system{ScoringSystem::Create(10000l, 1, 5, 0, 0)};

    WHEN("Score 100.0; qlen 80.") {
      float raw_score{100.0f};
      int qlen{80};

      THEN("Evalue 2.567e-55.") {
        double evalue{scoring_system.Evalue(raw_score, qlen, paste_parameters)};
        CHECK(helpers::FuzzyDoubleEquals(evalue, 2.567305814e-55));
      }
    }

    WHEN("Score 50.0; qlen 80.") {
      float raw_score{50.0f};
      int qlen{80};

      THEN("Evalue 3.916914544e-25.") {
        double evalue{scoring_system.Evalue(raw_score, qlen, paste_parameters)};
        CHECK(helpers::FuzzyDoubleEquals(evalue, 3.916914544e-25));
      }
    }
  }

  // evalue = K x qlen x database_size x (e ^ (-lambda x score))
  GIVEN("DatabaseSize 10,000, Reward 1, Penalty 5, OpenCost 3, ExtendCost 3.") {
    ScoringSystem scoring_system{ScoringSystem::Create(10000l, 1, 5, 3, 3)};

    WHEN("Score 100.0; qlen 160.") {
      float raw_score{100.0f};
      int qlen{160};

      THEN("Evalue 5.134611627-55.") {
        double evalue{scoring_system.Evalue(raw_score, qlen, paste_parameters)};
        CHECK(helpers::FuzzyDoubleEquals(evalue, 5.134611627e-55));
      }
    }

    WHEN("Score 100.0; qlen 80.") {
      float raw_score{100.0f};
      int qlen{80};

      THEN("Evalue 2.567305814e-55.") {
        double evalue{scoring_system.Evalue(raw_score, qlen, paste_parameters)};
        CHECK(helpers::FuzzyDoubleEquals(evalue, 2.567305814e-55));
      }
    }
  }

  // evalue = K x qlen x database_size x (e ^ (-lambda x score))
  GIVEN("DatabaseSize 10,000, Reward 4, Penalty 5, Megablast.") {
    ScoringSystem scoring_system{ScoringSystem::Create(10000l, 4, 5, 0, 0)};

    WHEN("Score 30,000.0; qlen 80.") {
      float raw_score{30000.0f};
      int qlen{80};

      THEN("Evalue 0.0.") {
        double evalue{scoring_system.Evalue(raw_score, qlen, paste_parameters)};
        CHECK(helpers::FuzzyDoubleEquals(evalue, 0.0));
      }
    }

    WHEN("Score -105.0; qlen 10,000.") {
      float raw_score{-105.0f};
      int qlen{10000};

      THEN("Evalue 6.569500756e16.") {
        double evalue{scoring_system.Evalue(raw_score, qlen, paste_parameters)};
        CHECK(helpers::FuzzyDoubleEquals(evalue, 6.569500756e16));
      }
    }
  }

  // evalue = K x qlen x database_size x (e ^ (-lambda x score))
  // This parameter value set requires score to be rounded down to nearest even.
  GIVEN("DatabaseSize 10,000, Reward 2, Penalty 3, OpenCost 0, ExtendCost 4.") {
    ScoringSystem scoring_system{ScoringSystem::Create(10000l, 2, 3, 0, 4)};

    WHEN("Score 155.0; qlen 10000.") {
      float raw_score{155.0f};
      int qlen{10000};

      THEN("Evalue 3.447280935e-30.") {
        double evalue{scoring_system.Evalue(raw_score, qlen, paste_parameters)};
        CHECK(helpers::FuzzyDoubleEquals(evalue, 3.447280935e-30));
      }
    }

    WHEN("Score 154.0; qlen 10000.") {
      float raw_score{154.0f};
      int qlen{10000};

      THEN("Evalue 3.447280935e-30.") {
        double evalue{scoring_system.Evalue(raw_score, qlen, paste_parameters)};
        CHECK(helpers::FuzzyDoubleEquals(evalue, 3.447280935e-30));
      }
    }

    WHEN("Score -155.0; qlen 10000.") {
      float raw_score{-155.0f};
      int qlen{10000};

      THEN("Evalue 3.843136784e44.") {
        double evalue{scoring_system.Evalue(raw_score, qlen, paste_parameters)};
        CHECK(helpers::FuzzyDoubleEquals(evalue, 3.843136784e44));
      }
    }

    WHEN("Score -154.0; qlen 10000.") {
      float raw_score{-154.0f};
      int qlen{10000};

      THEN("Evalue 1.279269106e44.") {
        double evalue{scoring_system.Evalue(raw_score, qlen, paste_parameters)};
        CHECK(helpers::FuzzyDoubleEquals(evalue, 1.279269106e44));
      }
    }
  }

  // evalue = K x qlen x database_size x (e ^ (-lambda x score))
  // This parameter value set requires score to be rounded down to nearest even.
  GIVEN("DatabaseSize 10,000, Reward 2, Penalty 5, OpenCost 2, ExtendCost 4.") {
    ScoringSystem scoring_system{ScoringSystem::Create(10000l, 2, 5, 2, 4)};

    WHEN("Score 3.0; qlen 10000.") {
      float raw_score{3.0f};
      int qlen{10000};

      THEN("Evalue 1.544889445e7.") {
        double evalue{scoring_system.Evalue(raw_score, qlen, paste_parameters)};
        CHECK(helpers::FuzzyDoubleEquals(evalue, 1.544889445e7));
      }
    }

    WHEN("Score 2.0; qlen 10000.") {
      float raw_score{2.0f};
      int qlen{10000};

      THEN("Evalue 1.544889445e7.") {
        double evalue{scoring_system.Evalue(raw_score, qlen, paste_parameters)};
        CHECK(helpers::FuzzyDoubleEquals(evalue, 1.544889445e7));
      }
    }

    WHEN("Score 1.0; qlen 10000.") {
      float raw_score{1.0f};
      int qlen{10000};

      THEN("Evalue 5.9e7.") {
        double evalue{scoring_system.Evalue(raw_score, qlen, paste_parameters)};
        CHECK(helpers::FuzzyDoubleEquals(evalue, 5.9e7));
      }
    }

    WHEN("Score 0.0; qlen 10000.") {
      float raw_score{0.0f};
      int qlen{10000};

      THEN("Evalue 5.9e7.") {
        double evalue{scoring_system.Evalue(raw_score, qlen, paste_parameters)};
        CHECK(helpers::FuzzyDoubleEquals(evalue, 5.9e7));
      }
    }

    WHEN("Score -1.0; qlen 10000.") {
      float raw_score{-1.0f};
      int qlen{10000};

      THEN("Evalue 2.253235668e8.") {
        double evalue{scoring_system.Evalue(raw_score, qlen, paste_parameters)};
        CHECK(helpers::FuzzyDoubleEquals(evalue, 2.253235668e8));
      }
    }
  }
}

} // namespace

} // namespace test

} // namespace paste_alignments