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

#include "parameter_map.h"

#define CATCH_CONFIG_MAIN
#define CATCH_CONFIG_COLOUR_NONE
#include "catch.h"

#include "string_conversions.h" // include after catch.h

#include <map>
#include <string>
#include <unordered_set>
#include <vector>

// Test correctness for:
// * GetPrimaryName
// * GetConfiguration(std::string)
// * GetConfiguration(int)
// * ConversionFunction
// * operator()
//
// Test invariants for:
// * operator()
//
// Test exceptions for:
// * IsFlag
// * GetId
// * GetPrimaryName
// * GetConfiguration(std::string)
// * GetConfiguration(int)
// * ConversionFunction
// * operator()

namespace arg_parse_convert {

namespace test {

struct TestType {
  int data;

  inline bool operator==(const TestType& other) const {
    return (data == other.data);
  }
};

inline TestType StringToTestType(const std::string& s) {
  TestType result;
  result.data = std::stoi(s);
  return result;
}

namespace {

SCENARIO("Test correctness of ParameterMap::operator().",
         "[ParameterMap][operator()][correctness]") {
  ParameterMap parameter_map;
  int next_id{static_cast<int>(parameter_map.size())};

  GIVEN("A list of names which are not registered and some test arguments.") {
    std::vector<std::string> test_args{"123", "456", "2020"};
    std::vector<std::string> names{"foo", "f", "foobar"};
    for (const std::string& name : names) {
      CHECK_THROWS_AS(parameter_map.GetId(name),
                      exceptions::ParameterAccessError);
    }

    WHEN("A positional parameter with one of the names is added.") {
      Parameter<std::string> foo{Parameter<std::string>::Positional(
                                 converters::StringIdentity, "foo", 1)};
      parameter_map(foo);

      THEN("The object corresponds the name to the next available id.") {
        CHECK(parameter_map.GetId("foo") == next_id);
      }

      THEN("The object stores the correct configuration for the parameter.") {
        CHECK(parameter_map.GetConfiguration("foo") == foo.configuration());
      }

      THEN("The parameter id is placed in its category with the correct key.") {
        CHECK(parameter_map.positional_parameters().find(
                  foo.configuration().position())->second
              == parameter_map.GetId("foo"));
        CHECK_FALSE(parameter_map.keyword_parameters()
                                 .count(parameter_map.GetId("foo")));
        CHECK_FALSE(parameter_map.flags().count(parameter_map.GetId("foo")));
      }

      THEN("Conversion function is stored correctly.") {
        for (const std::string& arg : test_args) {
          CHECK(parameter_map.ConversionFunction<std::string>("foo")(arg)
                == foo.converter()(arg));
        }
      }
    }

    WHEN("A keyword parameter with the names is added.") {
      Parameter<int> foo{Parameter<int>::Keyword(converters::stoi, names)};
      parameter_map(foo);

      THEN("The object corresponds all names to the same next available id.") {
        CHECK(parameter_map.GetId("foo") == next_id);
        CHECK(parameter_map.GetId("f") == next_id);
        CHECK(parameter_map.GetId("foobar") == next_id);
      }

      THEN("The object stores the correct configuration for the parameter.") {
        CHECK(parameter_map.GetConfiguration("foo") == foo.configuration());
      }

      THEN("The parameter id is placed in its category.") {
        CHECK(parameter_map.keyword_parameters()
                           .count(parameter_map.GetId("foo")));
        CHECK_FALSE(parameter_map.positional_parameters()
                                 .count(parameter_map.GetId("foo")));
        CHECK_FALSE(parameter_map.flags().count(parameter_map.GetId("foo")));
      }

      THEN("Conversion function is stored correctly.") {
        for (const std::string& arg : test_args) {
          CHECK(parameter_map.ConversionFunction<int>("foo")(arg)
                == foo.converter()(arg));
        }
      }
    }

    WHEN("A flag with the names is added.") {
      Parameter<bool> foo{Parameter<bool>::Flag(names)};
      parameter_map(foo);

      THEN("The object corresponds all names to the same next available id.") {
        CHECK(parameter_map.GetId("foo") == next_id);
        CHECK(parameter_map.GetId("f") == next_id);
        CHECK(parameter_map.GetId("foobar") == next_id);
      }

      THEN("The object stores the correct configuration for the parameter.") {
        CHECK(parameter_map.GetConfiguration("foo") == foo.configuration());
      }

      THEN("The parameter id is placed in its category.") {
        CHECK(parameter_map.flags().count(parameter_map.GetId("foo")));
        CHECK_FALSE(parameter_map.keyword_parameters()
                                 .count(parameter_map.GetId("foo")));
        CHECK_FALSE(parameter_map.positional_parameters()
                                 .count(parameter_map.GetId("foo")));
      }

      THEN("Conversion function is stored correctly.") {
        for (const std::string& arg : test_args) {
          CHECK(parameter_map.ConversionFunction<bool>("foo")(arg)
                == foo.converter()(arg));
        }
      }
    }

    WHEN("A some arguments are required and others aren't.") {
      Parameter<TestType> foo{Parameter<TestType>::Positional(StringToTestType,
                                                              names.at(0), 4)};
      foo.MinArgs(2);
      foo.SetDefault({"arg1", "arg2"});
      REQUIRE_FALSE(foo.configuration().IsRequired());

      Parameter<int> f{Parameter<int>::Keyword(converters::stoi,
                                                 {names.at(1)})};
      f.MinArgs(4);
      REQUIRE(f.configuration().IsRequired());

      Parameter<bool> foobar{Parameter<bool>::Flag({names.at(2)})};
      foobar.MinArgs(5);
      REQUIRE_FALSE(foobar.configuration().IsRequired());

      parameter_map(foo)(f)(foobar);

      THEN("Only required parameters are stored as such.") {
        CHECK(parameter_map.required_parameters()
                           .count(parameter_map.GetId("f")));
        CHECK_FALSE(parameter_map.required_parameters()
                                 .count(parameter_map.GetId("foo")));
        CHECK_FALSE(parameter_map.required_parameters()
                                 .count(parameter_map.GetId("foobar")));
      }
    }
  }
}

SCENARIO("Test invariant preservation by ParameterMap::operator().",
         "[ParameterMap][operator()][invariants]") {
  ParameterMap parameter_map;

  GIVEN("A list of names which are not registered.") {
    std::vector<std::string> names{"aaaa", "aaab", "aaba", "aabb", "abaa",
                                   "abab", "abba", "abbb", "baaa", "baab",
                                   "baba", "babb", "bbaa", "bbab", "bbba",
                                   "bbbb"};
    for (const std::string& name : names) {
      CHECK_THROWS_AS(parameter_map.GetId(name),
                      exceptions::ParameterAccessError);
    }

    WHEN("Adding some number of parameters to the object.") {
      int mid_count = GENERATE(take(5, random(0, 15)));

      std::vector<Parameter<std::string>> parameters;
      for (int i = 0; i < mid_count; ++i) {
        parameters.push_back(Parameter<std::string>::Keyword(
                             converters::StringIdentity, {names.at(i)}));
      }
      parameters.push_back(Parameter<std::string>::Keyword(
          converters::StringIdentity,
          std::vector<std::string>{names.begin() + mid_count, names.end()}));

      REQUIRE(parameters.size() == mid_count + 1);

      for (const Parameter<std::string>& p : parameters) {
        parameter_map(p);
      }

      THEN("The total number of parameters added is the size of the object.") {
        CHECK(parameter_map.size() == parameters.size());
      }

      THEN("All names of each parameter correspond to the same integer id.") {
        for (int id = 0; id < mid_count + 1; ++id) {
          for (const std::string& name
               : parameters.at(id).configuration().names()) {
            CHECK(parameter_map.GetId(name) == id);
          }
        }
      }
    }
  }
}

SCENARIO("Test exceptions thrown by ParameterMap::operator().",
         "[ParameterMap][operator()][exceptions]") {

  GIVEN("A `ParameterMap` object containing some parameters.") {
    ParameterMap parameter_map;
    parameter_map(Parameter<std::string>::Positional(
                      converters::StringIdentity, "foo", 1))
                 (Parameter<int>::Positional(
                      converters::stoi, "zig", 10))
                 (Parameter<std::string>::Keyword(
                      converters::StringIdentity, {"b", "bar", "barometer"}))
                 (Parameter<TestType>::Keyword(
                      StringToTestType, {"car", "c", "carometer"}))
                 (Parameter<bool>::Flag({"baz", "z", "bazinga"}))
                 (Parameter<bool>::Flag({"g"}));

    THEN("Reusing stored names causes exception.") {
      CHECK_THROWS_AS(parameter_map(Parameter<TestType>::Positional(
                          StringToTestType, "bazinga", 5)),
                      exceptions::ParameterRegistrationError);
      CHECK_THROWS_AS(parameter_map(Parameter<float>::Keyword(
                          converters::stof, {"foo"})),
                      exceptions::ParameterRegistrationError);
      CHECK_THROWS_AS(parameter_map(Parameter<bool>::Flag({"baz"})),
                      exceptions::ParameterRegistrationError);
    }

    THEN("Reusing positions for positional parameters causes exceptions") {
      CHECK_THROWS_AS(parameter_map(Parameter<double>::Positional(
                          converters::stod, "zip", 1)),
                      exceptions::ParameterRegistrationError);
      CHECK_THROWS_AS(parameter_map(Parameter<int>::Positional(
                          converters::stoi, "zap", 10)),
                      exceptions::ParameterRegistrationError);
    }
  }
}

SCENARIO("Test correctness of ParameterMap::GetPrimaryName.",
         "[ParameterMap][GetPrimaryName][correctness]") {

  GIVEN("A `ParameterMap` object.") {
    ParameterMap parameter_map;

    WHEN("Positional, keyword and flag parameters are stored.") {

      int foo, zig, b, bar, barometer, c, car, carometer, baz, z, bazinga, g;
      parameter_map(Parameter<std::string>::Positional(
                        converters::StringIdentity, "foo", 1))
                   (Parameter<int>::Positional(
                        converters::stoi, "zig", 10))
                   (Parameter<std::string>::Keyword(
                        converters::StringIdentity, {"b", "bar", "barometer"}))
                   (Parameter<TestType>::Keyword(
                        StringToTestType, {"car", "c", "carometer"}))
                   (Parameter<bool>::Flag({"baz", "z", "bazinga"}))
                   (Parameter<bool>::Flag({"g"}));
      foo = parameter_map.GetId("foo");
      zig = parameter_map.GetId("zig");
      b = parameter_map.GetId("b");
      bar = parameter_map.GetId("bar");
      barometer = parameter_map.GetId("barometer");
      car = parameter_map.GetId("car");
      c = parameter_map.GetId("c");
      carometer = parameter_map.GetId("carometer");
      baz = parameter_map.GetId("baz");
      z = parameter_map.GetId("z");
      bazinga = parameter_map.GetId("bazinga");
      g = parameter_map.GetId("g");

      THEN("The first stored names are the primary names.")
        CHECK(parameter_map.GetPrimaryName(foo) == "foo");
        CHECK(parameter_map.GetPrimaryName(zig) == "zig");
        CHECK(parameter_map.GetPrimaryName(b) == "b");
        CHECK(parameter_map.GetPrimaryName(bar) == "b");
        CHECK(parameter_map.GetPrimaryName(barometer) == "b");
        CHECK(parameter_map.GetPrimaryName(car) == "car");
        CHECK(parameter_map.GetPrimaryName(c) == "car");
        CHECK(parameter_map.GetPrimaryName(carometer) == "car");
        CHECK(parameter_map.GetPrimaryName(baz) == "baz");
        CHECK(parameter_map.GetPrimaryName(z) == "baz");
        CHECK(parameter_map.GetPrimaryName(bazinga) == "baz");
        CHECK(parameter_map.GetPrimaryName(g) == "g");
    }
  }
}

SCENARIO("Test exceptions thrown by ParameterMap::GetPrimaryName."
         "[ParameterMap][GetPrimaryName][exceptions]") {

  GIVEN("A `ParameterMap` object.") {
    ParameterMap parameter_map;

    WHEN("No parameters are registered.") {

      THEN("Any id will cause exception.") {
        CHECK_THROWS_AS(parameter_map.GetPrimaryName(-1),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(parameter_map.GetPrimaryName(-100),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(parameter_map.GetPrimaryName(0),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(parameter_map.GetPrimaryName(1),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(parameter_map.GetPrimaryName(100),
                        exceptions::ParameterAccessError);
      }
    }

    WHEN("Positional, keyword and flag parameters are stored.") {

      parameter_map(Parameter<std::string>::Positional(
                        converters::StringIdentity, "foo", 1))
                   (Parameter<int>::Positional(
                        converters::stoi, "zig", 10))
                   (Parameter<std::string>::Keyword(
                        converters::StringIdentity, {"b", "bar", "barometer"}))
                   (Parameter<TestType>::Keyword(
                        StringToTestType, {"car", "c", "carometer"}))
                   (Parameter<bool>::Flag({"baz", "z", "bazinga"}))
                   (Parameter<bool>::Flag({"g"}));

      THEN("Id's outside the range [0, size) will cause exception.") {
        CHECK_THROWS_AS(parameter_map.GetPrimaryName(-1),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(parameter_map.GetPrimaryName(-100),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(parameter_map.GetPrimaryName(6),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(parameter_map.GetPrimaryName(7),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(parameter_map.GetPrimaryName(100),
                        exceptions::ParameterAccessError);
      }

      THEN("Id's inside the range [0, size) will not cause exception.") {
        for (int i = 0; i < parameter_map.size(); ++i) {
          CHECK_NOTHROW(parameter_map.GetPrimaryName(i));
        }
      }
    }
  }
}

SCENARIO("Test correctness of ParameterMap::GetConfiguration(std::string)."
         "[ParameterMap][GetConfiguration(std::string)][correctness]") {

  GIVEN("A `ParameterMap` object.") {
    ParameterMap parameter_map;

    WHEN("Positional, keyword and flag parameters are stored.") {
      Parameter<std::string> foo{Parameter<std::string>::Positional(
                                 converters::StringIdentity, "foo", 1)};
      Parameter<int> zig{Parameter<int>::Positional(
                         converters::stoi, "zig", 10)};
      Parameter<std::string> bar{Parameter<std::string>::Keyword(
                                 converters::StringIdentity,
                                 {"b", "bar", "barometer"})};
      Parameter<TestType> car{Parameter<TestType>::Keyword(
                              StringToTestType, {"car", "c", "carometer"})};
      Parameter<bool> baz{Parameter<bool>::Flag({"baz", "z", "bazinga"})};
      Parameter<bool> g{Parameter<bool>::Flag({"g"})};
      parameter_map(foo)(zig)(bar)(car)(baz)(g);

      THEN("Parameter configurations are retrieved correctly.") {
        CHECK(parameter_map.GetConfiguration("foo") == foo.configuration());
        CHECK(parameter_map.GetConfiguration("zig") == zig.configuration());
        CHECK(parameter_map.GetConfiguration("bar") == bar.configuration());
        CHECK(parameter_map.GetConfiguration("car") == car.configuration());
        CHECK(parameter_map.GetConfiguration("baz") == baz.configuration());
        CHECK(parameter_map.GetConfiguration("g") == g.configuration());
      }
    }
  }
}

SCENARIO("Test exceptions thrown by"
         " ParameterMap::GetConfiguration(std::string).",
         "[ParameterMap][GetConfiguration(std::string)][exceptions]") {

  GIVEN("A `ParameterMap` object.") {
    ParameterMap parameter_map;

    WHEN("No parameters are registered.") {

      THEN("Any name will cause exception.") {
        CHECK_THROWS_AS(parameter_map.GetConfiguration("foo"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(parameter_map.GetConfiguration("bar"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(parameter_map.GetConfiguration("baz"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(parameter_map.GetConfiguration("barometer"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(parameter_map.GetConfiguration("ticklefight"),
                        exceptions::ParameterAccessError);
      }
    }

    WHEN("Positional, keyword and flag parameters are stored.") {

      parameter_map(Parameter<std::string>::Positional(
                        converters::StringIdentity, "foo", 1))
                   (Parameter<int>::Positional(
                        converters::stoi, "zig", 10))
                   (Parameter<std::string>::Keyword(
                        converters::StringIdentity, {"b", "bar", "barometer"}))
                   (Parameter<TestType>::Keyword(
                        StringToTestType, {"car", "c", "carometer"}))
                   (Parameter<bool>::Flag({"baz", "z", "bazinga"}))
                   (Parameter<bool>::Flag({"g"}));

      THEN("Names not referring to stored parameters will cause exception.") {
        CHECK_THROWS_AS(parameter_map.GetConfiguration("pricklypear"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(parameter_map.GetConfiguration("ticklefight"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(parameter_map.GetConfiguration("tempeh"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(parameter_map.GetConfiguration("istanbul"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(parameter_map.GetConfiguration("armstrong"),
                        exceptions::ParameterAccessError);
      }

      THEN("Names referring to stored parameters will not cause exception.") {
        for (const std::string& name : {"foo", "zig", "b", "bar", "barometer",
                                        "car", "c", "carometer", "baz", "z",
                                        "bazinga", "g"}) {
          CHECK_NOTHROW(parameter_map.GetConfiguration(name));
        }
      }
    }
  }
}

SCENARIO("Test correctness of ParameterMap::GetConfiguration(int)."
         "[ParameterMap][GetConfiguration(int)][correctness]") {

  GIVEN("A `ParameterMap` object.") {
    ParameterMap parameter_map;

    WHEN("Positional, keyword and flag parameters are stored.") {
      Parameter<std::string> foo{Parameter<std::string>::Positional(
                                 converters::StringIdentity, "foo", 1)};
      Parameter<int> zig{Parameter<int>::Positional(
                         converters::stoi, "zig", 10)};
      Parameter<std::string> bar{Parameter<std::string>::Keyword(
                                 converters::StringIdentity,
                                 {"b", "bar", "barometer"})};
      Parameter<TestType> car{Parameter<TestType>::Keyword(
                              StringToTestType, {"car", "c", "carometer"})};
      Parameter<bool> baz{Parameter<bool>::Flag({"baz", "z", "bazinga"})};
      Parameter<bool> g{Parameter<bool>::Flag({"g"})};
      parameter_map(foo)(zig)(bar)(car)(baz)(g);

      THEN("Parameter configurations are retrieved correctly.") {
        CHECK(parameter_map.GetConfiguration(0) == foo.configuration());
        CHECK(parameter_map.GetConfiguration(1) == zig.configuration());
        CHECK(parameter_map.GetConfiguration(2) == bar.configuration());
        CHECK(parameter_map.GetConfiguration(3) == car.configuration());
        CHECK(parameter_map.GetConfiguration(4) == baz.configuration());
        CHECK(parameter_map.GetConfiguration(5) == g.configuration());
      }
    }
  }
}

SCENARIO("Test exceptions thrown by ParameterMap::GetConfiguration(int).",
         "[ParameterMap][GetConfiguration(int)][exceptions]") {

  GIVEN("A `ParameterMap` object.") {
    ParameterMap parameter_map;

    WHEN("No parameters are registered.") {

      THEN("Any id will cause exception.") {
        CHECK_THROWS_AS(parameter_map.GetConfiguration(0),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(parameter_map.GetConfiguration(1),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(parameter_map.GetConfiguration(100),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(parameter_map.GetConfiguration(-1),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(parameter_map.GetConfiguration(-100),
                        exceptions::ParameterAccessError);
      }
    }

    WHEN("Positional, keyword and flag parameters are stored.") {

      parameter_map(Parameter<std::string>::Positional(
                        converters::StringIdentity, "foo", 1))
                   (Parameter<int>::Positional(
                        converters::stoi, "zig", 10))
                   (Parameter<std::string>::Keyword(
                        converters::StringIdentity, {"b", "bar", "barometer"}))
                   (Parameter<TestType>::Keyword(
                        StringToTestType, {"car", "c", "carometer"}))
                   (Parameter<bool>::Flag({"baz", "z", "bazinga"}))
                   (Parameter<bool>::Flag({"g"}));

      THEN("Id's not referring to stored parameters will cause exception.") {
        CHECK_THROWS_AS(parameter_map.GetConfiguration(6),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(parameter_map.GetConfiguration(7),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(parameter_map.GetConfiguration(100),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(parameter_map.GetConfiguration(-1),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(parameter_map.GetConfiguration(-100),
                        exceptions::ParameterAccessError);
      }

      THEN("Id's referring to stored parameters will not cause exception.") {
        int id = GENERATE(range(0, 6));
        CHECK_NOTHROW(parameter_map.GetConfiguration(id));
      }
    }
  }
}

SCENARIO("Test correctness of ParameterMap::ConversionFunction.",
         "[ParameterMap][ConversionFunction][correctness]") {

  GIVEN("A `ParameterMap` object.") {
    ParameterMap parameter_map;

    WHEN("Positional, keyword and flag parameters are stored.") {
      parameter_map(Parameter<std::string>::Positional(
                        converters::StringIdentity, "foo", 1))
                   (Parameter<float>::Positional(
                        converters::stof, "zig", 10))
                   (Parameter<std::string>::Keyword(
                        [](const std::string& s){
                            return std::string{s}.append("_converted");},
                        {"b", "bar", "barometer"}))
                   (Parameter<TestType>::Keyword(
                        StringToTestType, {"car", "c", "carometer"}))
                   (Parameter<bool>::Flag({"baz", "z", "bazinga"}))
                   (Parameter<bool>::Flag({"g"}));

      THEN("Conversion functions are associated with the correct names.") {
        CHECK(parameter_map.ConversionFunction<std::string>("foo")("arg")
              == "arg");
        CHECK(parameter_map.ConversionFunction<float>("zig")("123.456")
              == Approx(123.456f));
        CHECK(parameter_map.ConversionFunction<std::string>("bar")("arg")
              == "arg_converted");
        CHECK(parameter_map.ConversionFunction<TestType>("car")("123").data
              == 123);
        CHECK(parameter_map.ConversionFunction<bool>("baz")(""));
        CHECK(parameter_map.ConversionFunction<bool>("baz")("placeholder"));
        CHECK(parameter_map.ConversionFunction<bool>("g")(""));
        CHECK(parameter_map.ConversionFunction<bool>("g")("placeholder"));
      }
    }
  }  
}

SCENARIO("Test exceptions thrown by ParameterMap::ConversionFunction.",
         "[ParameterMap][ConversionFunction][exceptions]") {

  GIVEN("A `ParameterMap` object.") {
    ParameterMap parameter_map;

    WHEN("No parameters are registered.") {

      THEN("Any name will cause exception.") {
        CHECK_THROWS_AS(parameter_map.ConversionFunction<std::string>("foo"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(parameter_map.ConversionFunction<bool>("bar"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(parameter_map.ConversionFunction<int>("baz"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(parameter_map.ConversionFunction<float>("barometer"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(parameter_map.ConversionFunction<TestType>("tickle"),
                        exceptions::ParameterAccessError);
      }
    }

    WHEN("Positional, keyword and flag parameters are stored.") {

      parameter_map(Parameter<std::string>::Positional(
                        converters::StringIdentity, "foo", 1))
                   (Parameter<int>::Positional(
                        converters::stoi, "zig", 10))
                   (Parameter<std::string>::Keyword(
                        converters::StringIdentity, {"b", "bar", "barometer"}))
                   (Parameter<TestType>::Keyword(
                        StringToTestType, {"car", "c", "carometer"}))
                   (Parameter<bool>::Flag({"baz", "z", "bazinga"}))
                   (Parameter<bool>::Flag({"g"}));

      THEN("Names not referring to stored parameters will cause exception.") {
        CHECK_THROWS_AS(parameter_map.ConversionFunction<int>("pricklypear"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(parameter_map.ConversionFunction<TestType>("tickle"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(parameter_map.ConversionFunction<float>("tempeh"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(parameter_map.ConversionFunction<long>("istanbul"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(parameter_map.ConversionFunction<short>("armstrong"),
                        exceptions::ParameterAccessError);
      }

      THEN("Using stored name but incorrect type will cause exception.") {
        CHECK_THROWS_AS(parameter_map.ConversionFunction<int>("foo"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(parameter_map.ConversionFunction<TestType>("zig"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(parameter_map.ConversionFunction<float>("b"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(parameter_map.ConversionFunction<float>("bar"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(parameter_map.ConversionFunction<float>("barometer"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(parameter_map.ConversionFunction<long>("car"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(parameter_map.ConversionFunction<long>("c"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(parameter_map.ConversionFunction<long>("carometer"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(parameter_map.ConversionFunction<short>("baz"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(parameter_map.ConversionFunction<short>("z"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(parameter_map.ConversionFunction<short>("bazinga"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(parameter_map.ConversionFunction<short>("g"),
                        exceptions::ParameterAccessError);
      }

      THEN("Using stored name with correct type will not cause exception.") {
        CHECK_NOTHROW(parameter_map.ConversionFunction<std::string>("foo"));
        CHECK_NOTHROW(parameter_map.ConversionFunction<int>("zig"));
        CHECK_NOTHROW(parameter_map.ConversionFunction<std::string>("b"));
        CHECK_NOTHROW(parameter_map.ConversionFunction<std::string>("bar"));
        CHECK_NOTHROW(parameter_map.ConversionFunction<std::string>("barometer"));
        CHECK_NOTHROW(parameter_map.ConversionFunction<TestType>("car"));
        CHECK_NOTHROW(parameter_map.ConversionFunction<TestType>("c"));
        CHECK_NOTHROW(parameter_map.ConversionFunction<TestType>("carometer"));
        CHECK_NOTHROW(parameter_map.ConversionFunction<bool>("baz"));
        CHECK_NOTHROW(parameter_map.ConversionFunction<bool>("z"));
        CHECK_NOTHROW(parameter_map.ConversionFunction<bool>("bazinga"));
        CHECK_NOTHROW(parameter_map.ConversionFunction<bool>("g"));
      }
    }
  }
}

SCENARIO("Test exceptions thrown by ParameterMap::GetId.",
         "[ParameterMap][GetId][exceptions]") {

  GIVEN("A `ParameterMap` object.") {
    ParameterMap parameter_map;

    WHEN("No parameters are registered.") {

      THEN("Any name will cause exception.") {
        CHECK_THROWS_AS(parameter_map.GetId("foo"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(parameter_map.GetId("bar"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(parameter_map.GetId("baz"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(parameter_map.GetId("barometer"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(parameter_map.GetId("ticklefight"),
                        exceptions::ParameterAccessError);
      }
    }

    WHEN("Positional, keyword and flag parameters are stored.") {

      parameter_map(Parameter<std::string>::Positional(
                        converters::StringIdentity, "foo", 1))
                   (Parameter<int>::Positional(
                        converters::stoi, "zig", 10))
                   (Parameter<std::string>::Keyword(
                        converters::StringIdentity, {"b", "bar", "barometer"}))
                   (Parameter<TestType>::Keyword(
                        StringToTestType, {"car", "c", "carometer"}))
                   (Parameter<bool>::Flag({"baz", "z", "bazinga"}))
                   (Parameter<bool>::Flag({"g"}));

      THEN("Names not referring to stored parameters will cause exception.") {
        CHECK_THROWS_AS(parameter_map.GetId("pricklypear"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(parameter_map.GetId("ticklefight"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(parameter_map.GetId("tempeh"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(parameter_map.GetId("istanbul"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(parameter_map.GetId("armstrong"),
                        exceptions::ParameterAccessError);
      }

      THEN("Names referring to stored parameters will not cause exception.") {
        for (const std::string& name : {"foo", "zig", "b", "bar", "barometer",
                                        "car", "c", "carometer", "baz", "z",
                                        "bazinga", "g"}) {
          CHECK_NOTHROW(parameter_map.GetId(name));
        }
      }
    }
  }
}

} // namespace

} // namespace test

} // namespace arg_parse_convert