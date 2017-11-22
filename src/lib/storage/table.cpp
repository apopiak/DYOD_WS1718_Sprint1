#include "table.hpp"

#include <algorithm>
#include <iomanip>
#include <limits>
#include <memory>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

#include "dictionary_column.hpp"
#include "value_column.hpp"

#include "resolve_type.hpp"
#include "types.hpp"
#include "utils/assert.hpp"

namespace opossum {

Table::Table(const uint32_t chunk_size)
    // we want to limit the size of a chunk to the maximum size of our size type in any case though
    : _max_chunk_size(chunk_size > 0 ? chunk_size : std::numeric_limits<uint32_t>::max()) {
  create_new_chunk();
}

void Table::add_column_definition(const std::string& name, const std::string& type) {
  _column_names.push_back(name);
  _column_types.push_back(type);
}

void Table::add_column(const std::string& name, const std::string& type) {
  for (auto& chunk : _chunks) {
    chunk.add_column(make_shared_by_column_type<BaseColumn, ValueColumn>(type));
  }
  add_column_definition(name, type);
}

void Table::append(std::vector<AllTypeVariant> values) {
  auto& chunk = _chunks.back();
  // because the unlimited size leads to a maximum size of `numeric_limits<uint32_t>::max()`
  // we always check against the `_max_chunk_size`
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
  // all chunks except the last one have exactly `_max_chunk_size` many elements
  return (chunk_count() - 1) * _max_chunk_size + _chunks.back().size();
}

ChunkID Table::chunk_count() const {
  // we know that the size will never be more than `numeric_limits<uint32_t>::max()`
  return ChunkID{static_cast<uint32_t>(_chunks.size())};
}

ColumnID Table::column_id_by_name(const std::string& column_name) const {
  auto column_it = std::find(_column_names.cbegin(), _column_names.cend(), column_name);
  if (column_it == _column_names.cend()) {
    throw std::invalid_argument("column with name '" + column_name + "' not found");
  }
  return ColumnID{static_cast<uint16_t>(std::distance(_column_names.cbegin(), column_it))};
}

uint32_t Table::chunk_size() const {
  // since we set _max_chunk_size to numeric limits if the user specifies it as 0, we also want
  // to return 0 if he asks for the chunk_size see constructor initializer list
  return _max_chunk_size == std::numeric_limits<uint32_t>::max() ? 0 : _max_chunk_size;
}

const std::vector<std::string>& Table::column_names() const { return _column_names; }

const std::string& Table::column_name(ColumnID column_id) const { return _column_names.at(column_id.t); }

const std::string& Table::column_type(ColumnID column_id) const { return _column_types.at(column_id.t); }

Chunk& Table::get_chunk(ChunkID chunk_id) { return _chunks.at(chunk_id.t); }

const Chunk& Table::get_chunk(ChunkID chunk_id) const { return _chunks.at(chunk_id.t); }

void Table::compress_chunk(ChunkID chunk_id) {
  Chunk dict_chunk;
  Chunk& value_chunk = get_chunk(chunk_id);

  for (ColumnID i = ColumnID(0); i < value_chunk.size(); ++i) {
    const auto column = value_chunk.get_column(i);
    dict_chunk.add_column(make_shared_by_column_type<BaseColumn, DictionaryColumn>(column_type(i), column));
  }

  value_chunk = std::move(dict_chunk);
}

void Table::emplace_chunk(Chunk chunk) {
  if(chunk_count() == 1 && _chunks.back().size() == 0) {
    _chunks[0] = std::move(chunk);
  } else {
    _chunks.emplace_back(std::move(chunk));
  }
}

}  // namespace opossum
