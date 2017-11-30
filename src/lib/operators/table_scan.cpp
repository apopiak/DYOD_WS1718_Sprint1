#include "table_scan.hpp"

#include <functional>

#include "resolve_type.hpp"
#include "storage/dictionary_column.hpp"
#include "storage/table.hpp"
#include "storage/chunk.hpp"
#include "storage/dictionary_column.hpp"
#include "storage/reference_column.hpp"
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
        DebugAssert(false, "Operator not Implemented");
        return [](T t1, T t2){ return false; };
    }
}

namespace opossum {

TableScan::TableScan(const std::shared_ptr<const AbstractOperator> in, ColumnID column_id, const ScanType scan_type,
            const AllTypeVariant search_value, const std::optional<AllTypeVariant> opt) 
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
    return scan_impl->scan(table, _column_id, _scan_type, _search_value);
}

template <typename T>
std::shared_ptr<const Table> TableScan::TableScanImpl<T>::scan(std::shared_ptr<const Table> table, const ColumnID& column_id, const ScanType scan_type, const AllTypeVariant& value) {
    // this holds the positions of all found values
    auto pos_list = std::make_shared<PosList>();
    // table that contains the chunk with reference columns
    auto output_table = std::make_shared<Table>();

    for(ColumnID i{0}; i < table->col_count(); ++i) {
        output_table->add_column_definition(table->column_name(i), table->column_type(i));
    }

    const auto t_comparator = make_comparator<T>(scan_type);
    const auto t_value = type_cast<T>(value);

    if(table->chunk_count() == 1) {
        const auto& chunk = table->get_chunk(ChunkID{0});
        auto reference_column = std::dynamic_pointer_cast<const ReferenceColumn>(chunk.get_column(column_id));
        if (reference_column) {
            auto ref_table = reference_column->referenced_table();
            auto ref_column_id = reference_column->referenced_column_id();
            auto ref_pos_list = reference_column->pos_list();

            auto ref_chunk_id = ChunkID{0};
            const std::vector<T>* ref_val_vector = nullptr;
            auto ref_dict_col = std::shared_ptr<const DictionaryColumn<T>>();
            // go through the position list
            for(const RowID & r : *ref_pos_list) {
                // we assume that the RowIDs are sorted by ChunkID (ascending)
                // thus we only have to fetch the columns once per referenced chunk
                if(ref_chunk_id != r.chunk_id || (!ref_val_vector && !ref_dict_col) ) {
                    ref_chunk_id = r.chunk_id;
                    const auto& ref_chunk = ref_table->get_chunk(ref_chunk_id);
                    ref_dict_col = std::dynamic_pointer_cast<const DictionaryColumn<T>>(ref_chunk.get_column(ref_column_id));
                    auto ref_val_col = std::dynamic_pointer_cast<const ValueColumn<T>>(ref_chunk.get_column(ref_column_id));
                    ref_val_vector = ref_val_col ? &ref_val_col->values() : nullptr;
                }
                if(ref_dict_col) {
                    if(t_comparator(ref_dict_col->get(r.chunk_offset), t_value)) {
                        pos_list->push_back(r);
                    }
                    continue;
                }
                if(ref_val_vector) {
                    if(t_comparator((*ref_val_vector)[r.chunk_offset], t_value)) {
                        pos_list->push_back(r);
                    }
                    continue;
                }
                DebugAssert(false, "Unexpected Column Type when dereferencing ReferenceColumn");
            }
            Chunk reference_chunk;
    
            // create reference columns
            for(ColumnID i{0}; i < ref_table->col_count(); ++i) {
                auto reference_column = std::make_shared<ReferenceColumn>(ref_table, i, pos_list);
                reference_chunk.add_column(reference_column);
            }
            
            output_table->emplace_chunk(std::move(reference_chunk));

            return output_table;
        }
    }

    auto ValueID_comparator = make_comparator<ValueID>(scan_type);
    // search through every chunk of the input table
    // store finds in pos_list
    for (auto chunk_id = ChunkID(0); chunk_id < table->chunk_count(); ++chunk_id) {
        const auto& chunk = table->get_chunk(chunk_id);
        // get the column that will be searched
        auto column = chunk.get_column(column_id);

        // Columns can be of types ValueColumn or DictColumn
        auto value_column = std::dynamic_pointer_cast<const ValueColumn<T>>(column);
        if(value_column) {
            // since ValueColumns are not sorted, just search through them linearly
            const auto& values = value_column->values();
            for(auto chunk_offset = ChunkOffset(0); chunk_offset < values.size(); ++chunk_offset) {
                if(t_comparator(values[chunk_offset], t_value)) {
                    pos_list->push_back(RowID{chunk_id,chunk_offset});
                }
            }
            continue;
        } 

        auto dict_column = std::dynamic_pointer_cast<const DictionaryColumn<T>>(column);
        DebugAssert(dict_column, "Unknown Column Type in Table Scan"); // make sure the above is the case; only in debug mode
        
        const auto dictionary = dict_column->dictionary();

        // depending on the operator we need to select a proper ValueID
        // which accurately represents the Value we are searching for
        // (keep in mind it can be the case that the value we are searching for is not contained)

        bool value_contained = std::binary_search(dictionary->cbegin(), dictionary->cend(), t_value);
        
        if(!value_contained) {
            if(scan_type == ScanType::OpEquals) {
                // no value can be equal
                continue;
            }
            if(scan_type == ScanType::OpNotEquals) {
                add_all_values(*pos_list, chunk_id, dict_column->size());
                continue;
            }
        }

        ValueID comp_value;
        switch(scan_type) {
            case ScanType::OpEquals: 
            case ScanType::OpNotEquals:
            case ScanType::OpLessThan: 
            case ScanType::OpGreaterThanEquals:
                comp_value = dict_column->lower_bound(t_value);
                break;
            case ScanType::OpLessThanEquals:
            case ScanType::OpGreaterThan:
                comp_value = dict_column->upper_bound(t_value);
                break;
        }

        if(comp_value == ValueID{0u}) {
            if(scan_type == ScanType::OpLessThanEquals || scan_type == ScanType::OpLessThan) {
                // value is smaller than all the values in the column, there are no matches
                // we can go to the next chunk
                continue;
            }
            if(scan_type == ScanType::OpGreaterThan || scan_type == ScanType::OpGreaterThanEquals) {
                // every value is greater than the search value - add every position
                add_all_values(*pos_list, chunk_id, dict_column->size());
                continue;
            }
        }

        // if upper_bound goes past the end
        if(comp_value == INVALID_VALUE_ID) {
            if(scan_type == ScanType::OpLessThanEquals || scan_type == ScanType::OpLessThan) {
                // value is greater than all the values in the column
                add_all_values(*pos_list, chunk_id, dict_column->size());
                continue;
            }
            if(scan_type == ScanType::OpGreaterThan || scan_type == ScanType::OpGreaterThanEquals) {
                // every value is smaller than the search value 
                continue;
            }
        }
        
        auto attribute_vector = dict_column->attribute_vector();

        for(ChunkOffset i = 0; i < attribute_vector->size(); ++i) {
            if(ValueID_comparator(attribute_vector->get(i),comp_value)) {
                pos_list->push_back(RowID{chunk_id, i});
            }
        }
    } // end for (chunks)

    Chunk reference_chunk;
    
    // create reference columns
    for(ColumnID i{0}; i < table->col_count(); ++i) {
        auto reference_column = std::make_shared<ReferenceColumn>(table, i, pos_list);
        reference_chunk.add_column(reference_column);
    }

    output_table->emplace_chunk(std::move(reference_chunk));

    return output_table;
}

void add_all_values(PosList& pos_list, ChunkID chunk_id, ChunkOffset number_of_values) {
    pos_list.reserve(number_of_values);
    for(ChunkOffset i = 0; i < number_of_values; i++) {
        pos_list.push_back(RowID{chunk_id, i});
    }
}

EXPLICITLY_INSTANTIATE_COLUMN_TYPES(TableScan::TableScanImpl);

} // namespace opossum
