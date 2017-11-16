#include "table_scan.hpp"

#include <functional>

#include "resolve_type.hpp"
#include "storage/table.hpp"
#include "types.hpp"
#include "type_cast.hpp"

namespace {
    template <typename T>
    std::function<bool(const T&, const T&)> make_comparator(ScanType scan_type) {
        switch(scan_type) {
            case OpEquals: return std::equal_to<T>;
            case OpNotEquals: return std::not_equal_to<T>;
            case OpLessThan: return std::less<T>;
            case OpLessThanEquals: return std::less_equal<T>;
            case OpGreaterThan: return std::greater<T>;
            case OpGreaterThanEquals: return std::greater_equal<T>;
        }
    }
}

namespace opossum {
TableScan::TableScan(const std::shared_ptr<const AbstractOperator> in, ColumnID column_id, const ScanType scan_type,
            const AllTypeVariant search_value) 
            : AbstractOperator(in)
            , _column_id(column_id)
            , _scan_type(scan_type)
            , _search_value(search_value) {}

ColumnID TableScan::column_id() const {
    return _column_id;
}
ScanType TableScan::scan_type() const { 
    return _scan_type;
}
const AllTypeVariant& TableScan::search_value() const {
    return _search_value;
}

std::shared_ptr<const Table> TableScan::_on_execute() {
    auto table = _input_table_left();
    auto scan_impl = make_unique_by_column_type<TableScanImplBase, TableScanImpl>(table->column_type(_column_id));
    return scan_impl->scan(*table, _column_id, _scan_type, _search_value);
}

template <typename T>
std::shared_ptr<const Table> TableScan::TableScanImpl<T>::scan(const Table& table, const ColumnID& _column_id, const ScanType& _scan_type, const AllTypeVariant& value) {
    auto pos_list = std::make_shared<PosList>();
    auto output_table = std::make_shared<Table>();
    auto comparator = make_comparator(scan_type);
    auto t_value = type_cast<T>(value);
    for (auto chunk_id = ChunkID(0); chunk_id < table.chunk_count; ++chunk_id) {
        const auto& chunk = table.get_chunk(chunk_id);
        auto column = chunk.get_column(_column_id);
        auto value_column = std::dynamic_pointer_cast<const ValueColumn<T>>(column);
        if(value_column) {
            const auto& values = value_column.values();
            for(auto chunk_offset = ChunkOffset(0); chunk_offset < values.size(); chunk_offset++) {
                if(comparator(values[chunk_offset], t_value)) {
                    pos_list.push_back(RowID{chunk_id,chunk_offset});
                }
            }
        } else {
            auto dict_column = std::dynamic_pointer_cast<const DictionaryColumn<T>>(column);
            DebugAssert(dict_column, "Unknown Column Type in Table Scan");
        }
    }
}
} // namespace opossum
