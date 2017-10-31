#include "table.hpp"

#include <algorithm>
#include <iomanip>
#include <limits>
#include <memory>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

#include "value_column.hpp"

#include "resolve_type.hpp"
#include "types.hpp"
#include "utils/assert.hpp"

namespace opossum {

Table::Table(const uint32_t chunk_size)
    : _max_chunk_size(chunk_size > 0 ? chunk_size : std::numeric_limits<uint32_t>::max()) {
  create_new_chunk();
}

void Table::add_column_definition(const std::string& name, const std::string& type) {
  _column_names.push_back(name);
  _column_types.push_back(type);
}

void Table::add_column(const std::string& name, const std::string& type) {
  add_column_definition(name, type);
  for (auto& chunk : _chunks) {
    chunk.add_column(make_shared_by_column_type<BaseColumn, ValueColumn>(type));
  }
}

void Table::append(std::vector<AllTypeVariant> values) {
  auto& chunk = _chunks.back();
  if (chunk.size() < _max_chunk_size) {
    chunk.append(values);
  } else {
    create_new_chunk();
    _chunks.back().append(values);
  }
}

void Table::create_new_chunk() {
  Chunk chunk;
  for (const auto& type : _column_types) {
    chunk.add_column(make_shared_by_column_type<BaseColumn, ValueColumn>(type));
  }
  _chunks.push_back(std::move(chunk));
}

uint16_t Table::col_count() const { return _column_names.size(); }

uint64_t Table::row_count() const {
  // all chunks except the last one got exactly _max_chunk_size many elements
  return (chunk_count() - 1) * _max_chunk_size + _chunks.back().size();
}

ChunkID Table::chunk_count() const { return ChunkID{_chunks.size()}; }

ColumnID Table::column_id_by_name(const std::string& column_name) const {
  auto column_it = std::find(_column_names.cbegin(), _column_names.cend(), column_name);
  if (column_it == _column_names.cend()) {
    throw std::invalid_argument("column with name '" + column_name + "' not found");
  }
  return ColumnID{std::distance(_column_names.cbegin(), column_it)};
}

uint32_t Table::chunk_size() const { return _max_chunk_size; }

const std::vector<std::string>& Table::column_names() const { return _column_names; }

const std::string& Table::column_name(ColumnID column_id) const {
  DebugAssert(column_id < _column_names.size(),
              "Table::column_name: column_id out of range: " + std::to_string(column_id));
  return _column_names[column_id.t];
}

const std::string& Table::column_type(ColumnID column_id) const {
  DebugAssert(column_id < _column_types.size(),
              "Table::column_type: column_id out of range: " + std::to_string(column_id));
  return _column_types[column_id.t];
}

Chunk& Table::get_chunk(ChunkID chunk_id) {
  DebugAssert(chunk_id < _chunks.size(), "Table::get_chunk: chunk_id out of range: " + std::to_string(chunk_id));
  return _chunks[chunk_id.t];
}

const Chunk& Table::get_chunk(ChunkID chunk_id) const {
  DebugAssert(chunk_id < _chunks.size(), "Table::get_chunk: chunk_id out of range: " + std::to_string(chunk_id));
  return _chunks[chunk_id.t];
}

void Table::compress_chunk(ChunkID chunk_id) { throw std::runtime_error("TODO"); }

}  // namespace opossum
