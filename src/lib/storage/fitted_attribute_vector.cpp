#include <limits>

#include "fitted_attribute_vector.hpp"

namespace opossum {

template <typename T>
ValueID FittedAttributeVector<T>::get(const size_t i) const {
  return static_cast<ValueID>(_attribute_vector.at(i));
}

template <typename T>
void FittedAttributeVector<T>::set(const size_t i, const ValueID value_id) {
  if (value_id > std::numeric_limits<T>::max()) {
    throw std::invalid_argument("Value out of range: " + std::to_string(value_id));
  } else if (i == _attribute_vector.size()) {
    _attribute_vector.push_back(static_cast<T>(value_id));
  } else if (i >= 0 && i < _attribute_vector.size()) {
    const auto it = _attribute_vector.cbegin() + i;
    _attribute_vector.insert(it, static_cast<T>(value_id));
  } else {
    throw std::out_of_range("Index out of range: " + std::to_string(i));
  }
}

template <typename T>
size_t FittedAttributeVector<T>::size() const {
  return _attribute_vector.size();
}

template <typename T>
AttributeVectorWidth FittedAttributeVector<T>::width() const {
  return sizeof(T);
}

template class FittedAttributeVector<uint64_t>;
template class FittedAttributeVector<uint32_t>;
template class FittedAttributeVector<uint16_t>;
template class FittedAttributeVector<uint8_t>;

}  // namespace opossum
