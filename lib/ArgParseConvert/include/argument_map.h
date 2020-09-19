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

#ifndef ARG_PARSE_CONVERT_ARGUMENT_MAP_H_
#define ARG_PARSE_CONVERT_ARGUMENT_MAP_H_

#include <any>
#include <string>
#include <vector>

#include "exceptions.h"
#include "parameter.h"
#include "parameter_map.h"

namespace arg_parse_convert {

/// @addtogroup ArgParseConvert-Reference
///
/// @{

/// @brief Stores arguments for an internal `ParameterMap` member's parameters
///  and allows for retrieving the parameter values.
///
/// @invariant Number of stored argument lists is always the same as the size of
///  the `ParameterMap` member.
///
/// @invariant Number of stored value lists is always the same as the size of
///  the `ParameterMap` member.
///
class ArgumentMap {
 public:
  using size_type = std::vector<std::vector<std::string>>::size_type;
  /// @name Constructors:
  ///
  /// @{

  /// @brief Constructs object with copy of `parameters`.
  ///
  ArgumentMap(ParameterMap&& parameters)
      : parameters_{parameters},
        arguments_{parameters_.size(), std::vector<std::string>{}},
        value_lists_{parameters_.size(), std::vector<std::any>{}} {}

  /// @brief Copy constructor.
  ///
  ArgumentMap(const ArgumentMap& other) = default;

  /// @brief Move constructor.
  ///
  ArgumentMap(ArgumentMap&& other) noexcept = default;
  /// @}
  
  /// @name Assignment:
  ///
  /// @{
  
  /// @brief Copy assignments.
  ///
  ArgumentMap& operator=(const ArgumentMap& other) = default;

  /// @brief Move assignment.
  ///
  ArgumentMap& operator=(ArgumentMap&& other) noexcept = default;
  /// @}

  /// @name Mutators:
  ///
  /// @{
  
  /// @brief Sets default argument lists for non-flag parameters lacking
  ///  arguments.
  ///
  /// @details Default argument lists are set for each parameter that is not a
  ///  flag and has an empty argument list.
  ///
  /// @exceptions Basic guarantee.
  ///
  inline void SetDefaultArguments() {
    for (int i = 0; i < static_cast<int>(arguments_.size()); ++i) {
      if (parameters_.GetConfiguration(i).category() != ParameterCategory::kFlag
          && arguments_.at(i).size() == 0) {
        arguments_.at(i) = parameters_.GetConfiguration(i).default_arguments();
      }
    }
  }

  /// @brief Adds an argument to the list of arguments of the parameter
  ///  identified by `name`.
  ///
  /// @details Does nothing if list of arguments is already full.
  ///
  /// @exceptions Strong guarantee. Throws `exceptions::ParameterAccessError` if
  ///  object's `ParameterMap` member contains no parameter identified by
  ///  `name`.
  ///
  inline void AddArgument(const std::string& name, std::string arg) {
    int id{parameters_.GetId(name)},
        max_num_args{parameters_.GetConfiguration(name).max_num_arguments()};
    if (max_num_args == 0
        || static_cast<int>(arguments_.at(id).size()) < max_num_args) {
      arguments_.at(parameters_.GetId(name)).emplace_back(std::move(arg));
    }
  }
  /// @}

  /// @name Accessors:
  ///
  /// @{

  /// @brief Returns the number of argument lists stored in the object.
  ///
  /// @exceptions Strong guarantee.
  ///
  inline size_type size() const {return arguments_.size();}

  /// @brief Returns the `ParameterMap` member of the object.
  ///
  /// @exceptions Strong guarantee.
  ///
  inline const ParameterMap& Parameters() const {return parameters_;}

  /// @brief Returns the lists of arguments for the parameters.
  ///
  /// @details The position of the argument list of a parameter in the returned
  ///  vector is its id.
  ///
  /// @exceptions Strong guarantee.
  ///
  inline const std::vector<std::vector<std::string>>& Arguments() const {
    return arguments_;
  }

  /// @brief Returns the lists of values for the parameters.
  ///
  /// @details The position of the argument list of a parameter in the returned
  ///  vector is its id. Argument lists are only filled up to the positions for
  ///  which `GetValue` was called, or filled all the way if `GetAllValues` was
  ///  called for the corresponding paramters.
  ///
  /// @exceptions Strong guarantee.
  ///
  inline const std::vector<std::vector<std::any>>& Values() const {
    return value_lists_;
  }

