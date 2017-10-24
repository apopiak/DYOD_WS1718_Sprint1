#include "storage_manager.hpp"

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
  _tables.emplace(name, table);
}

void StorageManager::drop_table(const std::string& name) {
  if (has_table(name)) {
    _tables.erase(name);
  } else {
    throw std::runtime_error(std::string("not table named: '") + name + "'");
  }
}

std::shared_ptr<Table> StorageManager::get_table(const std::string& name) const {
  return _tables.at(name);
}

bool StorageManager::has_table(const std::string& name) const {
  return _tables.count(name);
}

std::vector<std::string> StorageManager::table_names() const {
  std::vector<std::string> keys;
  keys.reserve(_tables.size());
  std::transform(_tables.cbegin(), _tables.cend(), std::back_inserter(keys), [] (const auto &pair) { return pair.first; });
  return keys;
}

void StorageManager::print(std::ostream& out) const {
  out << "StorageManager{ ";
  for (const auto &name : table_names()) {
    out << "'" << name << "' ";
  }
  out << "}";
}

void StorageManager::reset() {
  get() = StorageManager();
}

}  // namespace opossum
