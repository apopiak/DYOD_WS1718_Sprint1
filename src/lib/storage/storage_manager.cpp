#include "storage_manager.hpp"

#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "utils/assert.hpp"

namespace opossum {

StorageManager& StorageManager::get() {
  static StorageManager manager;
  return manager;
}

void StorageManager::add_table(const std::string& name, std::shared_ptr<Table> table) {
  if (has_table(name)) {
    throw std::invalid_argument("Table already exists: " + name);
  }
  _tables.emplace(name, table);
}

void StorageManager::drop_table(const std::string& name) {
  if (!_tables.erase(name)) {
    throw std::invalid_argument(std::string("no table named: '") + name + "'");
  }
}

std::shared_ptr<Table> StorageManager::get_table(const std::string& name) const { return _tables.at(name); }

bool StorageManager::has_table(const std::string& name) const { return _tables.count(name); }

std::vector<std::string> StorageManager::table_names() const {
  std::vector<std::string> keys;
  keys.reserve(_tables.size());
  std::transform(_tables.cbegin(), _tables.cend(), std::back_inserter(keys),
                 [](const auto& pair) { return pair.first; });
  return keys;
}

void StorageManager::print(std::ostream& out) const {
  for (const auto& pair : _tables) {
    const auto& table = *(pair.second);
    out << "'" << pair.first << "': " << table.col_count() << " columns, " << table.row_count() << " rows, "
        << table.chunk_count() << " chunks" << std::endl;
  }
}

void StorageManager::reset() { get() = StorageManager(); }

}  // namespace opossum
