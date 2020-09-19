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

#ifndef ARG_PARSE_CONVERT_PARAMETER_H_
#define ARG_PARSE_CONVERT_PARAMETER_H_

#include <sstream>
#include <string>
#include <vector>

#include "conversion_functions.h"
#include "exceptions.h"

namespace arg_parse_convert {

/// @addtogroup ArgParseConvert-Reference
///
/// @{

/// @brief Enumeration of the different parameter categories.
///
enum class ParameterCategory {
  /// @brief Can be used for any type and its arguments are detected in a list
  ///  of arguments based on their position.
  ///
  kPositionalParameter,

  /// @brief Can be used for any type and its argument list is indicated by a
  ///  preceding keyword.
  ///
  kKeywordParameter,

  /// @brief Boolean valued parameter, which is either set or not set.
  ///
  kFlag
};

/// @brief Stores various properties of a parameter relevant to a containing
///  `ParameterMap` object.
///
/// @details Instantiated via `Parameter` factory methods.
///
/// @invariant Maximum number of arguments is either 0 or greater than or equal
///  to minimum number of arguments.
///
/// @invariant Object has at least one name.
///
class ParameterConfiguration {
 public:
  /// @name Constructors:
  ///
  /// @{
  
  /// @brief Copy constructor.
  ///
  ParameterConfiguration(const ParameterConfiguration& other) = default;

  /// @brief Move constructor.
  ///
  ParameterConfiguration(ParameterConfiguration&& other) noexcept = default;
  /// @}
  
  /// @name Assignment:
  ///
  /// @{
  
  /// @brief Copy assignment.
  ///
  ParameterConfiguration& operator=(const ParameterConfiguration& other)
      = default;

  /// @brief Move assignment.
  ///
  ParameterConfiguration& operator=(ParameterConfiguration&& other) noexcept
      = default;
  /// @}

  /// @name Accessors:
  ///
  /// @{

  /// @brief Returns parameter's names.
  ///
  /// @exceptions Strong guarantee.
  ///
  inline const std::vector<std::string>& names() const {return names_;}

  /// @brief Returns the primary name.
  ///
  /// @exceptions Strong guarantee.
  ///
  inline const std::string& PrimaryName() const {return names_.at(0);}

  /// @brief Returns the category.
  ///
  /// @exceptions Strong guarantee.
  ///
  inline ParameterCategory category() const {return category_;}

  /// @brief Returns the list of default arguments.
  ///
  /// @exceptions Strong guarantee.
  ///
  inline const std::vector<std::string>& default_arguments() const {
    return default_arguments_;
  }

  /// @brief Returns the position.
  ///
  /// @exceptions Strong guarantee.
  ///
  inline int position() const {return position_;}

  /// @brief Returns the minimum number of arguments.
  ///
  /// @exceptions Strong guarantee.
  ///
  inline int min_num_arguments() const {return min_num_arguments_;}

  /// @brief Returns the maximum number of arguments.
  ///
  /// @exceptions Strong guarantee.
  ///
  inline int max_num_arguments() const {return max_num_arguments_;}

  /// @brief Indicates whether the parameter requires arguments and does not
  ///  have enough arguments by default.
  ///
  /// @details Always returns false if the parameter is a flag.
  ///
  /// @exceptions Strong guarantee.
  ///
  inline bool IsRequired() const {
    if (category_ == ParameterCategory::kFlag) {
      return false;
    } else {
      return (min_num_arguments_ > static_cast<int>(default_arguments_.size()));
    }
  }

  /// @brief Returns the parameter descriptions.
  ///
  /// @exceptions Strong guarantee.
  ///
  inline const std::string& description() const {return description_;}

  /// @brief Returns the placeholder for the parameter argument in help strings.
  ///
  /// @exceptions Strong guarantee.
  ///
  inline const std::string& placeholder() const {return argument_placeholder_;}
  /// @}

  /// @name Mutators:
  ///
  /// @{

  /// @brief Sets parameter names.
  ///
  /// @exceptions Strong guarantee.
  ///  Throws `exceptions::ParameterConfigurationError` if
  ///  * `names` is empty.
  ///  * One of the items in `names` is empty.
  ///
  inline void names(std::vector<std::string> names) {
    if (names.empty()) {
      throw exceptions::ParameterConfigurationError(
          "All parameters must be given at least one name.");
    }
    for (const std::string& name : names) {
      if (name.empty()) {
        throw exceptions::ParameterConfigurationError(
            "All parameter names must be non-empty strings.");
      }
    }
    names_ = std::move(names);
  }

