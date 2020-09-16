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

#include "parameter.h"

#define CATCH_CONFIG_MAIN
#define CATCH_CONFIG_COLOUR_NONE
#include "catch.h"

#include "string_conversions.h" // include after catch.h

#include <string>
#include <typeinfo>
#include <vector>

// ParameterConfiguration tests
//
// Test correctness for:
// * IsRequired
// * MinArgs
// * MaxArgs
// 
// Test invariants for:
// * names
// * MinArgs
// * MaxArgs
//
// Test exceptions for:
// * names
// * AddDefault
// * SetDefault
// * MinArgs
// * MaxArgs

// Parameter tests
//
// Test correctness for:
// * Flag
// * Keyword
// * Positional
//
// Test invariants for:
// * Flag
// * Keyword
// * Positional
//
// Test exceptions for:
// * Flag
// * Keyword
// * Positional

namespace arg_parse_convert {

namespace test {

int DummyConverter(const std::string& s) {return 2020;}

namespace {

// ParameterConfiguration tests.
SCENARIO("Test correctness of ParameterConfiguration::IsRequired.",
         "[ParameterConfiguration][IsRequired][correctness]") {

  GIVEN("Positional, keyword, and flag parameters.") {
    ParameterConfiguration foo{Parameter<std::string>::Positional(
        converters::StringIdentity, "foo", 1).configuration()};
    ParameterConfiguration bar{Parameter<std::string>::Keyword(
        converters::StringIdentity, {"bar"}).configuration()};
    ParameterConfiguration baz{Parameter<bool>::Flag({"baz"}).configuration()};

    WHEN("Minimum number of arguments is 0.") {
      foo.MinArgs(0);
      bar.MinArgs(0);
      baz.MinArgs(0);

      THEN("Arguments are not required.") {
        CHECK_FALSE(foo.IsRequired());
        CHECK_FALSE(bar.IsRequired());
        CHECK_FALSE(baz.IsRequired());
      }
    }

    WHEN("Minimum arguments requested and no default arguments given.") {
      foo.MinArgs(1);
      bar.MinArgs(2);
      baz.MinArgs(3);

      THEN("Arguments are required unless they're flags.") {
        CHECK(foo.IsRequired());
        CHECK(bar.IsRequired());
        CHECK_FALSE(baz.IsRequired());
      }
    }

    WHEN("Minimum arguments requested but fewer default arguments given.") {
      foo.MinArgs(2);
      foo.AddDefault("foo");
      bar.MinArgs(3);
      bar.AddDefault("bar");
      baz.MinArgs(4);
      baz.AddDefault("baz");

      THEN("Arguments are required unless they're flags.") {
        CHECK(foo.IsRequired());
        CHECK(bar.IsRequired());
        CHECK_FALSE(baz.IsRequired());
      }
    }

    WHEN("Minimum arguments requested but enough default arguments given.") {
      foo.MinArgs(2);
      foo.SetDefault({"foo1", "foo2"});
      bar.MinArgs(3);
      bar.SetDefault({"bar1", "bar2", "bar3"});
      baz.MinArgs(4);
      baz.SetDefault({"baz1", "baz2", "baz3", "baz4"});

      THEN("Arguments are required.") {
        CHECK_FALSE(foo.IsRequired());
        CHECK_FALSE(bar.IsRequired());
        CHECK_FALSE(baz.IsRequired());
      }
    }
  }
}

SCENARIO("Test correctness of ParameterConfiguration::MinArgs.",
         "[ParameterConfiguration][MinArgs][correctness]") {

  GIVEN("Positional, keyword, and flag parameters, and minimum numbers.") {
    ParameterConfiguration foo{Parameter<std::string>::Positional(
        converters::StringIdentity, "foo", 1).configuration()};
    int foo_min = 2;

    ParameterConfiguration bar{Parameter<std::string>::Keyword(
        converters::StringIdentity, {"bar"}).configuration()};
    int bar_min = 3;

    ParameterConfiguration baz{Parameter<bool>::Flag({"baz"}).configuration()};
    int baz_min = 4;

    WHEN("Maximums are unlimited.") {
      int foo_max = foo.max_num_arguments();
      foo.MinArgs(foo_min);

      int bar_max = bar.max_num_arguments();
      bar.MinArgs(bar_min);

      int baz_max = baz.max_num_arguments();
      baz.MinArgs(baz_min);

      THEN("Only minimums change.") {
        CHECK(foo.min_num_arguments() == foo_min);
        CHECK(foo.max_num_arguments() == foo_max);

        CHECK(bar.min_num_arguments() == bar_min);
        CHECK(bar.max_num_arguments() == bar_max);

        CHECK(baz.min_num_arguments() == baz_min);
        CHECK(baz.max_num_arguments() == baz_max);
      }
    }

    WHEN("Maximums are larger than minimums.") {
      foo.MaxArgs(foo_min + 1);
      int foo_max = foo.max_num_arguments();
      foo.MinArgs(foo_min);

      bar.MaxArgs(bar_min + 1);
      int bar_max = bar.max_num_arguments();
      bar.MinArgs(bar_min);

      baz.MaxArgs(baz_min + 1);
      int baz_max = baz.max_num_arguments();
      baz.MinArgs(baz_min);

      THEN("Only minimums change.") {
        CHECK(foo.min_num_arguments() == foo_min);
        CHECK(foo.max_num_arguments() == foo_max);

        CHECK(bar.min_num_arguments() == bar_min);
        CHECK(bar.max_num_arguments() == bar_max);

        CHECK(baz.min_num_arguments() == baz_min);
        CHECK(baz.max_num_arguments() == baz_max);
      }
    }

    WHEN("Maximums are equal to minimums.") {
      foo.MaxArgs(foo_min);
      int foo_max = foo.max_num_arguments();
      foo.MinArgs(foo_min);

      bar.MaxArgs(bar_min);
      int bar_max = bar.max_num_arguments();
      bar.MinArgs(bar_min);

      baz.MaxArgs(baz_min);
      int baz_max = baz.max_num_arguments();
      baz.MinArgs(baz_min);

      THEN("Only minimums change.") {
        CHECK(foo.min_num_arguments() == foo_min);
        CHECK(foo.max_num_arguments() == foo_max);

        CHECK(bar.min_num_arguments() == bar_min);
        CHECK(bar.max_num_arguments() == bar_max);
        
        CHECK(baz.min_num_arguments() == baz_min);
        CHECK(baz.max_num_arguments() == baz_max);
      }
    }

    WHEN("Maximums are positive, but smaller than minimums.") {
      foo.MaxArgs(foo_min - 1);
      int foo_max = foo.max_num_arguments();
      foo.MinArgs(foo_min);

      bar.MaxArgs(bar_min - 1);
      int bar_max = bar.max_num_arguments();
      bar.MinArgs(bar_min);

      baz.MaxArgs(baz_min - 1);
      int baz_max = baz.max_num_arguments();
      baz.MinArgs(baz_min);

      THEN("Maximums are set equal to minimums.") {
        CHECK(foo.min_num_arguments() == foo_min);
        CHECK_FALSE(foo.max_num_arguments() == foo_max);
        CHECK(foo.max_num_arguments() == foo_min);

        CHECK(bar.min_num_arguments() == bar_min);
        CHECK_FALSE(bar.max_num_arguments() == bar_max);
        CHECK(bar.max_num_arguments() == bar_min);
        
        CHECK(baz.min_num_arguments() == baz_min);
        CHECK_FALSE(baz.max_num_arguments() == baz_max);
        CHECK(baz.max_num_arguments() == baz_min);
      }
    }
  }
}

SCENARIO("Test invariant preservation by ParameterConfiguration::MinArgs.",
         "[ParameterConfiguration][MinArgs][invariants]") {

  GIVEN("Positional, keyword, and flag parameters, and minimum numbers.") {
    ParameterConfiguration foo{Parameter<std::string>::Positional(
        converters::StringIdentity, "foo", 1).configuration()};
    int foo_min = 2;

    ParameterConfiguration bar{Parameter<std::string>::Keyword(
        converters::StringIdentity, {"bar"}).configuration()};
    int bar_min = 3;

    ParameterConfiguration baz{Parameter<bool>::Flag({"baz"}).configuration()};
    int baz_min = 4;

    WHEN("Maximums are unlimited.") {
      foo.MinArgs(foo_min);
      bar.MinArgs(bar_min);
      baz.MinArgs(baz_min);

      THEN("Maximums remain unlimited.") {
        CHECK(foo.max_num_arguments() == 0);
        CHECK(bar.max_num_arguments() == 0);
        CHECK(baz.max_num_arguments() == 0);
      }
    }

    WHEN("Maximums are larger than minimums.") {
      foo.MaxArgs(foo_min + 1);
      foo.MinArgs(foo_min);

      bar.MaxArgs(bar_min + 1);
      bar.MinArgs(bar_min);

      baz.MaxArgs(baz_min + 1);
      baz.MinArgs(baz_min);

      THEN("Maximums remain greater than or equal to minimums.") {
        CHECK(foo.min_num_arguments() <= foo.max_num_arguments());
        CHECK(bar.min_num_arguments() <= bar.max_num_arguments());
        CHECK(baz.min_num_arguments() <= baz.max_num_arguments());
      }
    }

    WHEN("Maximums are equal to minimums.") {
      foo.MaxArgs(foo_min);
      foo.MinArgs(foo_min);

      bar.MaxArgs(bar_min);
      bar.MinArgs(bar_min);

      baz.MaxArgs(baz_min);
      baz.MinArgs(baz_min);

      THEN("Maximums remain greater than or equal to minimums.") {
        CHECK(foo.min_num_arguments() <= foo.max_num_arguments());
        CHECK(bar.min_num_arguments() <= bar.max_num_arguments());
        CHECK(baz.min_num_arguments() <= baz.max_num_arguments());
      }
    }

    WHEN("Maximums are positive, but smaller than minimums.") {
      foo.MaxArgs(foo_min - 1);
      foo.MinArgs(foo_min);

      bar.MaxArgs(bar_min - 1);
      bar.MinArgs(bar_min);

      baz.MaxArgs(baz_min - 1);
      baz.MinArgs(baz_min);

      THEN("Maximums are set at least as large as minimums.") {
        CHECK(foo.min_num_arguments() <= foo.max_num_arguments());
        CHECK(bar.min_num_arguments() <= bar.max_num_arguments());
        CHECK(baz.min_num_arguments() <= baz.max_num_arguments());
      }
    }
  }
}

SCENARIO("Test exceptions thrown by ParameterConfiguration::MinArgs.",
         "[ParameterConfiguration][MinArgs][exceptions]") {

  GIVEN("Positional, keyword, and flag parameters, and minimum numbers.") {
    ParameterConfiguration foo{Parameter<std::string>::Positional(
        converters::StringIdentity, "foo", 1).configuration()};
    ParameterConfiguration bar{Parameter<std::string>::Keyword(
        converters::StringIdentity, {"bar"}).configuration()};
    ParameterConfiguration baz{Parameter<bool>::Flag({"baz"}).configuration()};

    THEN("Setting negative minimum number of arguments causes exception.") {
      CHECK_THROWS_AS(foo.MinArgs(-1), exceptions::ParameterConfigurationError);
      CHECK_THROWS_AS(bar.MinArgs(-2), exceptions::ParameterConfigurationError);
      CHECK_THROWS_AS(baz.MinArgs(-3), exceptions::ParameterConfigurationError);
    }
  }
}

SCENARIO("Test correctness of ParameterConfiguration::MaxArgs.",
         "[ParameterConfiguration][MaxArgs][correctness]") {

  GIVEN("Positional, keyword, and flag parameters, and maximum numbers.") {
    ParameterConfiguration foo{Parameter<std::string>::Positional(
        converters::StringIdentity, "foo", 1).configuration()};
    int foo_max = 2;

    ParameterConfiguration bar{Parameter<std::string>::Keyword(
        converters::StringIdentity, {"bar"}).configuration()};
    int bar_max = 3;

    ParameterConfiguration baz{Parameter<bool>::Flag({"baz"}).configuration()};
    int baz_max = 4;

    WHEN("Minimums are smaller than maximums.") {
      foo.MinArgs(foo_max - 1);
      int foo_min = foo.min_num_arguments();
      foo.MaxArgs(foo_max);

      bar.MinArgs(bar_max - 1);
      int bar_min = bar.min_num_arguments();
      bar.MaxArgs(bar_max);

      baz.MinArgs(baz_max - 1);
      int baz_min = baz.min_num_arguments();
      baz.MaxArgs(baz_max);

      THEN("Only maximums change.") {
        CHECK(foo.min_num_arguments() == foo_min);
        CHECK(foo.max_num_arguments() == foo_max);

        CHECK(bar.min_num_arguments() == bar_min);
        CHECK(bar.max_num_arguments() == bar_max);

        CHECK(baz.min_num_arguments() == baz_min);
        CHECK(baz.max_num_arguments() == baz_max);
      }
    }

    WHEN("Minimums equal to maximums.") {
      foo.MinArgs(foo_max);
      int foo_min = foo.min_num_arguments();
      foo.MaxArgs(foo_max);

      bar.MinArgs(bar_max);
      int bar_min = bar.min_num_arguments();
      bar.MaxArgs(bar_max);

      baz.MinArgs(baz_max);
      int baz_min = baz.min_num_arguments();
      baz.MaxArgs(baz_max);

      THEN("Only maximums change.") {
        CHECK(foo.min_num_arguments() == foo_min);
        CHECK(foo.max_num_arguments() == foo_max);

        CHECK(bar.min_num_arguments() == bar_min);
        CHECK(bar.max_num_arguments() == bar_max);

        CHECK(baz.min_num_arguments() == baz_min);
        CHECK(baz.max_num_arguments() == baz_max);
      }
    }

    WHEN("Minimums are bigger than maximums.") {
      foo.MinArgs(foo_max + 1);
      int foo_min = foo.min_num_arguments();
      foo.MaxArgs(foo_max);

      bar.MinArgs(bar_max + 1);
      int bar_min = bar.min_num_arguments();
      bar.MaxArgs(bar_max);

      baz.MinArgs(baz_max + 1);
      int baz_min = baz.min_num_arguments();
      baz.MaxArgs(baz_max);

      THEN("Minimums are set equal to maximums.") {
        CHECK_FALSE(foo.min_num_arguments() == foo_min);
        CHECK(foo.min_num_arguments() == foo_max);
        CHECK(foo.max_num_arguments() == foo_max);

        CHECK_FALSE(bar.min_num_arguments() == bar_min);
        CHECK(bar.min_num_arguments() == bar_max);
        CHECK(bar.max_num_arguments() == bar_max);

        CHECK_FALSE(baz.min_num_arguments() == baz_min);
        CHECK(baz.min_num_arguments() == baz_max);
        CHECK(baz.max_num_arguments() == baz_max);
      }
    }
  }
}

SCENARIO("Test invariant preservation by ParameterConfiguration::MaxArgs.",
         "[ParameterConfiguration][MaxArgs][invariants]") {

  GIVEN("Positional, keyword, and flag parameters, and maximum numbers.") {
    ParameterConfiguration foo{Parameter<std::string>::Positional(
        converters::StringIdentity, "foo", 1).configuration()};
    int foo_max = 2;

    ParameterConfiguration bar{Parameter<std::string>::Keyword(
        converters::StringIdentity, {"bar"}).configuration()};
    int bar_max = 3;

    ParameterConfiguration baz{Parameter<bool>::Flag({"baz"}).configuration()};
    int baz_max = 4;

    WHEN("Minimums are smaller than maximums.") {
      foo.MinArgs(foo_max - 1);
      foo.MaxArgs(foo_max);

      bar.MinArgs(bar_max - 1);
      bar.MaxArgs(bar_max);

      baz.MinArgs(baz_max - 1);
      baz.MaxArgs(baz_max);

      THEN("Maximums remain greater than or equal to minimums.") {
        CHECK(foo.min_num_arguments() <= foo.max_num_arguments());
        CHECK(bar.min_num_arguments() <= bar.max_num_arguments());
        CHECK(baz.min_num_arguments() <= baz.max_num_arguments());
      }
    }

    WHEN("Minimums are equal to maximums.") {
      foo.MinArgs(foo_max);
      foo.MaxArgs(foo_max);

      bar.MinArgs(bar_max);
      bar.MaxArgs(bar_max);

      baz.MinArgs(baz_max);
      baz.MaxArgs(baz_max);

      THEN("Maximums remain greater than or equal to minimums.") {
        CHECK(foo.min_num_arguments() <= foo.max_num_arguments());
        CHECK(bar.min_num_arguments() <= bar.max_num_arguments());
        CHECK(baz.min_num_arguments() <= baz.max_num_arguments());
      }
    }

    WHEN("Minimums are bigger than maximums.") {
      foo.MinArgs(foo_max + 1);
      foo.MaxArgs(foo_max);

      bar.MinArgs(bar_max + 1);
      bar.MaxArgs(bar_max);

      baz.MinArgs(baz_max + 1);
      baz.MaxArgs(baz_max);

      THEN("Minimums are set at most as large as maximums.") {
        CHECK(foo.min_num_arguments() <= foo.max_num_arguments());
        CHECK(bar.min_num_arguments() <= bar.max_num_arguments());
        CHECK(baz.min_num_arguments() <= baz.max_num_arguments());
      }
    }
  }
}

SCENARIO("Test exceptions thrown by ParameterConfiguration::MaxArgs.",
         "[ParameterConfiguration][MaxArgs][exceptions]") {

  GIVEN("Positional, keyword, and flag parameters, and minimum numbers.") {
    ParameterConfiguration foo{Parameter<std::string>::Positional(
        converters::StringIdentity, "foo", 1).configuration()};
    ParameterConfiguration bar{Parameter<std::string>::Keyword(
        converters::StringIdentity, {"bar"}).configuration()};
    ParameterConfiguration baz{Parameter<bool>::Flag({"baz"}).configuration()};

    THEN("Setting negative maximum number of arguments causes exception.") {
      CHECK_THROWS_AS(foo.MaxArgs(-1), exceptions::ParameterConfigurationError);
      CHECK_THROWS_AS(bar.MaxArgs(-2), exceptions::ParameterConfigurationError);
      CHECK_THROWS_AS(baz.MaxArgs(-3), exceptions::ParameterConfigurationError);
    }
  }
}

SCENARIO("Test invariant preservation by ParameterConfiguration::names.",
         "[ParameterConfiguration][names][invariants]") {

  GIVEN("Positional, keyword, and flag parameters, and names.") {
    ParameterConfiguration foo{Parameter<std::string>::Positional(
        converters::StringIdentity, "foo", 1).configuration()};
    std::vector<std::string> foo_new_names{"foo_new", "foo_another_new"};
    ParameterConfiguration bar{Parameter<std::string>::Keyword(
        converters::StringIdentity, {"b", "bar"}).configuration()};
    std::vector<std::string> bar_new_names{"bar_new"};
    ParameterConfiguration baz{Parameter<bool>::Flag(
        {"z", "baz", "bazinga"}).configuration()};
    std::vector<std::string> baz_new_names{"bazinga"};

    WHEN("New names are assigned.") {
      foo.names(std::move(foo_new_names));
      bar.names(std::move(bar_new_names));
      baz.names(std::move(baz_new_names));

      THEN("No name list is empty.") {
        CHECK_FALSE(foo.names().empty());
        CHECK_FALSE(bar.names().empty());
        CHECK_FALSE(baz.names().empty());
      }
    }
  }
}

SCENARIO("Test exceptions thrown by ParameterConfiguration::names.",
         "[ParameterConfiguration][names][exceptions]") {

  GIVEN("Positional, keyword, and flag parameters.") {
    ParameterConfiguration foo{Parameter<std::string>::Positional(
        converters::StringIdentity, "foo", 1).configuration()};
    ParameterConfiguration bar{Parameter<std::string>::Keyword(
        converters::StringIdentity, {"b", "bar"}).configuration()};
    ParameterConfiguration baz{Parameter<bool>::Flag(
        {"z", "baz", "bazinga"}).configuration()};

    THEN("Setting empty name list causes exception.") {
      CHECK_THROWS_AS(foo.names(std::vector<std::string>{}),
                      exceptions::ParameterConfigurationError);
      CHECK_THROWS_AS(bar.names(std::vector<std::string>{}),
                      exceptions::ParameterConfigurationError);
      CHECK_THROWS_AS(baz.names(std::vector<std::string>{}),
                      exceptions::ParameterConfigurationError);
    }

    THEN("Setting list containing empty names causes exception.") {
      CHECK_THROWS_AS(foo.names({"", "name2", "name3"}),
                      exceptions::ParameterConfigurationError);
      CHECK_THROWS_AS(bar.names({"name1", "", "name3"}),
                      exceptions::ParameterConfigurationError);
      CHECK_THROWS_AS(baz.names({"name1", "name2", ""}),
                      exceptions::ParameterConfigurationError);
    }
  }
}

SCENARIO("Test exceptions thrown by ParameterConfiguration::AddDefault.",
         "[ParameterConfiguration][AddDefault][exceptions]") {

  GIVEN("Positional, keyword, and flag parameters.") {
    ParameterConfiguration foo{Parameter<std::string>::Positional(
        converters::StringIdentity, "foo", 1).configuration()};
    ParameterConfiguration bar{Parameter<std::string>::Keyword(
        converters::StringIdentity, {"b", "bar"}).configuration()};
    ParameterConfiguration baz{Parameter<bool>::Flag(
        {"z", "baz", "bazinga"}).configuration()};

    THEN("Adding empty default argument causes exception.") {
      CHECK_THROWS_AS(foo.AddDefault(""),
                      exceptions::ParameterConfigurationError);
      CHECK_THROWS_AS(bar.AddDefault(std::string{}),
                      exceptions::ParameterConfigurationError);
      CHECK_THROWS_AS(baz.AddDefault(""),
                      exceptions::ParameterConfigurationError);
    }
  }
}

SCENARIO("Test exceptions thrown by ParameterConfiguration::SetDefault.",
         "[ParameterConfiguration][SetDefault][exceptions]") {

  GIVEN("Positional, keyword, and flag parameters.") {
    ParameterConfiguration foo{Parameter<std::string>::Positional(
        converters::StringIdentity, "foo", 1).configuration()};
    ParameterConfiguration bar{Parameter<std::string>::Keyword(
        converters::StringIdentity, {"b", "bar"}).configuration()};
    ParameterConfiguration baz{Parameter<bool>::Flag(
        {"z", "baz", "bazinga"}).configuration()};

    THEN("Setting empty default arguments causes exception.") {
      CHECK_THROWS_AS(foo.SetDefault({"first", "", "third"}),
                      exceptions::ParameterConfigurationError);
      CHECK_THROWS_AS(bar.SetDefault({"first", "second", std::string{}}),
                      exceptions::ParameterConfigurationError);
      CHECK_THROWS_AS(baz.SetDefault(std::vector<std::string>{""}),
                      exceptions::ParameterConfigurationError);
    }

    THEN("Setting empty default argument list does not cause exception.") {
      CHECK_NOTHROW(foo.SetDefault({}));
      CHECK_NOTHROW(bar.SetDefault(std::vector<std::string>{}));
      CHECK_NOTHROW(baz.SetDefault({}));
    }
  }
}

// Parameter tests.
SCENARIO("Test correctness of Parameter::Flag factory.",
         "[Parameter][Flag][correctness]") {

  WHEN("Arguments make sense.") {
    Parameter<bool> foo{Parameter<bool>::Flag({"foo", "f", "foo_longer"})};
    Parameter<bool> bar{Parameter<bool>::Flag({"bar"})};
    Parameter<bool> baz{Parameter<bool>::Flag({"baz", "b", "f"})};

    THEN("Configuration and converter are set appropriately.") {
      CHECK(foo.configuration().names()
            == std::vector<std::string>{"foo", "f", "foo_longer"});
      CHECK(foo.configuration().category() == ParameterCategory::kFlag);
      CHECK(foo.configuration().default_arguments().empty());
      CHECK(foo.configuration().position() == 0);
      CHECK(foo.configuration().min_num_arguments() == 0);
      CHECK(foo.configuration().max_num_arguments() == 0);
      CHECK(foo.configuration().description().empty());
      CHECK(foo.configuration().placeholder().empty());
      CHECK(foo.converter()(""));
      CHECK(foo.converter()("true"));
      CHECK(foo.converter()("placeholder"));

      CHECK(bar.configuration().names()
            == std::vector<std::string>{"bar"});
      CHECK(bar.configuration().category() == ParameterCategory::kFlag);
      CHECK(bar.configuration().default_arguments().empty());
      CHECK(bar.configuration().position() == 0);
      CHECK(bar.configuration().min_num_arguments() == 0);
      CHECK(bar.configuration().max_num_arguments() == 0);
      CHECK(bar.configuration().description().empty());
      CHECK(bar.configuration().placeholder().empty());
      CHECK(bar.converter()(""));
      CHECK(bar.converter()("true"));
      CHECK(bar.converter()("placeholder"));

      CHECK(baz.configuration().names()
            == std::vector<std::string>{"baz", "b", "f"});
      CHECK(baz.configuration().category() == ParameterCategory::kFlag);
      CHECK(baz.configuration().default_arguments().empty());
      CHECK(baz.configuration().position() == 0);
      CHECK(baz.configuration().min_num_arguments() == 0);
      CHECK(baz.configuration().max_num_arguments() == 0);
      CHECK(baz.configuration().description().empty());
      CHECK(baz.configuration().placeholder().empty());
      CHECK(baz.converter()(""));
      CHECK(baz.converter()("true"));
      CHECK(baz.converter()("placeholder"));
    }
  }

  WHEN("Duplicate names are given.") {
    Parameter<bool> foo{Parameter<bool>::Flag({"foo", "foo", "f", "foo"})};

    THEN("All occurrences of the name are included.") {
      CHECK(foo.configuration().names()
            == std::vector<std::string>{"foo", "foo", "f", "foo"});
    }
  }
}

SCENARIO("Test invariant preservation by Parameter::Flag factory.",
         "[Parameter][Flag][invariants]") {

  WHEN("Arguments make sense.") {
    Parameter<bool> foo{Parameter<bool>::Flag({"foo", "f", "foo_longer"})};
    Parameter<bool> bar{Parameter<bool>::Flag({"bar"})};
    Parameter<bool> baz{Parameter<bool>::Flag({"baz", "b", "f"})};

    THEN("Converter is not empty.") {
      CHECK(foo.converter() != nullptr);
      CHECK(bar.converter() != nullptr);
      CHECK(baz.converter() != nullptr);
    }

    THEN("Configuration does not limit number of arguments.") {
      CHECK(foo.configuration().max_num_arguments() == 0);
      CHECK(bar.configuration().max_num_arguments() == 0);
      CHECK(baz.configuration().max_num_arguments() == 0);
    }

    THEN("Configuration has at least one name assigned.") {
      CHECK_FALSE(foo.configuration().names().empty());
      CHECK_FALSE(bar.configuration().names().empty());
      CHECK_FALSE(baz.configuration().names().empty());
    }
  }

  WHEN("Duplicate names are given.") {
    Parameter<bool> foo{Parameter<bool>::Flag({"foo", "foo", "f", "foo"})};

    THEN("Converter is not empty.") {
      CHECK(foo.converter() != nullptr);
    }

    THEN("Configuration does not limit number of arguments.") {
      CHECK(foo.configuration().max_num_arguments() == 0);
    }

    THEN("Configuration has at least one name assigned.") {
      CHECK_FALSE(foo.configuration().names().empty());
    }
  }
}

SCENARIO("Test exceptions thrown by Parameter::Flag factory.",
         "[Parameter][Flag][exceptions]") {

  THEN("Providing empty name lists causes exception.") {
    CHECK_THROWS_AS(Parameter<bool>::Flag({}),
                    exceptions::ParameterConfigurationError);
    CHECK_THROWS_AS(Parameter<bool>::Flag(std::vector<std::string>{}),
                    exceptions::ParameterConfigurationError);
  }

  THEN("Providing list containing an empty string causes exception.") {
    CHECK_THROWS_AS(Parameter<bool>::Flag({"", "name2", "name3"}),
                    exceptions::ParameterConfigurationError);
    CHECK_THROWS_AS(Parameter<bool>::Flag({"name1", "", "name3"}),
                    exceptions::ParameterConfigurationError);
    CHECK_THROWS_AS(Parameter<bool>::Flag({"name1", "name2", ""}),
                    exceptions::ParameterConfigurationError);
  }
}

SCENARIO("Test correctness of Parameter::Keyword factory.",
         "[Parameter][Keyword][correctness]") {

  WHEN("Arguments make sense.") {
    Parameter<std::string> foo{Parameter<std::string>::Keyword(
        [](const std::string& s){return std::string{s}.append("_converted");},
        {"foo", "f", "foo_longer"})};
    Parameter<float> bar{Parameter<float>::Keyword(
        converters::stof, {"bar"})};
    Parameter<int> baz{Parameter<int>::Keyword(
        DummyConverter, {"baz", "b", "f"})};

    THEN("Configuration and converter are set appropriately.") {
      CHECK(foo.configuration().names()
            == std::vector<std::string>{"foo", "f", "foo_longer"});
      CHECK(foo.configuration().category()
            == ParameterCategory::kKeywordParameter);
      CHECK(foo.configuration().default_arguments().empty());
      CHECK(foo.configuration().position() == 0);
      CHECK(foo.configuration().min_num_arguments() == 0);
      CHECK(foo.configuration().max_num_arguments() == 0);
      CHECK(foo.configuration().description().empty());
      CHECK(foo.configuration().placeholder() == "<ARG>");
      CHECK(foo.converter()("arg") == "arg_converted");

      CHECK(bar.configuration().names()
            == std::vector<std::string>{"bar"});
      CHECK(bar.configuration().category()
            == ParameterCategory::kKeywordParameter);
      CHECK(bar.configuration().default_arguments().empty());
      CHECK(bar.configuration().position() == 0);
      CHECK(bar.configuration().min_num_arguments() == 0);
      CHECK(bar.configuration().max_num_arguments() == 0);
      CHECK(bar.configuration().description().empty());
      CHECK(bar.configuration().placeholder() == "<ARG>");
      CHECK(bar.converter()("1.2345") == Approx(1.2345f));

      CHECK(baz.configuration().names()
            == std::vector<std::string>{"baz", "b", "f"});
      CHECK(baz.configuration().category()
            == ParameterCategory::kKeywordParameter);
      CHECK(baz.configuration().default_arguments().empty());
      CHECK(baz.configuration().position() == 0);
      CHECK(baz.configuration().min_num_arguments() == 0);
      CHECK(baz.configuration().max_num_arguments() == 0);
      CHECK(baz.configuration().description().empty());
      CHECK(baz.configuration().placeholder() == "<ARG>");
      CHECK(baz.converter()("arg") == 2020);
    }
  }

  WHEN("Duplicate names are given.") {
    Parameter<std::string> foo{Parameter<std::string>::Keyword(
        converters::StringIdentity, {"foo", "foo", "f", "foo"})};

    THEN("All occurrences of the name are included.") {
      CHECK(foo.configuration().names()
            == std::vector<std::string>{"foo", "foo", "f", "foo"});
    }
  }
}

SCENARIO("Test invariant preservation by Parameter::Keyword factory.",
         "[Parameter][Keyword][invariants]") {

  WHEN("Arguments make sense.") {
    Parameter<std::string> foo{Parameter<std::string>::Keyword(
        [](const std::string& s){return std::string{s}.append("_converted");},
        {"foo", "f", "foo_longer"})};
    Parameter<float> bar{Parameter<float>::Keyword(
        converters::stof, {"bar"})};
    Parameter<int> baz{Parameter<int>::Keyword(
        DummyConverter, {"baz", "b", "f"})};

    THEN("Converter is not empty.") {
      CHECK(foo.converter() != nullptr);
      CHECK(bar.converter() != nullptr);
      CHECK(baz.converter() != nullptr);
    }

    THEN("Configuration does not limit number of arguments.") {
      CHECK(foo.configuration().max_num_arguments() == 0);
      CHECK(bar.configuration().max_num_arguments() == 0);
      CHECK(baz.configuration().max_num_arguments() == 0);
    }

    THEN("Configuration has at least one name assigned.") {
      CHECK_FALSE(foo.configuration().names().empty());
      CHECK_FALSE(bar.configuration().names().empty());
      CHECK_FALSE(baz.configuration().names().empty());
    }
  }

  WHEN("Duplicate names are given.") {
    Parameter<std::string> foo{Parameter<std::string>::Keyword(
        converters::StringIdentity, {"foo", "foo", "f", "foo"})};

    THEN("Converter is not empty.") {
      CHECK(foo.converter() != nullptr);
    }

    THEN("Configuration does not limit number of arguments.") {
      CHECK(foo.configuration().max_num_arguments() == 0);
    }

    THEN("Configuration has at least one name assigned.") {
      CHECK_FALSE(foo.configuration().names().empty());
    }
  }
}

SCENARIO("Test exceptions thrown by Parameter::Keyword factory.",
         "[Parameter][Keyword][exceptions]") {

  THEN("Providing empty name lists causes exception.") {
    CHECK_THROWS_AS(Parameter<float>::Keyword(converters::stof, {}),
                    exceptions::ParameterConfigurationError);
    CHECK_THROWS_AS(Parameter<int>::Keyword(DummyConverter,
                                            std::vector<std::string>{}),
                    exceptions::ParameterConfigurationError);
  }

  THEN("Providing list containing an empty string causes exception.") {
    CHECK_THROWS_AS(Parameter<std::string>::Keyword(converters::StringIdentity,
                                                    {"", "name2", "name3"}),
                    exceptions::ParameterConfigurationError);
    CHECK_THROWS_AS(Parameter<float>::Keyword(converters::stof,
                                              {"name1", "", "name3"}),
                    exceptions::ParameterConfigurationError);
    CHECK_THROWS_AS(Parameter<int>::Keyword(DummyConverter,
                                            {"name1", "name2", ""}),
                    exceptions::ParameterConfigurationError);
  }
}

SCENARIO("Test correctness of Parameter::Positional factory.",
         "[Parameter][Positional][correctness]") {

  WHEN("Arguments make sense.") {
    Parameter<std::string> foo{Parameter<std::string>::Positional(
        [](const std::string& s){return std::string{s}.append("_converted");},
        "foo", -2020)};
    Parameter<float> bar{Parameter<float>::Positional(
        converters::stof, "bar", 1)};
    Parameter<int> baz{Parameter<int>::Positional(
        DummyConverter, "baz", 2020)};

    THEN("Configuration and converter are set appropriately.") {
      CHECK(foo.configuration().names()
            == std::vector<std::string>{"foo"});
      CHECK(foo.configuration().category()
            == ParameterCategory::kPositionalParameter);
      CHECK(foo.configuration().default_arguments().empty());
      CHECK(foo.configuration().position() == -2020);
      CHECK(foo.configuration().min_num_arguments() == 0);
      CHECK(foo.configuration().max_num_arguments() == 0);
      CHECK(foo.configuration().description().empty());
      CHECK(foo.configuration().placeholder() == "foo");
      CHECK(foo.converter()("arg") == "arg_converted");

      CHECK(bar.configuration().names()
            == std::vector<std::string>{"bar"});
      CHECK(bar.configuration().category()
            == ParameterCategory::kPositionalParameter);
      CHECK(bar.configuration().default_arguments().empty());
      CHECK(bar.configuration().position() == 1);
      CHECK(bar.configuration().min_num_arguments() == 0);
      CHECK(bar.configuration().max_num_arguments() == 0);
      CHECK(bar.configuration().description().empty());
      CHECK(bar.configuration().placeholder() == "bar");
      CHECK(bar.converter()("1.2345") == Approx(1.2345f));

      CHECK(baz.configuration().names()
            == std::vector<std::string>{"baz"});
      CHECK(baz.configuration().category()
            == ParameterCategory::kPositionalParameter);
      CHECK(baz.configuration().default_arguments().empty());
      CHECK(baz.configuration().position() == 2020);
      CHECK(baz.configuration().min_num_arguments() == 0);
      CHECK(baz.configuration().max_num_arguments() == 0);
      CHECK(baz.configuration().description().empty());
      CHECK(baz.configuration().placeholder() == "baz");
      CHECK(baz.converter()("arg") == 2020);
    }
  }
}

SCENARIO("Test invariant preservation by Parameter::Positional factory.",
         "[Parameter][Positional][invariants]") {

  WHEN("Arguments make sense.") {
    Parameter<std::string> foo{Parameter<std::string>::Positional(
        [](const std::string& s){return std::string{s}.append("_converted");},
        "foo", -2020)};
    Parameter<float> bar{Parameter<float>::Positional(
        converters::stof, "bar", 1)};
    Parameter<int> baz{Parameter<int>::Positional(
        DummyConverter, "baz", 2020)};

    THEN("Converter is not empty.") {
      CHECK(foo.converter() != nullptr);
      CHECK(bar.converter() != nullptr);
      CHECK(baz.converter() != nullptr);
    }

    THEN("Configuration does not limit number of arguments.") {
      CHECK(foo.configuration().max_num_arguments() == 0);
      CHECK(bar.configuration().max_num_arguments() == 0);
      CHECK(baz.configuration().max_num_arguments() == 0);
    }

    THEN("Configuration has at least one name assigned.") {
      CHECK_FALSE(foo.configuration().names().empty());
      CHECK_FALSE(bar.configuration().names().empty());
      CHECK_FALSE(baz.configuration().names().empty());
    }
  }
}

SCENARIO("Test exceptions thrown by Parameter::Positional factory.",
         "[Parameter][Positional][exceptions]") {

  THEN("Providing empty name causes exception.") {
    CHECK_THROWS_AS(Parameter<float>::Positional(
                    converters::stof, "", 1),
                    exceptions::ParameterConfigurationError);
    CHECK_THROWS_AS(Parameter<int>::Positional(
                    DummyConverter, std::string{}, 2020),
                    exceptions::ParameterConfigurationError);
  }
}

} // namespace

} // namespace test

} // namespace arg_parse_convert