#pragma once

#include <experimental/optional>
#include <memory>
#include <string>
#include <vector>

#include "abstract_operator.hpp"
#include "all_type_variant.hpp"
#include "types.hpp"
#include "utils/assert.hpp"

namespace opossum {

class Table;

class TableScan : public AbstractOperator {
 public:
  TableScan(const std::shared_ptr<const AbstractOperator> in, ColumnID column_id, const ScanType scan_type,
            const AllTypeVariant search_value,
            const std::optional<AllTypeVariant> opt = std::optional<AllTypeVariant>());

  virtual ~TableScan() = default;

  ColumnID column_id() const;
  ScanType scan_type() const;
  const AllTypeVariant& search_value() const;

  class TableScanImplBase {
   public:
    virtual std::shared_ptr<const Table> scan(std::shared_ptr<const Table> table, const ColumnID& _column_id,
                                              const ScanType _scan_type, const AllTypeVariant& value) = 0;
  };
  template <typename T>
  class TableScanImpl : public TableScanImplBase {
   public:
    std::shared_ptr<const Table> scan(std::shared_ptr<const Table> table, const ColumnID& _column_id,
                                      const ScanType _scan_type, const AllTypeVariant& value) override;
  };

 protected:
  std::shared_ptr<const Table> _on_execute() override;

  ColumnID _column_id;
  ScanType _scan_type;
  AllTypeVariant _search_value;
};

}  // namespace opossum
