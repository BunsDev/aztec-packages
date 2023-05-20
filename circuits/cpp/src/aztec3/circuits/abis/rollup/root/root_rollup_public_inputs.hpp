
#pragma once

#include "aztec3/circuits/abis/append_only_tree_snapshot.hpp"
#include <aztec3/utils/msgpack_derived_output.hpp>
#include <aztec3/utils/types/circuit_types.hpp>
#include <aztec3/utils/types/convert.hpp>
#include <aztec3/utils/types/native_types.hpp>

#include "barretenberg/crypto/sha256/sha256.hpp"
#include "barretenberg/serialize/msgpack.hpp"

#include <ostream>

namespace aztec3::circuits::abis {

using aztec3::utils::types::CircuitTypes;
using aztec3::utils::types::NativeTypes;


template <typename NCT> struct RootRollupPublicInputs {
    using fr = typename NCT::fr;
    using AggregationObject = typename NCT::AggregationObject;

    // All below are shared between the base and merge rollups
    AggregationObject end_aggregation_object;

    AppendOnlyTreeSnapshot<NCT> start_private_data_tree_snapshot;
    AppendOnlyTreeSnapshot<NCT> end_private_data_tree_snapshot;

    AppendOnlyTreeSnapshot<NCT> start_nullifier_tree_snapshot;
    AppendOnlyTreeSnapshot<NCT> end_nullifier_tree_snapshot;

    AppendOnlyTreeSnapshot<NCT> start_contract_tree_snapshot;
    AppendOnlyTreeSnapshot<NCT> end_contract_tree_snapshot;

    fr start_public_data_tree_root;
    fr end_public_data_tree_root;

    AppendOnlyTreeSnapshot<NCT> start_tree_of_historic_private_data_tree_roots_snapshot;
    AppendOnlyTreeSnapshot<NCT> end_tree_of_historic_private_data_tree_roots_snapshot;

    AppendOnlyTreeSnapshot<NCT> start_tree_of_historic_contract_tree_roots_snapshot;
    AppendOnlyTreeSnapshot<NCT> end_tree_of_historic_contract_tree_roots_snapshot;

    AppendOnlyTreeSnapshot<NCT> start_l1_to_l2_messages_tree_snapshot;
    AppendOnlyTreeSnapshot<NCT> end_l1_to_l2_messages_tree_snapshot;

    AppendOnlyTreeSnapshot<NCT> start_tree_of_historic_l1_to_l2_messages_tree_roots_snapshot;
    AppendOnlyTreeSnapshot<NCT> end_tree_of_historic_l1_to_l2_messages_tree_roots_snapshot;

    std::array<fr, 2> calldata_hash;
    std::array<fr, 2> l1_to_l2_messages_hash;

    // for serialization, update with new fields
    MSGPACK_FIELDS(end_aggregation_object,
                   start_private_data_tree_snapshot,
                   end_private_data_tree_snapshot,
                   start_nullifier_tree_snapshot,
                   end_nullifier_tree_snapshot,
                   start_contract_tree_snapshot,
                   end_contract_tree_snapshot,
                   start_public_data_tree_root,
                   end_public_data_tree_root,
                   start_tree_of_historic_private_data_tree_roots_snapshot,
                   end_tree_of_historic_private_data_tree_roots_snapshot,
                   start_tree_of_historic_contract_tree_roots_snapshot,
                   end_tree_of_historic_contract_tree_roots_snapshot,
                   start_l1_to_l2_messages_tree_snapshot,
                   end_l1_to_l2_messages_tree_snapshot,
                   start_tree_of_historic_l1_to_l2_messages_tree_roots_snapshot,
                   end_tree_of_historic_l1_to_l2_messages_tree_roots_snapshot,
                   calldata_hash,
                   l1_to_l2_messages_hash);
    bool operator==(RootRollupPublicInputs<NCT> const&) const = default;

    fr hash() const
    {
        std::vector<uint8_t> buf;

        // TODO(AD) should we have hash() methods in each class that get built up?
        // eventually would be nice for this serialization to go away
        write(buf, start_private_data_tree_snapshot);
        write(buf, start_nullifier_tree_snapshot);
        write(buf, start_contract_tree_snapshot);
        write(buf, start_tree_of_historic_private_data_tree_roots_snapshot);
        write(buf, start_tree_of_historic_contract_tree_roots_snapshot);
        write(buf, start_public_data_tree_root);
        write(buf, start_l1_to_l2_messages_tree_snapshot);
        write(buf, start_tree_of_historic_l1_to_l2_messages_tree_roots_snapshot);
        write(buf, end_private_data_tree_snapshot);
        write(buf, end_nullifier_tree_snapshot);
        write(buf, end_contract_tree_snapshot);
        write(buf, end_tree_of_historic_private_data_tree_roots_snapshot);
        write(buf, end_tree_of_historic_contract_tree_roots_snapshot);
        write(buf, end_public_data_tree_root);
        write(buf, end_l1_to_l2_messages_tree_snapshot);
        write(buf, end_tree_of_historic_l1_to_l2_messages_tree_roots_snapshot);

        // Stitching calldata hash together
        auto high_buffer = calldata_hash[0].to_buffer();
        auto low_buffer = calldata_hash[1].to_buffer();

        for (uint8_t i = 0; i < 16; i++) {
            buf.push_back(high_buffer[16 + i]);
        }
        for (uint8_t i = 0; i < 16; i++) {
            buf.push_back(low_buffer[16 + i]);
        }

        // Stitch l1_to_l2_messages_hash
        auto high_buffer_m = l1_to_l2_messages_hash[0].to_buffer();
        auto low_buffer_m = l1_to_l2_messages_hash[1].to_buffer();

        for (uint8_t i = 0; i < 16; i++) {
            buf.push_back(high_buffer_m[16 + i]);
        }
        for (uint8_t i = 0; i < 16; i++) {
            buf.push_back(low_buffer_m[16 + i]);
        }

        return sha256::sha256_to_field(buf);
    }
};

template <typename NCT> std::ostream& operator<<(std::ostream& os, RootRollupPublicInputs<NCT> const& obj)
{
    utils::msgpack_derived_output(os, obj);
    return os;
};

}  // namespace aztec3::circuits::abis