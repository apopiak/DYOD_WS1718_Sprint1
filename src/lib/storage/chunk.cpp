#include <iomanip>
#include <iterator>
#include <limits>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include "base_column.hpp"
#include "chunk.hpp"

#include "utils/assert.hpp"

namespace opossum {

void Chunk::add_column(std::shared_ptr<BaseColumn> column) { _columns.push_back(column); }

void Chunk::append(const std::vector<AllTypeVariant>& values) {
  DebugAssert(values.size() == col_count(), "chunk::append: number of values does not match the number of columns");

  auto value_it = values.begin();
  auto value_end = values.end();
  auto column_it = _columns.begin();
  for (; value_it != value_end; ++value_it, ++column_it) {
    (*column_it)->append(*value_it);
  }
}

std::shared_ptr<BaseColumn> Chunk::get_column(ColumnID column_id) const {
  DebugAssert(column_id < _columns.size(), "Chunk::get_column: column_id out of range: " + std::to_string(column_id));
  return _columns[column_id];
}

uint16_t Chunk::col_count() const { return _columns.size(); }

uint32_t Chunk::size() const {
  if (!_columns.size()) {
    return 0;
  } else {
    return _columns[0]->size();
  }
}

}  // namespace opossum
