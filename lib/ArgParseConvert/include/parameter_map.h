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

#ifndef ARG_PARSE_CONVERT_PARAMETER_MAP_H_
#define ARG_PARSE_CONVERT_PARAMETER_MAP_H_

#include <any>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "exceptions.h"
#include "parameter.h"

namespace arg_parse_convert {

/// @addtogroup ArgParseConvert-Reference
///
/// @{

/// @brief A `ParameterMap` stores the configurations of parameters.
///
/// @details The different parameters stored in a `ParameterMap` are identified
///  by their respective names, or alternatively a unique integer-identifier.
///  Each parameter has a `ParameterConfiguration` object and a conversion
///  function associated with it.
///
/// @invariant The size of the object is equal to the number of
///  `ParameterConfiguration` objects stored in the map, or equivalently, is
///  equal to the number of conversion functions stored in the map.
///
/// @invariant The stored string-identifiers consist of the collection of all
///  names listed by the stored `ParameterConfiguration` objects and the
///  corresponding integers are the integer identifiers of the objects and their
///  respective associated conversion functions.
///
class ParameterMap {

 public:
  using size_type = std::vector<ParameterConfiguration>::size_type;
  /// @name Constructors:
  ///
  /// @{

  /// @brief Default constructor.
  ///
  ParameterMap() = default;

  /// @brief Copy constructor.
  ///
  ParameterMap(const ParameterMap& other) = default;

  /// @brief Move constructor.
  ///
  ParameterMap(ParameterMap&& other) noexcept = default;
  /// @}
  
  /// @name Assignment:
  ///
  /// @{
  
  /// @brief Copy assignment.
  ///
  ParameterMap& operator=(const ParameterMap& other) = default;

  /// @brief Move assignment.
  ///
  ParameterMap& operator=(ParameterMap&& other) noexcept = default;
  /// @}

  /// @name Accessors:
  ///
  /// @{

  /// @brief Returns the number of arguments stored in the object.
  ///
  /// @exceptions Strong guarantee.
  ///
  inline size_type size() const {
    return parameter_configurations_.size();
  }

  /// @brief Indicates whether object contains parameter identified by `name`.
  ///
  /// @exceptions Strong guarantee.
  ///
  inline bool Contains(const std::string& name) const {
    return (name_to_id_.count(name) > 0);
  }

  /// @brief Indicates whether object contains a flag identified by `name`.
  ///
  /// @exceptions Strong guarantee.
  ///
  inline bool IsFlag(const std::string& name) const {
    return (Contains(name) && flags_.count(GetId(name)) > 0);
  }

  /// @brief Indicates whether object contains a keyword parameter identified by
  ///  `name`.
  ///
  /// @exceptions Strong guarantee.
  ///
  inline bool IsKeyword(const std::string& name) const {
    return (Contains(name) && keyword_parameters_.count(GetId(name)) > 0);
  }

  /// @brief Returns integer-identifier for parameter with string-identifier
  ///  `name`.
  ///
  /// @exceptions Strong guarantee. Throws `exceptions::ParameterAccessError` if
  ///  object contains no parameter identified by `name`.
  ///
  inline int GetId(const std::string& name) const {
    std::stringstream error_message;
    try {
      return name_to_id_.at(name);
    } catch (const std::out_of_range& e) {
      error_message << "Unable to find parameter named: '" << name << "'.";
      throw exceptions::ParameterAccessError(error_message.str());
    }
  }

  /// @brief Returns primary string-identifier for parameter with
  ///  integer-identifier `id`.
  ///
  /// @details The primary string-identifier associated with a parameter is the
  ///  first string-identifier provided during its construction.
  ///
  /// @exceptions Strong guarantee. Throws `exceptions::ParameterAccessError` if
  ///  object contains no parameter identified by `id`.
  ///
  inline const std::string& GetPrimaryName(int id) const {
    std::stringstream error_message;
    try {
      return parameter_configurations_.at(id).names().at(0);
    } catch (const std::out_of_range& e) {
      error_message << "Unable to find parameter with id: '" << id << "'.";
      throw exceptions::ParameterAccessError(error_message.str());
    }
  }

