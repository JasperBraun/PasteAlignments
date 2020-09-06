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

#include "helpers.h"

#define CATCH_CONFIG_MAIN
#define CATCH_CONFIG_COLOUR_NONE
#include "catch.h"

#include "string_conversions.h" // include after catch.h

#include <algorithm>
#include <cctype>
#include <limits>
#include <string>

#include "exceptions.h"

// helpers tests
//
// Test correctness for:
// * TestInRange
// * TestPositive(int)
// * TestPositive(long)
// * TestNonNegative(int)
// * TestNonNegative(long)
// * TestNonEmpty(string)
// * TestNonEmpty(string_view)
// * StringViewToInteger
// * FuzzyFloatEquals
// * MegablastExtendCost
// 
// Test invariants for:
//
// Test exceptions for:
// * TestInRange
// * TestPositive(int)
// * TestPositive(long)
// * TestNonNegative(int)
// * TestNonNegative(long)
// * TestNonEmpty(string)
// * TestNonEmpty(string)
// * StringViewToInteger

namespace paste_alignments {

namespace test {

namespace {

SCENARIO("Test correctness of helpers::TestInRange.",
         "[helpers][TestInRange][correctness]") {

  GIVEN("Various integer ranges and a query integer inside each range.") {
    int a, b, c, first, last, median, query;
    a = GENERATE(take(3, random(std::numeric_limits<int>::min(),
                                std::numeric_limits<int>::max())));
    b = GENERATE(take(3, random(std::numeric_limits<int>::min(),
                                std::numeric_limits<int>::max())));
    c = GENERATE(take(3, random(std::numeric_limits<int>::min(),
                                std::numeric_limits<int>::max())));
    first = std::min({a, b, c});
    last = std::max({a, b, c});
    median = std::max(std::min(a,b), std::min(std::max(a,b),c));
    query = median;

    THEN("Query is returned.") {
      CHECK(helpers::TestInRange(first, last, median) == query);
    }
  }
}

SCENARIO("Test exceptions thrown by helpers::TestInRange.",
         "[helpers][TestInRange][exceptions]") {

  GIVEN("Various integer ranges and query integers.") {
    int a, b, c, min, max, median;
    a = GENERATE(take(3, random(std::numeric_limits<int>::min() + 1,
                                std::numeric_limits<int>::max() - 1)));
    b = GENERATE(take(3, random(std::numeric_limits<int>::min() + 1,
                                std::numeric_limits<int>::max() - 1)));
    c = GENERATE(take(3, random(std::numeric_limits<int>::min() + 1,
                                std::numeric_limits<int>::max() - 1)));
    min = std::min({a, b, c}) - 1;
    max = std::max({a, b, c}) + 1;
    median = std::max(std::min(a,b), std::min(std::max(a,b),c));

    THEN("Query greater than right end of range causes exception.") {
      CHECK_THROWS_AS(helpers::TestInRange(min, median, max),
                      exceptions::OutOfRange);
    }

    THEN("Query less than left end of range causes exception.") {
      CHECK_THROWS_AS(helpers::TestInRange(median, max, min),
                      exceptions::OutOfRange);
    }
  }
}

SCENARIO("Test correctness of helpers::TestPositive(int).",
         "[helpers][TestPositive(int)][correctness]") {

  WHEN("Query integer is positive.") {
    int x, query;
    x = GENERATE(take(10, random(1, std::numeric_limits<int>::max())));
    query = x;

    THEN("Query is returned.") {
      CHECK(helpers::TestPositive(x) == query);
    }
  }
}

SCENARIO("Test exceptions thrown by helpers::TestPositive(int).",
         "[helpers][TestPositive(int)][exceptions]") {

  THEN("Zero query causes exception.") {
    CHECK_THROWS_AS(helpers::TestPositive(0), exceptions::OutOfRange);
  }

  THEN("Negative query causes exception.") {
    int x;
    x = GENERATE(take(10, random(std::numeric_limits<int>::min(), -1)));

    CHECK_THROWS_AS(helpers::TestPositive(x), exceptions::OutOfRange);
  }
}

SCENARIO("Test correctness of helpers::TestPositive(long).",
         "[helpers][TestPositive(long)][correctness]") {

  WHEN("Query integer is positive.") {
    long x, query;
    x = GENERATE(take(10, random(1l, std::numeric_limits<long>::max())));
    query = x;

    THEN("Query is returned.") {
      CHECK(helpers::TestPositive(x) == query);
    }
  }
}

SCENARIO("Test exceptions thrown by helpers::TestPositive(long).",
         "[helpers][TestPositive(long)][exceptions]") {

  THEN("Zero query causes exception.") {
    CHECK_THROWS_AS(helpers::TestPositive(0l), exceptions::OutOfRange);
  }

  THEN("Negative query causes exception.") {
    long x;
    x = GENERATE(take(10, random(std::numeric_limits<long>::min(), -1l)));

    CHECK_THROWS_AS(helpers::TestPositive(x), exceptions::OutOfRange);
  }
}

SCENARIO("Test correctness of helpers::TestNonNegative(int).",
         "[helpers][TestNonNegative(int)][correctness]") {

  THEN("Zero query is returned.") {
    CHECK(helpers::TestNonNegative(0) == 0);
  }

  WHEN("Query integer is positive.") {
    int x, query;
    x = GENERATE(take(10, random(1, std::numeric_limits<int>::max())));
    query = x;

    THEN("Query is returned.") {
      CHECK(helpers::TestNonNegative(x) == query);
    }
  }
}

SCENARIO("Test exceptions thrown by helpers::TestNonNegative(int).",
         "[helpers][TestNonNegative(int)][exceptions]") {

  THEN("Negative query causes exception.") {
    int x;
    x = GENERATE(take(10, random(std::numeric_limits<int>::min(), -1)));

    CHECK_THROWS_AS(helpers::TestNonNegative(x), exceptions::OutOfRange);
  }
}

SCENARIO("Test correctness of helpers::TestNonNegative(long).",
         "[helpers][TestNonNegative(long)][correctness]") {

  THEN("Zero query is returned.") {
    CHECK(helpers::TestNonNegative(0l) == 0l);
  }

  WHEN("Query integer is positive.") {
    long x, query;
    x = GENERATE(take(10, random(1l, std::numeric_limits<long>::max())));
    query = x;

    THEN("Query is returned.") {
      CHECK(helpers::TestNonNegative(x) == query);
    }
  }
}

SCENARIO("Test exceptions thrown by helpers::TestNonNegative(long).",
         "[helpers][TestNonNegative(long)][exceptions]") {

  THEN("Negative query causes exception.") {
    long x;
    x = GENERATE(take(10, random(std::numeric_limits<long>::min(), -1l)));

    CHECK_THROWS_AS(helpers::TestNonNegative(x), exceptions::OutOfRange);
  }
}

SCENARIO("Test correctness of helpers::TestNonEmpty(string).",
         "[helpers][TestNonEmpty(string)][correctness]") {

  GIVEN("Some character.") {
    char c = GENERATE(take(10, random(std::numeric_limits<char>::min(),
                                      std::numeric_limits<char>::max())));

    THEN("A query string consisting of that character only is returned.") {
      std::string s{c}, query;
      query = s;
      CHECK(helpers::TestNonEmpty(s) == query);
    }

    WHEN("Concatenations of various counts of the character are queried.") {
      int count = GENERATE(take(10, random(1, 64)));
      std::string s(count, c), query;
      query = s;

      THEN("Query strings are returned.") {
        CHECK(helpers::TestNonEmpty(s) == query);
      }
    }
  }
}

SCENARIO("Test exceptions thrown by helpers::TestNonEmpty(string).",
         "[helpers][TestNonEmpty(string)][exceptions]") {

  THEN("An empty string causes an exception.") {
    std::string s;
    CHECK_THROWS_AS(helpers::TestNonEmpty(s),
                    exceptions::UnexpectedEmptyString);
  }
}

SCENARIO("Test correctness of helpers::TestNonEmpty(string_view).",
         "[helpers][TestNonEmpty(string_view)][correctness]") {

  GIVEN("Some character.") {
    char c = GENERATE(take(10, random(std::numeric_limits<char>::min(),
                                      std::numeric_limits<char>::max())));

    THEN("A query string consisting of that character only is returned.") {
      std::string s{c};
      std::string_view s_view{s}, query{s};
      CHECK(helpers::TestNonEmpty(s_view) == query);
    }

    WHEN("Concatenations of various counts of the character are queried.") {
      int count = GENERATE(take(10, random(1, 64)));
      std::string s(count, c);
      std::string_view s_view{s}, query{s};

      THEN("Query strings are returned.") {
        CHECK(helpers::TestNonEmpty(s_view) == query);
      }
    }
  }
}

SCENARIO("Test exceptions thrown by helpers::TestNonEmpty(string_view).",
         "[helpers][TestNonEmpty(string_view)][exceptions]") {

  THEN("An empty string causes an exception.") {
    std::string_view s_view;
    CHECK_THROWS_AS(helpers::TestNonEmpty(s_view),
                    exceptions::UnexpectedEmptyString);
  }
}

SCENARIO("Test correctness of helpers::StringViewToInteger.",
         "[helpers][StringViewToInteger][correctness]") {

  THEN("Function converts string representation of 0 to 0.") {
    std::string_view query_view{"0"};
    CHECK(helpers::StringViewToInteger(query_view) == 0);
  }

  GIVEN("A variety of non-negative integers represented as strings.") {
    int x;
    std::string query;
    std::string_view query_view;
    x = GENERATE(take(10, random(1, std::numeric_limits<int>::max())));
    query = std::to_string(x);
    query_view = query;

    THEN("Function has same effect as std::stoi.") {
      CHECK(helpers::StringViewToInteger(query_view) == std::stoi(query));
    }

    THEN("Function converts back to original integer.") {
      CHECK(helpers::StringViewToInteger(query_view) == x);
    }
  }
}

SCENARIO("Test exceptions thrown by helpers::StringViewToInteger.",
         "[helpers][StringViewToInteger][exceptions]") {

  THEN("String representations of negative integers cause exception.") {
    int x;
    std::string query;
    std::string_view query_view;
    x = GENERATE(take(10, random(std::numeric_limits<int>::min(), -1)));
    query = std::to_string(x);
    query_view = query;

    CHECK_THROWS_AS(helpers::StringViewToInteger(query_view),
                    exceptions::ParsingError);
  }

  THEN("String representations of floats cause exception.") {
    float x;
    std::string query;
    std::string_view query_view;
    x = GENERATE(take(10, random(1.0f, std::numeric_limits<float>::max())));
    query = std::to_string(x);
    query_view = query;

    CHECK_THROWS_AS(helpers::StringViewToInteger(query_view),
                    exceptions::ParsingError);
  }

  THEN("String representations of doubles cause exception.") {
    double x;
    std::string query;
    std::string_view query_view;
    x = GENERATE(take(10, random(1.0, std::numeric_limits<double>::max())));
    query = std::to_string(x);
    query_view = query;

    CHECK_THROWS_AS(helpers::StringViewToInteger(query_view),
                    exceptions::ParsingError);
  }

  THEN("String representations of too large integers range cause exception.") {
    long x;
    std::string query;
    std::string_view query_view;
    x = GENERATE(take(10, random(2l * std::numeric_limits<int>::max(),
                                 std::numeric_limits<long>::max())));
    query = std::to_string(x);
    query_view = query;

    CHECK_THROWS_AS(helpers::StringViewToInteger(query_view),
                    exceptions::ParsingError);
  }

  GIVEN("Strings containing non-digit characters.") {
    int prefix, suffix;
    char c;
    std::string::size_type count;
    std::string infix;
    std::string query, query_noprefix, query_nosuffix, query_nofix;
    std::string prefix_str, suffix_str;

    prefix = GENERATE(take(2, random(1, std::numeric_limits<int>::max())));
    suffix = GENERATE(take(2, random(1, std::numeric_limits<int>::max())));
    prefix_str = std::to_string(prefix);
    suffix_str = std::to_string(suffix);
    c = GENERATE(take(2, random(std::numeric_limits<char>::min(),
                                std::numeric_limits<char>::max())));
    while (std::isdigit(c)) {
      if (c == std::numeric_limits<char>::max()) {
        c = 'X';
      } else {
        ++c;
      }
    }
    count = GENERATE(take(2, random(1, 64)));
    infix.append(count, c);

    query.reserve(prefix_str.length() + suffix_str.length() + infix.length());
    query_noprefix.reserve(suffix_str.length() + infix.length());
    query_nosuffix.reserve(prefix_str.length() + infix.length());

    query = prefix_str;
    query.append(infix.cbegin(), infix.cend());
    query.append(suffix_str.cbegin(), suffix_str.cend());
    query_noprefix = infix;
    query_noprefix.append(suffix_str.cbegin(), suffix_str.cend());
    query_nosuffix = prefix_str;
    query_nosuffix.append(infix.cbegin(), infix.cend());

    THEN("Exception is thrown.") {
      CHECK_THROWS_AS(helpers::StringViewToInteger(query),
                      exceptions::ParsingError);
      CHECK_THROWS_AS(helpers::StringViewToInteger(query_noprefix),
                      exceptions::ParsingError);
      CHECK_THROWS_AS(helpers::StringViewToInteger(query_nosuffix),
                      exceptions::ParsingError);
      CHECK_THROWS_AS(helpers::StringViewToInteger(infix),
                      exceptions::ParsingError);
    }
  }
}

SCENARIO("Test correctness of helpers::FuzzyFloatEquals.",
         "[helpers][FuzzyFloatEquals][correctness]") {

  GIVEN("A variety of floats.") {
    float float_step_size, first, second, distance, min_magnitude;
    float_step_size = std::numeric_limits<float>::epsilon();
    first = GENERATE(take(5, random(std::numeric_limits<float>::min()
                                    + std::numeric_limits<float>::epsilon(),
                                    std::numeric_limits<float>::max()
                                    - std::numeric_limits<float>::epsilon())));
    second = GENERATE(take(5, random(std::numeric_limits<float>::min()
                                    + std::numeric_limits<float>::epsilon(),
                                    std::numeric_limits<float>::max()
                                    - std::numeric_limits<float>::epsilon())));
    if (first == second) {
      second += float_step_size;
    }
    distance = std::abs(first - second);
    if (first == 0.0f) {
      min_magnitude = std::abs(second);
    } else if (second == 0.0f) {
      min_magnitude = std::abs(first);
    } else {
      min_magnitude = std::min(std::abs(first), std::abs(second));
    }

    THEN("If max allowed distance is too small, function returns false.") {
      CHECK_FALSE(helpers::FuzzyFloatEquals(
          first, second, distance / min_magnitude - 100.0f * float_step_size));
    }

    THEN("If max allowed distance is large enough, function returns true.") {
      CHECK(helpers::FuzzyFloatEquals(
          first, second, distance / min_magnitude + 100.0f * float_step_size));
    }
  }
}

SCENARIO("Test correctness of helpers::FuzzyDoubleEquals.",
         "[helpers][FuzzyDoubleEquals][correctness]") {

  GIVEN("A variety of doubles.") {
    double double_step_size, first, second, distance, min_magnitude;
    double_step_size = std::numeric_limits<double>::epsilon();
    first = GENERATE(take(5, random(std::numeric_limits<double>::min()
                                    + std::numeric_limits<double>::epsilon(),
                                    std::numeric_limits<double>::max()
                                    - std::numeric_limits<double>::epsilon())));
    second = GENERATE(take(5, random(std::numeric_limits<double>::min()
                                    + std::numeric_limits<double>::epsilon(),
                                    std::numeric_limits<double>::max()
                                    - std::numeric_limits<double>::epsilon())));
    if (first == second) {
      second += double_step_size;
    }
    distance = std::abs(first - second);
    if (first == 0.0) {
      min_magnitude = std::abs(second);
    } else if (second == 0.0) {
      min_magnitude = std::abs(first);
    } else {
      min_magnitude = std::min(std::abs(first), std::abs(second));
    }

    THEN("If max allowed distance is too small, function returns false.") {
      CHECK_FALSE(helpers::FuzzyDoubleEquals(
          first, second, distance / min_magnitude - 1000.0 * double_step_size));
    }

    THEN("If max allowed distance is large enough, function returns true.") {
      CHECK(helpers::FuzzyDoubleEquals(
          first, second, distance / min_magnitude + 1000.0 * double_step_size));
    }
  }
}

SCENARIO("Test correctness of helpers::MegablastExtendCost.",
         "[helpers][MegablastExtendCost][correctness]") {

  GIVEN("Various positive values for match reward and mismatch penalty.") {
    int reward, penalty;
    reward = GENERATE(take(5, random(1, 100)));
    penalty = GENERATE(take(5, random(1, 100)));
    float expected_return_value{(reward / 2.0f) + penalty};

    THEN("Function calculates as expected.") {
      CHECK(helpers::FuzzyFloatEquals(
          helpers::MegablastExtendCost(reward, penalty),
          expected_return_value));
    }
  }
}

SCENARIO("Test exceptions thrown by helpers::MegablastExtendCost.",
         "[helpers][MegablastExtendCost][exceptions]") {

  THEN("Both reward and penalty equal to 0 causes exception.") {
    CHECK_THROWS_AS(helpers::MegablastExtendCost(0, 0),
                    exceptions::OutOfRange);
  }

  THEN("At least one of reward and penalty not positive causes exception.") {
    int pos_reward, pos_penalty, neg_reward, neg_penalty;
    pos_reward = GENERATE(take(3, random(1, 100)));
    pos_penalty = GENERATE(take(3, random(1, 100)));
    neg_reward = GENERATE(take(3, random(-100, -1)));
    neg_penalty = GENERATE(take(3, random(-100, -1)));

    CHECK_THROWS_AS(helpers::MegablastExtendCost(pos_reward, 0),
                    exceptions::OutOfRange);
    CHECK_THROWS_AS(helpers::MegablastExtendCost(0, pos_penalty),
                    exceptions::OutOfRange);
    CHECK_THROWS_AS(helpers::MegablastExtendCost(pos_reward, neg_penalty),
                    exceptions::OutOfRange);
    CHECK_THROWS_AS(helpers::MegablastExtendCost(neg_reward, pos_penalty),
                    exceptions::OutOfRange);
    CHECK_THROWS_AS(helpers::MegablastExtendCost(neg_reward, neg_penalty),
                    exceptions::OutOfRange);
    CHECK_THROWS_AS(helpers::MegablastExtendCost(neg_reward, 0),
                    exceptions::OutOfRange);
    CHECK_THROWS_AS(helpers::MegablastExtendCost(0, neg_penalty),
                    exceptions::OutOfRange);
  }
}

} // namespace

} // namespace test

} // namespace paste_alignments