#pragma once

#include <memory>
#include <experimental/optional>
#include <string>
#include <vector>

#include "abstract_operator.hpp"
#include "all_type_variant.hpp"
#include "types.hpp"
#include "utils/assert.hpp"

namespace opossum {

class BaseTableScanImpl;
class Table;

class TableScan : public AbstractOperator {
 public:
  TableScan(const std::shared_ptr<const AbstractOperator> in, ColumnID column_id, const ScanType scan_type,
            const AllTypeVariant search_value);

  ~TableScan();

  ColumnID column_id() const;
  ScanType scan_type() const;
  const AllTypeVariant& search_value() const;

 protected:
  std::shared_ptr<const Table> _on_execute() override;

  class TableScanImplBase {
    public:
      virtual std::shared_ptr<const Table> scan(const Table& table, const ColumnID& column_id, ScanType scan_type, const AllTypeVariant& value);
  };
  template <typename T>
  class TableScanImpl : public TableScanImplBase {
    public:
      std::shared_ptr<const Table> scan(const Table& table, const ColumnID& column_id, ScanType scan_type, const AllTypeVariant& value) override;    
  };

  ColumnID _column_id;
  ScanType _scan_type;
  AllTypeVariant _search_value;
  
};

}  // namespace opossum