  /// @brief Returns `ParameterConfiguration` object associated with the
  ///  parameter identified by `name`.
  ///
  /// @exceptions Strong guarantee. Throws `exceptions::ParameterAccessError` if
  ///  object contains no parameter identified by `name`.
  ///
  inline const ParameterConfiguration& GetConfiguration(
      const std::string& name) const {
    std::stringstream error_message;
    try {
      return parameter_configurations_.at(GetId(name));
    } catch (const std::out_of_range& e) {
      error_message << "Unable to find parameter with name: '" << name << "'.";
      throw exceptions::ParameterAccessError(error_message.str());
    }
  }

  /// @brief Returns `ParameterConfiguration` object associated with the
  ///  parameter identified by `id`.
  ///
  /// @exceptions Strong guarantee. Throws `exceptions::ParameterAccessError` if
  ///  object contains no parameter identified by `id`.
  ///
  inline const ParameterConfiguration& GetConfiguration(size_type id) const {
    std::stringstream error_message;
    try {
      return parameter_configurations_.at(id);
    } catch (const std::out_of_range& e) {
      error_message << "Unable to find parameter with id: '" << id << "'.";
      throw exceptions::ParameterAccessError(error_message.str());
    }
  }

  /// @brief Returns conversion function associated with the parameter
  ///  identified by `name`.
  ///
  /// @exceptions Strong guarantee. Throws `exceptions::ParameterAccessError` if
  ///  * object contains no parameter identified by `name`. 
  ///  * `ParameterType` is not the type of the parameter identified by `name`.
  ///
  template <class ParameterType>
  std::function<ParameterType(const std::string&)>
  ConversionFunction(const std::string& name) const;
  
  /// @brief Returns integer-identifiers for the contained required parameters.
  ///
  /// @exceptions Strong guarantee.
  ///
  inline const std::unordered_set<int>& required_parameters() const {
    return required_parameters_;
  }

  /// @brief Returns integer-identifiers for the contained positional parameters
  ///  ordered by their associated position.
  ///
  /// @exceptions Strong guarantee.
  ///
  inline const std::map<int, int>& positional_parameters() const {
    return positional_parameters_;
  }

  /// @brief Returns integer-identifiers for the contained keyword parameters.
  ///
  /// @exceptions Strong guarantee.
  ///
  inline const std::unordered_set<int>& keyword_parameters() const {
    return keyword_parameters_;
  }

  /// @brief Returns integer-identifiers of the contained flags.
  ///
  /// @exceptions Strong guarantee.
  ///
  inline const std::unordered_set<int>& flags() const {
    return flags_;
  }
  /// @}

  /// @name Mutators:
  ///
  /// @{

  /// @brief Inserts `parameter` into the object.
  ///
  /// @details Returns a reference to the object after `parameter` was inserted.
  ///
  /// @exceptions Strong guarantee.
  ///  * Throws `exceptions::ParameterRegistrationError` when:
  ///    - `ParameterConfiguration` member of `parameter` contains no names
  ///      (i.e. `parameter.configuration.names().empty()` == true).
  ///    - One of `parameter`'s names is already taken by another parameter
  ///      contained in the object.
  ///    - `parameter` is a positional parameter and its position is already
  ///      taken by another parameter contained in the object.
  ///  * Constructor of `parameter`'s conversion function may throw.
  ///
  template<class ParameterType>
  ParameterMap& operator()(Parameter<ParameterType> parameter);
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
  
  /// @brief Map of string-identifiers to integer-identifiers of parameters
  ///  stored in the object.
  ///
  /// @details Each parameter is identified in the other data members of the
  ///  object by its integer-identifier.
  ///
  std::unordered_map<std::string, int> name_to_id_;

  /// @brief Configurations of parameters stored in the object.
  ///
  /// @details Parameters' integer identifiers are the positions of the
  ///  associated `ParameterConfiguration` objects.
  ///
  std::vector<ParameterConfiguration> parameter_configurations_;

  /// @brief Conversion functions of parameters stored in the object.
  ///
  /// @details Parameters' integer identifiers are the positions of the
  ///  associated `ParameterConfiguration` objects.
  ///
  std::vector<std::any> converters_;

  /// @brief Contains integer-identifiers of required parameters.
  ///
  std::unordered_set<int> required_parameters_;

  /// @brief Orders integer-identifiers of positional parameters by position.
  ///
  std::map<int, int> positional_parameters_;

  /// @brief Contains integer-identifiers of keyword parameters.
  ///
  std::unordered_set<int> keyword_parameters_;

