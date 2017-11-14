#pragma once

#include <vector>

#include "base_attribute_vector.hpp"
#include "types.hpp"

namespace opossum {

template <typename T>
class FittedAttributeVector : public BaseAttributeVector {
 public:
  // returns the value at a given position
  ValueID get(const size_t i) const override;

  // inserts the value_id at a given position
  void set(const size_t i, const ValueID value_id) override;

  // returns the number of values
  size_t size() const override;

  // returns the width of the values in bytes
  AttributeVectorWidth width() const override;

 protected:
  std::vector<T> _attribute_vector;
};

}  // namespace opossum