  /// @brief Sets parameter category.
  ///
  /// @exceptions Strong guarantee.
  ///
  inline void category(ParameterCategory category) {category_ = category;}

  /// @brief Appends an argument to the list of default arguments.
  ///
  /// @exceptions Strong guarantee.
  ///  Throws `exceptions::ParameterConfigurationError` if `argument` is an
  ///  empty string.
  ///
  inline void AddDefault(std::string argument) {
    std::stringstream error_message;
    if (argument.empty()) {
      error_message << "Attempted to add empty default argument; (parameter"
                    << " name: '" << PrimaryName() << "').";
      throw exceptions::ParameterConfigurationError(error_message.str());
    }
    default_arguments_.emplace_back(std::move(argument));
  }
  
  /// @brief Sets the list of default arguments.
  ///
  /// @exceptions Strong guarantee.
  ///  Throws `exceptions::ParameterConfigurationError` if one of the members of
  ///  `default_arguments` is an empty string.
  ///
  inline void SetDefault(std::vector<std::string> default_arguments) {
    std::stringstream error_message;
    for (const std::string& argument : default_arguments) {
      if (argument.empty()) {
        error_message << "Attempted to assign list of default arguments"
                      << " containing an empty argument; (parameter name: '"
                      << PrimaryName() << "').";
        throw exceptions::ParameterConfigurationError(error_message.str());
      }
    }
    default_arguments_ = std::move(default_arguments);
  }

  /// @brief Sets the minimum number of arguments.
  ///
  /// @details Maximum number of arguments is set to `min` if it has a positive
  ///  value less than `min`.
  ///
  /// @exceptions Strong guarantee.
  ///  Throws `exceptions::ParameterConfigurationError` if `min` is negative.
  ///
  inline void MinArgs(int min) {
    std::stringstream error_message;
    if (min < 0) {
      error_message << "Cannot set negative minimum number of arguments: '"
                    << min << "'; (parameter name: '" << PrimaryName() << "').";
      throw exceptions::ParameterConfigurationError(error_message.str());
    }
    min_num_arguments_ = min;
    if (max_num_arguments_ > 0 && max_num_arguments_ < min) {
      max_num_arguments_ = min;
    }
  }

  /// @brief Sets the maximum number of arguments.
  ///
  /// @details Minimum number of arguments is set to `max` if it has a value
  ///  larger than `max`.
  ///
  /// @exceptions Strong guarantee.
  ///  Throws `exceptions::ParameterConfigurationError` if `max` is negative.
  ///
  inline void MaxArgs(int max) {
    std::stringstream error_message;
    if (max < 0) {
      error_message << "Cannot set negative maximum number of arguments: '"
                    << max << "'; (parameter name: '" << PrimaryName() << "').";
      throw exceptions::ParameterConfigurationError(error_message.str());
    }
    max_num_arguments_ = max;
    if (min_num_arguments_ > max) {
      min_num_arguments_ = max;
    }
  }

  /// @brief Sets the description.
  ///
  /// @exception Strong guarantee.
  ///
  inline void description(std::string description) {
    description_ = std::move(description);
  }

  /// @brief Sets the argument placeholder for help string generation.
  ///
  /// @exception Strong guarantee.
  ///
  inline void placeholder(std::string placeholder) {
    argument_placeholder_ = std::move(placeholder);
  }
  /// @}

  /// @name Other:
  ///
  /// @{
  
  /// @brief Compares the object to `other`.
  ///
  inline bool operator==(const ParameterConfiguration& other) const {
    return (names_ == other.names_
            && category_ == other.category_
            && default_arguments_ == other.default_arguments_
            && position_ == other.position_
            && min_num_arguments_ == other.min_num_arguments_
            && max_num_arguments_ == other.max_num_arguments_
            && description_ == other.description_
            && argument_placeholder_ == other.argument_placeholder_);
  }
  /// @}

  /// @name Other:
  ///
  /// @{
  
  /// @brief Returns a string describing the object's state.
  ///
  /// @exceptions Strong guarantee.
  ///
  std::string DebugString() const;
  /// @}
 private:
  /// @brief `Parameter` class wraps around a `ParameterConfiguration` to
  ///  provide a conversion function along with the configuration.
  ///
  template <class ParameterType>
  friend class Parameter;

