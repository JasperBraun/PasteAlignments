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

#include "help_string_formatters.h"

#define CATCH_CONFIG_MAIN
#define CATCH_CONFIG_COLOUR_NONE
#include "catch.h"

#include "string_conversions.h" // include after catch.h

#include "parameter.h"
#include "parameter_map.h"

// Test correctness for:
// * FormattedHelpString(ParameterMap)
//
// Test invariants for:
//
// Test exceptions for:
// * FormattedHelpString(ParameterMap)

namespace arg_parse_convert {

namespace test {

namespace {

SCENARIO("Test correctness of FormattedHelpString(ParameterMap).",
         "[FormattedHelpString(ParameterMap)][correctness]") {

  GIVEN("Various required and non-required parameters.") {
    Parameter<int> pos_min_nodefault{
        Parameter<int>::Positional(converters::stoi, "pos_min_nodefault", 0)
            .MinArgs(2)};
    Parameter<std::string> pos_min_default{
        Parameter<std::string>::Positional(converters::StringIdentity,
                                           "pos_min_default", 1)
            .MinArgs(1).AddDefault("arg")};
    Parameter<float> pos_nomin{
        Parameter<float>::Positional(converters::stof, "pos_nomin", 2)};

    Parameter<int> kw_min_nodefault{
        Parameter<int>::Keyword(converters::stoi, {"kw_min_nodefault", "foo"})
            .MinArgs(2)};
    Parameter<std::string> kw_min_default{
        Parameter<std::string>::Keyword(converters::StringIdentity,
                                        {"kw_min_default"})
            .MinArgs(1).AddDefault("arg")};
    Parameter<float> kw_nomin{
        Parameter<float>::Keyword(converters::stof, {"kw_nomin", "kwno", "k"})};

    Parameter<bool> a_flag{Parameter<bool>::Flag({"a_flag", "a", "a_long"})};
    Parameter<bool> b_flag{Parameter<bool>::Flag({"b", "b_flag"})};

    WHEN("No header or footer given and default width and whitespace used.") {
      ParameterMap parameter_map;
      parameter_map(pos_min_nodefault)(pos_min_default)(pos_nomin)
                   (kw_min_nodefault)(kw_min_default)(kw_nomin)
                   (a_flag)(b_flag);

      THEN("Parameters are printed without descriptions.") {
        CHECK(FormattedHelpString(parameter_map)
              == "\n"
                 "Required parameters:\n"
                 "    pos_min_nodefault\n"
                 "    --kw_min_nodefault, --foo <ARG>\n"
                 "\n"
                 "Optional positional parameters:\n"
                 "    pos_min_default ( = arg)\n"
                 "    pos_nomin\n"
                 "\n"
                 "Optional keyword parameters:\n"
                 "    --kw_min_default <ARG> ( = arg)\n"
                 "    --kw_nomin, --kwno, -k <ARG>\n"
                 "\n"
                 "Flags:\n"
                 "    --a_flag, -a, --a_long\n"
                 "    -b, --b_flag\n");
      }
    }

    WHEN("Descriptions are given.") {
      pos_min_nodefault.Description(
          "Description of pos_min_nodefault. The description will be broken up"
          " into rows of length `width` - `parameter_indentation`. Line breaks"
          " are attempted between words.");
      pos_min_default.Description(
          "Description of pos_min_default. The description will be broken up"
          " into rows of length `width` - `parameter_indentation`. Line breaks"
          " are attempted between words.");
      pos_nomin.Description(
          "Description of pos_nomin. The description will be broken up into"
          " rows of length `width` - `parameter_indentation`. Line breaks are"
          " attempted between words.");

      kw_min_nodefault.Description(
          "Description of kw_min_nodefault. The description will be broken up"
          " into rows of length `width` - `parameter_indentation`. Line breaks"
          " are attempted between words.");
      kw_min_default.Description(
          "Description of kw_min_default. The description will be broken up"
          " into rows of length `width` - `parameter_indentation`. Line breaks"
          " are attempted between words.");
      kw_nomin.Description(
          "Description of kw_nomin. The description will be broken up into rows"
          " of length `width` - `parameter_indentation`. Line breaks are"
          " attempted between words.");

      a_flag.Description(
          "Description of a_flag. The description will be broken up into rows"
          " of length `width` - `parameter_indentation`. Line breaks are"
          " attempted between words.");
      b_flag.Description(
          "Description of b_flag. The description will be broken up into rows"
          " of length `width` - `parameter_indentation`. Line breaks are"
          " attempted between words.");
      ParameterMap parameter_map;
      parameter_map(pos_min_nodefault)(pos_min_default)(pos_nomin)
                   (kw_min_nodefault)(kw_min_default)(kw_nomin)
                   (a_flag)(b_flag);
      THEN("Descriptions are indented and smart linebreaks attempted.") {
        CHECK(FormattedHelpString(parameter_map)
              == "\n"
                 "Required parameters:\n"
                 "    pos_min_nodefault\n"
                 "        Description of pos_min_nodefault. The description will be broken up into\n"
                 "        rows of length `width` - `parameter_indentation`. Line breaks are\n"
                 "        attempted between words.\n"
                 "    --kw_min_nodefault, --foo <ARG>\n"
                 "        Description of kw_min_nodefault. The description will be broken up into\n"
                 "        rows of length `width` - `parameter_indentation`. Line breaks are\n"
                 "        attempted between words.\n"
                 "\n"
                 "Optional positional parameters:\n"
                 "    pos_min_default ( = arg)\n"
                 "        Description of pos_min_default. The description will be broken up into\n"
                 "        rows of length `width` - `parameter_indentation`. Line breaks are\n"
                 "        attempted between words.\n"
                 "    pos_nomin\n"
                 "        Description of pos_nomin. The description will be broken up into rows of\n"
                 "        length `width` - `parameter_indentation`. Line breaks are attempted\n"
                 "        between words.\n"
                 "\n"
                 "Optional keyword parameters:\n"
                 "    --kw_min_default <ARG> ( = arg)\n"
                 "        Description of kw_min_default. The description will be broken up into\n"
                 "        rows of length `width` - `parameter_indentation`. Line breaks are\n"
                 "        attempted between words.\n"
                 "    --kw_nomin, --kwno, -k <ARG>\n"
                 "        Description of kw_nomin. The description will be broken up into rows of\n"
                 "        length `width` - `parameter_indentation`. Line breaks are attempted\n"
                 "        between words.\n"
                 "\n"
                 "Flags:\n"
                 "    --a_flag, -a, --a_long\n"
                 "        Description of a_flag. The description will be broken up into rows of\n"
                 "        length `width` - `parameter_indentation`. Line breaks are attempted\n"
                 "        between words.\n"
                 "    -b, --b_flag\n"
                 "        Description of b_flag. The description will be broken up into rows of\n"
                 "        length `width` - `parameter_indentation`. Line breaks are attempted\n"
                 "        between words.\n");
      }
    }

    WHEN("Argument placeholders are given.") {
      pos_min_nodefault.Placeholder("POSARG1 POSARG2");
      pos_min_default.Placeholder("POSARG");
      pos_nomin.Placeholder("POSARGS...");

      kw_min_nodefault.Placeholder("KWARG1 KWARG2");
      kw_min_default.Placeholder("KWARG");
      kw_nomin.Placeholder("KWARGS...");

      a_flag.Placeholder("should not appear");
      b_flag.Placeholder("should not appear");
      ParameterMap parameter_map;
      parameter_map(pos_min_nodefault)(pos_min_default)(pos_nomin)
                   (kw_min_nodefault)(kw_min_default)(kw_nomin)
                   (a_flag)(b_flag);
      THEN("Placeholder replace default placeholders.") {
        CHECK(FormattedHelpString(parameter_map)
              == "\n"
                 "Required parameters:\n"
                 "    POSARG1 POSARG2\n"
                 "    --kw_min_nodefault, --foo KWARG1 KWARG2\n"
                 "\n"
                 "Optional positional parameters:\n"
                 "    POSARG ( = arg)\n"
                 "    POSARGS...\n"
                 "\n"
                 "Optional keyword parameters:\n"
                 "    --kw_min_default KWARG ( = arg)\n"
                 "    --kw_nomin, --kwno, -k KWARGS...\n"
                 "\n"
                 "Flags:\n"
                 "    --a_flag, -a, --a_long\n"
                 "    -b, --b_flag\n");
      }
    }

    WHEN("Default arguments are given.") {
      pos_min_nodefault.AddDefault("POSARG");
      pos_min_default.SetDefault({"POSARG"});
      pos_nomin.SetDefault({"POSARG1", "POSARG2", "POSARG3"});

      kw_min_nodefault.AddDefault("KWARG");
      kw_min_default.SetDefault({"KWARG"});
      kw_nomin.SetDefault({"KWARG1", "KWARG2", "KWARG3"});

      a_flag.AddDefault("should not appear");
      b_flag.AddDefault("should not appear");
      ParameterMap parameter_map;
      parameter_map(pos_min_nodefault)(pos_min_default)(pos_nomin)
                   (kw_min_nodefault)(kw_min_default)(kw_nomin)
                   (a_flag)(b_flag);
      THEN("Default arguments are printed at the end of lines.") {
        CHECK(FormattedHelpString(parameter_map)
              == "\n"
                 "Required parameters:\n"
                 "    pos_min_nodefault ( = POSARG)\n"
                 "    --kw_min_nodefault, --foo <ARG> ( = KWARG)\n"
                 "\n"
                 "Optional positional parameters:\n"
                 "    pos_min_default ( = POSARG)\n"
                 "    pos_nomin ( = POSARG1 POSARG2 POSARG3)\n"
                 "\n"
                 "Optional keyword parameters:\n"
                 "    --kw_min_default <ARG> ( = KWARG)\n"
                 "    --kw_nomin, --kwno, -k <ARG> ( = KWARG1 KWARG2 KWARG3)\n"
                 "\n"
                 "Flags:\n"
                 "    --a_flag, -a, --a_long\n"
                 "    -b, --b_flag\n");
      }
    }

    WHEN("There are no required parameters.") {
      ParameterMap parameter_map;
      parameter_map(pos_min_default)(pos_nomin)
                   (kw_min_default)(kw_nomin)
                   (a_flag)(b_flag);

      THEN("Required parameters section is left out.") {
        CHECK(FormattedHelpString(parameter_map)
              == "\n"
                 "Optional positional parameters:\n"
                 "    pos_min_default ( = arg)\n"
                 "    pos_nomin\n"
                 "\n"
                 "Optional keyword parameters:\n"
                 "    --kw_min_default <ARG> ( = arg)\n"
                 "    --kw_nomin, --kwno, -k <ARG>\n"
                 "\n"
                 "Flags:\n"
                 "    --a_flag, -a, --a_long\n"
                 "    -b, --b_flag\n");
      }
    }

    WHEN("There are no optional positional parameters.") {
      ParameterMap parameter_map;
      parameter_map(pos_min_nodefault)
                   (kw_min_nodefault)(kw_min_default)(kw_nomin)
                   (a_flag)(b_flag);

      THEN("Optional positional parameters section is left out.") {
        CHECK(FormattedHelpString(parameter_map)
              == "\n"
                 "Required parameters:\n"
                 "    pos_min_nodefault\n"
                 "    --kw_min_nodefault, --foo <ARG>\n"
                 "\n"
                 "Optional keyword parameters:\n"
                 "    --kw_min_default <ARG> ( = arg)\n"
                 "    --kw_nomin, --kwno, -k <ARG>\n"
                 "\n"
                 "Flags:\n"
                 "    --a_flag, -a, --a_long\n"
                 "    -b, --b_flag\n");
      }
    }

    WHEN("There are no optional keyword parameters.") {
      ParameterMap parameter_map;
      parameter_map(pos_min_nodefault)(pos_min_default)(pos_nomin)
                   (kw_min_nodefault)
                   (a_flag)(b_flag);

      THEN("Optional keyword parameters section is left out.") {
        CHECK(FormattedHelpString(parameter_map)
              == "\n"
                 "Required parameters:\n"
                 "    pos_min_nodefault\n"
                 "    --kw_min_nodefault, --foo <ARG>\n"
                 "\n"
                 "Optional positional parameters:\n"
                 "    pos_min_default ( = arg)\n"
                 "    pos_nomin\n"
                 "\n"
                 "Flags:\n"
                 "    --a_flag, -a, --a_long\n"
                 "    -b, --b_flag\n");
      }
    }

    WHEN("There are no flags.") {
      ParameterMap parameter_map;
      parameter_map(pos_min_nodefault)(pos_min_default)(pos_nomin)
                   (kw_min_nodefault)(kw_min_default)(kw_nomin);

      THEN("Flags section is left out.") {
        CHECK(FormattedHelpString(parameter_map)
              == "\n"
                 "Required parameters:\n"
                 "    pos_min_nodefault\n"
                 "    --kw_min_nodefault, --foo <ARG>\n"
                 "\n"
                 "Optional positional parameters:\n"
                 "    pos_min_default ( = arg)\n"
                 "    pos_nomin\n"
                 "\n"
                 "Optional keyword parameters:\n"
                 "    --kw_min_default <ARG> ( = arg)\n"
                 "    --kw_nomin, --kwno, -k <ARG>\n");
      }
    }

    WHEN("A header is given.") {
      ParameterMap parameter_map;
      parameter_map(pos_min_nodefault)(pos_min_default)(pos_nomin)
                   (kw_min_nodefault)(kw_min_default)(kw_nomin)
                   (a_flag)(b_flag);
      std::string header{"\n"
                         "command version 1.2.3\n"
                         "command [options] --foo KWARG1 KWARG2 POSARGS\n"};

      THEN("Header is prepended.") {
        CHECK(FormattedHelpString(parameter_map, header)
              == "\n"
                 "command version 1.2.3\n"
                 "command [options] --foo KWARG1 KWARG2 POSARGS\n"
                 "\n"
                 "Required parameters:\n"
                 "    pos_min_nodefault\n"
                 "    --kw_min_nodefault, --foo <ARG>\n"
                 "\n"
                 "Optional positional parameters:\n"
                 "    pos_min_default ( = arg)\n"
                 "    pos_nomin\n"
                 "\n"
                 "Optional keyword parameters:\n"
                 "    --kw_min_default <ARG> ( = arg)\n"
                 "    --kw_nomin, --kwno, -k <ARG>\n"
                 "\n"
                 "Flags:\n"
                 "    --a_flag, -a, --a_long\n"
                 "    -b, --b_flag\n");
      }
    }

    WHEN("A footer is given.") {
      ParameterMap parameter_map;
      parameter_map(pos_min_nodefault)(pos_min_default)(pos_nomin)
                   (kw_min_nodefault)(kw_min_default)(kw_nomin)
                   (a_flag)(b_flag);
      std::string footer{"\n"
                         "For more information visit https://github.com/JasperBraun/ArgParseConvert.git\n"
                         "Copyright (c) 2020 Jasper Braun\n"};

      THEN("Footer is appended.") {
        CHECK(FormattedHelpString(parameter_map, "", footer)
              == "\n"
                 "Required parameters:\n"
                 "    pos_min_nodefault\n"
                 "    --kw_min_nodefault, --foo <ARG>\n"
                 "\n"
                 "Optional positional parameters:\n"
                 "    pos_min_default ( = arg)\n"
                 "    pos_nomin\n"
                 "\n"
                 "Optional keyword parameters:\n"
                 "    --kw_min_default <ARG> ( = arg)\n"
                 "    --kw_nomin, --kwno, -k <ARG>\n"
                 "\n"
                 "Flags:\n"
                 "    --a_flag, -a, --a_long\n"
                 "    -b, --b_flag\n"
                 "\n"
                 "For more information visit https://github.com/JasperBraun/ArgParseConvert.git\n"
                 "Copyright (c) 2020 Jasper Braun\n");
      }
    }

    WHEN("Description is given and width and whitespace are modified.") {
      pos_min_nodefault.Description(
          "Description of pos_min_nodefault. The description will be broken up"
          " into rows of length `width` - `parameter_indentation`. Line breaks"
          " are attempted between words.");
      pos_min_default.Description(
          "Description of pos_min_default. The description will be broken up"
          " into rows of length `width` - `parameter_indentation`. Line breaks"
          " are attempted between words.");
      pos_nomin.Description(
          "Description of pos_nomin. The description will be broken up into"
          " rows of length `width` - `parameter_indentation`. Line breaks are"
          " attempted between words.");

      kw_min_nodefault.Description(
          "Description of kw_min_nodefault. The description will be broken up"
          " into rows of length `width` - `parameter_indentation`. Line breaks"
          " are attempted between words.");
      kw_min_default.Description(
          "Description of kw_min_default. The description will be broken up"
          " into rows of length `width` - `parameter_indentation`. Line breaks"
          " are attempted between words.");
      kw_nomin.Description(
          "Description of kw_nomin. The description will be broken up into rows"
          " of length `width` - `parameter_indentation`. Line breaks are"
          " attempted between words.");

      a_flag.Description(
          "Description of a_flag. The description will be broken up into rows"
          " of length `width` - `parameter_indentation`. Line breaks are"
          " attempted between words.");
      b_flag.Description(
          "Description of b_flag. The description will be broken up into rows"
          " of length `width` - `parameter_indentation`. Line breaks are"
          " attempted between words.");

      ParameterMap parameter_map;
      parameter_map(pos_min_nodefault)(pos_min_default)(pos_nomin)
                   (kw_min_nodefault)(kw_min_default)(kw_nomin)
                   (a_flag)(b_flag);

      int width{40}, parameter_indentation{2}, description_indentation{4};

      THEN("Parameters are printed without descriptions.") {
        CHECK(FormattedHelpString(parameter_map, "", "", width,
                                  parameter_indentation,
                                  description_indentation)
              == "\n"
                 "Required parameters:\n"
                 "  pos_min_nodefault\n"
                 "    Description of pos_min_nodefault.\n"
                 "    The description will be broken up\n"
                 "    into rows of length `width` -\n"
                 "    `parameter_indentation`. Line breaks\n"
                 "    are attempted between words.\n"
                 "  --kw_min_nodefault, --foo <ARG>\n"
                 "    Description of kw_min_nodefault. The\n"
                 "    description will be broken up into\n"
                 "    rows of length `width` -\n"
                 "    `parameter_indentation`. Line breaks\n"
                 "    are attempted between words.\n"
                 "\n"
                 "Optional positional parameters:\n"
                 "  pos_min_default ( = arg)\n"
                 "    Description of pos_min_default. The\n"
                 "    description will be broken up into\n"
                 "    rows of length `width` -\n"
                 "    `parameter_indentation`. Line breaks\n"
                 "    are attempted between words.\n"
                 "  pos_nomin\n"
                 "    Description of pos_nomin. The\n"
                 "    description will be broken up into\n"
                 "    rows of length `width` -\n"
                 "    `parameter_indentation`. Line breaks\n"
                 "    are attempted between words.\n"
                 "\n"
                 "Optional keyword parameters:\n"
                 "  --kw_min_default <ARG> ( = arg)\n"
                 "    Description of kw_min_default. The\n"
                 "    description will be broken up into\n"
                 "    rows of length `width` -\n"
                 "    `parameter_indentation`. Line breaks\n"
                 "    are attempted between words.\n"
                 "  --kw_nomin, --kwno, -k <ARG>\n"
                 "    Description of kw_nomin. The\n"
                 "    description will be broken up into\n"
                 "    rows of length `width` -\n"
                 "    `parameter_indentation`. Line breaks\n"
                 "    are attempted between words.\n"
                 "\n"
                 "Flags:\n"
                 "  --a_flag, -a, --a_long\n"
                 "    Description of a_flag. The\n"
                 "    description will be broken up into\n"
                 "    rows of length `width` -\n"
                 "    `parameter_indentation`. Line breaks\n"
                 "    are attempted between words.\n"
                 "  -b, --b_flag\n"
                 "    Description of b_flag. The\n"
                 "    description will be broken up into\n"
                 "    rows of length `width` -\n"
                 "    `parameter_indentation`. Line breaks\n"
                 "    are attempted between words.\n");
      }
    }
  }
}

SCENARIO("Test exceptions thrown by FormattedHelpString(ParameterMap).",
         "[FormattedHelpString(ParameterMap)][exceptions]") {

  GIVEN("Various required and non-required parameters.") {
    Parameter<int> pos_min_nodefault{
        Parameter<int>::Positional(converters::stoi, "pos_min_nodefault", 0)
            .MinArgs(2)};
    Parameter<std::string> pos_min_default{
        Parameter<std::string>::Positional(converters::StringIdentity,
                                           "pos_min_default", 1)
            .MinArgs(1).AddDefault("arg")};
    Parameter<float> pos_nomin{
        Parameter<float>::Positional(converters::stof, "pos_nomin", 2)};

    Parameter<int> kw_min_nodefault{
        Parameter<int>::Keyword(converters::stoi, {"kw_min_nodefault", "foo"})
            .MinArgs(2)};
    Parameter<std::string> kw_min_default{
        Parameter<std::string>::Keyword(converters::StringIdentity,
                                        {"kw_min_default"})
            .MinArgs(1).AddDefault("arg")};
    Parameter<float> kw_nomin{
        Parameter<float>::Keyword(converters::stof, {"kw_nomin", "kwno", "k"})};

    Parameter<bool> a_flag{Parameter<bool>::Flag({"a_flag", "a", "a_long"})};
    Parameter<bool> b_flag{Parameter<bool>::Flag({"b", "b_flag"})};

    WHEN("No header or footer given.") {
      ParameterMap parameter_map;
      parameter_map(pos_min_nodefault)(pos_min_default)(pos_nomin)
                   (kw_min_nodefault)(kw_min_default)(kw_nomin)
                   (a_flag)(b_flag);

      THEN("Zero width causes exception.") {
        CHECK_THROWS_AS(FormattedHelpString(parameter_map, "", "", 0),
                        exceptions::HelpStringError);
      }

      THEN("Negative width causes exception.") {
        int width = GENERATE(take(3, range(-100, -1)));
        CHECK_THROWS_AS(FormattedHelpString(parameter_map, "", "", width),
                        exceptions::HelpStringError);
      }

      THEN("Negative parameter_indentation causes exception.") {
        int parameter_indentation = GENERATE(take(3, range(-100, -1)));
        CHECK_THROWS_AS(FormattedHelpString(parameter_map, "", "", 80,
                                            parameter_indentation),
                        exceptions::HelpStringError);
      }

      THEN("Zero parameter_indentation does not cause exception.") {
        CHECK_NOTHROW(FormattedHelpString(parameter_map, "", "", 80, 0));
      }

      THEN("Negative description_indentation causes exception.") {
        int description_indentation = GENERATE(take(3, range(-100, -1)));
        CHECK_THROWS_AS(FormattedHelpString(parameter_map, "", "", 80, 4,
                                            description_indentation),
                        exceptions::HelpStringError);
      }

      THEN("Zero description_indentation does not cause exception.") {
        CHECK_NOTHROW(FormattedHelpString(parameter_map, "", "", 80, 4, 0));
      }

      THEN("Width equal to parameter_indentation causes exception") {
        int width = GENERATE(take(3, range(10, 100)));
        int parameter_indentation{width};
        int description_indentation{0};
        CHECK_THROWS_AS(FormattedHelpString(parameter_map, "", "", width,
                                            parameter_indentation,
                                            description_indentation),
                        exceptions::HelpStringError);
      }

      THEN("Width equal to description_indentation causes exception") {
        int width = GENERATE(take(3, range(10, 100)));
        int parameter_indentation{0};
        int description_indentation{width};
        CHECK_THROWS_AS(FormattedHelpString(parameter_map, "", "", width,
                                            parameter_indentation,
                                            description_indentation),
                        exceptions::HelpStringError);
      }

      THEN("Width less than parameter_indentation causes exception") {
        int width = GENERATE(take(2, range(10, 100)));
        int distance = GENERATE(take(2, range(1, 100)));
        int parameter_indentation{width + distance};
        int description_indentation{0};
        CHECK_THROWS_AS(FormattedHelpString(parameter_map, "", "", width,
                                            parameter_indentation,
                                            description_indentation),
                        exceptions::HelpStringError);
      }

      THEN("Width less than description_indentation causes exception") {
        int width = GENERATE(take(2, range(10, 100)));
        int distance = GENERATE(take(2, range(1, 100)));
        int parameter_indentation{0};
        int description_indentation{width + distance};
        CHECK_THROWS_AS(FormattedHelpString(parameter_map, "", "", width,
                                            parameter_indentation,
                                            description_indentation),
                        exceptions::HelpStringError);
      }
    }
  }
}

} // namespace

} // namespace test

} // namespace arg_parse_convert