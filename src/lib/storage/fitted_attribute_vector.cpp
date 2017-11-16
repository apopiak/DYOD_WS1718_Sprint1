#include <limits>

#include "fitted_attribute_vector.hpp"
#include "utils/assert.hpp"

namespace opossum {

template <typename T>
ValueID FittedAttributeVector<T>::get(const size_t i) const {
  return ValueID(_attribute_vector.at(i));
}

template <typename T>
void FittedAttributeVector<T>::set(const size_t i, const ValueID value_id) {
  DebugAssert(value_id.t <= std::numeric_limits<T>::max(), "Value out of range: " + std::to_string(value_id));
  DebugAssert(i <= _attribute_vector.size(), "Index out of range: " + std::to_string(i));
  _attribute_vector.insert(_attribute_vector.cbegin() + i, static_cast<T>(value_id));
}

template <typename T>
size_t FittedAttributeVector<T>::size() const {
  return _attribute_vector.size();
}

template <typename T>
AttributeVectorWidth FittedAttributeVector<T>::width() const {
  return sizeof(T);
}

template class FittedAttributeVector<uint32_t>;
template class FittedAttributeVector<uint16_t>;
template class FittedAttributeVector<uint8_t>;

}  // namespace opossum