  /// @brief Contains integer-identifiers of flags.
  ///
  std::unordered_set<int> flags_;
};
/// @}

// ParameterMap::ConversionFunction()
//
template <class ParameterType>
std::function<ParameterType(const std::string&)>
ParameterMap::ConversionFunction(const std::string& name) const {
  std::stringstream error_message;
  try {
    // Note placement of * and & here to obtain const reference.
    return std::any_cast<std::function<ParameterType(const std::string&)>>(
        converters_.at(GetId(name)));
  } catch (const std::out_of_range& e) {
    error_message << "Expected the name of a parameter contained in the"
                  << " object. No parameter with name: '" << name << "' was"
                  << " found.";
    throw exceptions::ParameterAccessError(error_message.str());
  } catch (const std::bad_any_cast& e) {
    error_message << "Expected the argument to match the original `Parameter`"
                  << " object's template argument. Function was called with:"
                  << " '" << typeid(ParameterType).name() << "', but original"
                  << " `Parameter` object has conversion function type"
                  << converters_.at(GetId(name)).type().name() << "'.";
    throw exceptions::ParameterAccessError(error_message.str());
  }
}

// ParameterMap::operator()
//
template<class ParameterType>
ParameterMap& ParameterMap::operator()(Parameter<ParameterType> parameter) {
  int id{static_cast<int>(parameter_configurations_.size())};
  std::vector<std::string> names{parameter.configuration().names()};
  std::any converter{
      std::make_any<std::function<ParameterType(const std::string&)>>(
          std::move(parameter.converter()))};
  ParameterCategory parameter_category{parameter.configuration().category()};
  int parameter_position{parameter.configuration().position()};
  std::stringstream error_message;
  int other_id;

  // Preconditions.
  if (names.empty()) {
    error_message << "Parameter must be given at least one name.";
    throw exceptions::ParameterRegistrationError(error_message.str());
  }
  for (const std::string& name : names) {
    if (name_to_id_.count(name)) {
      error_message << "Name '" << name << "' already taken by another"
                    << " parameter.";
      throw exceptions::ParameterRegistrationError(error_message.str());
    }
  }
  if (parameter_category == ParameterCategory::kPositionalParameter
      && positional_parameters_.count(parameter_position)) {

    other_id = positional_parameters_.at(parameter_position);
    error_message << "Position '" << parameter_position << "' for parameter"
                  << " named '" << names.at(0)
                  << "' already taken by parameter named: '"
                  << parameter_configurations_.at(other_id).names().at(0)
                  << "'.";
    throw exceptions::ParameterRegistrationError(error_message.str());
  }

  // Reserve space to trigger exceptions before modifying
  // Move constructor of std::function isn't 'noexcept' until C++20, swap is.
  converters_.emplace_back();
  name_to_id_.reserve(name_to_id_.size() + names.size());
  parameter_configurations_.reserve(parameter_configurations_.size() + 1);
  converters_.reserve(converters_.size() + 1);
  if (parameter.configuration().IsRequired()) {
    required_parameters_.reserve(required_parameters_.size() + 1);
  }
  switch (parameter_category) {
    case ParameterCategory::kKeywordParameter: {
      keyword_parameters_.reserve(keyword_parameters_.size() + 1);
      break;
    }
    case ParameterCategory::kFlag: {
      flags_.reserve(flags_.size() + 1);
      break;
    }
    default: {
      // Positional parameters are inserted into std::map; no reserve needed.
    }
  }

  // Insert names.
  for (std::string& name : names) {
    name_to_id_.emplace(std::move(name), id);
  }
  // Insert converter.
  converters_.back().swap(converter);
  // Insert into appropriate categories.
  if (parameter.configuration().IsRequired()) {
    required_parameters_.emplace(id);
  }
  switch (parameter_category) {
    case ParameterCategory::kPositionalParameter: {
      positional_parameters_.emplace(parameter_position, id);
      break;
    }
    case ParameterCategory::kKeywordParameter: {
      keyword_parameters_.emplace(id);
      break;
    }
    case ParameterCategory::kFlag: {
      flags_.emplace(id);
      break;
    }
    default: {
      // Already handled above.
    }
  }
  // Insert configuration object.
  parameter_configurations_.emplace_back(std::move(parameter.configuration()));
  return *this;
}

} // namespace arg_parse_convert

#endif // ARG_PARSE_CONVERT_PARAMETER_MAP_H_