  /// @brief Private default constructor to force creation by friend class.
  ///
  ParameterConfiguration() = default;

  /// @brief Parameter's string-identifiers.
  ///
  std::vector<std::string> names_;

  /// @brief Parameter category determines how parameter is treated by various
  ///  functions.
  ///
  ParameterCategory category_;

  /// @brief Parameter may have default arguments.
  ///
  std::vector<std::string> default_arguments_;

  /// @brief Position is only relevant if parameter is a positional parameter.
  ///
  int position_{0};

  /// @brief Minimum number of arguments the parameter takes. If positive, and less than
  /// the number of default arguments given, the parameter is considered a
  /// required parameter.
  ///
  int min_num_arguments_{0};

  /// @brief Maximum number of arguments the parameter takes.
  ///
  int max_num_arguments_{0};

  /// @brief Description included in the parameter's help-string.
  ///
  std::string description_;

  /// @brief Placeholder for parameter's argument in its help-string.
  ///
  std::string argument_placeholder_{"<ARG>"};
};

/// @brief Bundles `ParameterConfiguration` object with conversion function and
///  provides factories.
///
/// @invariant Conversion function member is not empty.
///
template <class ParameterType>
class Parameter {
 public:
  /// @name Factories:
  ///
  /// @{

  /// @brief Creates a flag identified by `names`.
  ///
  /// @exceptions Strong guarantee.
  ///  Throws `exceptions::ParameterConfigurationError` if `names` is empty.
  ///
  static Parameter<bool> Flag(std::vector<std::string> names);

  /// @brief Creates a keyword parameter identified by `names`, with provided
  ///  conversion function.
  ///
  /// @exceptions Strong guarantee.
  ///  Throws `exceptions::ParameterConfigurationError` if `names` is empty.
  ///
  static Parameter<ParameterType> Keyword(
      std::function<ParameterType(const std::string&)> converter,
      std::vector<std::string> names);

  /// @brief Creates a positional parameter identified by `name`, with provided
  ///  relative position and conversion function.
  ///
  /// @exceptions Strong guarantee.
  ///  Throws `exceptions::ParameterConfigurationError` if `name` is empty.
  ///
  static Parameter<ParameterType> Positional(
      std::function<ParameterType(const std::string&)> converter,
      std::string name, int position);
  /// @}

  /// @name Constructors:
  ///
  /// @{

  /// @brief Copy constructor.
  ///
  Parameter(const Parameter& other) = default;

  /// @brief Move constructor.
  ///
  Parameter(Parameter&& other) = default;
  /// @}
  
  /// @name Assignment:
  ///
  /// @{
  
  /// @brief Copy assignment.
  ///
  Parameter& operator=(const Parameter& other) = default;

  /// @brief Move assignment.
  ///
  Parameter& operator=(Parameter&& other) = default;
  /// @}

  /// @name Accessors:
  ///
  /// @{
  
  /// @brief Returns the parameter's configuration.
  ///
  /// @exceptions Strong guarantee.
  ///
  inline const ParameterConfiguration& configuration() const {
    return configuration_;
  }

  /// @brief Returns the parameter's converter.
  ///
  /// @exceptions Strong guarantee.
  ///
  inline const std::function<ParameterType(const std::string&)>&
  converter() const {
    return converter_;
  }
  /// @}

  /// @name Mutators:
  ///
  /// @{

  /// @brief Appends an argument to the parameter's list of default arguments.
  ///
  /// @exceptions Strong guarantee.
  ///  Throws `exceptions::ParameterConfigurationError` if `argument` is an
  ///  empty string.
  ///
  inline Parameter<ParameterType>& AddDefault(std::string argument) {
    configuration_.AddDefault(std::move(argument));
    return *this;
  }
  
  /// @brief Sets the parameter's list of default arguments.
  ///
  /// @exceptions Strong guarantee.
  ///  Throws `exceptions::ParameterConfigurationError` if one of the members of
  ///  `default_arguments` is an empty string.
  ///
  inline Parameter<ParameterType>& SetDefault(
      std::vector<std::string> default_arguments) {
    configuration_.SetDefault(std::move(default_arguments));
    return *this;
  }

  /// @brief Sets the minimum number of arguments.
  ///
  /// @details Maximum number of arguments is set to `min` if it has a positive
  ///  value less than `min`.
  ///
  /// @exceptions Strong guarantee.
  ///  Throws `exceptions::ParameterConfigurationError` if `min` is negative.
  ///
  inline Parameter<ParameterType>& MinArgs(int min) {
    configuration_.MinArgs(min);
    return *this;
  }

