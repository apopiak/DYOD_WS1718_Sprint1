#include <iostream>
#include <memory>

#include "../lib/storage/chunk.hpp"
#include "../lib/storage/value_column.hpp"
#include "../lib/utils/assert.hpp"

int main() {
  opossum::Assert(true, "We can use opossum files here :)");

  auto column = std::make_shared<opossum::ValueColumn<int>>();
  opossum::Chunk chunk;
  chunk.add_column(column);
  chunk.append({42});
  return 0;
}
