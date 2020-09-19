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

#include "parsers.h"

#define CATCH_CONFIG_MAIN
#define CATCH_CONFIG_COLOUR_NONE
#include "catch.h"

#include "string_conversions.h" // include after catch.h

#include <array>
#include <string>
#include <unordered_set>
#include <vector>

// Test correctness for:
// * ParseArgs
// * ParseFile
//
// Test invariants for:
// * ParseArgs
// * ParseFile
//
// Test exceptions for:
// * ParseArgs
// * ParseFile

namespace arg_parse_convert {

namespace test {

namespace {

SCENARIO("Test correctness of ParseArgs.", "[ParseArgs][correctness]") {

  GIVEN("An ArgumentMap with flags.") {
    ParameterMap parameter_map;
    parameter_map(Parameter<bool>::Flag({"flag_a", "a", "long_flag_a"}))
                 (Parameter<bool>::Flag({"flag_b", "b", "long_flag_b"}))
                 (Parameter<bool>::Flag({"flag_c", "c", "long_flag_c"}))
                 (Parameter<bool>::Flag({"flag"}))
                 (Parameter<bool>::Flag({"e", "f"}));
    ArgumentMap argument_map{std::move(parameter_map)};
    std::vector<std::string> return_value;
    std::unordered_set<std::string> additional_arguments;

    WHEN("Parsing no arguments.") {
      const int argc{1};
      const char* argv[argc] = {"command"};
      return_value = ParseArgs(argc, argv, argument_map);
      additional_arguments = std::unordered_set<std::string>{
        return_value.cbegin(), return_value.cend()};
      THEN("Return object is empty.") {
        CHECK(additional_arguments.empty());
      }

      THEN("Flags are assigned no arguments.") {
        CHECK(argument_map.ArgumentsOf("flag_a") == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("flag_b") == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("flag_c") == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("flag") == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("e") == std::vector<std::string>{});
      }
    }

    WHEN("Parsing some long flags.") {
      const int argc{4};
      const char* argv[argc] = {"command", "--flag_a", "--f", "--flag"};
      return_value = ParseArgs(argc, argv, argument_map);
      additional_arguments = std::unordered_set<std::string>{
        return_value.cbegin(), return_value.cend()};
      THEN("Return object is empty.") {
        CHECK(additional_arguments.empty());
      }

      THEN("Parsed flags are assigned an argument.") {
        CHECK(argument_map.ArgumentsOf("flag_a")
              == std::vector<std::string>{"flag_a"});
        CHECK(argument_map.ArgumentsOf("flag_b") == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("flag_c") == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("flag")
              == std::vector<std::string>{"flag"});
        CHECK(argument_map.ArgumentsOf("e")
              == std::vector<std::string>{"f"});
      }
    }

    WHEN("Parsing options lists consisting of flags.") {
      const int argc{3};
      const char* argv[argc] = {"command", "-abf", "-c"};
      return_value = ParseArgs(argc, argv, argument_map);
      additional_arguments = std::unordered_set<std::string>{
        return_value.cbegin(), return_value.cend()};
      THEN("Return object is empty.") {
        CHECK(additional_arguments.empty());
      }

      THEN("Parsed flags are assigned an argument.") {
        CHECK(argument_map.ArgumentsOf("flag_a")
              == std::vector<std::string>{"a"});
        CHECK(argument_map.ArgumentsOf("flag_b")
              == std::vector<std::string>{"b"});
        CHECK(argument_map.ArgumentsOf("flag_c")
              == std::vector<std::string>{"c"});
        CHECK(argument_map.ArgumentsOf("flag") == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("e") == std::vector<std::string>{"f"});
      }
    }

    WHEN("Parsing same flags multiple times.") {
      const int argc{6};
      const char* argv[argc] = {"command", "-abf", "--long_flag_c", "--flag_a",
                                "--flag_c", "-e"};
      return_value = ParseArgs(argc, argv, argument_map);
      additional_arguments = std::unordered_set<std::string>{
        return_value.cbegin(), return_value.cend()};
      THEN("Return object is empty.") {
        CHECK(additional_arguments.empty());
      }

      THEN("Flags are assigned one argument for each time parsed.") {
        CHECK(argument_map.ArgumentsOf("flag_a")
              == std::vector<std::string>{"a", "flag_a"});
        CHECK(argument_map.ArgumentsOf("flag_b")
              == std::vector<std::string>{"b"});
        CHECK(argument_map.ArgumentsOf("flag_c")
              == std::vector<std::string>{"long_flag_c", "flag_c"});
        CHECK(argument_map.ArgumentsOf("flag") == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("e")
              == std::vector<std::string>{"f", "e"});
      }
    }

    WHEN("Parsing flags after separator.") {
      const int argc{8};
      const char* argv[argc] = {"command", "--f", "--flag", "--flag", "--",
                                "-ab", "--flag_c", "--flag"};
      return_value = ParseArgs(argc, argv, argument_map);
      additional_arguments = std::unordered_set<std::string>{
        return_value.cbegin(), return_value.cend()};
      THEN("Arguments after separator are returned as additional arguments.") {
        CHECK(additional_arguments
              == std::unordered_set<std::string>{"-ab", "--flag_c", "--flag"});
      }

      THEN("Only flags preceding separator are assigned argument(s).") {
        CHECK(argument_map.ArgumentsOf("flag_a") == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("flag_b") == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("flag_c") == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("flag")
              == std::vector<std::string>{"flag", "flag"});
        CHECK(argument_map.ArgumentsOf("e") == std::vector<std::string>{"f"});
      }
    }
  }

  GIVEN("An ArgumentMap with keyword parameters and a flag.") {
    ParameterMap parameter_map;
    parameter_map(Parameter<std::string>::Keyword(
                      converters::StringIdentity,
                      {"kwarg_u", "u", "long_kwarg_u"}))
                 (Parameter<std::string>::Keyword(
                      converters::StringIdentity,
                      {"kwarg_v", "v", "long_kwarg_v"}))
                 (Parameter<std::string>::Keyword(
                      converters::StringIdentity,
                      {"kwarg_w", "w", "long_kwarg_w"}))
                 (Parameter<std::string>::Keyword(
                      converters::StringIdentity,
                      {"kwarg"}))
                 (Parameter<std::string>::Keyword(
                      converters::StringIdentity,
                      {"y", "z"}));
    parameter_map(Parameter<bool>::Flag({"flag_a", "a", "long_flag_a"}));
    ArgumentMap argument_map{std::move(parameter_map)};
    std::vector<std::string> return_value;
    std::unordered_set<std::string> additional_arguments;

    WHEN("Parsing no arguments.") {
      const int argc{1};
      const char* argv[argc] = {"command"};
      return_value = ParseArgs(argc, argv, argument_map);
      additional_arguments = std::unordered_set<std::string>{
        return_value.cbegin(), return_value.cend()};
      THEN("Return object is empty.") {
        CHECK(additional_arguments.empty());
      }

      THEN("Parameters are assigned no arguments.") {
        CHECK(argument_map.ArgumentsOf("kwarg_u")
              == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("kwarg_v")
              == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("kwarg_w")
              == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("kwarg") == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("y") == std::vector<std::string>{});

        CHECK(argument_map.ArgumentsOf("flag_a") == std::vector<std::string>{});
      }
    }

    WHEN("Parsing some long keywords without arguments.") {
      const int argc{4};
      const char* argv[argc] = {"command", "--kwarg_v", "--kwarg", "--z"};
      return_value = ParseArgs(argc, argv, argument_map);
      additional_arguments = std::unordered_set<std::string>{
        return_value.cbegin(), return_value.cend()};
      THEN("Return object is empty.") {
        CHECK(additional_arguments.empty());
      }

      THEN("Parameters are assigned no arguments.") {
        CHECK(argument_map.ArgumentsOf("kwarg_u")
              == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("kwarg_v")
              == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("kwarg_w")
              == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("kwarg") == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("y") == std::vector<std::string>{});

        CHECK(argument_map.ArgumentsOf("flag_a") == std::vector<std::string>{});
      }
    }

    WHEN("Parsing some long keywords, some with arguments.") {
      const int argc{8};
      const char* argv[argc] = {"command", "--kwarg_v", "arg1", "arg2",
                                "--kwarg", "--z", "arg3", "arg4"};
      return_value = ParseArgs(argc, argv, argument_map);
      additional_arguments = std::unordered_set<std::string>{
        return_value.cbegin(), return_value.cend()};
      THEN("Return object is empty.") {
        CHECK(additional_arguments.empty());
      }

      THEN("Each argument list is assigned to preceding keyword.") {
        CHECK(argument_map.ArgumentsOf("kwarg_u")
              == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("kwarg_v")
              == std::vector<std::string>{"arg1", "arg2"});
        CHECK(argument_map.ArgumentsOf("kwarg_w")
              == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("kwarg") == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("y")
              == std::vector<std::string>{"arg3", "arg4"});

        CHECK(argument_map.ArgumentsOf("flag_a") == std::vector<std::string>{});
      }
    }

    WHEN("Parsing short keyword names at the ends of options lists.") {
      const int argc{8};
      const char* argv[argc] = {"command", "-az", "-au", "arg1",
                                "arg2", "arg3", "-w", "arg4"};
      return_value = ParseArgs(argc, argv, argument_map);
      additional_arguments = std::unordered_set<std::string>{
        return_value.cbegin(), return_value.cend()};
      THEN("Return object is empty.") {
        CHECK(additional_arguments.empty());
      }

      THEN("Argument list after options list is assigned to correct keyword.") {
        CHECK(argument_map.ArgumentsOf("kwarg_u")
              == std::vector<std::string>{"arg1", "arg2", "arg3"});
        CHECK(argument_map.ArgumentsOf("kwarg_v")
              == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("kwarg_w")
              == std::vector<std::string>{"arg4"});
        CHECK(argument_map.ArgumentsOf("kwarg") == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("y") == std::vector<std::string>{});

        CHECK(argument_map.ArgumentsOf("flag_a")
              == std::vector<std::string>{"a", "a"});
      }
    }

    WHEN("Parsing keywords after separator.") {
      const int argc{15};
      const char* argv[argc] = {"command", "--v", "arg1", "--y", "--kwarg_u",
                                "arg2", "arg3", "--kwarg", "arg4", "--", "-az",
                                "--kwarg", "arg5", "--kwarg_w", "arg6"};
      return_value = ParseArgs(argc, argv, argument_map);
      additional_arguments = std::unordered_set<std::string>{
        return_value.cbegin(), return_value.cend()};
      THEN("Arguments after separator are returned as additional arguments.") {
        CHECK(additional_arguments
              == std::unordered_set<std::string>{"-az", "--kwarg", "arg5",
                                                 "--kwarg_w", "arg6"});
      }

      THEN("Only keywords preceding separator are parsed as such.") {
        CHECK(argument_map.ArgumentsOf("kwarg_u")
              == std::vector<std::string>{"arg2", "arg3"});
        CHECK(argument_map.ArgumentsOf("kwarg_v")
              == std::vector<std::string>{"arg1"});
        CHECK(argument_map.ArgumentsOf("kwarg_w")
              == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("kwarg")
              == std::vector<std::string>{"arg4"});
        CHECK(argument_map.ArgumentsOf("y") == std::vector<std::string>{});

        CHECK(argument_map.ArgumentsOf("flag_a") == std::vector<std::string>{});
      }
    }

    WHEN("Parsing multiple argument lists for same keyword.") {
      const int argc{10};
      const char* argv[argc] = {"command", "--kwarg_v", "arg1", "arg2",
                                "--kwarg", "arg3", "--long_kwarg_v", "arg4",
                                "-av", "arg5"};
      return_value = ParseArgs(argc, argv, argument_map);
      additional_arguments = std::unordered_set<std::string>{
        return_value.cbegin(), return_value.cend()};
      THEN("The argument lists are concatenated.") {
        CHECK(argument_map.ArgumentsOf("kwarg_u")
              == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("kwarg_v")
              == std::vector<std::string>{"arg1", "arg2", "arg4", "arg5"});
        CHECK(argument_map.ArgumentsOf("kwarg_w")
              == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("kwarg")
              == std::vector<std::string>{"arg3"});
        CHECK(argument_map.ArgumentsOf("y") == std::vector<std::string>{});

        CHECK(argument_map.ArgumentsOf("flag_a")
              == std::vector<std::string>{"a"});
      }
    }

    WHEN("Argument lists are interrupted by keywords or flags.") {
      const int argc{11};
      const char* argv[argc] = {"command", "--kwarg_w", "arg1", "-a", "arg2",
                                "--kwarg", "arg3", "arg4", "--long_kwarg_v",
                                "--kwarg_u", "arg5"};
      return_value = ParseArgs(argc, argv, argument_map);
      additional_arguments = std::unordered_set<std::string>{
        return_value.cbegin(), return_value.cend()};
      THEN("Return object contains additional arguments.") {
        CHECK(additional_arguments
              == std::unordered_set<std::string>{"arg2"});
      }

      THEN("Shortened argument lists are assigned to correct keywords.") {
        CHECK(argument_map.ArgumentsOf("kwarg_u")
              == std::vector<std::string>{"arg5"});
        CHECK(argument_map.ArgumentsOf("kwarg_v")
              == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("kwarg_w")
              == std::vector<std::string>{"arg1"});
        CHECK(argument_map.ArgumentsOf("kwarg")
              == std::vector<std::string>{"arg3", "arg4"});
        CHECK(argument_map.ArgumentsOf("y") == std::vector<std::string>{});

        CHECK(argument_map.ArgumentsOf("a")
              == std::vector<std::string>{"a"});
      }
    }
  }

  GIVEN("An ArgumentMap of keyword parameters with argument limits.") {
    ParameterMap parameter_map;
    parameter_map(Parameter<std::string>::Keyword(
                      converters::StringIdentity,
                      {"no_max", "n", "no_max_long"}))
                 (Parameter<std::string>::Keyword(
                      converters::StringIdentity,
                      {"one_max", "o", "one_max_long"})
                      .MaxArgs(1))
                 (Parameter<std::string>::Keyword(
                      converters::StringIdentity,
                      {"two_max", "t", "two_max_long"})
                      .MaxArgs(2))
                 (Parameter<std::string>::Keyword(
                      converters::StringIdentity,
                      {"three_max"})
                      .MaxArgs(3))
                 (Parameter<std::string>::Keyword(
                      converters::StringIdentity,
                      {"f", "O", "u", "r"})
                      .MaxArgs(4));
    parameter_map(Parameter<bool>::Flag({"flag_a", "a", "long_flag_a"}));
    ArgumentMap argument_map{std::move(parameter_map)};
    std::vector<std::string> return_value;
    std::unordered_set<std::string> additional_arguments;

    WHEN("Argument lists are interrupted by keywords and flags.") {
      const int argc{11};
      const char* argv[argc] = {"command", "--two_max", "arg1", "-a", "arg2",
                                "--three_max", "arg3", "arg4", "--one_max_long",
                                "--no_max", "arg5"};
      return_value = ParseArgs(argc, argv, argument_map);
      additional_arguments = std::unordered_set<std::string>{
        return_value.cbegin(), return_value.cend()};
      THEN("Return object contains additional argument.") {
        CHECK(additional_arguments
              == std::unordered_set<std::string>{"arg2"});
      }

      THEN("Shortened argument lists are assigned to correct keywords.") {
        CHECK(argument_map.ArgumentsOf("no_max")
              == std::vector<std::string>{"arg5"});
        CHECK(argument_map.ArgumentsOf("one_max")
              == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("two_max")
              == std::vector<std::string>{"arg1"});
        CHECK(argument_map.ArgumentsOf("three_max")
              == std::vector<std::string>{"arg3", "arg4"});
        CHECK(argument_map.ArgumentsOf("f") == std::vector<std::string>{});

        CHECK(argument_map.ArgumentsOf("a")
              == std::vector<std::string>{"a"});
      }
    }

    WHEN("Argument lists are longer than the keyword parameter maximums.") {
      const int argc{9};
      const char* argv[argc] = {"command", "--one_max", "arg1", "arg2",
                                "--two_max_long", "arg3", "arg4", "arg5",
                                "arg6"};
      return_value = ParseArgs(argc, argv, argument_map);
      additional_arguments = std::unordered_set<std::string>{
        return_value.cbegin(), return_value.cend()};
      THEN("Return object contains excess arguments.") {
        CHECK(additional_arguments
              == std::unordered_set<std::string>{"arg2", "arg5", "arg6"});
      }

      THEN("Argument lists end when maximum argument number is reached.") {
        CHECK(argument_map.ArgumentsOf("no_max")
              == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("one_max")
              == std::vector<std::string>{"arg1"});
        CHECK(argument_map.ArgumentsOf("two_max")
              == std::vector<std::string>{"arg3", "arg4"});
        CHECK(argument_map.ArgumentsOf("three_max")
              == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("f") == std::vector<std::string>{});

        CHECK(argument_map.ArgumentsOf("a") == std::vector<std::string>{});
      }
    }

    WHEN("Argument lists are interrupted by separator.") {
      const int argc{7};
      const char* argv[argc] = {"command", "--no_max", "arg1", "--", "arg2",
                                "-f", "arg3"};
      return_value = ParseArgs(argc, argv, argument_map);
      additional_arguments = std::unordered_set<std::string>{
        return_value.cbegin(), return_value.cend()};
      THEN("Return object contains additional arguments.") {
        CHECK(additional_arguments
              == std::unordered_set<std::string>{"arg2", "-f", "arg3"});
      }

      THEN("Argument lists end when maximum argument number is reached.") {
        CHECK(argument_map.ArgumentsOf("no_max")
              == std::vector<std::string>{"arg1"});
        CHECK(argument_map.ArgumentsOf("one_max")
              == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("two_max")
              == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("three_max")
              == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("f") == std::vector<std::string>{});

        CHECK(argument_map.ArgumentsOf("a") == std::vector<std::string>{});
      }
    }
  }

  GIVEN("An ArgumentMap with positional and keyword parameters and a flag.") {
    ParameterMap parameter_map;
    parameter_map(Parameter<std::string>::Positional(
                      converters::StringIdentity, "pos1", -100))
                 (Parameter<std::string>::Positional(
                      converters::StringIdentity, "pos2", -1))
                 (Parameter<std::string>::Positional(
                      converters::StringIdentity, "pos3", 0))
                 (Parameter<std::string>::Positional(
                      converters::StringIdentity, "pos4", 1))
                 (Parameter<std::string>::Positional(
                      converters::StringIdentity, "pos5", 100));
    parameter_map(Parameter<std::string>::Keyword(
                      converters::StringIdentity,
                      {"two_max", "t", "two_max_long"})
                      .MaxArgs(2))
                 (Parameter<bool>::Flag({"flag_a", "a", "long_flag_a"}));
    ArgumentMap argument_map{std::move(parameter_map)};
    std::vector<std::string> return_value;
    std::unordered_set<std::string> additional_arguments;

    WHEN("Parsing no arguments.") {
      const int argc{1};
      const char* argv[argc] = {"command"};
      return_value = ParseArgs(argc, argv, argument_map);
      additional_arguments = std::unordered_set<std::string>{
        return_value.cbegin(), return_value.cend()};
      THEN("Return object is empty.") {
        CHECK(additional_arguments.empty());
      }

      THEN("Parameters are assigned no arguments.") {
        CHECK(argument_map.ArgumentsOf("pos1") == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("pos2") == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("pos3") == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("pos4") == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("pos5") == std::vector<std::string>{});

        CHECK(argument_map.ArgumentsOf("two_max")
              == std::vector<std::string>{});

        CHECK(argument_map.ArgumentsOf("flag_a") == std::vector<std::string>{});
      }
    }

    WHEN("Parsing some arguments not matching keywords or flag names.") {
      const int argc{5};
      const char* argv[argc] = {"command", "arg1", "pos5", "arg2", "x"};
      return_value = ParseArgs(argc, argv, argument_map);
      additional_arguments = std::unordered_set<std::string>{
        return_value.cbegin(), return_value.cend()};
      THEN("Return object is empty.") {
        CHECK(additional_arguments.empty());
      }

      THEN("Arguments are assigned to first positional parameter.") {
        CHECK(argument_map.ArgumentsOf("pos1")
              == std::vector<std::string>{"arg1", "pos5", "arg2", "x"});
        CHECK(argument_map.ArgumentsOf("pos2") == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("pos3") == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("pos4") == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("pos5") == std::vector<std::string>{});

        CHECK(argument_map.ArgumentsOf("two_max")
              == std::vector<std::string>{});

        CHECK(argument_map.ArgumentsOf("flag_a") == std::vector<std::string>{});
      }
    }

    WHEN("Parsing arguments, some matching keywords or flag names.") {
      const int argc{9};
      const char* argv[argc] = {"command", "arg1", "-a", "arg2", "arg3",
                                "--two_max", "arg4", "arg5", "arg6"};
      return_value = ParseArgs(argc, argv, argument_map);
      additional_arguments = std::unordered_set<std::string>{
        return_value.cbegin(), return_value.cend()};
      THEN("Return object is empty.") {
        CHECK(additional_arguments.empty());
      }

      THEN("Argument lists are assigned to parameters without concatenating.") {
        CHECK(argument_map.ArgumentsOf("pos1")
              == std::vector<std::string>{"arg1"});
        CHECK(argument_map.ArgumentsOf("pos2")
              == std::vector<std::string>{"arg2", "arg3"});
        CHECK(argument_map.ArgumentsOf("pos3")
              == std::vector<std::string>{"arg6"});
        CHECK(argument_map.ArgumentsOf("pos4") == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("pos5") == std::vector<std::string>{});

        CHECK(argument_map.ArgumentsOf("two_max")
              == std::vector<std::string>{"arg4", "arg5"});

        CHECK(argument_map.ArgumentsOf("flag_a")
              == std::vector<std::string>{"a"});
      }
    }

    WHEN("Parsing keyword or flag names after separator.") {
      const int argc{10};
      const char* argv[argc] = {"command", "arg1", "--two_max", "--", "arg2",
                                "-a", "--two_max_long", "arg3", "arg4", "arg5"};
      return_value = ParseArgs(argc, argv, argument_map);
      additional_arguments = std::unordered_set<std::string>{
        return_value.cbegin(), return_value.cend()};
      THEN("Return object is empty.") {
        CHECK(additional_arguments.empty());
      }

      THEN("Argument list after separator is considered uninterrupted.") {
        CHECK(argument_map.ArgumentsOf("pos1")
              == std::vector<std::string>{"arg1"});
        CHECK(argument_map.ArgumentsOf("pos2")
              == std::vector<std::string>{"arg2", "-a", "--two_max_long",
                                          "arg3", "arg4", "arg5"});
        CHECK(argument_map.ArgumentsOf("pos3") == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("pos4") == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("pos5") == std::vector<std::string>{});

        CHECK(argument_map.ArgumentsOf("two_max")
              == std::vector<std::string>{});

        CHECK(argument_map.ArgumentsOf("flag_a")
              == std::vector<std::string>{});
      }
    }

    WHEN("Positional parameter argument list is interrupted by separator.") {
      const int argc{8};
      const char* argv[argc] = {"command", "arg1", "arg2", "--", "arg3", "arg4",
                                "arg5", "arg6"};
      return_value = ParseArgs(argc, argv, argument_map);
      additional_arguments = std::unordered_set<std::string>{
        return_value.cbegin(), return_value.cend()};
      THEN("Return object is empty.") {
        CHECK(additional_arguments.empty());
      }

      THEN("Argument list is considered uninterrupted.") {
        CHECK(argument_map.ArgumentsOf("pos1")
              == std::vector<std::string>{"arg1", "arg2", "arg3", "arg4",
                                          "arg5", "arg6"});
        CHECK(argument_map.ArgumentsOf("pos2") == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("pos3") == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("pos4") == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("pos5") == std::vector<std::string>{});

        CHECK(argument_map.ArgumentsOf("two_max")
              == std::vector<std::string>{});

        CHECK(argument_map.ArgumentsOf("flag_a")
              == std::vector<std::string>{});
      }
    }

    WHEN("Keyword parameter argument list is interrupted by separator.") {
      const int argc{9};
      const char* argv[argc] = {"command", "arg1", "arg2", "-t", "arg3", "--",
                                "arg4", "arg5", "arg6"};
      return_value = ParseArgs(argc, argv, argument_map);
      additional_arguments = std::unordered_set<std::string>{
        return_value.cbegin(), return_value.cend()};
      THEN("Return object is empty.") {
        CHECK(additional_arguments.empty());
      }

      THEN("Keyword's argument list is considered interrupted.") {
        CHECK(argument_map.ArgumentsOf("pos1")
              == std::vector<std::string>{"arg1", "arg2"});
        CHECK(argument_map.ArgumentsOf("pos2")
              == std::vector<std::string>{"arg4", "arg5", "arg6"});
        CHECK(argument_map.ArgumentsOf("pos3") == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("pos4") == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("pos5") == std::vector<std::string>{});

        CHECK(argument_map.ArgumentsOf("two_max")
              == std::vector<std::string>{"arg3"});

        CHECK(argument_map.ArgumentsOf("flag_a")
              == std::vector<std::string>{});
      }
    }
  }

  GIVEN("An ArgumentMap with positional parameters with argument limits.") {
    ParameterMap parameter_map;
    parameter_map(Parameter<std::string>::Positional(
                      converters::StringIdentity, "three_max", -100)
                      .MaxArgs(3))
                 (Parameter<std::string>::Positional(
                      converters::StringIdentity, "two_max", -1)
                      .MaxArgs(2))
                 (Parameter<std::string>::Positional(
                      converters::StringIdentity, "no_max", 0))
                 (Parameter<std::string>::Positional(
                      converters::StringIdentity, "four_max", 1)
                      .MaxArgs(4))
                 (Parameter<std::string>::Positional(
                      converters::StringIdentity, "one_max", 100)
                      .MaxArgs(1));
    parameter_map(Parameter<std::string>::Keyword(
                      converters::StringIdentity,
                      {"kwarg_two_max", "t", "two_max_long"})
                      .MaxArgs(2))
                 (Parameter<bool>::Flag({"flag_a", "a", "long_flag_a"}));
    ArgumentMap argument_map{std::move(parameter_map)};
    std::vector<std::string> return_value;
    std::unordered_set<std::string> additional_arguments;

    WHEN("Parsing no arguments.") {
      const int argc{1};
      const char* argv[argc] = {"command"};
      return_value = ParseArgs(argc, argv, argument_map);
      additional_arguments = std::unordered_set<std::string>{
        return_value.cbegin(), return_value.cend()};
      THEN("Return object is empty.") {
        CHECK(additional_arguments.empty());
      }

      THEN("Parameters are assigned no arguments.") {
        CHECK(argument_map.ArgumentsOf("three_max")
              == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("two_max")
              == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("no_max") == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("four_max")
              == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("one_max")
              == std::vector<std::string>{});

        CHECK(argument_map.ArgumentsOf("kwarg_two_max")
              == std::vector<std::string>{});

        CHECK(argument_map.ArgumentsOf("flag_a") == std::vector<std::string>{});
      }
    }

    WHEN("Parsing some arguments not matching keywords or flag names.") {
      const int argc{12};
      const char* argv[argc] = {"command", "arg1", "pos5", "arg2", "x", "arg3",
                                "arg4", "arg5", "arg6", "arg7", "arg8", "arg9"};
      return_value = ParseArgs(argc, argv, argument_map);
      additional_arguments = std::unordered_set<std::string>{
        return_value.cbegin(), return_value.cend()};
      THEN("Return object is empty.") {
        CHECK(additional_arguments.empty());
      }

      THEN("Positional parameter argument lists are filled in order.") {
        CHECK(argument_map.ArgumentsOf("three_max")
              == std::vector<std::string>{"arg1", "pos5", "arg2"});
        CHECK(argument_map.ArgumentsOf("two_max")
              == std::vector<std::string>{"x", "arg3"});
        CHECK(argument_map.ArgumentsOf("no_max")
              == std::vector<std::string>{"arg4", "arg5", "arg6", "arg7",
                                          "arg8", "arg9"});
        CHECK(argument_map.ArgumentsOf("four_max")
              == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("one_max")
              == std::vector<std::string>{});

        CHECK(argument_map.ArgumentsOf("kwarg_two_max")
              == std::vector<std::string>{});

        CHECK(argument_map.ArgumentsOf("flag_a") == std::vector<std::string>{});
      }
    }

    WHEN("Parsing arguments, some matching keywords or flag names.") {
      const int argc{18};
      const char* argv[argc] = {"command", "arg1", "pos5", "arg2", "x", "arg3",
                                "arg4", "arg5", "-a", "arg6", "arg7", "arg8",
                                "--kwarg_two_max", "arg9", "arg10", "arg11",
                                "arg12", "arg13"};
      return_value = ParseArgs(argc, argv, argument_map);
      additional_arguments = std::unordered_set<std::string>{
        return_value.cbegin(), return_value.cend()};
      THEN("Returns arguments that didn't fit an argument list.") {
        CHECK(additional_arguments
              == std::unordered_set<std::string>{"arg12", "arg13"});
      }

      THEN("Even unlimited parameter lists are interrupted.") {
        CHECK(argument_map.ArgumentsOf("three_max")
              == std::vector<std::string>{"arg1", "pos5", "arg2"});
        CHECK(argument_map.ArgumentsOf("two_max")
              == std::vector<std::string>{"x", "arg3"});
        CHECK(argument_map.ArgumentsOf("no_max")
              == std::vector<std::string>{"arg4", "arg5"});
        CHECK(argument_map.ArgumentsOf("four_max")
              == std::vector<std::string>{"arg6", "arg7", "arg8"});
        CHECK(argument_map.ArgumentsOf("one_max")
              == std::vector<std::string>{"arg11"});

        CHECK(argument_map.ArgumentsOf("kwarg_two_max")
              == std::vector<std::string>{"arg9", "arg10"});

        CHECK(argument_map.ArgumentsOf("flag_a")
              == std::vector<std::string>{"a"});
      }
    }

    WHEN("Parsing keyword argument at the end of options list.") {
      const int argc{6};
      const char* argv[argc] = {"command", "arg1", "-at", "arg2", "arg3",
                                "arg4"};
      return_value = ParseArgs(argc, argv, argument_map);
      additional_arguments = std::unordered_set<std::string>{
        return_value.cbegin(), return_value.cend()};
      THEN("Return object is empty.") {
        CHECK(additional_arguments.empty());
      }

      THEN("Keyword parameter list takes precedence.") {
        CHECK(argument_map.ArgumentsOf("three_max")
              == std::vector<std::string>{"arg1"});
        CHECK(argument_map.ArgumentsOf("two_max")
              == std::vector<std::string>{"arg4"});
        CHECK(argument_map.ArgumentsOf("no_max") == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("four_max")
              == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("one_max")
              == std::vector<std::string>{});

        CHECK(argument_map.ArgumentsOf("kwarg_two_max")
              == std::vector<std::string>{"arg2", "arg3"});

        CHECK(argument_map.ArgumentsOf("flag_a")
              == std::vector<std::string>{"a"});
      }
    }

    WHEN("Arguments after separator are considered positional arguments.") {
      const int argc{15};
      const char* argv[argc] = {"command", "arg1", "--kwarg_two_max", "--",
                                "arg2", "-a", "--two_max_long", "arg3", "arg4",
                                "arg5", "arg6", "arg7", "arg8", "arg9",
                                "arg10"};
      return_value = ParseArgs(argc, argv, argument_map);
      additional_arguments = std::unordered_set<std::string>{
        return_value.cbegin(), return_value.cend()};
      THEN("Return object is empty.") {
        CHECK(additional_arguments.empty());
      }

      THEN("Argument list after separator is considered uninterrupted.") {
        CHECK(argument_map.ArgumentsOf("three_max")
              == std::vector<std::string>{"arg1"});
        CHECK(argument_map.ArgumentsOf("two_max")
              == std::vector<std::string>{"arg2", "-a"});
        CHECK(argument_map.ArgumentsOf("no_max")
              == std::vector<std::string>{"--two_max_long", "arg3", "arg4",
                                          "arg5", "arg6", "arg7", "arg8",
                                          "arg9", "arg10"});
        CHECK(argument_map.ArgumentsOf("four_max")
              == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("one_max")
              == std::vector<std::string>{});

        CHECK(argument_map.ArgumentsOf("kwarg_two_max")
              == std::vector<std::string>{});

        CHECK(argument_map.ArgumentsOf("flag_a")
              == std::vector<std::string>{});
      }
    }

    WHEN("Positional parameter argument list is interrupted by separator.") {
      const int argc{8};
      const char* argv[argc] = {"command", "arg1", "arg2", "--", "arg3", "arg4",
                                "arg5", "arg6"};
      return_value = ParseArgs(argc, argv, argument_map);
      additional_arguments = std::unordered_set<std::string>{
        return_value.cbegin(), return_value.cend()};

      THEN("Return object is empty.") {
        CHECK(additional_arguments.empty());
      }

      THEN("Argument list is considered uninterrupted.") {
        CHECK(argument_map.ArgumentsOf("three_max")
              == std::vector<std::string>{"arg1", "arg2", "arg3"});
        CHECK(argument_map.ArgumentsOf("two_max")
              == std::vector<std::string>{"arg4", "arg5"});
        CHECK(argument_map.ArgumentsOf("no_max")
              == std::vector<std::string>{"arg6"});
        CHECK(argument_map.ArgumentsOf("four_max")
              == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("one_max")
              == std::vector<std::string>{});

        CHECK(argument_map.ArgumentsOf("kwarg_two_max")
              == std::vector<std::string>{});

        CHECK(argument_map.ArgumentsOf("flag_a")
              == std::vector<std::string>{});
      }
    }

    WHEN("Keyword parameter argument list is interrupted by separator.") {
      const int argc{9};
      const char* argv[argc] = {"command", "arg1", "arg2", "-t", "arg3", "--",
                                "arg4", "arg5", "arg6"};
      return_value = ParseArgs(argc, argv, argument_map);
      additional_arguments = std::unordered_set<std::string>{
        return_value.cbegin(), return_value.cend()};
      THEN("Return object is empty.") {
        CHECK(additional_arguments.empty());
      }

      THEN("Keyword's argument list is considered interrupted.") {
        CHECK(argument_map.ArgumentsOf("three_max")
              == std::vector<std::string>{"arg1", "arg2"});
        CHECK(argument_map.ArgumentsOf("two_max")
              == std::vector<std::string>{"arg4", "arg5"});
        CHECK(argument_map.ArgumentsOf("no_max")
              == std::vector<std::string>{"arg6"});
        CHECK(argument_map.ArgumentsOf("four_max")
              == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("one_max")
              == std::vector<std::string>{});

        CHECK(argument_map.ArgumentsOf("kwarg_two_max")
              == std::vector<std::string>{"arg3"});

        CHECK(argument_map.ArgumentsOf("flag_a")
              == std::vector<std::string>{});
      }
    }
  }

  GIVEN("An ArgumentMap with various parameters.") {
    ParameterMap parameter_map;
    parameter_map(Parameter<std::string>::Positional(
                      converters::StringIdentity, "pos_three_max", -100)
                      .MaxArgs(3))
                 (Parameter<std::string>::Positional(
                      converters::StringIdentity, "pos_no_max", 0))
                 (Parameter<std::string>::Keyword(
                      converters::StringIdentity,
                      {"kwarg_two_max", "t", "kwarg_two_max_long"})
                      .MaxArgs(2))
                 (Parameter<std::string>::Keyword(
                      converters::StringIdentity,
                      {"kwarg_no_max", "n", "kwarg_no_max_long"}))
                 (Parameter<bool>::Flag({"flag_a", "a", "long_flag_a"}))
                 (Parameter<bool>::Flag({"flag_b", "b", "long_flag_b"}));
    ArgumentMap argument_map{std::move(parameter_map)};
    std::vector<std::string> return_value;
    std::unordered_set<std::string> additional_arguments;

    WHEN("Some parameters already have arguments.") {
      argument_map.AddArgument("pos_three_max", "old_arg1");
      argument_map.AddArgument("kwarg_two_max", "old_arg1");
      argument_map.AddArgument("kwarg_two_max", "old_arg2");
      argument_map.AddArgument("flag_a", "old_set");
      const int argc{13};
      const char* argv[argc] = {"command", "arg1", "arg2", "-t", "arg3",
                                "--kwarg_no_max_long", "arg4", "arg5", "arg6",
                                "-ab", "--", "arg7", "arg8"};
      return_value = ParseArgs(argc, argv, argument_map);
      additional_arguments = std::unordered_set<std::string>{
          return_value.cbegin(), return_value.cend()};

      THEN("Return object contains arguments for already filled parameters.") {
        CHECK(additional_arguments
              == std::unordered_set<std::string>{"arg1", "arg2", "arg3", "a"});
      }

      THEN("New arguments are assigned to empty parameters.") {
        CHECK(argument_map.ArgumentsOf("pos_three_max")
              == std::vector<std::string>{"old_arg1"});
        CHECK(argument_map.ArgumentsOf("pos_no_max")
              == std::vector<std::string>{"arg7", "arg8"});

        CHECK(argument_map.ArgumentsOf("kwarg_two_max")
              == std::vector<std::string>{"old_arg1", "old_arg2"});
        CHECK(argument_map.ArgumentsOf("kwarg_no_max")
              == std::vector<std::string>{"arg4", "arg5", "arg6"});

        CHECK(argument_map.ArgumentsOf("flag_a")
              == std::vector<std::string>{"old_set"});
        CHECK(argument_map.ArgumentsOf("flag_b")
              == std::vector<std::string>{"b"});
      }
    }
  }
}

SCENARIO("Test invariant preservation by ParseArgs.",
         "[ParseArgs][invariants]") {

  GIVEN("An ArgumentMap with various parameters.") {
    int size, parameters_size, arguments_size, values_size;
    ParameterMap parameter_map;
    parameter_map(Parameter<std::string>::Positional(
                      converters::StringIdentity, "pos_three_max", -100)
                      .MaxArgs(3))
                 (Parameter<std::string>::Positional(
                      converters::StringIdentity, "pos_no_max", 0))
                 (Parameter<std::string>::Keyword(
                      converters::StringIdentity,
                      {"kwarg_two_max", "t", "kwarg_two_max_long"})
                      .MaxArgs(2))
                 (Parameter<std::string>::Keyword(
                      converters::StringIdentity,
                      {"kwarg_no_max", "n", "kwarg_no_max_long"}))
                 (Parameter<bool>::Flag({"flag_a", "a", "long_flag_a"}))
                 (Parameter<bool>::Flag({"flag_b", "b", "long_flag_b"}));
    ArgumentMap argument_map{std::move(parameter_map)};
    size = argument_map.size();
    parameters_size = argument_map.Parameters().size();
    arguments_size = argument_map.Arguments().size();
    values_size = argument_map.Values().size();
    std::vector<std::string> return_value;
    std::unordered_set<std::string> additional_arguments;

    WHEN("Parsing without returning additional parameters.") {
      const int argc{13};
      const char* argv[argc] = {"command", "arg1", "arg2", "-t", "arg3",
                                "--kwarg_no_max_long", "arg4", "arg5", "arg6",
                                "-ab", "--", "arg7", "arg8"};
      return_value = ParseArgs(argc, argv, argument_map);
      additional_arguments = std::unordered_set<std::string>{
          return_value.cbegin(), return_value.cend()};
      REQUIRE(additional_arguments.empty());

      THEN("ArgumentMap's size doesn't change.") {
        CHECK(static_cast<int>(argument_map.size()) == size);
      }

      THEN("ArgumentMap's ParameterMap member's size doesn't change.") {
        CHECK(static_cast<int>(argument_map.Parameters().size())
              == parameters_size);
      }

      THEN("ArgumentMap's number of stored argument lists doesn't change.") {
        CHECK(static_cast<int>(argument_map.Arguments().size())
              == arguments_size);
      }

      THEN("ArgumentMap's number of stored value lists doesn't change.") {
        CHECK(static_cast<int>(argument_map.Values().size()) == values_size);
      }
    }

    WHEN("Parsing and returning additional parameters.") {
      const int argc{15};
      const char* argv[argc] = {"command", "arg1", "arg2", "arg3", "arg4", "-t",
                                "arg5", "arg6", "arg7", "-ab", "--kwarg_no_max",
                                "arg8", "--", "arg9", "arg10"};
      return_value = ParseArgs(argc, argv, argument_map);
      additional_arguments = std::unordered_set<std::string>{
          return_value.cbegin(), return_value.cend()};
      REQUIRE_FALSE(additional_arguments.empty());

      THEN("ArgumentMap's size doesn't change.") {
        CHECK(static_cast<int>(argument_map.size()) == size);
      }

      THEN("ArgumentMap's ParameterMap member's size doesn't change.") {
        CHECK(static_cast<int>(argument_map.Parameters().size())
              == parameters_size);
      }

      THEN("ArgumentMap's number of stored argument lists doesn't change.") {
        CHECK(static_cast<int>(argument_map.Arguments().size())
              == arguments_size);
      }

      THEN("ArgumentMap's number of stored value lists doesn't change.") {
        CHECK(static_cast<int>(argument_map.Values().size()) == values_size);
      }
    }
  }
}

SCENARIO("Test exceptions thrown by ParseArgs.",
         "[ParseArgs][exceptions]") {

  GIVEN("An ArgumentMap with various parameters.") {
    ParameterMap parameter_map;
    parameter_map(Parameter<std::string>::Positional(
                      converters::StringIdentity, "x", -100)
                      .MaxArgs(3))
                 (Parameter<std::string>::Positional(
                      converters::StringIdentity, "pos_no_max", 0))
                 (Parameter<std::string>::Keyword(
                      converters::StringIdentity,
                      {"kwarg_two_max", "t", "kwarg_two_max_long"})
                      .MaxArgs(2))
                 (Parameter<std::string>::Keyword(
                      converters::StringIdentity,
                      {"kwarg_no_max", "n", "kwarg_no_max_long"}))
                 (Parameter<bool>::Flag({"flag_a", "a", "long_flag_a"}))
                 (Parameter<bool>::Flag({"flag_b", "b", "long_flag_b"}));
    ArgumentMap argument_map{std::move(parameter_map)};

    THEN("Options list with keyword before last causes exception.") {
      const int argc{2};
      const char* argv[argc] = {"command", "-atb"};
      CHECK_THROWS_AS(ParseArgs(argc, argv, argument_map),
                      exceptions::ArgumentParsingError);
    }

    THEN("Options list with single-letter positional causes exception.") {
      const int argc{2};
      const char* argv[argc] = {"command", "-axb"};
      CHECK_THROWS_AS(ParseArgs(argc, argv, argument_map),
                      exceptions::ArgumentParsingError);
    }

    THEN("Options list with long flag name causes exception.") {
      const int argc{2};
      const char* argv[argc] = {"command", "-ab_flag"};
      CHECK_THROWS_AS(ParseArgs(argc, argv, argument_map),
                      exceptions::ArgumentParsingError);
    }

    THEN("Options list with unknown letter causes exception.") {
      const int argc{2};
      const char* argv[argc] = {"command", "-acb"};
      CHECK_THROWS_AS(ParseArgs(argc, argv, argument_map),
                      exceptions::ArgumentParsingError);
    }

    THEN("Options list with positional at end causes exception.") {
      const int argc{2};
      const char* argv[argc] = {"command", "-abx"};
      CHECK_THROWS_AS(ParseArgs(argc, argv, argument_map),
                      exceptions::ArgumentParsingError);
    }

    THEN("Options list with unknown letter at end causes exception.") {
      const int argc{2};
      const char* argv[argc] = {"command", "-abA"};
      CHECK_THROWS_AS(ParseArgs(argc, argv, argument_map),
                      exceptions::ArgumentParsingError);
    }

    THEN("Options list starting with two hyphens causes exception.") {
      const int argc{2};
      const char* argv[argc] = {"command", "--ab"};
      CHECK_THROWS_AS(ParseArgs(argc, argv, argument_map),
                      exceptions::ArgumentParsingError);
    }

    THEN("Options list starting without hyphens does not cause exception.") {
      const int argc{2};
      const char* argv[argc] = {"command", "ab"};
      CHECK_NOTHROW(ParseArgs(argc, argv, argument_map));
    }

    THEN("Options list starting three hyphens causes exception.") {
      const int argc{2};
      const char* argv[argc] = {"command", "--ab"};
      CHECK_THROWS_AS(ParseArgs(argc, argv, argument_map),
                      exceptions::ArgumentParsingError);
    }

    THEN("Empty options list does not cause exception.") {
      const int argc{2};
      const char* argv[argc] = {"command", "-"};
      CHECK_NOTHROW(ParseArgs(argc, argv, argument_map));
    }

    THEN("Options list with short flags only does not cause exception.") {
      const int argc{2};
      const char* argv[argc] = {"command", "-aba"};
      CHECK_NOTHROW(ParseArgs(argc, argv, argument_map));
    }

    THEN("Options list with single short keyword does not cause exception.") {
      const int argc{2};
      const char* argv[argc] = {"command", "-t"};
      CHECK_NOTHROW(ParseArgs(argc, argv, argument_map));
    }

    THEN("Options list ending in short keyword does not cause exception.") {
      const int argc{2};
      const char* argv[argc] = {"command", "-abbt"};
      CHECK_NOTHROW(ParseArgs(argc, argv, argument_map));
    }

    THEN("Double hyphens followed by positional causes exception.") {
      const int argc{2};
      const char* argv[argc] = {"command", "--pos_no_max"};
      CHECK_THROWS_AS(ParseArgs(argc, argv, argument_map),
                      exceptions::ArgumentParsingError);
    }

    THEN("Double hyphens followed by unknown name causes exception.") {
      const int argc{2};
      const char* argv[argc] = {"command", "--unknown_name"};
      CHECK_THROWS_AS(ParseArgs(argc, argv, argument_map),
                      exceptions::ArgumentParsingError);
    }

    THEN("Double hyphens followed by multiple long flags causes exception.") {
      const int argc{2};
      const char* argv[argc] = {"command", "--flag_aflag_b"};
      CHECK_THROWS_AS(ParseArgs(argc, argv, argument_map),
                      exceptions::ArgumentParsingError);
    }

    THEN("Double hyphens followed by multiple short flags causes exception.") {
      const int argc{2};
      const char* argv[argc] = {"command", "--ab"};
      CHECK_THROWS_AS(ParseArgs(argc, argv, argument_map),
                      exceptions::ArgumentParsingError);
    }

    THEN("Double hyphens followed by multiple long keywords causes exception.") {
      const int argc{2};
      const char* argv[argc] = {"command", "--kwarg_two_maxkwarg_no_max"};
      CHECK_THROWS_AS(ParseArgs(argc, argv, argument_map),
                      exceptions::ArgumentParsingError);
    }

    THEN("Double hyphens followed by multiple short keywords causes exception.") {
      const int argc{2};
      const char* argv[argc] = {"command", "--tn"};
      CHECK_THROWS_AS(ParseArgs(argc, argv, argument_map),
                      exceptions::ArgumentParsingError);
    }

    THEN("Double hyphens with single short flag does not cause exception.") {
      const int argc{2};
      const char* argv[argc] = {"command", "--a"};
      CHECK_NOTHROW(ParseArgs(argc, argv, argument_map));
    }

    THEN("Double hyphens with single long flag does not cause exception.") {
      const int argc{2};
      const char* argv[argc] = {"command", "--flag_b"};
      CHECK_NOTHROW(ParseArgs(argc, argv, argument_map));
    }

    THEN("Double hyphens with single short keyword does not cause exception.") {
      const int argc{2};
      const char* argv[argc] = {"command", "--t"};
      CHECK_NOTHROW(ParseArgs(argc, argv, argument_map));
    }

    THEN("Double hyphens with single long keyword does not cause exception.") {
      const int argc{2};
      const char* argv[argc] = {"command", "--kwarg_no_max_long"};
      CHECK_NOTHROW(ParseArgs(argc, argv, argument_map));
    }
  }
}

SCENARIO("Test correctness of ParseFile.", "[ParseFile][correctness]") {

  GIVEN("An ArgumentMap with flags.") {
    ParameterMap parameter_map;
    parameter_map(Parameter<bool>::Flag({"flag_a", "a", "long_flag_a"}))
                 (Parameter<bool>::Flag({"flag_b", "b", "long_flag_b"}))
                 (Parameter<bool>::Flag({"flag_c", "c", "long_flag_c"}))
                 (Parameter<bool>::Flag({"flag"}))
                 (Parameter<bool>::Flag({"e", "f"}));
    ArgumentMap argument_map{std::move(parameter_map)};
    std::vector<std::string> return_value;
    std::unordered_set<std::string> additional_arguments;

    WHEN("Parsing empty stream.") {
      std::istringstream iss{};
      return_value = ParseFile(iss, argument_map);
      additional_arguments = std::unordered_set<std::string>{
          return_value.cbegin(), return_value.cend()};

      THEN("Return object is empty.") {
        CHECK(additional_arguments.empty());
      }

      THEN("Flags are assigned no arguments.") {
        CHECK(argument_map.ArgumentsOf("flag_a") == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("flag_b") == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("flag_c") == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("flag") == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("e") == std::vector<std::string>{});
      }
    }

    WHEN("Parsing some flags set to true.") {
      std::istringstream iss{"flag_a=TRUE\n"
                             "b=True\n"
                             "long_flag_c=true\n"
                             "e=1"};
      return_value = ParseFile(iss, argument_map);
      additional_arguments = std::unordered_set<std::string>{
          return_value.cbegin(), return_value.cend()};

      THEN("Return object is empty.") {
        CHECK(additional_arguments.empty());
      }

      THEN("Flags set to true are assigned an argument.") {
        CHECK(argument_map.ArgumentsOf("flag_a")
              == std::vector<std::string>{"flag_a"});
        CHECK(argument_map.ArgumentsOf("flag_b")
              == std::vector<std::string>{"b"});
        CHECK(argument_map.ArgumentsOf("flag_c")
              == std::vector<std::string>{"long_flag_c"});
        CHECK(argument_map.ArgumentsOf("flag")
              == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("e")
              == std::vector<std::string>{"e"});
      }
    }

    WHEN("Parsing some flags set to false.") {
      std::istringstream iss{"flag_a=FALSE\n"
                             "b=False\n"
                             "flag=false\n"
                             "f=0"};
      return_value = ParseFile(iss, argument_map);
      additional_arguments = std::unordered_set<std::string>{
          return_value.cbegin(), return_value.cend()};

      THEN("Return object is empty.") {
        CHECK(additional_arguments.empty());
      }

      THEN("No flag is assigned an argument.") {
        CHECK(argument_map.ArgumentsOf("flag_a")
              == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("flag_b")
              == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("flag_c")
              == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("flag")
              == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("e")
              == std::vector<std::string>{});
      }
    }

    WHEN("Parsing some flags set to true and others to false.") {
      std::istringstream iss{"b=true\n"
                             "flag=False\n"
                             "flag_a=FALSE\n"
                             "f=0\n"
                             "flag_c=TRUE"};
      return_value = ParseFile(iss, argument_map);
      additional_arguments = std::unordered_set<std::string>{
          return_value.cbegin(), return_value.cend()};

      THEN("Return object is empty.") {
        CHECK(additional_arguments.empty());
      }

      THEN("Flags set to true are assigned an argument.") {
        CHECK(argument_map.ArgumentsOf("flag_a")
              == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("flag_b")
              == std::vector<std::string>{"b"});
        CHECK(argument_map.ArgumentsOf("flag_c")
              == std::vector<std::string>{"flag_c"});
        CHECK(argument_map.ArgumentsOf("flag")
              == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("e")
              == std::vector<std::string>{});
      }
    }

    WHEN("Parsing same flags multiple times.") {
      std::istringstream iss{"b=true\n"
                             "flag=False\n"
                             "b=false\n"
                             "flag=True\n"
                             "flag_a=TRUE\n"
                             "flag_a=FALSE\n"
                             "flag_a=TRUE\n"
                             "a=TRUE\n"
                             "long_flag_c=0\n"
                             "flag_c=false"};
      return_value = ParseFile(iss, argument_map);
      additional_arguments = std::unordered_set<std::string>{
          return_value.cbegin(), return_value.cend()};

      THEN("Return object is empty.") {
        CHECK(additional_arguments.empty());
      }

      THEN("Flags are assigned one argument for each time assigned true.") {
        CHECK(argument_map.ArgumentsOf("flag_a")
              == std::vector<std::string>{"flag_a", "flag_a", "a"});
        CHECK(argument_map.ArgumentsOf("flag_b")
              == std::vector<std::string>{"b"});
        CHECK(argument_map.ArgumentsOf("flag_c")
              == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("flag")
              == std::vector<std::string>{"flag"});
        CHECK(argument_map.ArgumentsOf("e")
              == std::vector<std::string>{});
      }
    }
  }

  GIVEN("An ArgumentMap with keyword parameters.") {
    ParameterMap parameter_map;
    parameter_map(Parameter<std::string>::Keyword(
                      converters::StringIdentity,
                      {"kwarg_u", "u", "long_kwarg_u"}))
                 (Parameter<std::string>::Keyword(
                      converters::StringIdentity,
                      {"kwarg_v", "v", "long_kwarg_v"}))
                 (Parameter<std::string>::Keyword(
                      converters::StringIdentity,
                      {"kwarg_w", "w", "long_kwarg_w"}))
                 (Parameter<std::string>::Keyword(
                      converters::StringIdentity,
                      {"kwarg"}))
                 (Parameter<std::string>::Keyword(
                      converters::StringIdentity,
                      {"y", "z"}));
    ArgumentMap argument_map{std::move(parameter_map)};
    std::vector<std::string> return_value;
    std::unordered_set<std::string> additional_arguments;

    WHEN("Parsing empty stream.") {
      std::istringstream iss{};
      return_value = ParseFile(iss, argument_map);
      additional_arguments = std::unordered_set<std::string>{
          return_value.cbegin(), return_value.cend()};

      THEN("Return object is empty.") {
        CHECK(additional_arguments.empty());
      }

      THEN("Parameters are assigned no arguments.") {
        CHECK(argument_map.ArgumentsOf("kwarg_u")
              == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("kwarg_v")
              == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("kwarg_w")
              == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("kwarg") == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("y") == std::vector<std::string>{});
      }
    }

    WHEN("Parsing some keywords with arguments.") {
      std::istringstream iss{"kwarg_v=arg1 arg2\n"
                             "z=arg3 arg4"};
      return_value = ParseFile(iss, argument_map);
      additional_arguments = std::unordered_set<std::string>{
          return_value.cbegin(), return_value.cend()};

      THEN("Return object is empty.") {
        CHECK(additional_arguments.empty());
      }

      THEN("Each argument list is assigned to respective keyword.") {
        CHECK(argument_map.ArgumentsOf("kwarg_u")
              == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("kwarg_v")
              == std::vector<std::string>{"arg1", "arg2"});
        CHECK(argument_map.ArgumentsOf("kwarg_w")
              == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("kwarg") == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("y")
              == std::vector<std::string>{"arg3", "arg4"});
      }
    }

    WHEN("Multiple argument lists for same keyword.") {
      std::istringstream iss{"kwarg_v=arg1 arg2\n"
                             "kwarg=arg3\n"
                             "long_kwarg_v=arg4\n"
                             "v=arg5"};
      return_value = ParseFile(iss, argument_map);
      additional_arguments = std::unordered_set<std::string>{
          return_value.cbegin(), return_value.cend()};

      THEN("The argument lists are concatenated.") {
        CHECK(argument_map.ArgumentsOf("kwarg_u")
              == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("kwarg_v")
              == std::vector<std::string>{"arg1", "arg2", "arg4", "arg5"});
        CHECK(argument_map.ArgumentsOf("kwarg_w")
              == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("kwarg")
              == std::vector<std::string>{"arg3"});
        CHECK(argument_map.ArgumentsOf("y") == std::vector<std::string>{});
      }
    }
  }

  GIVEN("An ArgumentMap of keyword parameters with argument limits.") {
    ParameterMap parameter_map;
    parameter_map(Parameter<std::string>::Keyword(
                      converters::StringIdentity,
                      {"no_max", "n", "no_max_long"}))
                 (Parameter<std::string>::Keyword(
                      converters::StringIdentity,
                      {"one_max", "o", "one_max_long"})
                      .MaxArgs(1))
                 (Parameter<std::string>::Keyword(
                      converters::StringIdentity,
                      {"two_max", "t", "two_max_long"})
                      .MaxArgs(2))
                 (Parameter<std::string>::Keyword(
                      converters::StringIdentity,
                      {"three_max"})
                      .MaxArgs(3))
                 (Parameter<std::string>::Keyword(
                      converters::StringIdentity,
                      {"f", "O", "u", "r"})
                      .MaxArgs(4));
    ArgumentMap argument_map{std::move(parameter_map)};
    std::vector<std::string> return_value;
    std::unordered_set<std::string> additional_arguments;

    WHEN("Parsing multiple argument lists end prematurely.") {
      std::istringstream iss{"two_max=arg1\n"
                             "three_max=arg2 arg3\n"
                             "no_max=arg4"};
      return_value = ParseFile(iss, argument_map);
      additional_arguments = std::unordered_set<std::string>{
          return_value.cbegin(), return_value.cend()};

      THEN("Return object is empty.") {
        CHECK(additional_arguments.empty());
      }

      THEN("Shortened argument lists are assigned to correct keywords.") {
        CHECK(argument_map.ArgumentsOf("no_max")
              == std::vector<std::string>{"arg4"});
        CHECK(argument_map.ArgumentsOf("one_max")
              == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("two_max")
              == std::vector<std::string>{"arg1"});
        CHECK(argument_map.ArgumentsOf("three_max")
              == std::vector<std::string>{"arg2", "arg3"});
        CHECK(argument_map.ArgumentsOf("f") == std::vector<std::string>{});
      }
    }

    WHEN("Argument lists are longer than the keyword parameter maximums.") {
      std::istringstream iss{"two_max=arg1 arg2 arg3 arg4\n"
                             "three_max=arg5 arg6 arg7 arg8\n"
                             "no_max=arg9 arg10 arg11 arg12 arg13 arg14"};
      return_value = ParseFile(iss, argument_map);
      additional_arguments = std::unordered_set<std::string>{
          return_value.cbegin(), return_value.cend()};

      THEN("Return object contains excess arguments.") {
        CHECK(additional_arguments
              == std::unordered_set<std::string>{"arg3", "arg4", "arg8"});
      }

      THEN("Shortened argument lists are assigned to correct keywords.") {
        CHECK(argument_map.ArgumentsOf("no_max")
              == std::vector<std::string>{"arg9", "arg10", "arg11", "arg12",
                                          "arg13", "arg14"});
        CHECK(argument_map.ArgumentsOf("one_max")
              == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("two_max")
              == std::vector<std::string>{"arg1", "arg2"});
        CHECK(argument_map.ArgumentsOf("three_max")
              == std::vector<std::string>{"arg5", "arg6", "arg7"});
        CHECK(argument_map.ArgumentsOf("f") == std::vector<std::string>{});
      }
    }
  }

  GIVEN("An ArgumentMap with positional parameters.") {
    ParameterMap parameter_map;
    parameter_map(Parameter<std::string>::Positional(
                      converters::StringIdentity, "pos1", -100))
                 (Parameter<std::string>::Positional(
                      converters::StringIdentity, "pos2", -1))
                 (Parameter<std::string>::Positional(
                      converters::StringIdentity, "pos3", 0))
                 (Parameter<std::string>::Positional(
                      converters::StringIdentity, "pos4", 1))
                 (Parameter<std::string>::Positional(
                      converters::StringIdentity, "pos5", 100));
    ArgumentMap argument_map{std::move(parameter_map)};
    std::vector<std::string> return_value;
    std::unordered_set<std::string> additional_arguments;

    WHEN("Parsing empty stream.") {
      std::istringstream iss{};
      return_value = ParseFile(iss, argument_map);
      additional_arguments = std::unordered_set<std::string>{
          return_value.cbegin(), return_value.cend()};

      THEN("Return object is empty.") {
        CHECK(additional_arguments.empty());
      }

      THEN("Parameters are assigned no arguments.") {
        CHECK(argument_map.ArgumentsOf("pos1") == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("pos2") == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("pos3") == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("pos4") == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("pos5") == std::vector<std::string>{});
      }
    }

    WHEN("Parsing some positional parameter names with arguments.") {
      std::istringstream iss{"pos4=arg1 arg2\n"
                             "pos2=arg3 arg4"};
      return_value = ParseFile(iss, argument_map);
      additional_arguments = std::unordered_set<std::string>{
          return_value.cbegin(), return_value.cend()};

      THEN("Return object is empty.") {
        CHECK(additional_arguments.empty());
      }

      THEN("Each argument list is assigned to respective parameter.") {
        CHECK(argument_map.ArgumentsOf("pos1") == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("pos2")
              == std::vector<std::string>{"arg3", "arg4"});
        CHECK(argument_map.ArgumentsOf("pos3") == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("pos4")
              == std::vector<std::string>{"arg1", "arg2"});
        CHECK(argument_map.ArgumentsOf("pos5") == std::vector<std::string>{});
      }
    }

    WHEN("Parsing multiple argument lists for same parameter name.") {
      std::istringstream iss{"pos2=arg1 arg2\n"
                             "pos4=arg3\n"
                             "pos2=arg4\n"
                             "pos2=arg5"};
      return_value = ParseFile(iss, argument_map);
      additional_arguments = std::unordered_set<std::string>{
          return_value.cbegin(), return_value.cend()};

      THEN("The argument lists are concatenated.") {
        CHECK(argument_map.ArgumentsOf("pos1") == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("pos2")
              == std::vector<std::string>{"arg1", "arg2", "arg4", "arg5"});
        CHECK(argument_map.ArgumentsOf("pos3") == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("pos4")
              == std::vector<std::string>{"arg3"});
        CHECK(argument_map.ArgumentsOf("pos5") == std::vector<std::string>{});
      }
    }
  }

  GIVEN("An ArgumentMap with positional parameters with argument limits.") {
    ParameterMap parameter_map;
    parameter_map(Parameter<std::string>::Positional(
                      converters::StringIdentity, "three_max", -100)
                      .MaxArgs(3))
                 (Parameter<std::string>::Positional(
                      converters::StringIdentity, "two_max", -1)
                      .MaxArgs(2))
                 (Parameter<std::string>::Positional(
                      converters::StringIdentity, "no_max", 0))
                 (Parameter<std::string>::Positional(
                      converters::StringIdentity, "four_max", 1)
                      .MaxArgs(4))
                 (Parameter<std::string>::Positional(
                      converters::StringIdentity, "one_max", 100)
                      .MaxArgs(1));
    ArgumentMap argument_map{std::move(parameter_map)};
    std::vector<std::string> return_value;
    std::unordered_set<std::string> additional_arguments;

    WHEN("Multiple argument lists end prematurely.") {
      std::istringstream iss{"two_max=arg1\n"
                             "three_max=arg2 arg3\n"
                             "no_max=arg4"};
      return_value = ParseFile(iss, argument_map);
      additional_arguments = std::unordered_set<std::string>{
          return_value.cbegin(), return_value.cend()};

      THEN("Return object is empty.") {
        CHECK(additional_arguments.empty());
      }

      THEN("Shortened argument lists are assigned to correct keywords.") {
        CHECK(argument_map.ArgumentsOf("three_max")
              == std::vector<std::string>{"arg2", "arg3"});
        CHECK(argument_map.ArgumentsOf("two_max")
              == std::vector<std::string>{"arg1"});
        CHECK(argument_map.ArgumentsOf("no_max")
              == std::vector<std::string>{"arg4"});
        CHECK(argument_map.ArgumentsOf("four_max")
              == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("one_max")
              == std::vector<std::string>{});
      }
    }

    WHEN("Argument lists are longer than the keyword parameter maximums.") {
      std::istringstream iss{"two_max=arg1 arg2 arg3 arg4\n"
                             "three_max=arg5 arg6 arg7 arg8\n"
                             "no_max=arg9 arg10 arg11 arg12 arg13 arg14"};
      return_value = ParseFile(iss, argument_map);
      additional_arguments = std::unordered_set<std::string>{
          return_value.cbegin(), return_value.cend()};

      THEN("Return object contains excess arguments.") {
        CHECK(additional_arguments
              == std::unordered_set<std::string>{"arg3", "arg4", "arg8"});
      }

      THEN("Shortened argument lists are assigned to correct keywords.") {
        CHECK(argument_map.ArgumentsOf("three_max")
              == std::vector<std::string>{"arg5", "arg6", "arg7"});
        CHECK(argument_map.ArgumentsOf("two_max")
              == std::vector<std::string>{"arg1", "arg2"});
        CHECK(argument_map.ArgumentsOf("no_max")
              == std::vector<std::string>{"arg9", "arg10", "arg11", "arg12",
                                          "arg13", "arg14"});
        CHECK(argument_map.ArgumentsOf("four_max")
              == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("one_max")
              == std::vector<std::string>{});
      }
    }
  }

  GIVEN("An ArgumentMap with various parameters.") {
    ParameterMap parameter_map;
    parameter_map(Parameter<std::string>::Positional(
                      converters::StringIdentity, "pos_three_max", -100)
                      .MaxArgs(3))
                 (Parameter<std::string>::Positional(
                      converters::StringIdentity, "pos_no_max", 0))
                 (Parameter<std::string>::Keyword(
                      converters::StringIdentity,
                      {"kwarg_two_max", "t", "kwarg_two_max_long"})
                      .MaxArgs(2))
                 (Parameter<std::string>::Keyword(
                      converters::StringIdentity,
                      {"kwarg_no_max", "n", "kwarg_no_max_long"}))
                 (Parameter<bool>::Flag({"flag_a", "a", "long_flag_a"}))
                 (Parameter<bool>::Flag({"flag_b", "b", "long_flag_b"}));
    ArgumentMap argument_map{std::move(parameter_map)};
    std::vector<std::string> return_value;
    std::unordered_set<std::string> additional_arguments;

    WHEN("Some parameters already have arguments.") {
      argument_map.AddArgument("pos_three_max", "old_arg1");
      argument_map.AddArgument("kwarg_two_max", "old_arg1");
      argument_map.AddArgument("kwarg_two_max", "old_arg2");
      argument_map.AddArgument("flag_a", "old_set");
      std::istringstream iss{"pos_three_max=arg1 arg2\n"
                             "kwarg_two_max=arg3\n"
                             "kwarg_no_max_long=arg4 arg5 arg6\n"
                             "flag_a=TRUE\n"
                             "long_flag_b=TRUE\n"
                             "pos_no_max=arg7 arg8"};
      return_value = ParseFile(iss, argument_map);
      additional_arguments = std::unordered_set<std::string>{
          return_value.cbegin(), return_value.cend()};

      THEN("Return object contains arguments for already filled parameters.") {
        CHECK(additional_arguments
              == std::unordered_set<std::string>{"arg1", "arg2", "arg3",
                                                 "flag_a"});
      }

      THEN("New arguments are assigned to empty parameters.") {
        CHECK(argument_map.ArgumentsOf("pos_three_max")
              == std::vector<std::string>{"old_arg1"});
        CHECK(argument_map.ArgumentsOf("pos_no_max")
              == std::vector<std::string>{"arg7", "arg8"});

        CHECK(argument_map.ArgumentsOf("kwarg_two_max")
              == std::vector<std::string>{"old_arg1", "old_arg2"});
        CHECK(argument_map.ArgumentsOf("kwarg_no_max")
              == std::vector<std::string>{"arg4", "arg5", "arg6"});

        CHECK(argument_map.ArgumentsOf("flag_a")
              == std::vector<std::string>{"old_set"});
        CHECK(argument_map.ArgumentsOf("flag_b")
              == std::vector<std::string>{"long_flag_b"});
      }
    }

    WHEN("Stream contains comment lines and blank lines.") {
      std::istringstream iss{"# Comments:\n"
                             "#pos_three_max=arg1 arg2\n"
                             "# Some comment.\n"
                             "\n"
                             "#kwarg_two_max=arg3\n"
                             "\n"
                             "kwarg_no_max_long=arg4 arg5 arg6\n"
                             "##MORE COMMENTS\n"
                             "flag_a=TRUE\n"
                             "#long_flag_b=TRUE\n"
                             "pos_no_max=arg7 arg8\n"};
      return_value = ParseFile(iss, argument_map);
      additional_arguments = std::unordered_set<std::string>{
          return_value.cbegin(), return_value.cend()};

      THEN("Commented lines and blank lines are ignored.") {
        CHECK(additional_arguments.empty());

        CHECK(argument_map.ArgumentsOf("pos_three_max")
              == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("pos_no_max")
              == std::vector<std::string>{"arg7", "arg8"});

        CHECK(argument_map.ArgumentsOf("kwarg_two_max")
              == std::vector<std::string>{});
        CHECK(argument_map.ArgumentsOf("kwarg_no_max")
              == std::vector<std::string>{"arg4", "arg5", "arg6"});

        CHECK(argument_map.ArgumentsOf("flag_a")
              == std::vector<std::string>{"flag_a"});
        CHECK(argument_map.ArgumentsOf("flag_b")
              == std::vector<std::string>{});
      }
    }
  }
}

SCENARIO("Test invariant preservation by ParseFile.",
         "[ParseFile][invariants]") {

  GIVEN("An ArgumentMap with various parameters.") {
    int size, parameters_size, arguments_size, values_size;
    ParameterMap parameter_map;
    parameter_map(Parameter<std::string>::Positional(
                      converters::StringIdentity, "pos_three_max", -100)
                      .MaxArgs(3))
                 (Parameter<std::string>::Positional(
                      converters::StringIdentity, "pos_no_max", 0))
                 (Parameter<std::string>::Keyword(
                      converters::StringIdentity,
                      {"kwarg_two_max", "t", "kwarg_two_max_long"})
                      .MaxArgs(2))
                 (Parameter<std::string>::Keyword(
                      converters::StringIdentity,
                      {"kwarg_no_max", "n", "kwarg_no_max_long"}))
                 (Parameter<bool>::Flag({"flag_a", "a", "long_flag_a"}))
                 (Parameter<bool>::Flag({"flag_b", "b", "long_flag_b"}));
    ArgumentMap argument_map{std::move(parameter_map)};
    size = argument_map.size();
    parameters_size = argument_map.Parameters().size();
    arguments_size = argument_map.Arguments().size();
    values_size = argument_map.Values().size();
    std::vector<std::string> return_value;
    std::unordered_set<std::string> additional_arguments;

    WHEN("Parsing without returning additional parameters.") {
      std::istringstream iss{"# Comments:\n"
                             "#pos_three_max=arg1 arg2\n"
                             "# Some comment.\n"
                             "\n"
                             "#kwarg_two_max=arg3\n"
                             "\n"
                             "kwarg_no_max_long=arg4 arg5 arg6\n"
                             "##MORE COMMENTS\n"
                             "flag_a=TRUE\n"
                             "#long_flag_b=TRUE\n"
                             "pos_no_max=arg7 arg8\n"};
      return_value = ParseFile(iss, argument_map);
      additional_arguments = std::unordered_set<std::string>{
          return_value.cbegin(), return_value.cend()};
      REQUIRE(additional_arguments.empty());

      THEN("ArgumentMap's size doesn't change.") {
        CHECK(static_cast<int>(argument_map.size()) == size);
      }

      THEN("ArgumentMap's ParameterMap member's size doesn't change.") {
        CHECK(static_cast<int>(argument_map.Parameters().size())
              == parameters_size);
      }

      THEN("ArgumentMap's number of stored argument lists doesn't change.") {
        CHECK(static_cast<int>(argument_map.Arguments().size())
              == arguments_size);
      }

      THEN("ArgumentMap's number of stored value lists doesn't change.") {
        CHECK(static_cast<int>(argument_map.Values().size()) == values_size);
      }
    }

    WHEN("Parsing and returning additional parameters.") {
      std::istringstream iss{"# Comments:\n"
                             "#pos_three_max=arg1 arg2 arg3 arg4\n"
                             "# Some comment.\n"
                             "\n"
                             "kwarg_two_max=arg5 arg6 arg7 arg8\n"
                             "\n"
                             "kwarg_no_max_long=arg9 arg10 arg11\n"
                             "##MORE COMMENTS\n"
                             "flag_a=TRUE\n"
                             "#long_flag_b=TRUE\n"
                             "pos_no_max=arg12\n"};
      return_value = ParseFile(iss, argument_map);
      additional_arguments = std::unordered_set<std::string>{
          return_value.cbegin(), return_value.cend()};
      REQUIRE_FALSE(additional_arguments.empty());

      THEN("ArgumentMap's size doesn't change.") {
        CHECK(static_cast<int>(argument_map.size()) == size);
      }

      THEN("ArgumentMap's ParameterMap member's size doesn't change.") {
        CHECK(static_cast<int>(argument_map.Parameters().size())
              == parameters_size);
      }

      THEN("ArgumentMap's number of stored argument lists doesn't change.") {
        CHECK(static_cast<int>(argument_map.Arguments().size())
              == arguments_size);
      }

      THEN("ArgumentMap's number of stored value lists doesn't change.") {
        CHECK(static_cast<int>(argument_map.Values().size()) == values_size);
      }
    }
  }
}

SCENARIO("Test exceptions thrown by ParseFile.", "[ParseFile][exceptions]") {

  GIVEN("An ArgumentMap with various parameters.") {
    ParameterMap parameter_map;
    parameter_map(Parameter<std::string>::Positional(
                      converters::StringIdentity, "x", -100)
                      .MaxArgs(3))
                 (Parameter<std::string>::Positional(
                      converters::StringIdentity, "pos_no_max", 0))
                 (Parameter<std::string>::Keyword(
                      converters::StringIdentity,
                      {"kwarg_two_max", "t", "kwarg_two_max_long"})
                      .MaxArgs(2))
                 (Parameter<std::string>::Keyword(
                      converters::StringIdentity,
                      {"kwarg_no_max", "n", "kwarg_no_max_long"}))
                 (Parameter<bool>::Flag({"flag_a", "a", "long_flag_a"}))
                 (Parameter<bool>::Flag({"flag_b", "b", "long_flag_b"}));
    ArgumentMap argument_map{std::move(parameter_map)};

    THEN("Missing '=' symbol in non-empty non-comment line causes exctpion.") {
      std::istringstream iss1{"pos_no_max:arg1 arg2"};
      std::istringstream iss2{" "};
      std::istringstream iss3{"kwarg_two_max arg"};
      CHECK_THROWS_AS(ParseFile(iss1, argument_map),
                      exceptions::ArgumentParsingError);
      CHECK_THROWS_AS(ParseFile(iss2, argument_map),
                      exceptions::ArgumentParsingError);
      CHECK_THROWS_AS(ParseFile(iss3, argument_map),
                      exceptions::ArgumentParsingError);
    }

    THEN("Unrecognized parameter in non-comment line causes exception.") {
      std::istringstream iss1{"--kwarg_two_max=arg1 arg2"};
      std::istringstream iss2{"unknown_name=arg"};
      std::istringstream iss3{"flag_a=TRUE\n"
                              "-b=FALSE"};
      CHECK_THROWS_AS(ParseFile(iss1, argument_map),
                      exceptions::ArgumentParsingError);
      CHECK_THROWS_AS(ParseFile(iss2, argument_map),
                      exceptions::ArgumentParsingError);
      CHECK_THROWS_AS(ParseFile(iss3, argument_map),
                      exceptions::ArgumentParsingError);
    }

    THEN("Parameter name without argument list causes exception.") {
      std::istringstream iss1{"kwarg_two_max="};
      std::istringstream iss2{"pos_no_max="};
      std::istringstream iss3{"flag_a=TRUE\n"
                              "flag_b="};
      CHECK_THROWS_AS(ParseFile(iss1, argument_map),
                      exceptions::ArgumentParsingError);
      CHECK_THROWS_AS(ParseFile(iss2, argument_map),
                      exceptions::ArgumentParsingError);
      CHECK_THROWS_AS(ParseFile(iss3, argument_map),
                      exceptions::ArgumentParsingError);
    }

    THEN("Invalid flag argument causes exception.") {
      std::istringstream iss1{"flag_a=TRue"};
      std::istringstream iss2{"flag_b=true true"};
      std::istringstream iss3{"a=TRUE\n"
                              "long_flag_b=YES"};
      CHECK_THROWS_AS(ParseFile(iss1, argument_map),
                      exceptions::ArgumentParsingError);
      CHECK_THROWS_AS(ParseFile(iss2, argument_map),
                      exceptions::ArgumentParsingError);
      CHECK_THROWS_AS(ParseFile(iss3, argument_map),
                      exceptions::ArgumentParsingError);
    }
  }
}

} // namespace

} // namespace test

} // namespace arg_parse_convert