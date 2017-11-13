#include <limits>
#include <string>
#include <vector>

#include "../base_test.hpp"
#include "gtest/gtest.h"

#include "../../lib/storage/fitted_attribute_vector.hpp"
#include "../../lib/types.hpp"

namespace opossum {

class StorageFittedAttributeVectorTest : public BaseTest {
 protected:
  FittedAttributeVector<uint8_t> fav;
};

TEST_F(StorageFittedAttributeVectorTest, Set) {
  EXPECT_EQ(fav.width(), sizeof(uint8_t));
  fav.set(0, ValueID{1});
  fav.set(0, ValueID{1});
  EXPECT_EQ(fav.size(), 2u);
  EXPECT_THROW(fav.set(2, static_cast<ValueID>(std::numeric_limits<uint16_t>::max())), std::exception);
  EXPECT_THROW(fav.set(5, ValueID{1}), std::exception);
}

}  // namespace opossum
