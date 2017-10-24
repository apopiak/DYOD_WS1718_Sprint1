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

void Chunk::add_column(std::shared_ptr<BaseColumn> column) {
  _columns.push_back(column);
}

void Chunk::append(const std::vector<AllTypeVariant> values) {
  DebugAssert(values.size() == _columns.size(),
    "Tried to append value vector with differing amount of columns to the chunk.");
  for (uint16_t index = 0; index < values.size(); ++index) {
    _columns[index]->append(values[index]);
  }
}

std::shared_ptr<BaseColumn> Chunk::get_column(ColumnID column_id) const {
  return _columns[column_id];
}

uint16_t Chunk::col_count() const {
  return _columns.size();
}

uint32_t Chunk::size() const {
  if (!_columns.size()) {
    return 0;
  } else {
    return _columns[0]->size();
  }
}

}  // namespace opossum
