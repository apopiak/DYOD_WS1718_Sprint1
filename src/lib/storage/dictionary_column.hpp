#pragma once

#include <iostream>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <limits>
#include <algorithm>

#include "all_type_variant.hpp"
#include "base_attribute_vector.hpp"
#include "fitted_attribute_vector.hpp"
#include "type_cast.hpp"
#include "types.hpp"
#include "value_column.hpp"

namespace opossum {

class BaseColumn;

// Even though ValueIDs do not have to use the full width of ValueID (uint32_t), this will also work for smaller ValueID
// types (uint8_t, uint16_t) since after a down-cast INVALID_VALUE_ID will look like their numeric_limit::max()
constexpr ValueID INVALID_VALUE_ID{std::numeric_limits<ValueID::base_type>::max()};

// Dictionary is a specific column type that stores all its values in a vector
template <typename T>
class DictionaryColumn : public BaseColumn {
 public:
  DictionaryColumn() = delete;

  /**
   * Creates a Dictionary column from a given value column.
   */
  explicit DictionaryColumn(const std::shared_ptr<BaseColumn>& base_column)
      : _dictionary(nullptr), _attribute_vector(nullptr) {
    auto value_column = std::dynamic_pointer_cast<ValueColumn<T>>(base_column);
    DebugAssert(value_column != nullptr, "Type of ValueColumn does not match type of DictionaryColumn.");

    const std::vector<T>& values = value_column->values();
    std::set<T> sorter(values.cbegin(), values.end());

    _dictionary = std::make_shared<std::vector<T>>();
    _dictionary->reserve(sorter.size());
    std::copy(sorter.begin(), sorter.end(), std::back_inserter(*_dictionary));

    _attribute_vector = _create_attribute_vector(sorter.size());

    for (size_t i = 0; i < values.size(); ++i) {
      auto it = sorter.find(values.at(i));
      _attribute_vector->set(_attribute_vector->size(), static_cast<ValueID>(std::distance(sorter.begin(), it)));
    }
  }

  // SEMINAR INFORMATION: Since most of these methods depend on the template parameter, you will have to implement
  // the DictionaryColumn in this file. Replace the method signatures with actual implementations.

  // return the value at a certain position. If you want to write efficient operators, back off!
  const AllTypeVariant operator[](const size_t i) const override { return _dictionary->at(_attribute_vector->get(i)); }

  // return the value at a certain position.
  const T get(const size_t i) const { return _dictionary->at(_attribute_vector->get(i)); }

  // dictionary columns are immutable
  void append(const AllTypeVariant&) override { throw std::runtime_error("dictionary columns are immutable"); }

  // returns an underlying dictionary
  std::shared_ptr<const std::vector<T>> dictionary() const { return _dictionary; }

  // returns an underlying data structure
  std::shared_ptr<const BaseAttributeVector> attribute_vector() const { return _attribute_vector; }

  // return the value represented by a given ValueID
  const T& value_by_value_id(ValueID value_id) const { return _dictionary->at(value_id); }

  // returns the first value ID that refers to a value >= the search value
  // returns INVALID_VALUE_ID if all values are smaller than the search value
  ValueID lower_bound(T value) const {
    for (auto it = _dictionary->begin(); it < _dictionary->end(); ++it) {
      if (*it >= value) {
        return static_cast<ValueID>(it - _dictionary->begin());
      }
    }
    return INVALID_VALUE_ID;
  }

  // same as lower_bound(T), but accepts an AllTypeVariant
  ValueID lower_bound(const AllTypeVariant& value) const { return lower_bound(type_cast<T>(value)); }

  // returns the first value ID that refers to a value > the search value
  // returns INVALID_VALUE_ID if all values are smaller than or equal to the search value
  ValueID upper_bound(T value) const {
    for (auto it = _dictionary->begin(); it < _dictionary->end(); ++it) {
      if (*it > value) {
        return static_cast<ValueID>(it - _dictionary->begin());
      }
    }
    return INVALID_VALUE_ID;
  }

  // same as upper_bound(T), but accepts an AllTypeVariant
  ValueID upper_bound(const AllTypeVariant& value) const { return upper_bound(type_cast<T>(value)); }

  // return the number of unique_values (dictionary entries)
  size_t unique_values_count() const { return _dictionary->size(); }

  // return the number of entries
  size_t size() const override { return _attribute_vector->size(); }

 protected:
  std::shared_ptr<std::vector<T>> _dictionary;
  std::shared_ptr<BaseAttributeVector> _attribute_vector;

  std::shared_ptr<BaseAttributeVector> _create_attribute_vector(uint64_t size) {
    if (size > std::numeric_limits<uint32_t>::max()) {
      return std::make_shared<FittedAttributeVector<uint64_t>>();
    } else if (size > std::numeric_limits<uint16_t>::max()) {
      return std::make_shared<FittedAttributeVector<uint32_t>>();
    } else if (size > std::numeric_limits<uint8_t>::max()) {
      return std::make_shared<FittedAttributeVector<uint16_t>>();
    } else {
      return std::make_shared<FittedAttributeVector<uint8_t>>();
    }
  }
};

}  // namespace opossum