  /// @brief Returns list of unfilled parameters.
  ///
  /// @details The primary string-identifier of each unfilled parameter is
  ///  returned. A parameter is unfilled if the number of arguments parsed or
  ///  set by default is less than the minimum number of arguments the parameter
  ///  expects.
  ///
  /// @exceptions Strong guarantee.
  ///
  inline std::vector<std::string> GetUnfilledParameters() const {
    std::vector<std::string> result;
    for (int i = 0; i < static_cast<int>(parameters_.size()); ++i) {
      if (parameters_.GetConfiguration(i).min_num_arguments()
          > static_cast<int>(arguments_.at(i).size())) {
        result.emplace_back(parameters_.GetPrimaryName(i));
      }
    }
    return result;
  }

  /// @brief Indicates whether an argument was assigned to parameter identified
  ///  by `name`.
  ///
  /// @details Returns true if an argument was parsed for the parameter, or set
  ///  by default.
  ///
  /// @exceptions Strong guarantee. Throws `exceptions::ParameterAccessError` if
  ///  object's `ParameterMap` member contains no parameter identified by
  ///  `name`.
  ///
  inline bool HasArgument(const std::string& name) const {
    return (arguments_.at(parameters_.GetId(name)).size() > 0);
  }

  /// @brief Returns the arguments assigned to parameter identified by `name`.
  ///
  /// @exceptions Strong guarantee. Throws `exceptions::ParameterAccessError` if
  ///  object's `ParameterMap` member contains no parameter identified by
  ///  `name`.
  ///
  inline const std::vector<std::string>&
  ArgumentsOf(const std::string& name) const {
    return (arguments_.at(parameters_.GetId(name)));
  }

  /// @brief Returns value of parameter.
  ///
  /// @details Returned value is the value of conversion function associated
  ///  with parameter identified by `name` with type `ParameterType` evaluated
  ///  at its first parsed argument. If `pos` is specified, the conversion
  ///  function is evaluated at the argument at that position instead.
  ///
  /// @exceptions Strong guarantee.
  ///  * Throws `exceptions::ParameterAccessError` if
  ///    - object's `ParameterMap` member contains no parameter identified by
  ///      `name`.
  ///    - `ParameterType` is not the type of the parameter identified by
  ///      `name`.
  ///  * Throws `exceptions::ValueAccessError` if
  ///    - parameter identified by `name` was not assigned conversion function.
  ///    - `pos` is greater than or equal to the size of the argument list of
  ///      parameter identified by `name` (i.e. argument list is empty if `pos`
  ///      unspecified).
  ///    - `name` identifies a flag.
  ///  * Conversion function may throw.
  ///
  template <class ParameterType>
  ParameterType GetValue(const std::string& name, int pos = 0);

  /// @brief Returns list of values of parameter.
  ///
  /// @details Returned list is list of values of conversion function associated
  ///  with parameter identified by `name` with type `ParameterType` evaluated
  ///  at each argument in its argument list. Returned list is empty if argument
  ///  list is empty.
  ///
  /// @exceptions Strong guarantee.
  ///  * Throws `exceptions::ParameterAccessError` if
  ///    - object's `ParameterMap` member contains no parameter identified by
  ///      `name`.
  ///    - `ParameterType` is not the type of the parameter identified by
  ///      `name`.
  ///  * Throws `exceptions::ValueAccessError` if
  ///    - parameter identified by `name` was not assigned conversion function.
  ///    - `name` identifies a flag.
  ///  * Conversion function may throw.
  ///
  template <class ParameterType>
  std::vector<ParameterType> GetAllValues(const std::string& name);

