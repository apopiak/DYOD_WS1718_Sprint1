#include <memory>
#include <string>
#include <iostream>
#include <sstream>

#include "gtest/gtest.h"

#include "../../lib/resolve_type.hpp"
#include "../../lib/type_cast.hpp"
#include "../../lib/storage/base_column.hpp"
#include "../../lib/storage/dictionary_column.hpp"
#include "../../lib/storage/value_column.hpp"
#include "../../lib/storage/fitted_attribute_vector.hpp"

class StorageDictionaryColumnTest : public ::testing::Test {
 protected:
  std::shared_ptr<opossum::ValueColumn<int>> vc_int = std::make_shared<opossum::ValueColumn<int>>();
  std::shared_ptr<opossum::ValueColumn<std::string>> vc_str = std::make_shared<opossum::ValueColumn<std::string>>();
};

TEST_F(StorageDictionaryColumnTest, CompressColumnString) {
  vc_str->append("Bill");
  vc_str->append("Steve");
  vc_str->append("Alexander");
  vc_str->append("Steve");
  vc_str->append("Hasso");
  vc_str->append("Bill");

  auto col = opossum::make_shared_by_column_type<opossum::BaseColumn, opossum::DictionaryColumn>("string", vc_str);
  auto dict_col = std::dynamic_pointer_cast<opossum::DictionaryColumn<std::string>>(col);

  // Test attribute_vector size
  EXPECT_EQ(dict_col->size(), 6u);

  // Test dictionary size (uniqueness)
  EXPECT_EQ(dict_col->unique_values_count(), 4u);

  // Test sorting
  auto dict = dict_col->dictionary();
  EXPECT_EQ((*dict)[0], "Alexander");
  EXPECT_EQ((*dict)[1], "Bill");
  EXPECT_EQ((*dict)[2], "Hasso");
  EXPECT_EQ((*dict)[3], "Steve");

  // Test accessors
  EXPECT_EQ(opossum::type_cast<std::string>((*dict_col)[0]), "Bill");
  EXPECT_EQ(dict_col->get(0), "Bill");

  // Test order of initial values
  EXPECT_EQ(dict_col->get(0), "Bill");
  EXPECT_EQ(dict_col->get(1), "Steve");
  EXPECT_EQ(dict_col->get(2), "Alexander");
  EXPECT_EQ(dict_col->get(3), "Steve");
  EXPECT_EQ(dict_col->get(4), "Hasso");
  EXPECT_EQ(dict_col->get(5), "Bill");

  // Test immutability
  EXPECT_THROW(dict_col->append(1), std::exception);  
}

TEST_F(StorageDictionaryColumnTest, LowerUpperBound) {
  for (int i = 0; i <= 10; i += 2) vc_int->append(i);
  auto col = opossum::make_shared_by_column_type<opossum::BaseColumn, opossum::DictionaryColumn>("int", vc_int);
  auto dict_col = std::dynamic_pointer_cast<opossum::DictionaryColumn<int>>(col);

  EXPECT_EQ(dict_col->lower_bound(4), (opossum::ValueID)2);
  EXPECT_EQ(dict_col->upper_bound(4), (opossum::ValueID)3);

  EXPECT_EQ(dict_col->lower_bound(5), (opossum::ValueID)3);
  EXPECT_EQ(dict_col->upper_bound(5), (opossum::ValueID)3);

  EXPECT_EQ(dict_col->lower_bound(15), opossum::INVALID_VALUE_ID);
  EXPECT_EQ(dict_col->upper_bound(15), opossum::INVALID_VALUE_ID);
}

TEST_F(StorageDictionaryColumnTest, VariableWidthAttributeVector) {
  {
    auto v = std::make_shared<opossum::ValueColumn<int>>(opossum::ValueColumn<int>());
    auto limit = std::numeric_limits<uint8_t>::max() + 1;
    for(uint16_t i = 0; i < limit ; i++) {
      v->append(i);   
    }

    opossum::DictionaryColumn<int> dict_col(v);
    auto attribute_vector = dict_col.attribute_vector();
    EXPECT_EQ(static_cast<uint>(attribute_vector->width()), sizeof(uint16_t));
  }
  {
    auto v = std::make_shared<opossum::ValueColumn<std::string>>(opossum::ValueColumn<std::string>());
    v->append("0");
    
    opossum::DictionaryColumn<std::string> dict_col(v);
    auto attribute_vector = dict_col.attribute_vector();
    EXPECT_EQ(static_cast<uint>(attribute_vector->width()), sizeof(uint8_t));
  }
}
//TODO(student): You should add some more tests here (full coverage would be appreciated) and possibly in other files.
