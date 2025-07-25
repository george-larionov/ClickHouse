#pragma once

#include <Processors/Merges/IMergingTransform.h>
#include <Processors/Merges/Algorithms/GraphiteRollupSortedAlgorithm.h>

namespace DB
{

/// Implementation of IMergingTransform via GraphiteRollupSortedAlgorithm.
class GraphiteRollupSortedTransform final : public IMergingTransform<GraphiteRollupSortedAlgorithm>
{
public:
    GraphiteRollupSortedTransform(
        SharedHeader header,
        size_t num_inputs,
        SortDescription description_,
        size_t max_block_size_rows,
        size_t max_block_size_bytes,
        Graphite::Params params_,
        time_t time_of_merge_)
        : IMergingTransform(
            num_inputs, header, header, /*have_all_inputs_=*/ true, /*limit_hint_=*/ 0, /*always_read_till_end_=*/ false,
            header,
            num_inputs,
            std::move(description_),
            max_block_size_rows,
            max_block_size_bytes,
            std::move(params_),
            time_of_merge_)
    {
    }

    String getName() const override { return "GraphiteRollupSortedTransform"; }
};

}
