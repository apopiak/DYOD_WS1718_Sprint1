#include "reference_column.hpp"

#include "utils/assert.hpp"

namespace opossum {

ReferenceColumn::ReferenceColumn(const std::shared_ptr<const Table> referenced_table, const ColumnID referenced_column_id,
  const std::shared_ptr<const PosList> pos)
  : _referenced_table(referenced_table)
  , _referenced_column_id(referenced_column_id)
  , _pos_list(pos) {
}

const AllTypeVariant ReferenceColumn::operator[](const size_t i) const {
  DebugAssert(i < _pos_list->size(), "index out of range");
  auto& row_id = (*_pos_list)[i];
  auto& chunk = _referenced_table->get_chunk(row_id.chunk_id);
  auto column = chunk.get_column(_referenced_column_id);
  return (*column)[i];
}

}  // namespace opossum
