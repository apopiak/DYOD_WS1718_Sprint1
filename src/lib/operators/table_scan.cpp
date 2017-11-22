#include "table_scan.hpp"

#include <functional>

#include "resolve_type.hpp"
#include "storage/dictionary_column.hpp"
#include "storage/table.hpp"
#include "storage/dictionary_column.hpp"
#include "types.hpp"
#include "type_cast.hpp"

namespace {
    template <typename T>
    std::function<bool(const T&, const T&)> make_comparator(opossum::ScanType scan_type) {
        using Type = opossum::ScanType;
        switch(scan_type) {
            case Type::OpEquals: return std::equal_to<T>();
            case Type::OpNotEquals: return std::not_equal_to<T>();
            case Type::OpLessThan: return std::less<T>();
            case Type::OpLessThanEquals: return std::less_equal<T>();
            case Type::OpGreaterThan: return std::greater<T>();
            case Type::OpGreaterThanEquals: return std::greater_equal<T>();
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
std::shared_ptr<const Table> TableScan::TableScanImpl<T>::scan(const Table& table, const ColumnID& column_id, ScanType scan_type, const AllTypeVariant& value) {
    // this holds the positions of all found values
    auto pos_list = std::make_shared<PosList>();
    // table that contains the chunk with reference columns
    auto output_table = std::make_shared<Table>();
    auto comparator = make_comparator<T>(scan_type);
    auto t_value = type_cast<T>(value);

    // search through every chunk of the input table
    // store finds in pos_list
    for (auto chunk_id = ChunkID(0); chunk_id < table.chunk_count(); ++chunk_id) {
        const auto& chunk = table.get_chunk(chunk_id);
        // get the column that will be searched
        auto column = chunk.get_column(column_id);

        // Columns can be of types ValueColumn or DictColumn
        auto value_column = std::dynamic_pointer_cast<const ValueColumn<T>>(column);
        if(value_column) {
            // since ValueColumns are not sorted, just search through them linearly
            const auto& values = value_column->values();
            for(auto chunk_offset = ChunkOffset(0); chunk_offset < values.size(); chunk_offset++) {
                if(comparator(values[chunk_offset], t_value)) {
                    pos_list->push_back(RowID{chunk_id,chunk_offset});
                }
            }
        } 
        // no need for extra if, because we know for certain that a column is either a Value- or DictColumn
        else {
            auto dict_column = std::dynamic_pointer_cast<const DictionaryColumn<T>>(column);
            DebugAssert(dict_column, "Unknown Column Type in Table Scan"); // make sure the above is the case; only in debug mode
            
            DebugAssert(false, "not implemented");
            // TODO: use
            // const auto& dictionary = dict_column->dictionary();
            // depending on the operator we need to select a proper ValueID
            // which accurately represents the Value we are searching for
            // (keep in mind it can be the case that the value we are searching for is not contained)
            // TODO: use this value
            // bool value_contained = std::binary_search(dictionary->cbegin(), dictionary->cend(), t_value);
            
            ValueID comp_value;
            using Type = opossum::ScanType;
            switch(scan_type) {
                case Type::OpEquals:
                case Type::OpNotEquals:
                case Type::OpLessThan:
                case Type::OpGreaterThanEquals:
                    comp_value = dict_column->lower_bound(t_value);
                case Type::OpLessThanEquals:
                case Type::OpGreaterThan:
                    comp_value = dict_column->upper_bound(t_value);
            }

            // now need to check edge cases
            // when using OpLessThanEquals and OpGreaterThan we need to use upperbound - 1
            // however if upper_bound returns the first item ...
            if(comp_value == ValueID{0u}) {
                if(scan_type == Type::OpLessThanEquals) {
                    // since value is smaller than all the values in the column, there are no matches
                    // we can go to the next chunk
                    continue;
                }
                if(scan_type == Type::OpGreaterThan) {
                    // every value is greater than the search value - add every position
                    add_all_values(*pos_list, chunk_id, dict_column->size());
                }
            }
        }
    }
    return output_table;
}

void add_all_values(PosList& pos_list, ChunkID chunk_id, ChunkOffset number_of_values) {
    pos_list.reserve(number_of_values);
    for(ChunkOffset i = 0; i < number_of_values; i++) {
        pos_list.push_back(RowID{chunk_id, i});
    }
}

} // namespace opossum
