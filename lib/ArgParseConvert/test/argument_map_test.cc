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

#include "argument_map.h"

#define CATCH_CONFIG_MAIN
#define CATCH_CONFIG_COLOUR_NONE
#include "catch.h"

#include "string_conversions.h" // include after catch.h

// Test correctness for:
// * ArgumentMap(ParameterMap)
// * SetDefaultArguments
// * AddArgument
// * GetUnfilledParameters
// * HasArgument
// * ArgumentsOf
// * GetValue
// * GetAllValues
// * IsSet
//
// Test invariants for:
// * ArgumentMap(ParameterMap)
// 
// Test exceptions for:
// * AddArgument
// * HasArgument
// * ArgumentsOf
// * GetValue
// * GetAllValues
// * IsSet

namespace arg_parse_convert {

namespace test {

struct TestType {
  int data;

  TestType() = default;

  TestType(int d) : data{d} {}

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

// NoMin, YesMin, NoMax, YesMax indicate whether minimum and maximum arguments
// are set.
// * NoArgs: no arguments
// * UnderfullArgs: some arguments, but less than its minimum
// * MinArgs: exactly as many arguments as its minimum
// * ManyArgs: more arguments than its minimum but less than its maximum
// * FullArgs: as many arguments as its maximum
//
Parameter<std::string> kNoMinNoMaxNoArgs{Parameter<std::string>::Positional(
    converters::StringIdentity, "kNoMinNoMaxNoArgs", 0)};
Parameter<std::string> kNoMinYesMaxNoArgs{Parameter<std::string>::Keyword(
    converters::StringIdentity, {"kNoMinYesMaxNoArgs",
                                 "alt_kNoMinYesMaxNoArgs"})
    .MaxArgs(8)};
Parameter<int> kNoMinYesMaxManyArgs{Parameter<int>::Positional(
    converters::stoi, "kNoMinYesMaxManyArgs", 1)
    .MaxArgs(8)};
Parameter<int> kNoMinYesMaxFullArgs{Parameter<int>::Keyword(
    converters::stoi, {"kNoMinYesMaxFullArgs", "alt_kNoMinYesMaxFullArgs"})
    .MaxArgs(8)};
Parameter<TestType> kYesMinNoMaxNoArgs{Parameter<TestType>::Positional(
    StringToTestType, "kYesMinNoMaxNoArgs", 2)
    .MinArgs(4)};
Parameter<TestType> kYesMinNoMaxUnderfullArgs{Parameter<TestType>::Keyword(
    StringToTestType, {"kYesMinNoMaxUnderfullArgs",
                       "alt_kYesMinNoMaxUnderfullArgs"})
    .MinArgs(4)};
Parameter<long> kYesMinNoMaxMinArgs{Parameter<long>::Positional(
    converters::stol, "kYesMinNoMaxMinArgs", 3)
    .MinArgs(4)};
Parameter<long> kYesMinYesMaxNoArgs{Parameter<long>::Keyword(
    converters::stol, {"kYesMinYesMaxNoArgs", "alt_kYesMinYesMaxNoArgs"})
    .MaxArgs(8).MinArgs(4)};
Parameter<float> kYesMinYesMaxUnderfullArgs{Parameter<float>::Positional(
    converters::stof, "kYesMinYesMaxUnderfullArgs", 4)
    .MaxArgs(8).MinArgs(4)};
Parameter<float> kYesMinYesMaxMinArgs{Parameter<float>::Keyword(
    converters::stof, {"kYesMinYesMaxMinArgs", "alt_kYesMinYesMaxMinArgs"})
    .MaxArgs(8).MinArgs(4)};
Parameter<double> kYesMinYesMaxManyArgs{Parameter<double>::Positional(
    converters::stod, "kYesMinYesMaxManyArgs", 5)
    .MaxArgs(8).MinArgs(4)};
Parameter<double> kYesMinYesMaxFullArgs{Parameter<double>::Keyword(
    converters::stod, {"kYesMinYesMaxFullArgs", "alt_kYesMinYesMaxFullArgs"})
    .MaxArgs(8).MinArgs(4)};

std::vector<std::string> kArgs{"1.0", "2.0", "2.0", "4.0", "5.0", "6.0", "7.0",
                               "8.0", "9.0", "10.0"};
std::vector<std::string> kDefaultArgs{"10.0", "20.0", "20.0", "40.0", "50.0",
                                      "60.0", "70.0", "80.0", "90.0", "100.0"};

Parameter<bool> kSetFlag{Parameter<bool>::Flag({"kSetFlag", "alt_kSetFlag"})};
Parameter<bool> kUnsetFlag{Parameter<bool>::Flag({"kUnsetFlag",
                                                  "alt_kUnsetFlag"})};
Parameter<bool> kMultipleSetFlag{Parameter<bool>::Flag(
    {"kMultipleSetFlag", "alt_kMultipleSetFlag"})};

Parameter<TestType> kNoConverterPositional{Parameter<TestType>::Positional(
    nullptr, "kNoConverterPositional", 6)};
Parameter<TestType> kNoConverterKeyword{Parameter<TestType>::Keyword(
    nullptr, {"kNoConverterKeyword", "alt_kNoConverterKeyword"})};

std::vector<std::string> kNames{
    "kNoMinNoMaxNoArgs", "kNoMinYesMaxNoArgs", "alt_kNoMinYesMaxNoArgs",
    "kNoMinYesMaxManyArgs", "kNoMinYesMaxFullArgs", "alt_kNoMinYesMaxFullArgs",
    "kYesMinNoMaxNoArgs", "kYesMinNoMaxUnderfullArgs",
    "alt_kYesMinNoMaxUnderfullArgs", "kYesMinNoMaxMinArgs",
    "kYesMinYesMaxNoArgs", "alt_kYesMinYesMaxNoArgs",
    "kYesMinYesMaxUnderfullArgs", "kYesMinYesMaxMinArgs",
    "alt_kYesMinYesMaxMinArgs", "kYesMinYesMaxManyArgs",
    "kYesMinYesMaxFullArgs", "alt_kYesMinYesMaxFullArgs",
    "kSetFlag", "alt_kSetFlag", "kUnsetFlag", "alt_kUnsetFlag",
    "kMultipleSetFlag", "alt_kMultipleSetFlag", "kNoConverterPositional",
    "kNoConverterKeyword", "alt_kNoConverterKeyword"};

SCENARIO("Test correctness of ArgumentMap::ArgumentMap(ParameterMap).",
         "[ArgumentMap][ArgumentMap(ParameterMap)][correctness]") {

  GIVEN("A `ParameterMap` object containing various parameters.") {
    ParameterMap parameter_map, reference;
    parameter_map(kNoMinNoMaxNoArgs)(kNoMinYesMaxNoArgs)
                 (kNoMinYesMaxManyArgs)(kNoMinYesMaxFullArgs)
                 (kYesMinNoMaxNoArgs)(kYesMinNoMaxUnderfullArgs)
                 (kYesMinNoMaxMinArgs)
                 (kYesMinYesMaxNoArgs)(kYesMinYesMaxUnderfullArgs)
                 (kYesMinYesMaxMinArgs)(kYesMinYesMaxManyArgs)
                 (kYesMinYesMaxFullArgs)
                 (kSetFlag)(kUnsetFlag)(kMultipleSetFlag)
                 (kNoConverterPositional)(kNoConverterKeyword);
    reference = parameter_map;

    WHEN("Constructed from the parameters.") {
      ArgumentMap argument_map(std::move(parameter_map));

      THEN("The `ParameterMap` becomes a member of the `ArgumentMap`.") {
        for (const std::string& name : kNames) {
          CHECK(argument_map.Parameters().GetConfiguration(name)
                == reference.GetConfiguration(name));
          CHECK(argument_map.Parameters().required_parameters()
                == reference.required_parameters());
          CHECK(argument_map.Parameters().positional_parameters()
                == reference.positional_parameters());
          CHECK(argument_map.Parameters().keyword_parameters()
                == reference.keyword_parameters());
          CHECK(argument_map.Parameters().flags() == reference.flags());
        }
      }

      THEN("All argument lists begin empty.") {
        for (const std::string& name : kNames) {
          CHECK(argument_map.ArgumentsOf(name).empty());
        }
      }
    }
  }
}

SCENARIO("Test invariant preservation by"
         " ArgumentMap::ArgumentMap(ParameterMap).",
         "[ArgumentMap][ArgumentMap(ParameterMap)][invariants]") {

  GIVEN("A `ParameterMap` object containing various parameters.") {
    int size = GENERATE(range(0, 21, 4));
    ParameterMap parameter_map, reference;
    for (int i = 0; i < size; i += 4) {
      parameter_map(Parameter<bool>::Flag({kNames.at(i)}))
                   (Parameter<int>::Keyword(
                        converters::stoi, {kNames.at(i+1),
                                           kNames.at(i+2)}))
                   (Parameter<std::string>::Positional(
                        converters::StringIdentity, kNames.at(i+3), i));
    }
    reference = parameter_map;

    WHEN("Constructed from the parameters.") {
      ArgumentMap argument_map(std::move(parameter_map));

      THEN("The object's size is the same as the number of parameters.") {
        CHECK(argument_map.size() == reference.size());
        CHECK(argument_map.Arguments().size() == reference.size());
        CHECK(argument_map.Values().size() == reference.size());
      }
    }
  }
}

SCENARIO("Test correctness of ArgumentMap::SetDefaultArguments.",
         "[ArgumentMap][SetDefaultArguments][correctness]") {

  GIVEN("Parameters without default arguments.") {
    ParameterMap parameter_map;
    parameter_map(kNoMinNoMaxNoArgs)(kNoMinYesMaxNoArgs)
                 (kNoMinYesMaxManyArgs)(kNoMinYesMaxFullArgs)
                 (kYesMinNoMaxNoArgs)(kYesMinNoMaxUnderfullArgs)
                 (kYesMinNoMaxMinArgs)
                 (kYesMinYesMaxNoArgs)(kYesMinYesMaxUnderfullArgs)
                 (kYesMinYesMaxMinArgs)(kYesMinYesMaxManyArgs)
                 (kYesMinYesMaxFullArgs)
                 (kSetFlag)(kUnsetFlag)(kMultipleSetFlag)
                 (kNoConverterPositional)(kNoConverterKeyword);

    WHEN("Argument lists are empty.") {
      ArgumentMap argument_map(std::move(parameter_map));
      ArgumentMap reference{argument_map};
      argument_map.SetDefaultArguments();

      THEN("Argument lists remain unchanged.") {
        CHECK(argument_map.ArgumentsOf("kNoMinNoMaxNoArgs")
              == reference.ArgumentsOf("kNoMinNoMaxNoArgs"));
        CHECK(argument_map.ArgumentsOf("kNoMinYesMaxNoArgs")
              == reference.ArgumentsOf("kNoMinYesMaxNoArgs"));
        CHECK(argument_map.ArgumentsOf("kNoMinYesMaxManyArgs")
              == reference.ArgumentsOf("kNoMinYesMaxManyArgs"));
        CHECK(argument_map.ArgumentsOf("kNoMinYesMaxFullArgs")
              == reference.ArgumentsOf("kNoMinYesMaxFullArgs"));
        CHECK(argument_map.ArgumentsOf("kYesMinNoMaxNoArgs")
              == reference.ArgumentsOf("kYesMinNoMaxNoArgs"));
        CHECK(argument_map.ArgumentsOf("kYesMinNoMaxUnderfullArgs")
              == reference.ArgumentsOf("kYesMinNoMaxUnderfullArgs"));
        CHECK(argument_map.ArgumentsOf("kYesMinNoMaxMinArgs")
              == reference.ArgumentsOf("kYesMinNoMaxMinArgs"));
        CHECK(argument_map.ArgumentsOf("kYesMinYesMaxNoArgs")
              == reference.ArgumentsOf("kYesMinYesMaxNoArgs"));
        CHECK(argument_map.ArgumentsOf("kYesMinYesMaxUnderfullArgs")
              == reference.ArgumentsOf("kYesMinYesMaxUnderfullArgs"));
        CHECK(argument_map.ArgumentsOf("kYesMinYesMaxMinArgs")
              == reference.ArgumentsOf("kYesMinYesMaxMinArgs"));
        CHECK(argument_map.ArgumentsOf("kYesMinYesMaxManyArgs")
              == reference.ArgumentsOf("kYesMinYesMaxManyArgs"));
        CHECK(argument_map.ArgumentsOf("kYesMinYesMaxFullArgs")
              == reference.ArgumentsOf("kYesMinYesMaxFullArgs"));
        CHECK(argument_map.ArgumentsOf("kSetFlag")
              == reference.ArgumentsOf("kSetFlag"));
        CHECK(argument_map.ArgumentsOf("kUnsetFlag")
              == reference.ArgumentsOf("kUnsetFlag"));
        CHECK(argument_map.ArgumentsOf("kMultipleSetFlag")
              == reference.ArgumentsOf("kMultipleSetFlag"));
        CHECK(argument_map.ArgumentsOf("kNoConverterPositional")
              == reference.ArgumentsOf("kNoConverterPositional"));
        CHECK(argument_map.ArgumentsOf("kNoConverterKeyword")
              == reference.ArgumentsOf("kNoConverterKeyword"));
      }
    }

    WHEN("Argument lists already contain some arguments.") {
      ArgumentMap argument_map(std::move(parameter_map));
      for (int i = 0; i < 2; ++i) {
        argument_map.AddArgument("kYesMinNoMaxUnderfullArgs", kArgs.at(i));
        argument_map.AddArgument("kYesMinYesMaxUnderfullArgs", kArgs.at(i));
      }
      for (int i = 0; i < 4; ++i) {
        argument_map.AddArgument("kYesMinNoMaxMinArgs", kArgs.at(i));
        argument_map.AddArgument("kYesMinYesMaxMinArgs", kArgs.at(i));
      }
      for (int i = 0; i < 6; ++i) {
        argument_map.AddArgument("kNoMinYesMaxManyArgs", kArgs.at(i));
        argument_map.AddArgument("kYesMinYesMaxManyArgs", kArgs.at(i));
      }
      for (int i = 0; i < 8; ++i) {
        argument_map.AddArgument("kNoMinYesMaxFullArgs", kArgs.at(i));
        argument_map.AddArgument("kYesMinYesMaxFullArgs", kArgs.at(i));
      }
      argument_map.AddArgument("kSetFlag", kArgs.at(0));
      for (const std::string& arg : kArgs) {
        argument_map.AddArgument("kMultipleSetFlag", arg);
        argument_map.AddArgument("kNoConverterPositional", arg);
        argument_map.AddArgument("kNoConverterKeyword", arg);
      }
      ArgumentMap reference{argument_map};
      argument_map.SetDefaultArguments();

      THEN("Argument lists remain unchanged.") {
        CHECK(argument_map.ArgumentsOf("kNoMinNoMaxNoArgs")
              == reference.ArgumentsOf("kNoMinNoMaxNoArgs"));
        CHECK(argument_map.ArgumentsOf("kNoMinYesMaxNoArgs")
              == reference.ArgumentsOf("kNoMinYesMaxNoArgs"));
        CHECK(argument_map.ArgumentsOf("kNoMinYesMaxManyArgs")
              == reference.ArgumentsOf("kNoMinYesMaxManyArgs"));
        CHECK(argument_map.ArgumentsOf("kNoMinYesMaxFullArgs")
              == reference.ArgumentsOf("kNoMinYesMaxFullArgs"));
        CHECK(argument_map.ArgumentsOf("kYesMinNoMaxNoArgs")
              == reference.ArgumentsOf("kYesMinNoMaxNoArgs"));
        CHECK(argument_map.ArgumentsOf("kYesMinNoMaxUnderfullArgs")
              == reference.ArgumentsOf("kYesMinNoMaxUnderfullArgs"));
        CHECK(argument_map.ArgumentsOf("kYesMinNoMaxMinArgs")
              == reference.ArgumentsOf("kYesMinNoMaxMinArgs"));
        CHECK(argument_map.ArgumentsOf("kYesMinYesMaxNoArgs")
              == reference.ArgumentsOf("kYesMinYesMaxNoArgs"));
        CHECK(argument_map.ArgumentsOf("kYesMinYesMaxUnderfullArgs")
              == reference.ArgumentsOf("kYesMinYesMaxUnderfullArgs"));
        CHECK(argument_map.ArgumentsOf("kYesMinYesMaxMinArgs")
              == reference.ArgumentsOf("kYesMinYesMaxMinArgs"));
        CHECK(argument_map.ArgumentsOf("kYesMinYesMaxManyArgs")
              == reference.ArgumentsOf("kYesMinYesMaxManyArgs"));
        CHECK(argument_map.ArgumentsOf("kYesMinYesMaxFullArgs")
              == reference.ArgumentsOf("kYesMinYesMaxFullArgs"));
        CHECK(argument_map.ArgumentsOf("kSetFlag")
              == reference.ArgumentsOf("kSetFlag"));
        CHECK(argument_map.ArgumentsOf("kUnsetFlag")
              == reference.ArgumentsOf("kUnsetFlag"));
        CHECK(argument_map.ArgumentsOf("kMultipleSetFlag")
              == reference.ArgumentsOf("kMultipleSetFlag"));
        CHECK(argument_map.ArgumentsOf("kNoConverterPositional")
              == reference.ArgumentsOf("kNoConverterPositional"));
        CHECK(argument_map.ArgumentsOf("kNoConverterKeyword")
              == reference.ArgumentsOf("kNoConverterKeyword"));
      }
    }
  }

  GIVEN("Parameters with default arguments.") {
    int count = GENERATE(range(1, 10));
    ParameterMap parameter_map;
    std::vector<std::string> default_args{kDefaultArgs.begin(),
                                          kDefaultArgs.begin() + count};
    parameter_map(kNoMinNoMaxNoArgs.SetDefault(default_args))
                 (kNoMinYesMaxNoArgs.SetDefault(default_args))
                 (kNoMinYesMaxManyArgs.SetDefault(default_args))
                 (kNoMinYesMaxFullArgs.SetDefault(default_args))
                 (kYesMinNoMaxNoArgs.SetDefault(default_args))
                 (kYesMinNoMaxUnderfullArgs.SetDefault(default_args))
                 (kYesMinNoMaxMinArgs.SetDefault(default_args))
                 (kYesMinYesMaxNoArgs.SetDefault(default_args))
                 (kYesMinYesMaxUnderfullArgs.SetDefault(default_args))
                 (kYesMinYesMaxMinArgs.SetDefault(default_args))
                 (kYesMinYesMaxManyArgs.SetDefault(default_args))
                 (kYesMinYesMaxFullArgs.SetDefault(default_args))
                 (kSetFlag.SetDefault(default_args))
                 (kUnsetFlag.SetDefault(default_args))
                 (kMultipleSetFlag.SetDefault(default_args))
                 (kNoConverterPositional.SetDefault(default_args))
                 (kNoConverterKeyword.SetDefault(default_args));

    WHEN("Argument lists are empty.") {
      ArgumentMap argument_map(std::move(parameter_map));
      ArgumentMap reference{argument_map};
      argument_map.SetDefaultArguments();

      THEN("Argument lists are added to where appropriate up to maximum.") {
        CHECK(argument_map.ArgumentsOf("kNoMinNoMaxNoArgs")
              == default_args);
        CHECK(argument_map.ArgumentsOf("kNoMinYesMaxNoArgs")
              == default_args);
        CHECK(argument_map.ArgumentsOf("kNoMinYesMaxManyArgs")
              == default_args);
        CHECK(argument_map.ArgumentsOf("kNoMinYesMaxFullArgs")
              == default_args);
        CHECK(argument_map.ArgumentsOf("kYesMinNoMaxNoArgs")
              == default_args);
        CHECK(argument_map.ArgumentsOf("kYesMinNoMaxUnderfullArgs")
              == default_args);
        CHECK(argument_map.ArgumentsOf("kYesMinNoMaxMinArgs")
              == default_args);
        CHECK(argument_map.ArgumentsOf("kYesMinYesMaxNoArgs")
              == default_args);
        CHECK(argument_map.ArgumentsOf("kYesMinYesMaxUnderfullArgs")
              == default_args);
        CHECK(argument_map.ArgumentsOf("kYesMinYesMaxMinArgs")
              == default_args);
        CHECK(argument_map.ArgumentsOf("kYesMinYesMaxManyArgs")
              == default_args);
        CHECK(argument_map.ArgumentsOf("kYesMinYesMaxFullArgs")
              == default_args);
        CHECK(argument_map.ArgumentsOf("kSetFlag")
              == reference.ArgumentsOf("kSetFlag"));
        CHECK(argument_map.ArgumentsOf("kUnsetFlag")
              == reference.ArgumentsOf("kUnsetFlag"));
        CHECK(argument_map.ArgumentsOf("kMultipleSetFlag")
              == reference.ArgumentsOf("kMultipleSetFlag"));
        CHECK(argument_map.ArgumentsOf("kNoConverterPositional")
              == default_args);
        CHECK(argument_map.ArgumentsOf("kNoConverterKeyword")
              == default_args);
      }
    }

    WHEN("Argument lists already contain some arguments.") {
      ArgumentMap argument_map(std::move(parameter_map));
      for (int i = 0; i < 2; ++i) {
        argument_map.AddArgument("kYesMinNoMaxUnderfullArgs", kArgs.at(i));
        argument_map.AddArgument("kYesMinYesMaxUnderfullArgs", kArgs.at(i));
      }
      for (int i = 0; i < 4; ++i) {
        argument_map.AddArgument("kYesMinNoMaxMinArgs", kArgs.at(i));
        argument_map.AddArgument("kYesMinYesMaxMinArgs", kArgs.at(i));
      }
      for (int i = 0; i < 6; ++i) {
        argument_map.AddArgument("kNoMinYesMaxManyArgs", kArgs.at(i));
        argument_map.AddArgument("kYesMinYesMaxManyArgs", kArgs.at(i));
      }
      for (int i = 0; i < 8; ++i) {
        argument_map.AddArgument("kNoMinYesMaxFullArgs", kArgs.at(i));
        argument_map.AddArgument("kYesMinYesMaxFullArgs", kArgs.at(i));
      }
      argument_map.AddArgument("kSetFlag", kArgs.at(0));
      for (const std::string& arg : kArgs) {
        argument_map.AddArgument("kMultipleSetFlag", arg);
        argument_map.AddArgument("kNoConverterPositional", arg);
        argument_map.AddArgument("kNoConverterKeyword", arg);
      }
      ArgumentMap reference{argument_map};
      argument_map.SetDefaultArguments();

      THEN("Arguments are assigned only to non-empty, non-flag parameters.") {
        CHECK(argument_map.ArgumentsOf("kNoMinNoMaxNoArgs")
              == default_args);
        CHECK(argument_map.ArgumentsOf("kNoMinYesMaxNoArgs")
              == default_args);
        CHECK(argument_map.ArgumentsOf("kNoMinYesMaxManyArgs")
              == reference.ArgumentsOf("kNoMinYesMaxManyArgs"));
        CHECK(argument_map.ArgumentsOf("kNoMinYesMaxFullArgs")
              == reference.ArgumentsOf("kNoMinYesMaxFullArgs"));
        CHECK(argument_map.ArgumentsOf("kYesMinNoMaxNoArgs")
              == default_args);
        CHECK(argument_map.ArgumentsOf("kYesMinNoMaxUnderfullArgs")
              == reference.ArgumentsOf("kYesMinNoMaxUnderfullArgs"));
        CHECK(argument_map.ArgumentsOf("kYesMinNoMaxMinArgs")
              == reference.ArgumentsOf("kYesMinNoMaxMinArgs"));
        CHECK(argument_map.ArgumentsOf("kYesMinYesMaxNoArgs")
              == default_args);
        CHECK(argument_map.ArgumentsOf("kYesMinYesMaxUnderfullArgs")
              == reference.ArgumentsOf("kYesMinYesMaxUnderfullArgs"));
        CHECK(argument_map.ArgumentsOf("kYesMinYesMaxMinArgs")
              == reference.ArgumentsOf("kYesMinYesMaxMinArgs"));
        CHECK(argument_map.ArgumentsOf("kYesMinYesMaxManyArgs")
              == reference.ArgumentsOf("kYesMinYesMaxManyArgs"));
        CHECK(argument_map.ArgumentsOf("kYesMinYesMaxFullArgs")
              == reference.ArgumentsOf("kYesMinYesMaxFullArgs"));
        CHECK(argument_map.ArgumentsOf("kSetFlag")
              == reference.ArgumentsOf("kSetFlag"));
        CHECK(argument_map.ArgumentsOf("kUnsetFlag")
              == reference.ArgumentsOf("kUnsetFlag"));
        CHECK(argument_map.ArgumentsOf("kMultipleSetFlag")
              == reference.ArgumentsOf("kMultipleSetFlag"));
        CHECK(argument_map.ArgumentsOf("kNoConverterPositional")
              == reference.ArgumentsOf("kNoConverterPositional"));
        CHECK(argument_map.ArgumentsOf("kNoConverterKeyword")
              == reference.ArgumentsOf("kNoConverterKeyword"));
      }
    }
  }
}

SCENARIO("Test correctness of ArgumentMap::AddArgument.",
         "[ArgumentMap][AddArgument][correctness]") {

  GIVEN("Parameters without default arguments.") {
    ParameterMap parameter_map;
    parameter_map(kNoMinNoMaxNoArgs)(kNoMinYesMaxNoArgs)
                 (kNoMinYesMaxManyArgs)(kNoMinYesMaxFullArgs)
                 (kYesMinNoMaxNoArgs)(kYesMinNoMaxUnderfullArgs)
                 (kYesMinNoMaxMinArgs)
                 (kYesMinYesMaxNoArgs)(kYesMinYesMaxUnderfullArgs)
                 (kYesMinYesMaxMinArgs)(kYesMinYesMaxManyArgs)
                 (kYesMinYesMaxFullArgs)
                 (kSetFlag)(kUnsetFlag)(kMultipleSetFlag)
                 (kNoConverterPositional)(kNoConverterKeyword);

    WHEN("Argument lists are empty.") {
      ArgumentMap argument_map(std::move(parameter_map));
      std::vector<std::string> args{kArgs.begin(), kArgs.begin() + 8};
      for (const std::string& arg : args) {
        argument_map.AddArgument("kNoMinNoMaxNoArgs", arg);
        argument_map.AddArgument("kNoMinYesMaxNoArgs", arg);
        argument_map.AddArgument("kNoMinYesMaxManyArgs", arg);
        argument_map.AddArgument("kNoMinYesMaxFullArgs", arg);
        argument_map.AddArgument("kYesMinNoMaxNoArgs", arg);
        argument_map.AddArgument("kYesMinNoMaxUnderfullArgs", arg);
        argument_map.AddArgument("kYesMinNoMaxMinArgs", arg);
        argument_map.AddArgument("kYesMinYesMaxNoArgs", arg);
        argument_map.AddArgument("kYesMinYesMaxUnderfullArgs", arg);
        argument_map.AddArgument("kYesMinYesMaxMinArgs", arg);
        argument_map.AddArgument("kYesMinYesMaxManyArgs", arg);
        argument_map.AddArgument("kYesMinYesMaxFullArgs", arg);
        argument_map.AddArgument("kSetFlag", arg);
        argument_map.AddArgument("kUnsetFlag", arg);
        argument_map.AddArgument("kMultipleSetFlag", arg);
        argument_map.AddArgument("kNoConverterPositional", arg);
        argument_map.AddArgument("kNoConverterKeyword", arg);
      }

      THEN("They are appended to the existing list of arguments.") {
        CHECK(argument_map.ArgumentsOf("kNoMinNoMaxNoArgs") == args);
        CHECK(argument_map.ArgumentsOf("kNoMinYesMaxNoArgs") == args);
        CHECK(argument_map.ArgumentsOf("kNoMinYesMaxManyArgs") == args);
        CHECK(argument_map.ArgumentsOf("kNoMinYesMaxFullArgs") == args);
        CHECK(argument_map.ArgumentsOf("kYesMinNoMaxNoArgs") == args);
        CHECK(argument_map.ArgumentsOf("kYesMinNoMaxUnderfullArgs") == args);
        CHECK(argument_map.ArgumentsOf("kYesMinNoMaxMinArgs") == args);
        CHECK(argument_map.ArgumentsOf("kYesMinYesMaxNoArgs") == args);
        CHECK(argument_map.ArgumentsOf("kYesMinYesMaxUnderfullArgs") == args);
        CHECK(argument_map.ArgumentsOf("kYesMinYesMaxMinArgs") == args);
        CHECK(argument_map.ArgumentsOf("kYesMinYesMaxManyArgs") == args);
        CHECK(argument_map.ArgumentsOf("kYesMinYesMaxFullArgs") == args);
        CHECK(argument_map.ArgumentsOf("kSetFlag") == args);
        CHECK(argument_map.ArgumentsOf("kUnsetFlag") == args);
        CHECK(argument_map.ArgumentsOf("kMultipleSetFlag") == args);
        CHECK(argument_map.ArgumentsOf("kNoConverterPositional") == args);
        CHECK(argument_map.ArgumentsOf("kNoConverterKeyword") == args);
      }

      THEN("More arguments are ignored by parameters with argument limits.") {
        for (auto it = kArgs.begin() + 8; it != kArgs.end(); ++it) {
          argument_map.AddArgument("kNoMinYesMaxNoArgs", *it);
          argument_map.AddArgument("kNoMinYesMaxManyArgs", *it);
          argument_map.AddArgument("kNoMinYesMaxFullArgs", *it);
          argument_map.AddArgument("kYesMinYesMaxNoArgs", *it);
          argument_map.AddArgument("kYesMinYesMaxUnderfullArgs", *it);
          argument_map.AddArgument("kYesMinYesMaxMinArgs", *it);
          argument_map.AddArgument("kYesMinYesMaxManyArgs", *it);
          argument_map.AddArgument("kYesMinYesMaxFullArgs", *it);
        }
        CHECK(argument_map.ArgumentsOf("kNoMinYesMaxNoArgs") == args);
        CHECK(argument_map.ArgumentsOf("kNoMinYesMaxManyArgs") == args);
        CHECK(argument_map.ArgumentsOf("kNoMinYesMaxFullArgs") == args);
        CHECK(argument_map.ArgumentsOf("kYesMinYesMaxNoArgs") == args);
        CHECK(argument_map.ArgumentsOf("kYesMinYesMaxUnderfullArgs") == args);
        CHECK(argument_map.ArgumentsOf("kYesMinYesMaxMinArgs") == args);
        CHECK(argument_map.ArgumentsOf("kYesMinYesMaxManyArgs") == args);
        CHECK(argument_map.ArgumentsOf("kYesMinYesMaxFullArgs") == args);
      }

      THEN("More arguments are added for parameters without argument limits.") {
        for (auto it = kArgs.begin() + 8; it != kArgs.end(); ++it) {
          argument_map.AddArgument("kNoMinNoMaxNoArgs", *it);
          argument_map.AddArgument("kYesMinNoMaxNoArgs", *it);
          argument_map.AddArgument("kYesMinNoMaxUnderfullArgs", *it);
          argument_map.AddArgument("kYesMinNoMaxMinArgs", *it);
          argument_map.AddArgument("kSetFlag", *it);
          argument_map.AddArgument("kUnsetFlag", *it);
          argument_map.AddArgument("kMultipleSetFlag", *it);
          argument_map.AddArgument("kNoConverterPositional", *it);
          argument_map.AddArgument("kNoConverterKeyword", *it);
        }
        CHECK(argument_map.ArgumentsOf("kNoMinNoMaxNoArgs") == kArgs);
        CHECK(argument_map.ArgumentsOf("kYesMinNoMaxNoArgs") == kArgs);
        CHECK(argument_map.ArgumentsOf("kYesMinNoMaxUnderfullArgs") == kArgs);
        CHECK(argument_map.ArgumentsOf("kYesMinNoMaxMinArgs") == kArgs);
        CHECK(argument_map.ArgumentsOf("kSetFlag") == kArgs);
        CHECK(argument_map.ArgumentsOf("kUnsetFlag") == kArgs);
        CHECK(argument_map.ArgumentsOf("kMultipleSetFlag") == kArgs);
        CHECK(argument_map.ArgumentsOf("kNoConverterPositional") == kArgs);
        CHECK(argument_map.ArgumentsOf("kNoConverterKeyword") == kArgs);
      }
    }

    WHEN("Argument lists already contain some arguments.") {
      ArgumentMap argument_map(std::move(parameter_map));
      for (int i = 0; i < 2; ++i) {
        argument_map.AddArgument("kYesMinNoMaxUnderfullArgs", kArgs.at(i));
        argument_map.AddArgument("kYesMinYesMaxUnderfullArgs", kArgs.at(i));
      }
      for (int i = 0; i < 4; ++i) {
        argument_map.AddArgument("kYesMinNoMaxMinArgs", kArgs.at(i));
        argument_map.AddArgument("kYesMinYesMaxMinArgs", kArgs.at(i));
      }
      for (int i = 0; i < 6; ++i) {
        argument_map.AddArgument("kNoMinYesMaxManyArgs", kArgs.at(i));
        argument_map.AddArgument("kYesMinYesMaxManyArgs", kArgs.at(i));
      }
      for (int i = 0; i < 8; ++i) {
        argument_map.AddArgument("kNoMinYesMaxFullArgs", kArgs.at(i));
        argument_map.AddArgument("kYesMinYesMaxFullArgs", kArgs.at(i));
      }
      argument_map.AddArgument("kSetFlag", kArgs.at(0));
      for (const std::string& arg : kArgs) {
        argument_map.AddArgument("kMultipleSetFlag", arg);
        argument_map.AddArgument("kNoConverterPositional", arg);
        argument_map.AddArgument("kNoConverterKeyword", arg);
      }
      std::vector<std::string> args{kArgs.begin(), kArgs.begin() + 8};
      for (int i = 0; i < 10; ++i) {
        argument_map.AddArgument("kNoMinNoMaxNoArgs", kArgs.at(i));
        argument_map.AddArgument("kNoMinYesMaxNoArgs", kArgs.at(i));
        argument_map.AddArgument("kYesMinNoMaxNoArgs", kArgs.at(i));
        argument_map.AddArgument("kYesMinYesMaxNoArgs", kArgs.at(i));
        argument_map.AddArgument("kUnsetFlag", kArgs.at(i));
        if (i >= 1) {
          argument_map.AddArgument("kSetFlag", kArgs.at(i));
        }
        if (i >= 2) {
          argument_map.AddArgument("kYesMinNoMaxUnderfullArgs", kArgs.at(i));
          argument_map.AddArgument("kYesMinYesMaxUnderfullArgs", kArgs.at(i));
        }
        if (i >= 4) {
          argument_map.AddArgument("kYesMinNoMaxMinArgs", kArgs.at(i));
          argument_map.AddArgument("kYesMinYesMaxMinArgs", kArgs.at(i));
        }
        if (i >= 6) {
          argument_map.AddArgument("kNoMinYesMaxManyArgs", kArgs.at(i));
          argument_map.AddArgument("kYesMinYesMaxManyArgs", kArgs.at(i));
        }
        if (i >= 8) {
          argument_map.AddArgument("kNoMinYesMaxFullArgs", kArgs.at(i));
          argument_map.AddArgument("kYesMinYesMaxFullArgs", kArgs.at(i));
        }
      }

      THEN("Arguments are added until argument limit is reached, if any.") {
        CHECK(argument_map.ArgumentsOf("kNoMinNoMaxNoArgs") == kArgs);
        CHECK(argument_map.ArgumentsOf("kNoMinYesMaxNoArgs") == args);
        CHECK(argument_map.ArgumentsOf("kNoMinYesMaxManyArgs") == args);
        CHECK(argument_map.ArgumentsOf("kNoMinYesMaxFullArgs") == args);
        CHECK(argument_map.ArgumentsOf("kYesMinNoMaxNoArgs") == kArgs);
        CHECK(argument_map.ArgumentsOf("kYesMinNoMaxUnderfullArgs") == kArgs);
        CHECK(argument_map.ArgumentsOf("kYesMinNoMaxMinArgs") == kArgs);
        CHECK(argument_map.ArgumentsOf("kYesMinYesMaxNoArgs") == args);
        CHECK(argument_map.ArgumentsOf("kYesMinYesMaxUnderfullArgs") == args);
        CHECK(argument_map.ArgumentsOf("kYesMinYesMaxMinArgs") == args);
        CHECK(argument_map.ArgumentsOf("kYesMinYesMaxManyArgs") == args);
        CHECK(argument_map.ArgumentsOf("kYesMinYesMaxFullArgs") == args);
        CHECK(argument_map.ArgumentsOf("kSetFlag") == kArgs);
        CHECK(argument_map.ArgumentsOf("kUnsetFlag") == kArgs);
        CHECK(argument_map.ArgumentsOf("kMultipleSetFlag") == kArgs);
        CHECK(argument_map.ArgumentsOf("kNoConverterPositional") == kArgs);
        CHECK(argument_map.ArgumentsOf("kNoConverterKeyword") == kArgs);
      }
    }
  }
}

SCENARIO("Test exceptions thrown by ArgumentMap::AddArgument.",
         "[ArgumentMap][AddArgument][exceptions]") {
  GIVEN("Parameters without default arguments.") {
    ParameterMap parameter_map;
    parameter_map(kNoMinNoMaxNoArgs)(kNoMinYesMaxNoArgs)
                 (kNoMinYesMaxManyArgs)(kNoMinYesMaxFullArgs)
                 (kYesMinNoMaxNoArgs)(kYesMinNoMaxUnderfullArgs)
                 (kYesMinNoMaxMinArgs)
                 (kYesMinYesMaxNoArgs)(kYesMinYesMaxUnderfullArgs)
                 (kYesMinYesMaxMinArgs)(kYesMinYesMaxManyArgs)
                 (kYesMinYesMaxFullArgs)
                 (kSetFlag)(kUnsetFlag)(kMultipleSetFlag)
                 (kNoConverterPositional)(kNoConverterKeyword);
    ArgumentMap argument_map(std::move(parameter_map));

    THEN("Adding arguments to parameters of unknown names causes exception.") {
      CHECK_THROWS_AS(argument_map.AddArgument("unknown_name", "arg"),
                      exceptions::ParameterAccessError);
      CHECK_THROWS_AS(argument_map.AddArgument("foo", "arg"),
                      exceptions::ParameterAccessError);
      CHECK_THROWS_AS(argument_map.AddArgument("b", "ARG"),
                      exceptions::ParameterAccessError);
      CHECK_THROWS_AS(argument_map.AddArgument("", "arg"),
                      exceptions::ParameterAccessError);
      CHECK_THROWS_AS(argument_map.AddArgument("bazinga", ""),
                      exceptions::ParameterAccessError);
    }

    THEN("Adding arguments to recognized parameters doesn't cause exception.") {
      std::string name = GENERATE(from_range(kNames));
      CHECK_NOTHROW(argument_map.AddArgument(name, "arg"));
    }
  }
}

SCENARIO("Test correctness of ArgumentMap::GetUnfilledParameters.",
         "[ArgumentMap][GetUnfilledParameters][correctness]") {

  GIVEN("Various parameters.") {
    ParameterMap parameter_map;
    parameter_map(kNoMinNoMaxNoArgs.SetDefault(kDefaultArgs))
                 (kNoMinYesMaxNoArgs.SetDefault(kDefaultArgs))
                 (kNoMinYesMaxManyArgs.SetDefault(kDefaultArgs))
                 (kNoMinYesMaxFullArgs.SetDefault(kDefaultArgs))
                 (kYesMinNoMaxNoArgs.SetDefault(kDefaultArgs))
                 (kYesMinNoMaxUnderfullArgs.SetDefault(kDefaultArgs))
                 (kYesMinNoMaxMinArgs.SetDefault(kDefaultArgs))
                 (kYesMinYesMaxNoArgs.SetDefault(kDefaultArgs))
                 (kYesMinYesMaxUnderfullArgs.SetDefault(kDefaultArgs))
                 (kYesMinYesMaxMinArgs.SetDefault(kDefaultArgs))
                 (kYesMinYesMaxManyArgs.SetDefault(kDefaultArgs))
                 (kYesMinYesMaxFullArgs.SetDefault(kDefaultArgs))
                 (kSetFlag.SetDefault(kDefaultArgs))
                 (kUnsetFlag.SetDefault(kDefaultArgs))
                 (kMultipleSetFlag.SetDefault(kDefaultArgs))
                 (kNoConverterPositional.SetDefault(kDefaultArgs))
                 (kNoConverterKeyword.SetDefault(kDefaultArgs));

    WHEN("Argument lists are empty.") {
      ArgumentMap argument_map(std::move(parameter_map));
      std::vector<std::string> unfilled_parameters{
          argument_map.GetUnfilledParameters()};

      THEN("Parameters with minimum argument numbers are unfilled.") {
        CHECK(unfilled_parameters == std::vector<std::string>{
            "kYesMinNoMaxNoArgs", "kYesMinNoMaxUnderfullArgs",
            "kYesMinNoMaxMinArgs", "kYesMinYesMaxNoArgs",
            "kYesMinYesMaxUnderfullArgs", "kYesMinYesMaxMinArgs",
            "kYesMinYesMaxManyArgs", "kYesMinYesMaxFullArgs"});
      }
    }

    WHEN("Argument lists already contain some arguments.") {
      ArgumentMap argument_map(std::move(parameter_map));
      for (int i = 0; i < 2; ++i) {
        argument_map.AddArgument("kYesMinNoMaxUnderfullArgs", kArgs.at(i));
        argument_map.AddArgument("kYesMinYesMaxUnderfullArgs", kArgs.at(i));
      }
      for (int i = 0; i < 4; ++i) {
        argument_map.AddArgument("kYesMinNoMaxMinArgs", kArgs.at(i));
        argument_map.AddArgument("kYesMinYesMaxMinArgs", kArgs.at(i));
      }
      for (int i = 0; i < 6; ++i) {
        argument_map.AddArgument("kNoMinYesMaxManyArgs", kArgs.at(i));
        argument_map.AddArgument("kYesMinYesMaxManyArgs", kArgs.at(i));
      }
      for (int i = 0; i < 8; ++i) {
        argument_map.AddArgument("kNoMinYesMaxFullArgs", kArgs.at(i));
        argument_map.AddArgument("kYesMinYesMaxFullArgs", kArgs.at(i));
      }
      argument_map.AddArgument("kSetFlag", kArgs.at(0));
      for (const std::string& arg : kArgs) {
        argument_map.AddArgument("kMultipleSetFlag", arg);
        argument_map.AddArgument("kNoConverterPositional", arg);
        argument_map.AddArgument("kNoConverterKeyword", arg);
      }
      std::vector<std::string> unfilled_parameters{
        argument_map.GetUnfilledParameters()};

      THEN("Parameters with too few arguments are unfilled.") {
        CHECK(unfilled_parameters == std::vector<std::string>{
            "kYesMinNoMaxNoArgs", "kYesMinNoMaxUnderfullArgs",
            "kYesMinYesMaxNoArgs", "kYesMinYesMaxUnderfullArgs"});
      }
    }
  }
}

SCENARIO("Test correctness of ArgumentMap::HasArgument.",
         "[ArgumentMap][HasArgument][correctness]") {

  GIVEN("An ArgumentMap containing various parameters.") {
    ParameterMap parameter_map;
    parameter_map(kNoMinNoMaxNoArgs.SetDefault(kDefaultArgs))
                 (kNoMinYesMaxNoArgs.SetDefault(kDefaultArgs))
                 (kNoMinYesMaxManyArgs.SetDefault(kDefaultArgs))
                 (kNoMinYesMaxFullArgs.SetDefault(kDefaultArgs))
                 (kYesMinNoMaxNoArgs.SetDefault(kDefaultArgs))
                 (kYesMinNoMaxUnderfullArgs.SetDefault(kDefaultArgs))
                 (kYesMinNoMaxMinArgs.SetDefault(kDefaultArgs))
                 (kYesMinYesMaxNoArgs.SetDefault(kDefaultArgs))
                 (kYesMinYesMaxUnderfullArgs.SetDefault(kDefaultArgs))
                 (kYesMinYesMaxMinArgs.SetDefault(kDefaultArgs))
                 (kYesMinYesMaxManyArgs.SetDefault(kDefaultArgs))
                 (kYesMinYesMaxFullArgs.SetDefault(kDefaultArgs))
                 (kSetFlag.SetDefault(kDefaultArgs))
                 (kUnsetFlag.SetDefault(kDefaultArgs))
                 (kMultipleSetFlag.SetDefault(kDefaultArgs))
                 (kNoConverterPositional.SetDefault(kDefaultArgs))
                 (kNoConverterKeyword.SetDefault(kDefaultArgs));
    ArgumentMap argument_map(std::move(parameter_map));

    WHEN("Argument lists are empty.") {

      THEN("None of them has arguments.") {
        CHECK_FALSE(argument_map.HasArgument("kNoMinNoMaxNoArgs"));
        CHECK_FALSE(argument_map.HasArgument("kNoMinYesMaxNoArgs"));
        CHECK_FALSE(argument_map.HasArgument("kNoMinYesMaxManyArgs"));
        CHECK_FALSE(argument_map.HasArgument("kNoMinYesMaxFullArgs"));
        CHECK_FALSE(argument_map.HasArgument("kYesMinNoMaxNoArgs"));
        CHECK_FALSE(argument_map.HasArgument("kYesMinNoMaxUnderfullArgs"));
        CHECK_FALSE(argument_map.HasArgument("kYesMinNoMaxMinArgs"));
        CHECK_FALSE(argument_map.HasArgument("kYesMinYesMaxNoArgs"));
        CHECK_FALSE(argument_map.HasArgument("kYesMinYesMaxUnderfullArgs"));
        CHECK_FALSE(argument_map.HasArgument("kYesMinYesMaxMinArgs"));
        CHECK_FALSE(argument_map.HasArgument("kYesMinYesMaxManyArgs"));
        CHECK_FALSE(argument_map.HasArgument("kYesMinYesMaxFullArgs"));
        CHECK_FALSE(argument_map.HasArgument("kSetFlag"));
        CHECK_FALSE(argument_map.HasArgument("kUnsetFlag"));
        CHECK_FALSE(argument_map.HasArgument("kMultipleSetFlag"));
        CHECK_FALSE(argument_map.HasArgument("kNoConverterPositional"));
        CHECK_FALSE(argument_map.HasArgument("kNoConverterKeyword"));
      }
    }

    WHEN("Argument lists are partially filled.") {
      for (int i = 0; i < 2; ++i) {
        argument_map.AddArgument("kYesMinNoMaxUnderfullArgs", kArgs.at(i));
        argument_map.AddArgument("kYesMinYesMaxUnderfullArgs", kArgs.at(i));
      }
      for (int i = 0; i < 4; ++i) {
        argument_map.AddArgument("kYesMinNoMaxMinArgs", kArgs.at(i));
        argument_map.AddArgument("kYesMinYesMaxMinArgs", kArgs.at(i));
      }
      for (int i = 0; i < 6; ++i) {
        argument_map.AddArgument("kNoMinYesMaxManyArgs", kArgs.at(i));
        argument_map.AddArgument("kYesMinYesMaxManyArgs", kArgs.at(i));
      }
      for (int i = 0; i < 8; ++i) {
        argument_map.AddArgument("kNoMinYesMaxFullArgs", kArgs.at(i));
        argument_map.AddArgument("kYesMinYesMaxFullArgs", kArgs.at(i));
      }
      argument_map.AddArgument("kSetFlag", kArgs.at(0));
      for (const std::string& arg : kArgs) {
        argument_map.AddArgument("kMultipleSetFlag", arg);
        argument_map.AddArgument("kNoConverterPositional", arg);
        argument_map.AddArgument("kNoConverterKeyword", arg);
      }

      THEN("All arguments with non-empty lists have arguments.") {
        CHECK_FALSE(argument_map.HasArgument("kNoMinNoMaxNoArgs"));
        CHECK_FALSE(argument_map.HasArgument("kNoMinYesMaxNoArgs"));
        CHECK(argument_map.HasArgument("kNoMinYesMaxManyArgs"));
        CHECK(argument_map.HasArgument("kNoMinYesMaxFullArgs"));
        CHECK_FALSE(argument_map.HasArgument("kYesMinNoMaxNoArgs"));
        CHECK(argument_map.HasArgument("kYesMinNoMaxUnderfullArgs"));
        CHECK(argument_map.HasArgument("kYesMinNoMaxMinArgs"));
        CHECK_FALSE(argument_map.HasArgument("kYesMinYesMaxNoArgs"));
        CHECK(argument_map.HasArgument("kYesMinYesMaxUnderfullArgs"));
        CHECK(argument_map.HasArgument("kYesMinYesMaxMinArgs"));
        CHECK(argument_map.HasArgument("kYesMinYesMaxManyArgs"));
        CHECK(argument_map.HasArgument("kYesMinYesMaxFullArgs"));
        CHECK(argument_map.HasArgument("kSetFlag"));
        CHECK_FALSE(argument_map.HasArgument("kUnsetFlag"));
        CHECK(argument_map.HasArgument("kMultipleSetFlag"));
        CHECK(argument_map.HasArgument("kNoConverterPositional"));
        CHECK(argument_map.HasArgument("kNoConverterKeyword"));
      }
    }

    WHEN("Default arguments are set for all arguments.") {
      argument_map.SetDefaultArguments();

      THEN("All non-flag parameters have arguments.") {
        CHECK(argument_map.HasArgument("kNoMinNoMaxNoArgs"));
        CHECK(argument_map.HasArgument("kNoMinYesMaxNoArgs"));
        CHECK(argument_map.HasArgument("kNoMinYesMaxManyArgs"));
        CHECK(argument_map.HasArgument("kNoMinYesMaxFullArgs"));
        CHECK(argument_map.HasArgument("kYesMinNoMaxNoArgs"));
        CHECK(argument_map.HasArgument("kYesMinNoMaxUnderfullArgs"));
        CHECK(argument_map.HasArgument("kYesMinNoMaxMinArgs"));
        CHECK(argument_map.HasArgument("kYesMinYesMaxNoArgs"));
        CHECK(argument_map.HasArgument("kYesMinYesMaxUnderfullArgs"));
        CHECK(argument_map.HasArgument("kYesMinYesMaxMinArgs"));
        CHECK(argument_map.HasArgument("kYesMinYesMaxManyArgs"));
        CHECK(argument_map.HasArgument("kYesMinYesMaxFullArgs"));
        CHECK_FALSE(argument_map.HasArgument("kSetFlag"));
        CHECK_FALSE(argument_map.HasArgument("kUnsetFlag"));
        CHECK_FALSE(argument_map.HasArgument("kMultipleSetFlag"));
        CHECK(argument_map.HasArgument("kNoConverterPositional"));
        CHECK(argument_map.HasArgument("kNoConverterKeyword"));
      }
    }
  }
}

SCENARIO("Test exceptions thrown by ArgumentMap::HasArgument.",
         "[ArgumentMap][HasArgument][exceptions]") {

  GIVEN("Parameters without default arguments.") {
    ParameterMap parameter_map;
    parameter_map(kNoMinNoMaxNoArgs)(kNoMinYesMaxNoArgs)
                 (kNoMinYesMaxManyArgs)(kNoMinYesMaxFullArgs)
                 (kYesMinNoMaxNoArgs)(kYesMinNoMaxUnderfullArgs)
                 (kYesMinNoMaxMinArgs)
                 (kYesMinYesMaxNoArgs)(kYesMinYesMaxUnderfullArgs)
                 (kYesMinYesMaxMinArgs)(kYesMinYesMaxManyArgs)
                 (kYesMinYesMaxFullArgs)
                 (kSetFlag)(kUnsetFlag)(kMultipleSetFlag)
                 (kNoConverterPositional)(kNoConverterKeyword);
    ArgumentMap argument_map(std::move(parameter_map));

    WHEN("Argument lists are empty.") {
      THEN("Testing arguments of parameters of unknown names causes exception.") {
        CHECK_THROWS_AS(argument_map.HasArgument("unknown_name"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.HasArgument("foo"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.HasArgument("b"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.HasArgument(""),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.HasArgument("bazinga"),
                        exceptions::ParameterAccessError);
      }

      THEN("Testing arguments of recognized names doesn't cause exception.") {
        std::string name = GENERATE(from_range(kNames));
        CHECK_NOTHROW(argument_map.HasArgument(name));
      }
    }

    WHEN("Argument lists are partially filled.") {
      for (int i = 0; i < 2; ++i) {
        argument_map.AddArgument("kYesMinNoMaxUnderfullArgs", kArgs.at(i));
        argument_map.AddArgument("kYesMinYesMaxUnderfullArgs", kArgs.at(i));
      }
      for (int i = 0; i < 4; ++i) {
        argument_map.AddArgument("kYesMinNoMaxMinArgs", kArgs.at(i));
        argument_map.AddArgument("kYesMinYesMaxMinArgs", kArgs.at(i));
      }
      for (int i = 0; i < 6; ++i) {
        argument_map.AddArgument("kNoMinYesMaxManyArgs", kArgs.at(i));
        argument_map.AddArgument("kYesMinYesMaxManyArgs", kArgs.at(i));
      }
      for (int i = 0; i < 8; ++i) {
        argument_map.AddArgument("kNoMinYesMaxFullArgs", kArgs.at(i));
        argument_map.AddArgument("kYesMinYesMaxFullArgs", kArgs.at(i));
      }
      argument_map.AddArgument("kSetFlag", kArgs.at(0));
      for (const std::string& arg : kArgs) {
        argument_map.AddArgument("kMultipleSetFlag", arg);
        argument_map.AddArgument("kNoConverterPositional", arg);
        argument_map.AddArgument("kNoConverterKeyword", arg);
      }

      THEN("Testing arguments of parameters of unknown names causes exception.") {
        CHECK_THROWS_AS(argument_map.HasArgument("unknown_name"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.HasArgument("foo"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.HasArgument("b"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.HasArgument(""),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.HasArgument("bazinga"),
                        exceptions::ParameterAccessError);
      }

      THEN("Testing arguments of recognized names doesn't cause exception.") {
        std::string name = GENERATE(from_range(kNames));
        CHECK_NOTHROW(argument_map.HasArgument(name));
      }
    }
  }
}

SCENARIO("Test correctness of ArgumentMap::ArgumentsOf.",
         "[ArgumentMap][ArgumentsOf][correctness]") {

  GIVEN("An ArgumentMap containing various parameters.") {
    ParameterMap parameter_map;
    parameter_map(kNoMinNoMaxNoArgs.SetDefault(kDefaultArgs))
                 (kNoMinYesMaxNoArgs.SetDefault(kDefaultArgs))
                 (kNoMinYesMaxManyArgs.SetDefault(kDefaultArgs))
                 (kNoMinYesMaxFullArgs.SetDefault(kDefaultArgs))
                 (kYesMinNoMaxNoArgs.SetDefault(kDefaultArgs))
                 (kYesMinNoMaxUnderfullArgs.SetDefault(kDefaultArgs))
                 (kYesMinNoMaxMinArgs.SetDefault(kDefaultArgs))
                 (kYesMinYesMaxNoArgs.SetDefault(kDefaultArgs))
                 (kYesMinYesMaxUnderfullArgs.SetDefault(kDefaultArgs))
                 (kYesMinYesMaxMinArgs.SetDefault(kDefaultArgs))
                 (kYesMinYesMaxManyArgs.SetDefault(kDefaultArgs))
                 (kYesMinYesMaxFullArgs.SetDefault(kDefaultArgs))
                 (kSetFlag.SetDefault(kDefaultArgs))
                 (kUnsetFlag.SetDefault(kDefaultArgs))
                 (kMultipleSetFlag.SetDefault(kDefaultArgs))
                 (kNoConverterPositional.SetDefault(kDefaultArgs))
                 (kNoConverterKeyword.SetDefault(kDefaultArgs));
    ArgumentMap argument_map(std::move(parameter_map));

    WHEN("Argument lists are empty.") {

      THEN("None of them has arguments.") {
        CHECK(argument_map.ArgumentsOf("kNoMinNoMaxNoArgs")
              == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("kNoMinYesMaxNoArgs")
              == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("kNoMinYesMaxManyArgs")
              == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("kNoMinYesMaxFullArgs")
              == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("kYesMinNoMaxNoArgs")
              == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("kYesMinNoMaxUnderfullArgs")
              == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("kYesMinNoMaxMinArgs")
              == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("kYesMinYesMaxNoArgs")
              == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("kYesMinYesMaxUnderfullArgs")
              == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("kYesMinYesMaxMinArgs")
              == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("kYesMinYesMaxManyArgs")
              == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("kYesMinYesMaxFullArgs")
              == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("kSetFlag")
              == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("kUnsetFlag")
              == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("kMultipleSetFlag")
              == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("kNoConverterPositional")
              == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("kNoConverterKeyword")
              == std::vector<std::string>{});
      }
    }

    WHEN("Argument lists are partially filled.") {
      for (int i = 0; i < 2; ++i) {
        argument_map.AddArgument("kYesMinNoMaxUnderfullArgs", kArgs.at(i));
        argument_map.AddArgument("kYesMinYesMaxUnderfullArgs", kArgs.at(i));
      }
      for (int i = 0; i < 4; ++i) {
        argument_map.AddArgument("kYesMinNoMaxMinArgs", kArgs.at(i));
        argument_map.AddArgument("kYesMinYesMaxMinArgs", kArgs.at(i));
      }
      for (int i = 0; i < 6; ++i) {
        argument_map.AddArgument("kNoMinYesMaxManyArgs", kArgs.at(i));
        argument_map.AddArgument("kYesMinYesMaxManyArgs", kArgs.at(i));
      }
      for (int i = 0; i < 8; ++i) {
        argument_map.AddArgument("kNoMinYesMaxFullArgs", kArgs.at(i));
        argument_map.AddArgument("kYesMinYesMaxFullArgs", kArgs.at(i));
      }
      argument_map.AddArgument("kSetFlag", kArgs.at(0));
      for (const std::string& arg : kArgs) {
        argument_map.AddArgument("kMultipleSetFlag", arg);
        argument_map.AddArgument("kNoConverterPositional", arg);
        argument_map.AddArgument("kNoConverterKeyword", arg);
      }

      THEN("Partial argument lists are returned.") {
        CHECK(argument_map.ArgumentsOf("kNoMinNoMaxNoArgs")
              == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("kNoMinYesMaxNoArgs")
              == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("kNoMinYesMaxManyArgs")
              == std::vector<std::string>{kArgs.begin(), kArgs.begin() + 6});
        CHECK(argument_map.ArgumentsOf("kNoMinYesMaxFullArgs")
              == std::vector<std::string>{kArgs.begin(), kArgs.begin() + 8});
        CHECK(argument_map.ArgumentsOf("kYesMinNoMaxNoArgs")
              == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("kYesMinNoMaxUnderfullArgs")
              == std::vector<std::string>{kArgs.begin(), kArgs.begin() + 2});
        CHECK(argument_map.ArgumentsOf("kYesMinNoMaxMinArgs")
              == std::vector<std::string>{kArgs.begin(), kArgs.begin() + 4});
        CHECK(argument_map.ArgumentsOf("kYesMinYesMaxNoArgs")
              == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("kYesMinYesMaxUnderfullArgs")
              == std::vector<std::string>{kArgs.begin(), kArgs.begin() + 2});
        CHECK(argument_map.ArgumentsOf("kYesMinYesMaxMinArgs")
              == std::vector<std::string>{kArgs.begin(), kArgs.begin() + 4});
        CHECK(argument_map.ArgumentsOf("kYesMinYesMaxManyArgs")
              == std::vector<std::string>{kArgs.begin(), kArgs.begin() + 6});
        CHECK(argument_map.ArgumentsOf("kYesMinYesMaxFullArgs")
              == std::vector<std::string>{kArgs.begin(), kArgs.begin() + 8});
        CHECK(argument_map.ArgumentsOf("kSetFlag")
              == std::vector<std::string>{kArgs.begin(), kArgs.begin() + 1});
        CHECK(argument_map.ArgumentsOf("kUnsetFlag")
              == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("kMultipleSetFlag") == kArgs);
        CHECK(argument_map.ArgumentsOf("kNoConverterPositional") == kArgs);
        CHECK(argument_map.ArgumentsOf("kNoConverterKeyword") == kArgs);
      }
    }

    WHEN("Default arguments are set for all parameters.") {
      argument_map.SetDefaultArguments();

      THEN("All default arguments are returned for non-flag parameters.") {
        CHECK(argument_map.ArgumentsOf("kNoMinNoMaxNoArgs") == kDefaultArgs);
        CHECK(argument_map.ArgumentsOf("kNoMinYesMaxNoArgs") == kDefaultArgs);
        CHECK(argument_map.ArgumentsOf("kNoMinYesMaxManyArgs") == kDefaultArgs);
        CHECK(argument_map.ArgumentsOf("kNoMinYesMaxFullArgs") == kDefaultArgs);
        CHECK(argument_map.ArgumentsOf("kYesMinNoMaxNoArgs") == kDefaultArgs);
        CHECK(argument_map.ArgumentsOf("kYesMinNoMaxUnderfullArgs")
              == kDefaultArgs);
        CHECK(argument_map.ArgumentsOf("kYesMinNoMaxMinArgs") == kDefaultArgs);
        CHECK(argument_map.ArgumentsOf("kYesMinYesMaxNoArgs") == kDefaultArgs);
        CHECK(argument_map.ArgumentsOf("kYesMinYesMaxUnderfullArgs")
              == kDefaultArgs);
        CHECK(argument_map.ArgumentsOf("kYesMinYesMaxMinArgs") == kDefaultArgs);
        CHECK(argument_map.ArgumentsOf("kYesMinYesMaxManyArgs")
              == kDefaultArgs);
        CHECK(argument_map.ArgumentsOf("kYesMinYesMaxFullArgs")
              == kDefaultArgs);
        CHECK(argument_map.ArgumentsOf("kSetFlag")
              == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("kUnsetFlag")
              == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("kMultipleSetFlag")
              == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("kNoConverterPositional")
              == kDefaultArgs);
        CHECK(argument_map.ArgumentsOf("kNoConverterKeyword") == kDefaultArgs);
      }
    }
  }
}

SCENARIO("Test exceptions thrown by ArgumentMap::ArgumentsOf.",
         "[ArgumentMap][ArgumentsOf][exceptions]") {

  GIVEN("Parameters without default arguments.") {
    ParameterMap parameter_map;
    parameter_map(kNoMinNoMaxNoArgs)(kNoMinYesMaxNoArgs)
                 (kNoMinYesMaxManyArgs)(kNoMinYesMaxFullArgs)
                 (kYesMinNoMaxNoArgs)(kYesMinNoMaxUnderfullArgs)
                 (kYesMinNoMaxMinArgs)
                 (kYesMinYesMaxNoArgs)(kYesMinYesMaxUnderfullArgs)
                 (kYesMinYesMaxMinArgs)(kYesMinYesMaxManyArgs)
                 (kYesMinYesMaxFullArgs)
                 (kSetFlag)(kUnsetFlag)(kMultipleSetFlag)
                 (kNoConverterPositional)(kNoConverterKeyword);
    ArgumentMap argument_map(std::move(parameter_map));

    WHEN("Argument lists are empty.") {

      THEN("Getting arguments of unknown parameters causes exception.") {
        CHECK_THROWS_AS(argument_map.ArgumentsOf("unknown_name"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.ArgumentsOf("foo"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.ArgumentsOf("b"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.ArgumentsOf(""),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.ArgumentsOf("bazinga"),
                        exceptions::ParameterAccessError);
      }

      THEN("Getting arguments of recognized names doesn't cause exception.") {
        std::string name = GENERATE(from_range(kNames));
        CHECK_NOTHROW(argument_map.ArgumentsOf(name));
      }
    }

    WHEN("Argument lists are partially filled.") {
      for (int i = 0; i < 2; ++i) {
        argument_map.AddArgument("kYesMinNoMaxUnderfullArgs", kArgs.at(i));
        argument_map.AddArgument("kYesMinYesMaxUnderfullArgs", kArgs.at(i));
      }
      for (int i = 0; i < 4; ++i) {
        argument_map.AddArgument("kYesMinNoMaxMinArgs", kArgs.at(i));
        argument_map.AddArgument("kYesMinYesMaxMinArgs", kArgs.at(i));
      }
      for (int i = 0; i < 6; ++i) {
        argument_map.AddArgument("kNoMinYesMaxManyArgs", kArgs.at(i));
        argument_map.AddArgument("kYesMinYesMaxManyArgs", kArgs.at(i));
      }
      for (int i = 0; i < 8; ++i) {
        argument_map.AddArgument("kNoMinYesMaxFullArgs", kArgs.at(i));
        argument_map.AddArgument("kYesMinYesMaxFullArgs", kArgs.at(i));
      }
      argument_map.AddArgument("kSetFlag", kArgs.at(0));
      for (const std::string& arg : kArgs) {
        argument_map.AddArgument("kMultipleSetFlag", arg);
        argument_map.AddArgument("kNoConverterPositional", arg);
        argument_map.AddArgument("kNoConverterKeyword", arg);
      }

      THEN("Testing arguments of unknown parameters causes exception.") {
        CHECK_THROWS_AS(argument_map.ArgumentsOf("unknown_name"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.ArgumentsOf("foo"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.ArgumentsOf("b"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.ArgumentsOf(""),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.ArgumentsOf("bazinga"),
                        exceptions::ParameterAccessError);
      }

      THEN("Testing arguments of recognized names doesn't cause exception.") {
        std::string name = GENERATE(from_range(kNames));
        CHECK_NOTHROW(argument_map.ArgumentsOf(name));
      }
    }
  }
}

SCENARIO("Test correctness of ArgumentMap::GetValue.",
         "[ArgumentMap][GetValue][correctness]") {

  GIVEN("An ArgumentMap containing various parameters.") {
    ParameterMap parameter_map;
    parameter_map(kNoMinNoMaxNoArgs.SetDefault(kDefaultArgs))
                 (kNoMinYesMaxNoArgs.SetDefault(kDefaultArgs))
                 (kNoMinYesMaxManyArgs.SetDefault(kDefaultArgs))
                 (kNoMinYesMaxFullArgs.SetDefault(kDefaultArgs))
                 (kYesMinNoMaxNoArgs.SetDefault(kDefaultArgs))
                 (kYesMinNoMaxUnderfullArgs.SetDefault(kDefaultArgs))
                 (kYesMinNoMaxMinArgs.SetDefault(kDefaultArgs))
                 (kYesMinYesMaxNoArgs.SetDefault(kDefaultArgs))
                 (kYesMinYesMaxUnderfullArgs.SetDefault(kDefaultArgs))
                 (kYesMinYesMaxMinArgs.SetDefault(kDefaultArgs))
                 (kYesMinYesMaxManyArgs.SetDefault(kDefaultArgs))
                 (kYesMinYesMaxFullArgs.SetDefault(kDefaultArgs))
                 (kSetFlag.SetDefault(kDefaultArgs))
                 (kUnsetFlag.SetDefault(kDefaultArgs))
                 (kMultipleSetFlag.SetDefault(kDefaultArgs))
                 (kNoConverterPositional.SetDefault(kDefaultArgs))
                 (kNoConverterKeyword.SetDefault(kDefaultArgs));
    ArgumentMap argument_map(std::move(parameter_map));

    WHEN("Argument lists are partially filled.") {
      for (int i = 0; i < 2; ++i) {
        argument_map.AddArgument("kYesMinNoMaxUnderfullArgs", kArgs.at(i));
        argument_map.AddArgument("kYesMinYesMaxUnderfullArgs", kArgs.at(i));
      }
      for (int i = 0; i < 4; ++i) {
        argument_map.AddArgument("kYesMinNoMaxMinArgs", kArgs.at(i));
        argument_map.AddArgument("kYesMinYesMaxMinArgs", kArgs.at(i));
      }
      for (int i = 0; i < 6; ++i) {
        argument_map.AddArgument("kNoMinYesMaxManyArgs", kArgs.at(i));
        argument_map.AddArgument("kYesMinYesMaxManyArgs", kArgs.at(i));
      }
      for (int i = 0; i < 8; ++i) {
        argument_map.AddArgument("kNoMinYesMaxFullArgs", kArgs.at(i));
        argument_map.AddArgument("kYesMinYesMaxFullArgs", kArgs.at(i));
      }
      argument_map.AddArgument("kSetFlag", kArgs.at(0));
      for (const std::string& arg : kArgs) {
        argument_map.AddArgument("kMultipleSetFlag", arg);
        argument_map.AddArgument("kNoConverterPositional", arg);
        argument_map.AddArgument("kNoConverterKeyword", arg);
      }

      THEN("Calling function without specifying position gets first value.") {
        CHECK(argument_map.GetValue<int>("kNoMinYesMaxManyArgs") == 1);
        CHECK(argument_map.GetValue<int>("kNoMinYesMaxFullArgs") == 1);
        CHECK(argument_map.GetValue<TestType>("kYesMinNoMaxUnderfullArgs")
              == TestType{1});
        CHECK(argument_map.GetValue<long>("kYesMinNoMaxMinArgs") == 1l);
        CHECK(argument_map.GetValue<float>("kYesMinYesMaxUnderfullArgs") 
              == Approx(1.0f));
        CHECK(argument_map.GetValue<float>("kYesMinYesMaxMinArgs")
              == Approx(1.0f));
        CHECK(argument_map.GetValue<double>("kYesMinYesMaxManyArgs")
              == Approx(1.0));
        CHECK(argument_map.GetValue<double>("kYesMinYesMaxFullArgs")
              == Approx(1.0));
      }

      THEN("Specifying valid position will return the requested value.") {
        int i = GENERATE(range(0, 7));
        if (i < 2) {
          CHECK(argument_map.GetValue<TestType>("kYesMinNoMaxUnderfullArgs", i)
                == TestType{std::stoi(kArgs.at(i))});
          CHECK(argument_map.GetValue<float>("kYesMinYesMaxUnderfullArgs", i) 
                == Approx(std::stof(kArgs.at(i))));
        }
        if (i < 4) {
          CHECK(argument_map.GetValue<long>("kYesMinNoMaxMinArgs", i)
                == std::stol(kArgs.at(i)));
          CHECK(argument_map.GetValue<float>("kYesMinYesMaxMinArgs", i)
                == Approx(std::stof(kArgs.at(i))));
        }
        if (i < 6) {
          CHECK(argument_map.GetValue<int>("kNoMinYesMaxManyArgs", i)
                == std::stoi(kArgs.at(i)));
          CHECK(argument_map.GetValue<double>("kYesMinYesMaxManyArgs", i)
                == Approx(std::stod(kArgs.at(i))));
        }
        if (i < 8) {
          CHECK(argument_map.GetValue<int>("kNoMinYesMaxFullArgs", i)
                == std::stoi(kArgs.at(i)));
          CHECK(argument_map.GetValue<double>("kYesMinYesMaxFullArgs", i)
                == Approx(std::stod(kArgs.at(i))));
        }
      }
    }

    WHEN("Default arguments are set for all parameters.") {
      argument_map.SetDefaultArguments();

      THEN("Calling function without specifying position gets first value.") {
        CHECK(argument_map.GetValue<std::string>("kNoMinNoMaxNoArgs")
              == converters::StringIdentity(kDefaultArgs.at(0)));
        CHECK(argument_map.GetValue<std::string>("kNoMinYesMaxNoArgs")
              == converters::StringIdentity(kDefaultArgs.at(0)));
        CHECK(argument_map.GetValue<int>("kNoMinYesMaxManyArgs")
              == std::stoi(kDefaultArgs.at(0)));
        CHECK(argument_map.GetValue<int>("kNoMinYesMaxFullArgs")
              == std::stoi(kDefaultArgs.at(0)));
        CHECK(argument_map.GetValue<TestType>("kYesMinNoMaxNoArgs")
              == StringToTestType(kDefaultArgs.at(0)));
        CHECK(argument_map.GetValue<TestType>("kYesMinNoMaxUnderfullArgs")
              == StringToTestType(kDefaultArgs.at(0)));
        CHECK(argument_map.GetValue<long>("kYesMinNoMaxMinArgs")
              == std::stol(kDefaultArgs.at(0)));
        CHECK(argument_map.GetValue<long>("kYesMinYesMaxNoArgs")
              == std::stol(kDefaultArgs.at(0)));
        CHECK(argument_map.GetValue<float>("kYesMinYesMaxUnderfullArgs") 
              == Approx(std::stof(kDefaultArgs.at(0))));
        CHECK(argument_map.GetValue<float>("kYesMinYesMaxMinArgs")
              == Approx(std::stof(kDefaultArgs.at(0))));
        CHECK(argument_map.GetValue<double>("kYesMinYesMaxManyArgs")
              == Approx(std::stod(kDefaultArgs.at(0))));
        CHECK(argument_map.GetValue<double>("kYesMinYesMaxFullArgs")
              == Approx(std::stod(kDefaultArgs.at(0))));
      }

      THEN("Specifying valid position will return the requested value.") {
        int i = GENERATE(range(0, 8));
        if (i < 2) {
          CHECK(argument_map.GetValue<TestType>("kYesMinNoMaxUnderfullArgs", i)
                == TestType{std::stoi(kDefaultArgs.at(i))});
          CHECK(argument_map.GetValue<float>("kYesMinYesMaxUnderfullArgs", i) 
                == Approx(std::stof(kDefaultArgs.at(i))));
        }
        if (i < 4) {
          CHECK(argument_map.GetValue<long>("kYesMinNoMaxMinArgs", i)
                == std::stol(kDefaultArgs.at(i)));
          CHECK(argument_map.GetValue<float>("kYesMinYesMaxMinArgs", i)
                == Approx(std::stof(kDefaultArgs.at(i))));
        }
        if (i < 6) {
          CHECK(argument_map.GetValue<int>("kNoMinYesMaxManyArgs", i)
                == std::stoi(kDefaultArgs.at(i)));
          CHECK(argument_map.GetValue<double>("kYesMinYesMaxManyArgs", i)
                == Approx(std::stod(kDefaultArgs.at(i))));
        }
        if (i < 8) {
          CHECK(argument_map.GetValue<int>("kNoMinYesMaxFullArgs", i)
                == std::stoi(kDefaultArgs.at(i)));
          CHECK(argument_map.GetValue<double>("kYesMinYesMaxFullArgs", i)
                == Approx(std::stod(kDefaultArgs.at(i))));
        }
      }
    }
  }
}

SCENARIO("Test exceptions thrown by ArgumentMap::GetValue.",
         "[ArgumentMap][GetValue][exceptions]") {

  GIVEN("An ArgumentMap containing various parameters.") {
    ParameterMap parameter_map;
    parameter_map(kNoMinNoMaxNoArgs.SetDefault(kDefaultArgs))
                 (kNoMinYesMaxNoArgs.SetDefault(kDefaultArgs))
                 (kNoMinYesMaxManyArgs.SetDefault(kDefaultArgs))
                 (kNoMinYesMaxFullArgs.SetDefault(kDefaultArgs))
                 (kYesMinNoMaxNoArgs.SetDefault(kDefaultArgs))
                 (kYesMinNoMaxUnderfullArgs.SetDefault(kDefaultArgs))
                 (kYesMinNoMaxMinArgs.SetDefault(kDefaultArgs))
                 (kYesMinYesMaxNoArgs.SetDefault(kDefaultArgs))
                 (kYesMinYesMaxUnderfullArgs.SetDefault(kDefaultArgs))
                 (kYesMinYesMaxMinArgs.SetDefault(kDefaultArgs))
                 (kYesMinYesMaxManyArgs.SetDefault(kDefaultArgs))
                 (kYesMinYesMaxFullArgs.SetDefault(kDefaultArgs))
                 (kSetFlag.SetDefault(kDefaultArgs))
                 (kUnsetFlag.SetDefault(kDefaultArgs))
                 (kMultipleSetFlag.SetDefault(kDefaultArgs))
                 (kNoConverterPositional.SetDefault(kDefaultArgs))
                 (kNoConverterKeyword.SetDefault(kDefaultArgs));
    ArgumentMap argument_map(std::move(parameter_map));

    WHEN("Argument lists are empty.") {
      
      THEN("Getting arguments of unknown parameters causes exception.") {
        CHECK_THROWS_AS(argument_map.GetValue<std::string>("unknown_name"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.GetValue<int>("foo"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.GetValue<TestType>("b"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.GetValue<long>(""),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.GetValue<float>("bazinga"),
                        exceptions::ParameterAccessError);
      }

      THEN("Mismatching types causes exception.") {
        CHECK_THROWS_AS(argument_map.GetValue<double>("kNoMinNoMaxNoArgs"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.GetValue<double>("kNoMinYesMaxNoArgs"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.GetValue<std::string>(
                            "kNoMinYesMaxManyArgs"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.GetValue<std::string>(
                            "kNoMinYesMaxFullArgs"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.GetValue<int>("kYesMinNoMaxNoArgs"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.GetValue<int>("kYesMinNoMaxUnderfullArgs"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.GetValue<TestType>("kYesMinNoMaxMinArgs"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.GetValue<TestType>("kYesMinYesMaxNoArgs"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.GetValue<long>(
                            "kYesMinYesMaxUnderfullArgs"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.GetValue<long>(
                            "kYesMinYesMaxMinArgs"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.GetValue<float>("kYesMinYesMaxManyArgs"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.GetValue<float>("kYesMinYesMaxFullArgs"),
                        exceptions::ParameterAccessError);
      }

      THEN("Parameters without conversion functions cause exception.") {
        CHECK_THROWS_AS(argument_map.GetValue<TestType>("kNoConverterPositional"),
                        exceptions::ValueAccessError);
        CHECK_THROWS_AS(argument_map.GetValue<TestType>("kNoConverterKeyword"),
                        exceptions::ValueAccessError);
      }

      THEN("Accessing value beyond argument list's range causes exception.") {
        CHECK_THROWS_AS(argument_map.GetValue<std::string>("kNoMinNoMaxNoArgs"),
                        exceptions::ValueAccessError);
        CHECK_THROWS_AS(argument_map.GetValue<std::string>(
                            "kNoMinYesMaxNoArgs"),
                        exceptions::ValueAccessError);
        CHECK_THROWS_AS(argument_map.GetValue<int>("kNoMinYesMaxManyArgs"),
                        exceptions::ValueAccessError);
        CHECK_THROWS_AS(argument_map.GetValue<int>("kNoMinYesMaxFullArgs"),
                        exceptions::ValueAccessError);
        CHECK_THROWS_AS(argument_map.GetValue<TestType>("kYesMinNoMaxNoArgs"),
                        exceptions::ValueAccessError);
        CHECK_THROWS_AS(argument_map.GetValue<TestType>(
                            "kYesMinNoMaxUnderfullArgs"),
                        exceptions::ValueAccessError);
        CHECK_THROWS_AS(argument_map.GetValue<long>("kYesMinNoMaxMinArgs"),
                        exceptions::ValueAccessError);
        CHECK_THROWS_AS(argument_map.GetValue<long>("kYesMinYesMaxNoArgs"),
                        exceptions::ValueAccessError);
        CHECK_THROWS_AS(argument_map.GetValue<float>(
                            "kYesMinYesMaxUnderfullArgs"),
                        exceptions::ValueAccessError);
        CHECK_THROWS_AS(argument_map.GetValue<float>("kYesMinYesMaxMinArgs"),
                        exceptions::ValueAccessError);
        CHECK_THROWS_AS(argument_map.GetValue<double>("kYesMinYesMaxManyArgs"),
                        exceptions::ValueAccessError);
        CHECK_THROWS_AS(argument_map.GetValue<double>("kYesMinYesMaxFullArgs"),
                        exceptions::ValueAccessError);
      }

      THEN("Flags cause exception.") {
        CHECK_THROWS_AS(argument_map.GetValue<bool>("kSetFlag"),
                        exceptions::ValueAccessError);
        CHECK_THROWS_AS(argument_map.GetValue<bool>("kUnsetFlag"),
                        exceptions::ValueAccessError);
        CHECK_THROWS_AS(argument_map.GetValue<bool>("kMultipleSetFlag"),
                        exceptions::ValueAccessError);
      }
    }

    WHEN("Argument lists are partially filled.") {
      for (int i = 0; i < 2; ++i) {
        argument_map.AddArgument("kYesMinNoMaxUnderfullArgs", kArgs.at(i));
        argument_map.AddArgument("kYesMinYesMaxUnderfullArgs", kArgs.at(i));
      }
      for (int i = 0; i < 4; ++i) {
        argument_map.AddArgument("kYesMinNoMaxMinArgs", kArgs.at(i));
        argument_map.AddArgument("kYesMinYesMaxMinArgs", kArgs.at(i));
      }
      for (int i = 0; i < 6; ++i) {
        argument_map.AddArgument("kNoMinYesMaxManyArgs", kArgs.at(i));
        argument_map.AddArgument("kYesMinYesMaxManyArgs", kArgs.at(i));
      }
      for (int i = 0; i < 8; ++i) {
        argument_map.AddArgument("kNoMinYesMaxFullArgs", kArgs.at(i));
        argument_map.AddArgument("kYesMinYesMaxFullArgs", kArgs.at(i));
      }
      argument_map.AddArgument("kSetFlag", kArgs.at(0));
      for (const std::string& arg : kArgs) {
        argument_map.AddArgument("kMultipleSetFlag", arg);
        argument_map.AddArgument("kNoConverterPositional", arg);
        argument_map.AddArgument("kNoConverterKeyword", arg);
      }
      
      THEN("Getting arguments of unknown parameters causes exception.") {
        CHECK_THROWS_AS(argument_map.GetValue<std::string>("unknown_name"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.GetValue<int>("foo"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.GetValue<TestType>("b"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.GetValue<long>(""),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.GetValue<float>("bazinga"),
                        exceptions::ParameterAccessError);
      }

      THEN("Mismatching types causes exception.") {
        CHECK_THROWS_AS(argument_map.GetValue<double>("kNoMinNoMaxNoArgs"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.GetValue<double>("kNoMinYesMaxNoArgs"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.GetValue<std::string>(
                            "kNoMinYesMaxManyArgs"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.GetValue<std::string>(
                            "kNoMinYesMaxFullArgs"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.GetValue<int>("kYesMinNoMaxNoArgs"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.GetValue<int>("kYesMinNoMaxUnderfullArgs"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.GetValue<TestType>("kYesMinNoMaxMinArgs"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.GetValue<TestType>("kYesMinYesMaxNoArgs"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.GetValue<long>(
                            "kYesMinYesMaxUnderfullArgs"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.GetValue<long>(
                            "kYesMinYesMaxMinArgs"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.GetValue<float>("kYesMinYesMaxManyArgs"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.GetValue<float>("kYesMinYesMaxFullArgs"),
                        exceptions::ParameterAccessError);
      }

      THEN("Parameters without conversion functions cause exception.") {
        CHECK_THROWS_AS(argument_map.GetValue<TestType>("kNoConverterPositional"),
                        exceptions::ValueAccessError);
        CHECK_THROWS_AS(argument_map.GetValue<TestType>("kNoConverterKeyword"),
                        exceptions::ValueAccessError);
      }

      THEN("Accessing value beyond argument list's range causes exception.") {
        CHECK_THROWS_AS(argument_map.GetValue<std::string>("kNoMinNoMaxNoArgs"),
                        exceptions::ValueAccessError);
        CHECK_THROWS_AS(argument_map.GetValue<std::string>(
                            "kNoMinYesMaxNoArgs", 0),
                        exceptions::ValueAccessError);
        CHECK_THROWS_AS(argument_map.GetValue<int>("kNoMinYesMaxManyArgs", 6),
                        exceptions::ValueAccessError);
        CHECK_THROWS_AS(argument_map.GetValue<int>("kNoMinYesMaxFullArgs", 8),
                        exceptions::ValueAccessError);
        CHECK_THROWS_AS(argument_map.GetValue<TestType>(
                            "kYesMinNoMaxNoArgs", kArgs.size()),
                        exceptions::ValueAccessError);
        CHECK_THROWS_AS(argument_map.GetValue<TestType>(
                            "kYesMinNoMaxUnderfullArgs", 2),
                        exceptions::ValueAccessError);
        CHECK_THROWS_AS(argument_map.GetValue<long>("kYesMinNoMaxMinArgs", 4),
                        exceptions::ValueAccessError);
        CHECK_THROWS_AS(argument_map.GetValue<long>("kYesMinYesMaxNoArgs", 1),
                        exceptions::ValueAccessError);
        CHECK_THROWS_AS(argument_map.GetValue<float>(
                            "kYesMinYesMaxUnderfullArgs", kArgs.size()),
                        exceptions::ValueAccessError);
        CHECK_THROWS_AS(argument_map.GetValue<float>(
                            "kYesMinYesMaxMinArgs", kArgs.size()),
                        exceptions::ValueAccessError);
        CHECK_THROWS_AS(argument_map.GetValue<double>(
                            "kYesMinYesMaxManyArgs", kArgs.size()),
                        exceptions::ValueAccessError);
        CHECK_THROWS_AS(argument_map.GetValue<double>(
                            "kYesMinYesMaxFullArgs", kArgs.size()),
                        exceptions::ValueAccessError);
      }

      THEN("Flags cause exception.") {
        CHECK_THROWS_AS(argument_map.GetValue<bool>("kSetFlag"),
                        exceptions::ValueAccessError);
        CHECK_THROWS_AS(argument_map.GetValue<bool>("kUnsetFlag"),
                        exceptions::ValueAccessError);
        CHECK_THROWS_AS(argument_map.GetValue<bool>("kMultipleSetFlag"),
                        exceptions::ValueAccessError);
      }
    }

    WHEN("Default arguments are set for all parameters.") {
      argument_map.SetDefaultArguments();

      THEN("Getting arguments of unknown parameters causes exception.") {
        CHECK_THROWS_AS(argument_map.GetValue<std::string>("unknown_name"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.GetValue<int>("foo"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.GetValue<TestType>("b"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.GetValue<long>(""),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.GetValue<float>("bazinga"),
                        exceptions::ParameterAccessError);
      }

      THEN("Mismatching types causes exception.") {
        CHECK_THROWS_AS(argument_map.GetValue<double>("kNoMinNoMaxNoArgs"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.GetValue<double>("kNoMinYesMaxNoArgs"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.GetValue<std::string>(
                            "kNoMinYesMaxManyArgs"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.GetValue<std::string>(
                            "kNoMinYesMaxFullArgs"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.GetValue<int>("kYesMinNoMaxNoArgs"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.GetValue<int>("kYesMinNoMaxUnderfullArgs"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.GetValue<TestType>("kYesMinNoMaxMinArgs"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.GetValue<TestType>("kYesMinYesMaxNoArgs"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.GetValue<long>(
                            "kYesMinYesMaxUnderfullArgs"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.GetValue<long>(
                            "kYesMinYesMaxMinArgs"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.GetValue<float>("kYesMinYesMaxManyArgs"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.GetValue<float>("kYesMinYesMaxFullArgs"),
                        exceptions::ParameterAccessError);
      }

      THEN("Parameters without conversion functions cause exception.") {
        CHECK_THROWS_AS(argument_map.GetValue<TestType>("kNoConverterPositional"),
                        exceptions::ValueAccessError);
        CHECK_THROWS_AS(argument_map.GetValue<TestType>("kNoConverterKeyword"),
                        exceptions::ValueAccessError);
      }

      THEN("Accessing value beyond argument list's range causes exception.") {
        CHECK_THROWS_AS(argument_map.GetValue<std::string>(
                            "kNoMinNoMaxNoArgs", kDefaultArgs.size()),
                        exceptions::ValueAccessError);
        CHECK_THROWS_AS(argument_map.GetValue<std::string>(
                            "kNoMinYesMaxNoArgs", kDefaultArgs.size()),
                        exceptions::ValueAccessError);
        CHECK_THROWS_AS(argument_map.GetValue<int>(
                            "kNoMinYesMaxManyArgs", kDefaultArgs.size()),
                        exceptions::ValueAccessError);
        CHECK_THROWS_AS(argument_map.GetValue<int>(
                            "kNoMinYesMaxFullArgs", kDefaultArgs.size()),
                        exceptions::ValueAccessError);
        CHECK_THROWS_AS(argument_map.GetValue<TestType>(
                            "kYesMinNoMaxNoArgs", kDefaultArgs.size()),
                        exceptions::ValueAccessError);
        CHECK_THROWS_AS(argument_map.GetValue<TestType>(
                            "kYesMinNoMaxUnderfullArgs", kDefaultArgs.size()),
                        exceptions::ValueAccessError);
        CHECK_THROWS_AS(argument_map.GetValue<long>(
                            "kYesMinNoMaxMinArgs", kDefaultArgs.size()),
                        exceptions::ValueAccessError);
        CHECK_THROWS_AS(argument_map.GetValue<long>(
                            "kYesMinYesMaxNoArgs", kDefaultArgs.size()),
                        exceptions::ValueAccessError);
        CHECK_THROWS_AS(argument_map.GetValue<float>(
                            "kYesMinYesMaxUnderfullArgs", kDefaultArgs.size()),
                        exceptions::ValueAccessError);
        CHECK_THROWS_AS(argument_map.GetValue<float>(
                            "kYesMinYesMaxMinArgs", kDefaultArgs.size()),
                        exceptions::ValueAccessError);
        CHECK_THROWS_AS(argument_map.GetValue<double>(
                            "kYesMinYesMaxManyArgs", kDefaultArgs.size()),
                        exceptions::ValueAccessError);
        CHECK_THROWS_AS(argument_map.GetValue<double>(
                            "kYesMinYesMaxFullArgs", kDefaultArgs.size()),
                        exceptions::ValueAccessError);
      }

      THEN("Flags cause exception.") {
        CHECK_THROWS_AS(argument_map.GetValue<bool>("kSetFlag"),
                        exceptions::ValueAccessError);
        CHECK_THROWS_AS(argument_map.GetValue<bool>("kUnsetFlag"),
                        exceptions::ValueAccessError);
        CHECK_THROWS_AS(argument_map.GetValue<bool>("kMultipleSetFlag"),
                        exceptions::ValueAccessError);
      }
    }
  }
}

SCENARIO("Test correctness of ArgumentMap::GetAllValues.",
         "[ArgumentMap][GetAllValues][correctness]") {

  GIVEN("An ArgumentMap containing various parameters.") {
    ParameterMap parameter_map;
    parameter_map(kNoMinNoMaxNoArgs.SetDefault(kDefaultArgs))
                 (kNoMinYesMaxNoArgs.SetDefault(kDefaultArgs))
                 (kNoMinYesMaxManyArgs.SetDefault(kDefaultArgs))
                 (kNoMinYesMaxFullArgs.SetDefault(kDefaultArgs))
                 (kYesMinNoMaxNoArgs.SetDefault(kDefaultArgs))
                 (kYesMinNoMaxUnderfullArgs.SetDefault(kDefaultArgs))
                 (kYesMinNoMaxMinArgs.SetDefault(kDefaultArgs))
                 (kYesMinYesMaxNoArgs.SetDefault(kDefaultArgs))
                 (kYesMinYesMaxUnderfullArgs.SetDefault(kDefaultArgs))
                 (kYesMinYesMaxMinArgs.SetDefault(kDefaultArgs))
                 (kYesMinYesMaxManyArgs.SetDefault(kDefaultArgs))
                 (kYesMinYesMaxFullArgs.SetDefault(kDefaultArgs))
                 (kSetFlag.SetDefault(kDefaultArgs))
                 (kUnsetFlag.SetDefault(kDefaultArgs))
                 (kMultipleSetFlag.SetDefault(kDefaultArgs))
                 (kNoConverterPositional.SetDefault(kDefaultArgs))
                 (kNoConverterKeyword.SetDefault(kDefaultArgs));
    ArgumentMap argument_map(std::move(parameter_map));

    WHEN("Argument lists are empty.") {

      THEN("Empty value lists are returned.") {
        CHECK(argument_map.GetAllValues<std::string>("kNoMinNoMaxNoArgs")
              == std::vector<std::string>{});
        CHECK(argument_map.GetAllValues<std::string>("kNoMinYesMaxNoArgs")
              == std::vector<std::string>{});
        CHECK(argument_map.GetAllValues<int>("kNoMinYesMaxManyArgs")
              == std::vector<int>{});
        CHECK(argument_map.GetAllValues<int>("kNoMinYesMaxFullArgs")
              == std::vector<int>{});
        CHECK(argument_map.GetAllValues<TestType>("kYesMinNoMaxNoArgs")
              == std::vector<TestType>{});
        CHECK(argument_map.GetAllValues<TestType>("kYesMinNoMaxUnderfullArgs")
              == std::vector<TestType>{});
        CHECK(argument_map.GetAllValues<long>("kYesMinNoMaxMinArgs")
              == std::vector<long>{});
        CHECK(argument_map.GetAllValues<long>("kYesMinYesMaxNoArgs")
              == std::vector<long>{});
        CHECK(argument_map.GetAllValues<float>("kYesMinYesMaxUnderfullArgs") 
              == std::vector<float>{});
        CHECK(argument_map.GetAllValues<float>("kYesMinYesMaxMinArgs")
              == std::vector<float>{});
        CHECK(argument_map.GetAllValues<double>("kYesMinYesMaxManyArgs")
              == std::vector<double>{});
        CHECK(argument_map.GetAllValues<double>("kYesMinYesMaxFullArgs")
              == std::vector<double>{});
      }
    }

    WHEN("Argument lists are partially filled.") {
      for (int i = 0; i < 2; ++i) {
        argument_map.AddArgument("kYesMinNoMaxUnderfullArgs", kArgs.at(i));
        argument_map.AddArgument("kYesMinYesMaxUnderfullArgs", kArgs.at(i));
      }
      for (int i = 0; i < 4; ++i) {
        argument_map.AddArgument("kYesMinNoMaxMinArgs", kArgs.at(i));
        argument_map.AddArgument("kYesMinYesMaxMinArgs", kArgs.at(i));
      }
      for (int i = 0; i < 6; ++i) {
        argument_map.AddArgument("kNoMinYesMaxManyArgs", kArgs.at(i));
        argument_map.AddArgument("kYesMinYesMaxManyArgs", kArgs.at(i));
      }
      for (int i = 0; i < 8; ++i) {
        argument_map.AddArgument("kNoMinYesMaxFullArgs", kArgs.at(i));
        argument_map.AddArgument("kYesMinYesMaxFullArgs", kArgs.at(i));
      }
      argument_map.AddArgument("kSetFlag", kArgs.at(0));
      for (const std::string& arg : kArgs) {
        argument_map.AddArgument("kMultipleSetFlag", arg);
        argument_map.AddArgument("kNoConverterPositional", arg);
        argument_map.AddArgument("kNoConverterKeyword", arg);
      }

      std::vector<std::string> kNoMinNoMaxNoArgs_vals{
          argument_map.GetAllValues<std::string>("kNoMinNoMaxNoArgs")};
      std::vector<std::string> kNoMinYesMaxNoArgs_vals{
          argument_map.GetAllValues<std::string>("kNoMinYesMaxNoArgs")};
      std::vector<int> kNoMinYesMaxManyArgs_vals{
          argument_map.GetAllValues<int>("kNoMinYesMaxManyArgs")};
      std::vector<int> kNoMinYesMaxFullArgs_vals{
          argument_map.GetAllValues<int>("kNoMinYesMaxFullArgs")};
      std::vector<TestType> kYesMinNoMaxNoArgs_vals{
          argument_map.GetAllValues<TestType>("kYesMinNoMaxNoArgs")};
      std::vector<TestType> kYesMinNoMaxUnderfullArgs_vals{
          argument_map.GetAllValues<TestType>("kYesMinNoMaxUnderfullArgs")};
      std::vector<long> kYesMinNoMaxMinArgs_vals{
          argument_map.GetAllValues<long>("kYesMinNoMaxMinArgs")};
      std::vector<long> kYesMinYesMaxNoArgs_vals{
          argument_map.GetAllValues<long>("kYesMinYesMaxNoArgs")};
      std::vector<float> kYesMinYesMaxUnderfullArgs_vals{
          argument_map.GetAllValues<float>("kYesMinYesMaxUnderfullArgs")};
      std::vector<float> kYesMinYesMaxMinArgs_vals{
          argument_map.GetAllValues<float>("kYesMinYesMaxMinArgs")};
      std::vector<double> kYesMinYesMaxManyArgs_vals{
          argument_map.GetAllValues<double>("kYesMinYesMaxManyArgs")};
      std::vector<double> kYesMinYesMaxFullArgs_vals{
          argument_map.GetAllValues<double>("kYesMinYesMaxFullArgs")};

      THEN("Values for all arguments are returned.") {
        CHECK(kNoMinNoMaxNoArgs_vals == std::vector<std::string>{});
        CHECK(kNoMinYesMaxNoArgs_vals == std::vector<std::string>{});
        CHECK(kYesMinNoMaxNoArgs_vals == std::vector<TestType>{});
        CHECK(kYesMinYesMaxNoArgs_vals == std::vector<long>{});
        for (int i = 0; i < 2; ++i) {
          CHECK(kYesMinNoMaxUnderfullArgs_vals.at(i)
                == StringToTestType(
                  argument_map.ArgumentsOf("kYesMinNoMaxUnderfullArgs").at(i)));
          CHECK(kYesMinYesMaxUnderfullArgs_vals.at(i)
                == std::stof(argument_map.ArgumentsOf(
                    "kYesMinYesMaxUnderfullArgs").at(i)));
        }
        for (int i = 0; i < 4; ++i) {
          CHECK(kYesMinNoMaxMinArgs_vals.at(i)
                == std::stol(argument_map.ArgumentsOf("kYesMinNoMaxMinArgs")
                    .at(i)));
          CHECK(kYesMinYesMaxMinArgs_vals.at(i)
                == std::stof(argument_map.ArgumentsOf("kYesMinYesMaxMinArgs")
                    .at(i)));
        }
        for (int i = 0; i < 6; ++i) {
          CHECK(kNoMinYesMaxManyArgs_vals.at(i)
                == std::stoi(argument_map.ArgumentsOf("kNoMinYesMaxManyArgs")
                    .at(i)));
          CHECK(kYesMinYesMaxManyArgs_vals.at(i)
                == std::stod(argument_map.ArgumentsOf("kYesMinYesMaxManyArgs")
                    .at(i)));
        }
        for (int i = 0; i < 8; ++i) {
          CHECK(kNoMinYesMaxFullArgs_vals.at(i)
                == std::stoi(argument_map.ArgumentsOf("kNoMinYesMaxFullArgs")
                    .at(i)));
          CHECK(kYesMinYesMaxFullArgs_vals.at(i)
                == std::stod(argument_map.ArgumentsOf("kYesMinYesMaxFullArgs")
                    .at(i)));
        }
      }
    }

    WHEN("Default arguments are set for all parameters.") {
      argument_map.SetDefaultArguments();
      std::vector<std::string> kNoMinNoMaxNoArgs_vals{
          argument_map.GetAllValues<std::string>("kNoMinNoMaxNoArgs")};
      std::vector<std::string> kNoMinYesMaxNoArgs_vals{
          argument_map.GetAllValues<std::string>("kNoMinYesMaxNoArgs")};
      std::vector<int> kNoMinYesMaxManyArgs_vals{
          argument_map.GetAllValues<int>("kNoMinYesMaxManyArgs")};
      std::vector<int> kNoMinYesMaxFullArgs_vals{
          argument_map.GetAllValues<int>("kNoMinYesMaxFullArgs")};
      std::vector<TestType> kYesMinNoMaxNoArgs_vals{
          argument_map.GetAllValues<TestType>("kYesMinNoMaxNoArgs")};
      std::vector<TestType> kYesMinNoMaxUnderfullArgs_vals{
          argument_map.GetAllValues<TestType>("kYesMinNoMaxUnderfullArgs")};
      std::vector<long> kYesMinNoMaxMinArgs_vals{
          argument_map.GetAllValues<long>("kYesMinNoMaxMinArgs")};
      std::vector<long> kYesMinYesMaxNoArgs_vals{
          argument_map.GetAllValues<long>("kYesMinYesMaxNoArgs")};
      std::vector<float> kYesMinYesMaxUnderfullArgs_vals{
          argument_map.GetAllValues<float>("kYesMinYesMaxUnderfullArgs")};
      std::vector<float> kYesMinYesMaxMinArgs_vals{
          argument_map.GetAllValues<float>("kYesMinYesMaxMinArgs")};
      std::vector<double> kYesMinYesMaxManyArgs_vals{
          argument_map.GetAllValues<double>("kYesMinYesMaxManyArgs")};
      std::vector<double> kYesMinYesMaxFullArgs_vals{
          argument_map.GetAllValues<double>("kYesMinYesMaxFullArgs")};

      THEN("Values for all arguments are returned.") {
        for (int i = 0; i < static_cast<int>(kDefaultArgs.size()); ++i) {
          CHECK(kNoMinNoMaxNoArgs_vals.at(i)
                == converters::StringIdentity(
                  argument_map.ArgumentsOf("kNoMinNoMaxNoArgs").at(i)));
          CHECK(kNoMinYesMaxNoArgs_vals.at(i)
                == converters::StringIdentity(
                  argument_map.ArgumentsOf("kNoMinYesMaxNoArgs").at(i)));
          CHECK(kNoMinYesMaxManyArgs_vals.at(i)
                == std::stoi(argument_map.ArgumentsOf("kNoMinYesMaxManyArgs")
                    .at(i)));
          CHECK(kNoMinYesMaxFullArgs_vals.at(i)
                == std::stoi(argument_map.ArgumentsOf("kNoMinYesMaxFullArgs")
                    .at(i)));
          CHECK(kYesMinNoMaxNoArgs_vals.at(i)
                == StringToTestType(
                  argument_map.ArgumentsOf("kYesMinNoMaxNoArgs").at(i)));
          CHECK(kYesMinNoMaxUnderfullArgs_vals.at(i)
                == StringToTestType(
                  argument_map.ArgumentsOf("kYesMinNoMaxUnderfullArgs").at(i)));
          CHECK(kYesMinNoMaxMinArgs_vals.at(i)
                == std::stol(argument_map.ArgumentsOf("kYesMinNoMaxMinArgs")
                    .at(i)));
          CHECK(kYesMinYesMaxNoArgs_vals.at(i)
                == std::stol(argument_map.ArgumentsOf("kYesMinYesMaxNoArgs")
                    .at(i)));
          CHECK(kYesMinYesMaxUnderfullArgs_vals.at(i)
                == std::stof(argument_map.ArgumentsOf(
                    "kYesMinYesMaxUnderfullArgs").at(i)));
          CHECK(kYesMinYesMaxMinArgs_vals.at(i)
                == std::stof(argument_map.ArgumentsOf("kYesMinYesMaxMinArgs")
                    .at(i)));
          CHECK(kYesMinYesMaxManyArgs_vals.at(i)
                == std::stod(argument_map.ArgumentsOf("kYesMinYesMaxManyArgs")
                    .at(i)));
          CHECK(kYesMinYesMaxFullArgs_vals.at(i)
                == std::stod(argument_map.ArgumentsOf("kYesMinYesMaxFullArgs")
                    .at(i)));
        }
      }
    }
  }
}

SCENARIO("Test exceptions thrown by ArgumentMap::GetAllValues.",
         "[ArgumentMap][GetAllValues][exceptions]") {

  GIVEN("An ArgumentMap containing various parameters.") {
    ParameterMap parameter_map;
    parameter_map(kNoMinNoMaxNoArgs.SetDefault(kDefaultArgs))
                 (kNoMinYesMaxNoArgs.SetDefault(kDefaultArgs))
                 (kNoMinYesMaxManyArgs.SetDefault(kDefaultArgs))
                 (kNoMinYesMaxFullArgs.SetDefault(kDefaultArgs))
                 (kYesMinNoMaxNoArgs.SetDefault(kDefaultArgs))
                 (kYesMinNoMaxUnderfullArgs.SetDefault(kDefaultArgs))
                 (kYesMinNoMaxMinArgs.SetDefault(kDefaultArgs))
                 (kYesMinYesMaxNoArgs.SetDefault(kDefaultArgs))
                 (kYesMinYesMaxUnderfullArgs.SetDefault(kDefaultArgs))
                 (kYesMinYesMaxMinArgs.SetDefault(kDefaultArgs))
                 (kYesMinYesMaxManyArgs.SetDefault(kDefaultArgs))
                 (kYesMinYesMaxFullArgs.SetDefault(kDefaultArgs))
                 (kSetFlag.SetDefault(kDefaultArgs))
                 (kUnsetFlag.SetDefault(kDefaultArgs))
                 (kMultipleSetFlag.SetDefault(kDefaultArgs))
                 (kNoConverterPositional.SetDefault(kDefaultArgs))
                 (kNoConverterKeyword.SetDefault(kDefaultArgs));
    ArgumentMap argument_map(std::move(parameter_map));

    WHEN("Argument lists are empty.") {
      
      THEN("Getting arguments of unknown parameters causes exception.") {
        CHECK_THROWS_AS(argument_map.GetAllValues<std::string>("unknown_name"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.GetAllValues<int>("foo"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.GetAllValues<TestType>("b"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.GetAllValues<long>(""),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.GetAllValues<float>("bazinga"),
                        exceptions::ParameterAccessError);
      }

      THEN("Mismatching types causes exception.") {
        CHECK_THROWS_AS(argument_map.GetAllValues<double>("kNoMinNoMaxNoArgs"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.GetAllValues<double>("kNoMinYesMaxNoArgs"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.GetAllValues<std::string>(
                            "kNoMinYesMaxManyArgs"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.GetAllValues<std::string>(
                            "kNoMinYesMaxFullArgs"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.GetAllValues<int>("kYesMinNoMaxNoArgs"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.GetAllValues<int>(
                            "kYesMinNoMaxUnderfullArgs"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.GetAllValues<TestType>(
                            "kYesMinNoMaxMinArgs"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.GetAllValues<TestType>(
                            "kYesMinYesMaxNoArgs"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.GetAllValues<long>(
                            "kYesMinYesMaxUnderfullArgs"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.GetAllValues<long>(
                            "kYesMinYesMaxMinArgs"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.GetAllValues<float>(
                            "kYesMinYesMaxManyArgs"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.GetAllValues<float>(
                            "kYesMinYesMaxFullArgs"),
                        exceptions::ParameterAccessError);
      }

      THEN("Parameters without conversion functions cause exception.") {
        CHECK_THROWS_AS(argument_map.GetAllValues<TestType>(
                            "kNoConverterPositional"),
                        exceptions::ValueAccessError);
        CHECK_THROWS_AS(argument_map.GetAllValues<TestType>(
                            "kNoConverterKeyword"),
                        exceptions::ValueAccessError);
      }

      THEN("Flags cause exception.") {
        CHECK_THROWS_AS(argument_map.GetAllValues<bool>("kSetFlag"),
                        exceptions::ValueAccessError);
        CHECK_THROWS_AS(argument_map.GetAllValues<bool>("kUnsetFlag"),
                        exceptions::ValueAccessError);
        CHECK_THROWS_AS(argument_map.GetAllValues<bool>("kMultipleSetFlag"),
                        exceptions::ValueAccessError);
      }
    }

    WHEN("Argument lists are partially filled.") {
      for (int i = 0; i < 2; ++i) {
        argument_map.AddArgument("kYesMinNoMaxUnderfullArgs", kArgs.at(i));
        argument_map.AddArgument("kYesMinYesMaxUnderfullArgs", kArgs.at(i));
      }
      for (int i = 0; i < 4; ++i) {
        argument_map.AddArgument("kYesMinNoMaxMinArgs", kArgs.at(i));
        argument_map.AddArgument("kYesMinYesMaxMinArgs", kArgs.at(i));
      }
      for (int i = 0; i < 6; ++i) {
        argument_map.AddArgument("kNoMinYesMaxManyArgs", kArgs.at(i));
        argument_map.AddArgument("kYesMinYesMaxManyArgs", kArgs.at(i));
      }
      for (int i = 0; i < 8; ++i) {
        argument_map.AddArgument("kNoMinYesMaxFullArgs", kArgs.at(i));
        argument_map.AddArgument("kYesMinYesMaxFullArgs", kArgs.at(i));
      }
      argument_map.AddArgument("kSetFlag", kArgs.at(0));
      for (const std::string& arg : kArgs) {
        argument_map.AddArgument("kMultipleSetFlag", arg);
        argument_map.AddArgument("kNoConverterPositional", arg);
        argument_map.AddArgument("kNoConverterKeyword", arg);
      }
      
      THEN("Getting arguments of unknown parameters causes exception.") {
        CHECK_THROWS_AS(argument_map.GetAllValues<std::string>("unknown_name"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.GetAllValues<int>("foo"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.GetAllValues<TestType>("b"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.GetAllValues<long>(""),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.GetAllValues<float>("bazinga"),
                        exceptions::ParameterAccessError);
      }

      THEN("Mismatching types causes exception.") {
        CHECK_THROWS_AS(argument_map.GetAllValues<double>("kNoMinNoMaxNoArgs"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.GetAllValues<double>("kNoMinYesMaxNoArgs"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.GetAllValues<std::string>(
                            "kNoMinYesMaxManyArgs"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.GetAllValues<std::string>(
                            "kNoMinYesMaxFullArgs"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.GetAllValues<int>("kYesMinNoMaxNoArgs"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.GetAllValues<int>(
                            "kYesMinNoMaxUnderfullArgs"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.GetAllValues<TestType>(
                            "kYesMinNoMaxMinArgs"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.GetAllValues<TestType>(
                            "kYesMinYesMaxNoArgs"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.GetAllValues<long>(
                            "kYesMinYesMaxUnderfullArgs"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.GetAllValues<long>(
                            "kYesMinYesMaxMinArgs"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.GetAllValues<float>(
                            "kYesMinYesMaxManyArgs"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.GetAllValues<float>(
                            "kYesMinYesMaxFullArgs"),
                        exceptions::ParameterAccessError);
      }

      THEN("Parameters without conversion functions cause exception.") {
        CHECK_THROWS_AS(argument_map.GetAllValues<TestType>(
                            "kNoConverterPositional"),
                        exceptions::ValueAccessError);
        CHECK_THROWS_AS(argument_map.GetAllValues<TestType>(
                            "kNoConverterKeyword"),
                        exceptions::ValueAccessError);
      }

      THEN("Flags cause exception.") {
        CHECK_THROWS_AS(argument_map.GetAllValues<bool>("kSetFlag"),
                        exceptions::ValueAccessError);
        CHECK_THROWS_AS(argument_map.GetAllValues<bool>("kUnsetFlag"),
                        exceptions::ValueAccessError);
        CHECK_THROWS_AS(argument_map.GetAllValues<bool>("kMultipleSetFlag"),
                        exceptions::ValueAccessError);
      }
    }

    WHEN("Default arguments are set for all parameters.") {
      argument_map.SetDefaultArguments();

      THEN("Getting arguments of unknown parameters causes exception.") {
        CHECK_THROWS_AS(argument_map.GetAllValues<std::string>("unknown_name"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.GetAllValues<int>("foo"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.GetAllValues<TestType>("b"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.GetAllValues<long>(""),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.GetAllValues<float>("bazinga"),
                        exceptions::ParameterAccessError);
      }

      THEN("Mismatching types causes exception.") {
        CHECK_THROWS_AS(argument_map.GetAllValues<double>("kNoMinNoMaxNoArgs"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.GetAllValues<double>("kNoMinYesMaxNoArgs"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.GetAllValues<std::string>(
                            "kNoMinYesMaxManyArgs"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.GetAllValues<std::string>(
                            "kNoMinYesMaxFullArgs"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.GetAllValues<int>("kYesMinNoMaxNoArgs"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.GetAllValues<int>(
                            "kYesMinNoMaxUnderfullArgs"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.GetAllValues<TestType>(
                            "kYesMinNoMaxMinArgs"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.GetAllValues<TestType>(
                            "kYesMinYesMaxNoArgs"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.GetAllValues<long>(
                            "kYesMinYesMaxUnderfullArgs"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.GetAllValues<long>(
                            "kYesMinYesMaxMinArgs"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.GetAllValues<float>(
                            "kYesMinYesMaxManyArgs"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.GetAllValues<float>(
                            "kYesMinYesMaxFullArgs"),
                        exceptions::ParameterAccessError);
      }

      THEN("Parameters without conversion functions cause exception.") {
        CHECK_THROWS_AS(argument_map.GetAllValues<TestType>(
                            "kNoConverterPositional"),
                        exceptions::ValueAccessError);
        CHECK_THROWS_AS(argument_map.GetAllValues<TestType>(
                            "kNoConverterKeyword"),
                        exceptions::ValueAccessError);
      }

      THEN("Flags cause exception if arguments are assigned.") {
        CHECK_THROWS_AS(argument_map.GetAllValues<bool>("kSetFlag"),
                        exceptions::ValueAccessError);
        CHECK_THROWS_AS(argument_map.GetAllValues<bool>("kUnsetFlag"),
                        exceptions::ValueAccessError);
        CHECK_THROWS_AS(argument_map.GetAllValues<bool>("kMultipleSetFlag"),
                        exceptions::ValueAccessError);
      }
    }
  }
}

SCENARIO("Test correctness of ArgumentMap::IsSet.",
         "[ArgumentMap][IsSet][correctness]") {

  GIVEN("An ArgumentMap containing various flags.") {
    ParameterMap parameter_map;
    parameter_map(kSetFlag.SetDefault(kDefaultArgs))
                 (kUnsetFlag.SetDefault(kDefaultArgs))
                 (kMultipleSetFlag.SetDefault(kDefaultArgs));
    ArgumentMap argument_map(std::move(parameter_map));

    WHEN("Argument lists are empty.") {

      THEN("Flags are not set.") {
        CHECK_FALSE(argument_map.IsSet("kSetFlag"));
        CHECK_FALSE(argument_map.IsSet("kUnsetFlag"));
        CHECK_FALSE(argument_map.IsSet("kMultipleSetFlag"));
      }
    }

    WHEN("Argument lists are partially filled.") {
      argument_map.AddArgument("kSetFlag", kArgs.at(0));
      for (const std::string& arg : kArgs) {
        argument_map.AddArgument("kMultipleSetFlag", arg);
      }

      THEN("Non-empty argument lists mean that flag is set.") {
        CHECK(argument_map.IsSet("kSetFlag"));
        CHECK_FALSE(argument_map.IsSet("kUnsetFlag"));
        CHECK(argument_map.IsSet("kMultipleSetFlag"));
      }
    }

    WHEN("Default arguments are set for all parameters.") {
      argument_map.SetDefaultArguments();

      THEN("Default arguments are never set for flags, so flags are not set.") {
        CHECK_FALSE(argument_map.IsSet("kSetFlag"));
        CHECK_FALSE(argument_map.IsSet("kUnsetFlag"));
        CHECK_FALSE(argument_map.IsSet("kMultipleSetFlag"));
      }
    }
  }
}

SCENARIO("Test exceptions thrown by ArgumentMap::IsSet.",
         "[ArgumentMap][IsSet][exceptions]") {

  GIVEN("An ArgumentMap containing various parameters.") {
    ParameterMap parameter_map;
    parameter_map(kNoMinNoMaxNoArgs.SetDefault(kDefaultArgs))
                 (kNoMinYesMaxNoArgs.SetDefault(kDefaultArgs))
                 (kNoMinYesMaxManyArgs.SetDefault(kDefaultArgs))
                 (kNoMinYesMaxFullArgs.SetDefault(kDefaultArgs))
                 (kYesMinNoMaxNoArgs.SetDefault(kDefaultArgs))
                 (kYesMinNoMaxUnderfullArgs.SetDefault(kDefaultArgs))
                 (kYesMinNoMaxMinArgs.SetDefault(kDefaultArgs))
                 (kYesMinYesMaxNoArgs.SetDefault(kDefaultArgs))
                 (kYesMinYesMaxUnderfullArgs.SetDefault(kDefaultArgs))
                 (kYesMinYesMaxMinArgs.SetDefault(kDefaultArgs))
                 (kYesMinYesMaxManyArgs.SetDefault(kDefaultArgs))
                 (kYesMinYesMaxFullArgs.SetDefault(kDefaultArgs))
                 (kSetFlag.SetDefault(kDefaultArgs))
                 (kUnsetFlag.SetDefault(kDefaultArgs))
                 (kMultipleSetFlag.SetDefault(kDefaultArgs))
                 (kNoConverterPositional.SetDefault(kDefaultArgs))
                 (kNoConverterKeyword.SetDefault(kDefaultArgs));
    ArgumentMap argument_map(std::move(parameter_map));

    WHEN("Argument lists are empty.") {
      
      THEN("Getting arguments of unknown parameters causes exception.") {
        CHECK_THROWS_AS(argument_map.IsSet("unknown_name"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.IsSet("foo"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.IsSet("b"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.IsSet(""),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.IsSet("bazinga"),
                        exceptions::ParameterAccessError);
      }

      THEN("Mismatching types causes exception.") {
        CHECK_THROWS_AS(argument_map.IsSet("kNoMinNoMaxNoArgs"),
                        exceptions::ValueAccessError);
        CHECK_THROWS_AS(argument_map.IsSet("kNoMinYesMaxNoArgs"),
                        exceptions::ValueAccessError);
        CHECK_THROWS_AS(argument_map.IsSet("kNoMinYesMaxManyArgs"),
                        exceptions::ValueAccessError);
        CHECK_THROWS_AS(argument_map.IsSet("kNoMinYesMaxFullArgs"),
                        exceptions::ValueAccessError);
        CHECK_THROWS_AS(argument_map.IsSet("kYesMinNoMaxNoArgs"),
                        exceptions::ValueAccessError);
        CHECK_THROWS_AS(argument_map.IsSet("kYesMinNoMaxUnderfullArgs"),
                        exceptions::ValueAccessError);
        CHECK_THROWS_AS(argument_map.IsSet("kYesMinNoMaxMinArgs"),
                        exceptions::ValueAccessError);
        CHECK_THROWS_AS(argument_map.IsSet("kYesMinYesMaxNoArgs"),
                        exceptions::ValueAccessError);
        CHECK_THROWS_AS(argument_map.IsSet("kYesMinYesMaxUnderfullArgs"),
                        exceptions::ValueAccessError);
        CHECK_THROWS_AS(argument_map.IsSet("kYesMinYesMaxMinArgs"),
                        exceptions::ValueAccessError);
        CHECK_THROWS_AS(argument_map.IsSet("kYesMinYesMaxManyArgs"),
                        exceptions::ValueAccessError);
        CHECK_THROWS_AS(argument_map.IsSet("kYesMinYesMaxFullArgs"),
                        exceptions::ValueAccessError);
        CHECK_THROWS_AS(argument_map.IsSet("kNoConverterPositional"),
                        exceptions::ValueAccessError);
        CHECK_THROWS_AS(argument_map.IsSet("kNoConverterKeyword"),
                        exceptions::ValueAccessError);
      }
    }

    WHEN("Argument lists are partially filled.") {
      for (int i = 0; i < 2; ++i) {
        argument_map.AddArgument("kYesMinNoMaxUnderfullArgs", kArgs.at(i));
        argument_map.AddArgument("kYesMinYesMaxUnderfullArgs", kArgs.at(i));
      }
      for (int i = 0; i < 4; ++i) {
        argument_map.AddArgument("kYesMinNoMaxMinArgs", kArgs.at(i));
        argument_map.AddArgument("kYesMinYesMaxMinArgs", kArgs.at(i));
      }
      for (int i = 0; i < 6; ++i) {
        argument_map.AddArgument("kNoMinYesMaxManyArgs", kArgs.at(i));
        argument_map.AddArgument("kYesMinYesMaxManyArgs", kArgs.at(i));
      }
      for (int i = 0; i < 8; ++i) {
        argument_map.AddArgument("kNoMinYesMaxFullArgs", kArgs.at(i));
        argument_map.AddArgument("kYesMinYesMaxFullArgs", kArgs.at(i));
      }
      argument_map.AddArgument("kSetFlag", kArgs.at(0));
      for (const std::string& arg : kArgs) {
        argument_map.AddArgument("kMultipleSetFlag", arg);
        argument_map.AddArgument("kNoConverterPositional", arg);
        argument_map.AddArgument("kNoConverterKeyword", arg);
      }
      
      THEN("Getting arguments of unknown parameters causes exception.") {
        CHECK_THROWS_AS(argument_map.IsSet("unknown_name"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.IsSet("foo"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.IsSet("b"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.IsSet(""),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.IsSet("bazinga"),
                        exceptions::ParameterAccessError);
      }

      THEN("Mismatching types causes exception.") {
        CHECK_THROWS_AS(argument_map.IsSet("kNoMinNoMaxNoArgs"),
                        exceptions::ValueAccessError);
        CHECK_THROWS_AS(argument_map.IsSet("kNoMinYesMaxNoArgs"),
                        exceptions::ValueAccessError);
        CHECK_THROWS_AS(argument_map.IsSet("kNoMinYesMaxManyArgs"),
                        exceptions::ValueAccessError);
        CHECK_THROWS_AS(argument_map.IsSet("kNoMinYesMaxFullArgs"),
                        exceptions::ValueAccessError);
        CHECK_THROWS_AS(argument_map.IsSet("kYesMinNoMaxNoArgs"),
                        exceptions::ValueAccessError);
        CHECK_THROWS_AS(argument_map.IsSet("kYesMinNoMaxUnderfullArgs"),
                        exceptions::ValueAccessError);
        CHECK_THROWS_AS(argument_map.IsSet("kYesMinNoMaxMinArgs"),
                        exceptions::ValueAccessError);
        CHECK_THROWS_AS(argument_map.IsSet("kYesMinYesMaxNoArgs"),
                        exceptions::ValueAccessError);
        CHECK_THROWS_AS(argument_map.IsSet("kYesMinYesMaxUnderfullArgs"),
                        exceptions::ValueAccessError);
        CHECK_THROWS_AS(argument_map.IsSet("kYesMinYesMaxMinArgs"),
                        exceptions::ValueAccessError);
        CHECK_THROWS_AS(argument_map.IsSet("kYesMinYesMaxManyArgs"),
                        exceptions::ValueAccessError);
        CHECK_THROWS_AS(argument_map.IsSet("kYesMinYesMaxFullArgs"),
                        exceptions::ValueAccessError);
        CHECK_THROWS_AS(argument_map.IsSet("kNoConverterPositional"),
                        exceptions::ValueAccessError);
        CHECK_THROWS_AS(argument_map.IsSet("kNoConverterKeyword"),
                        exceptions::ValueAccessError);
      }
    }

    WHEN("Default arguments are set for all parameters.") {
      argument_map.SetDefaultArguments();

      THEN("Getting arguments of unknown parameters causes exception.") {
        CHECK_THROWS_AS(argument_map.IsSet("unknown_name"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.IsSet("foo"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.IsSet("b"),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.IsSet(""),
                        exceptions::ParameterAccessError);
        CHECK_THROWS_AS(argument_map.IsSet("bazinga"),
                        exceptions::ParameterAccessError);
      }

      THEN("Mismatching types causes exception.") {
        CHECK_THROWS_AS(argument_map.IsSet("kNoMinNoMaxNoArgs"),
                        exceptions::ValueAccessError);
        CHECK_THROWS_AS(argument_map.IsSet("kNoMinYesMaxNoArgs"),
                        exceptions::ValueAccessError);
        CHECK_THROWS_AS(argument_map.IsSet("kNoMinYesMaxManyArgs"),
                        exceptions::ValueAccessError);
        CHECK_THROWS_AS(argument_map.IsSet("kNoMinYesMaxFullArgs"),
                        exceptions::ValueAccessError);
        CHECK_THROWS_AS(argument_map.IsSet("kYesMinNoMaxNoArgs"),
                        exceptions::ValueAccessError);
        CHECK_THROWS_AS(argument_map.IsSet("kYesMinNoMaxUnderfullArgs"),
                        exceptions::ValueAccessError);
        CHECK_THROWS_AS(argument_map.IsSet("kYesMinNoMaxMinArgs"),
                        exceptions::ValueAccessError);
        CHECK_THROWS_AS(argument_map.IsSet("kYesMinYesMaxNoArgs"),
                        exceptions::ValueAccessError);
        CHECK_THROWS_AS(argument_map.IsSet("kYesMinYesMaxUnderfullArgs"),
                        exceptions::ValueAccessError);
        CHECK_THROWS_AS(argument_map.IsSet("kYesMinYesMaxMinArgs"),
                        exceptions::ValueAccessError);
        CHECK_THROWS_AS(argument_map.IsSet("kYesMinYesMaxManyArgs"),
                        exceptions::ValueAccessError);
        CHECK_THROWS_AS(argument_map.IsSet("kYesMinYesMaxFullArgs"),
                        exceptions::ValueAccessError);
        CHECK_THROWS_AS(argument_map.IsSet("kNoConverterPositional"),
                        exceptions::ValueAccessError);
        CHECK_THROWS_AS(argument_map.IsSet("kNoConverterKeyword"),
                        exceptions::ValueAccessError);
      }
    }
  }
}

} // namespace

} // namespace test

} // namespace arg_parse_convert