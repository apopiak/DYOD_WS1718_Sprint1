#include <iostream>
#include <memory>

#include "../lib/storage/chunk.hpp"
#include "../lib/storage/storage_manager.hpp"
#include "../lib/storage/table.hpp"
#include "../lib/storage/value_column.hpp"
#include "../lib/utils/assert.hpp"

int main() {
  opossum::Assert(true, "We can use opossum files here :)");

  auto column = std::make_shared<opossum::ValueColumn<int>>();
  opossum::Chunk chunk;
  chunk.add_column(column);
  chunk.append({42});

  auto table = std::make_shared<opossum::Table>();
  table->add_column("values", "int");
  table->append({3});
  auto &man = opossum::StorageManager::get();
  man.add_table("my table", table);
  man.add_table("", table);
  man.print(std::cout);
  return 0;
}
