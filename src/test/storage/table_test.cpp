#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "../base_test.hpp"
#include "gtest/gtest.h"

#include "../lib/resolve_type.hpp"
#include "../lib/storage/dictionary_column.hpp"
#include "../lib/storage/table.hpp"
#include "../lib/type_cast.hpp"

namespace opossum {

class StorageTableTest : public BaseTest {
 protected:
  void SetUp() override {
    t.add_column("col_1", "int");
    t.add_column("col_2", "string");
  }

  Table t{2};
};

TEST_F(StorageTableTest, ChunkCount) {
  EXPECT_EQ(t.chunk_count(), 1u);
  t.append({4, "Hello,"});
  t.append({6, "world"});
  t.append({3, "!"});
  EXPECT_EQ(t.chunk_count(), 2u);
}

TEST_F(StorageTableTest, GetChunk) {
  t.get_chunk(ChunkID{0});
  // TODO(anyone): Do we want checks here?
  // EXPECT_THROW(t.get_chunk(ChunkID{q}), std::exception);
  t.append({4, "Hello,"});
  t.append({6, "world"});
  t.append({3, "!"});
  t.get_chunk(ChunkID{1});
}

TEST_F(StorageTableTest, ColCount) { EXPECT_EQ(t.col_count(), 2u); }

TEST_F(StorageTableTest, RowCount) {
  EXPECT_EQ(t.row_count(), 0u);
  t.append({4, "Hello,"});
  t.append({6, "world"});
  t.append({3, "!"});
  EXPECT_EQ(t.row_count(), 3u);
}

TEST_F(StorageTableTest, GetColumnName) {
  EXPECT_EQ(t.column_name(ColumnID{0}), "col_1");
  EXPECT_EQ(t.column_name(ColumnID{1}), "col_2");
  // TODO(anyone): Do we want checks here?
  // EXPECT_THROW(t.column_name(ColumnID{2}), std::exception);
}

TEST_F(StorageTableTest, GetColumnType) {
  EXPECT_EQ(t.column_type(ColumnID{0}), "int");
  EXPECT_EQ(t.column_type(ColumnID{1}), "string");
  // TODO(anyone): Do we want checks here?
  // EXPECT_THROW(t.column_type(ColumnID{2}), std::exception);
}

TEST_F(StorageTableTest, GetColumnIdByName) {
  EXPECT_EQ(t.column_id_by_name("col_2"), 1u);
  EXPECT_THROW(t.column_id_by_name("no_column_name"), std::exception);
}

TEST_F(StorageTableTest, GetChunkSize) { EXPECT_EQ(t.chunk_size(), 2u); }

TEST_F(StorageTableTest, CompressChunk) {
  t.append({1, "Hi"});
  t.append({1, "Ho"});
  t.compress_chunk(ChunkID{0});

  auto& chunk = t.get_chunk(ChunkID{0});
  EXPECT_EQ(t.chunk_count(), ChunkID{1});
  EXPECT_EQ(chunk.size(), 2u);
  EXPECT_EQ(chunk.col_count(), 2u);

  auto intColumn = std::dynamic_pointer_cast<DictionaryColumn<int>>(chunk.get_column(ColumnID{0}));
  EXPECT_EQ(intColumn->size(), 2u);
  EXPECT_EQ(intColumn->unique_values_count(), 1u);
  EXPECT_EQ(intColumn->get(0), 1);
  EXPECT_EQ(intColumn->get(1), 1);
}

TEST_F(StorageTableTest, CompressChunks) {
  t.append({1, "Hi"});
  t.append({1, "Ho"});
  t.append({2, "Bye"});
  t.append({2, "Ciao"});
  t.append({3, "More"});
  t.append({3, "Chunks"});

  t.compress_chunk(ChunkID{0});
  t.compress_chunk(ChunkID{1});

  auto& chunk = t.get_chunk(ChunkID{0});
  EXPECT_EQ(t.chunk_count(), ChunkID{3});
  EXPECT_EQ(chunk.size(), 2u);
  EXPECT_EQ(chunk.col_count(), 2u);

  auto intColumn = std::dynamic_pointer_cast<DictionaryColumn<int>>(chunk.get_column(ColumnID{0}));
  EXPECT_EQ(intColumn->size(), 2u);
  EXPECT_EQ(intColumn->unique_values_count(), 1u);
  EXPECT_EQ(intColumn->get(0), 1);
  EXPECT_EQ(intColumn->get(1), 1);

  auto& second_chunk = t.get_chunk(ChunkID{1});
  auto second_column = std::dynamic_pointer_cast<DictionaryColumn<int>>(second_chunk.get_column(ColumnID{0}));
  EXPECT_EQ(second_column->size(), 2u);
  EXPECT_EQ(second_column->unique_values_count(), 1u);
  EXPECT_EQ(second_column->get(0), 2);
  EXPECT_EQ(second_column->get(1), 2);
}

}  // namespace opossum