  /// @brief Sets the maximum number of arguments.
  ///
  /// @details Minimum number of arguments is set to `max` if it has a value
  ///  larger than `max`.
  ///
  /// @exceptions Strong guarantee.
  ///  Throws `exceptions::ParameterConfigurationError` if `max` is negative.
  ///
  inline Parameter<ParameterType>& MaxArgs(int max) {
    configuration_.MaxArgs(max);
    return *this;
  }

  /// @brief Sets the parameter's description.
  ///
  /// @exception Strong guarantee.
  ///
  inline Parameter<ParameterType>& Description(std::string description) {
    configuration_.description(std::move(description));
    return *this;
  }

  /// @brief Sets the argument's placeholder in the parameter's help string.
  ///
  /// @exception Strong guarantee.
  ///
  inline Parameter<ParameterType>& Placeholder(std::string placeholder) {
    configuration_.placeholder(std::move(placeholder));
    return *this;
  }
  /// @}

  /// @name Other:
  ///
  /// @{
  
  /// @brief Returns a string describing the object's state.
  ///
  /// @exceptions Strong guarantee.
  ///
  std::string DebugString() const;
  /// @}
 private:
  // Used only in factories to ensure proper initialization.
  Parameter() = default;

  /// @brief Returns a `Parameter` object with the given configuration, and
  ///  conversion function.
  ///
  /// @exceptions Strong guarantee.
  ///  Throws `exceptions::ParameterConfigurationError` if `names` is empty.
  ///
  static Parameter<ParameterType> Create(
      ParameterConfiguration configuration,
      std::function<ParameterType(const std::string&)> converter,
      std::vector<std::string> names);

  /// @brief Contains all information except conversion function.
  ///
  ParameterConfiguration configuration_;

  /// @brief The parameter's conversion function.
  ///
  std::function<ParameterType(const std::string&)> converter_;
};
/// @}

// Parameter::Create
//
template <class ParameterType>
Parameter<ParameterType> Parameter<ParameterType>::Create(
    ParameterConfiguration configuration,
    std::function<ParameterType(const std::string&)> converter,
    std::vector<std::string> names) {
  Parameter<ParameterType> parameter;
  parameter.configuration_ = std::move(configuration);
  parameter.configuration_.names(std::move(names));
  parameter.converter_ = std::move(converter);
  return parameter;
}

// Parameter::Flag
//
template <>
inline Parameter<bool> Parameter<bool>::Flag(std::vector<std::string> names) {
  Parameter<bool> result;
  ParameterConfiguration configuration;
  configuration.category_ = ParameterCategory::kFlag;
  configuration.argument_placeholder_.clear();
  result = Parameter<bool>::Create(std::move(configuration),
                                   converters::FlagConverter,
                                   std::move(names));
  return result;
}

// Parameter::Keyword
//
template <class ParameterType>
Parameter<ParameterType> Parameter<ParameterType>::Keyword(
    std::function<ParameterType(const std::string&)> converter,
    std::vector<std::string> names) {
  Parameter<ParameterType> result;
  ParameterConfiguration configuration;
  configuration.category_ = ParameterCategory::kKeywordParameter;
  result = Parameter<ParameterType>::Create(std::move(configuration),
                                            std::move(converter),
                                            std::move(names));
  return result;
}

// Parameter::Positional
//
template <class ParameterType>
Parameter<ParameterType> Parameter<ParameterType>::Positional(
    std::function<ParameterType(const std::string&)> converter,
    std::string name, int position) {
  Parameter<ParameterType> result;
  ParameterConfiguration configuration;
  configuration.category_ = ParameterCategory::kPositionalParameter;
  configuration.position_ = position;
  configuration.placeholder(name);
  result = Parameter<ParameterType>::Create(
      std::move(configuration), std::move(converter),
      std::vector<std::string>{std::move(name)});
  return result;
}

// Parameter::DebugString
//
template <class ParameterType>
std::string Parameter<ParameterType>::DebugString() const {
  std::stringstream ss;
  ss << "{configuration: "
     << configuration_.DebugString()
     << ", has converter: " << std::boolalpha
     << static_cast<bool>(converter_) << "}.";
  return ss.str();
}

} // namespace arg_parse_convert

#endif // ARG_PARSE_CONVERT_PARAMETER_H_