  /// @brief Returns whether or not the flag is set.
  ///
  /// @details Returns true if flag identified by `name` is set and false
  ///  otherwise.
  ///
  /// @exceptions Strong guarantee.
  ///  * Throws `exceptions::ParameterAccessError` if object's `ParameterMap`
  ///    member contains no parameter identified by `name`.
  ///  * Throws `exceptions::ValueAccessError` if `name` identifies a parameter
  ///    that is not a flag.
  ///
  inline bool IsSet(const std::string& name) const {
    std::stringstream error_message;
    if (parameters_.GetConfiguration(name).category()
        != ParameterCategory::kFlag) {
      error_message << "Parameter with name: '" << name << "' is not a flag."
                    << " Call `ArgumentMap::IsSet` only to check if a flag is"
                    << " set.";
      throw exceptions::ValueAccessError(error_message.str());
    }
    return (arguments_.at(parameters_.GetId(name)).size() > 0);
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
  friend std::vector<std::string> ParseArgs(int argc, const char** argv,
                                            ArgumentMap& arguments);
  friend std::vector<std::string> ParseFile(std::istream& config_is,
                                            ArgumentMap& arguments);
  /// @brief `ParameterMap` object associated with the object.
  ///
  ParameterMap parameters_;

  /// @brief Lists of arguments of parameters stored in the object.
  ///
  /// @details Parameters' integer identifiers are the positions of the
  ///  associated `ParameterConfiguration` objects in `parameters_`.
  ///
  std::vector<std::vector<std::string>> arguments_;

  /// @brief Lists of values of parameters stored in the object.
  ///
  /// @details Parameters' integer identifiers are the positions of the
  ///  associated `ParameterConfiguration` objects in `parameters_`. Each
  ///  parameter's values are stored in this list when call to `GetValue`, or
  ///  `GetValues` is made.
  ///
  std::vector<std::vector<std::any>> value_lists_;
};
/// @}

// ArgumentMap::GetValue
//
template <class ParameterType>
ParameterType ArgumentMap::GetValue(const std::string& name, int pos) {
  int id{parameters_.GetId(name)};
  std::function<ParameterType(const std::string&)> converter{
      parameters_.ConversionFunction<ParameterType>(name)};
  std::stringstream error_message;

  // Test if argument at position `pos` was assigned.
  if (static_cast<int>(arguments_.at(id).size()) <= pos) {
    error_message << "Attempted to access argument at position '" << pos
                  << "' for parameter named '" << name << "' but only '"
                  << arguments_.at(id).size()
                  << "' arguments were assigned.";
    throw exceptions::ValueAccessError(error_message.str());
  }
  // Test if parameter has a conversion function.
  if (converter == nullptr) {
    error_message << "Parameter identified by '" << name << "' has no"
                  << " conversion function associated with it.";
    throw exceptions::ValueAccessError(error_message.str());
  }
  // Test if `name` is a flag.
  if (parameters_.flags().count(id)) {
    error_message << "Attempted to use `ArgumentMap::GetValue` to check if flag"
                     " named: '" <<  name << "' was set. Use"
                     " `ArgumentMap::IsSet` to test flag values.";
    throw exceptions::ValueAccessError(error_message.str());
  }

  // Compute value only if it wasn't computed before.
  if (static_cast<int>(value_lists_.at(id).size()) <= pos
      || !value_lists_.at(id).at(pos).has_value()) {
    value_lists_.at(id).reserve(pos);
    for (int i = value_lists_.at(id).size(); i < pos; ++i) {
      value_lists_.at(id).emplace_back();
    }
    value_lists_.at(id).emplace_back(converter(arguments_.at(id).at(pos)));
  }
  return std::any_cast<ParameterType>(value_lists_.at(id).at(pos));
}

// ArgumentMap::GetAllValues
//
template <class ParameterType>
std::vector<ParameterType> ArgumentMap::GetAllValues(const std::string& name) {
  std::vector<ParameterType> result;
  int id{parameters_.GetId(name)};
  std::function<ParameterType(const std::string&)> converter{
      parameters_.ConversionFunction<ParameterType>(name)};
  std::stringstream error_message;

  // Test if parameter has a conversion function.
  if (converter == nullptr) {
    error_message << "Parameter identified by '" << name << "' has no"
                  << " conversion function associated with it.";
    throw exceptions::ValueAccessError(error_message.str());
  }
  // Test if `name` is a flag.
  if (parameters_.flags().count(id)) {
    error_message << "Attempted to use `ArgumentMap::GetValue` to check if flag"
                     " named: '" <<  name << "' was set. Use"
                     " `ArgumentMap::IsSet` to test flag values.";
    throw exceptions::ValueAccessError(error_message.str());
  }

  for (int pos = 0; pos < static_cast<int>(arguments_.at(id).size()); ++pos) {
    result.push_back(GetValue<ParameterType>(name, pos));
  }
  return result;
}

} // namespace arg_parse_convert

#endif // ARG_PARSE_CONVERT_ARGUMENT_MAP_H_