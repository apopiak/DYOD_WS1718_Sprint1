#pragma once

#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <set>
#include <iostream>

#include "all_type_variant.hpp"
#include "types.hpp"
#include "base_attribute_vector.hpp"
#include "type_cast.hpp"

namespace opossum {

class BaseColumn;

// Even though ValueIDs do not have to use the full width of ValueID (uint32_t), this will also work for smaller ValueID
// types (uint8_t, uint16_t) since after a down-cast INVALID_VALUE_ID will look like their numeric_limit::max()
constexpr ValueID INVALID_VALUE_ID{std::numeric_limits<ValueID::base_type>::max()};

// Dictionary is a specific column type that stores all its values in a vector
template <typename T>
class DictionaryColumn : public BaseColumn {
 public:
  /**
   * Creates a Dictionary column from a given value column.
   */
  explicit DictionaryColumn(const std::shared_ptr<BaseColumn>& base_column) {
    
    _attribute_vector = std::make_shared<std::vector<int64_t>>();

    std::set<T> sorter;
    for(size_t i = 0; i < base_column->size(); ++i) {
      sorter.insert(type_cast<T>((*base_column)[i]));
    }

    _dictionary = std::make_shared<std::vector<T>>();
    _dictionary->reserve(sorter.size());
    std::copy(sorter.begin(), sorter.end(), std::back_inserter(*_dictionary));

    for(size_t i = 0; i < base_column->size(); ++i) {
      auto it = sorter.find(type_cast<T>((*base_column)[i]));
      _attribute_vector->push_back(std::distance(sorter.begin(), it));
    } 
  }

  // SEMINAR INFORMATION: Since most of these methods depend on the template parameter, you will have to implement
  // the DictionaryColumn in this file. Replace the method signatures with actual implementations.

  // return the value at a certain position. If you want to write efficient operators, back off!
  const AllTypeVariant operator[](const size_t i) const override {
    return _dictionary->at(_attribute_vector->at(i));
  }

  // return the value at a certain position.
  const T get(const size_t i) const {
    return _dictionary->at(_attribute_vector->at(i));
  }

  // dictionary columns are immutable
  void append(const AllTypeVariant&) override {
    throw std::runtime_error("dictionary columns are immutable");
  }

  // returns an underlying dictionary
  std::shared_ptr<const std::vector<T>> dictionary() const {
    return _dictionary;
  }

  // returns an underlying data structure
  std::shared_ptr<const BaseAttributeVector> attribute_vector() const {
    return _attribute_vector;
  }

  // return the value represented by a given ValueID
  const T& value_by_value_id(ValueID value_id) const {
    return _dictionary->at(value_id);
  }

  // returns the first value ID that refers to a value >= the search value
  // returns INVALID_VALUE_ID if all values are smaller than the search value
  ValueID lower_bound(T value) const {
    for(auto it = _dictionary->begin(); it < _dictionary->end(); ++it) {
      if(*it >= value) {
        return static_cast<ValueID>(it - _dictionary->begin());
      }
    }
    return INVALID_VALUE_ID;
  }

  // same as lower_bound(T), but accepts an AllTypeVariant
  ValueID lower_bound(const AllTypeVariant& value) const {
    return lower_bound(type_cast<T>(value));
  }

  // returns the first value ID that refers to a value > the search value
  // returns INVALID_VALUE_ID if all values are smaller than or equal to the search value
  ValueID upper_bound(T value) const {
    for(auto it = _dictionary->begin(); it < _dictionary->end(); ++it) {
      if(*it > value) {
        return static_cast<ValueID>(it - _dictionary->begin());
      }
    }
    return INVALID_VALUE_ID;
  }

  // same as upper_bound(T), but accepts an AllTypeVariant
  ValueID upper_bound(const AllTypeVariant& value) const {
    return upper_bound(type_cast<T>(value));
  }

  // return the number of unique_values (dictionary entries)
  size_t unique_values_count() const {
    return _dictionary->size();
  }

  // return the number of entries
  size_t size() const override {
    return _attribute_vector->size();
  }

 protected:
  std::shared_ptr<std::vector<T>> _dictionary;
  std::shared_ptr<std::vector<int64_t>> _attribute_vector;
};

}  // namespace opossum
