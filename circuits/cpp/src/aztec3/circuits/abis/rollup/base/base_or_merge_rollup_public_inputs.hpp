#pragma once
#include "../../append_only_tree_snapshot.hpp"
#include "../constant_rollup_data.hpp"

#include <aztec3/utils/msgpack_derived_equals.hpp>
#include <aztec3/utils/types/circuit_types.hpp>
#include <aztec3/utils/types/convert.hpp>
#include <aztec3/utils/types/native_types.hpp>

namespace aztec3::circuits::abis {

using aztec3::utils::types::CircuitTypes;
using aztec3::utils::types::NativeTypes;
using std::is_same;

const uint32_t BASE_ROLLUP_TYPE = 0;
const uint32_t MERGE_ROLLUP_TYPE = 1;

template <typename NCT> struct BaseOrMergeRollupPublicInputs {
    using fr = typename NCT::fr;
    using AggregationObject = typename NCT::AggregationObject;

    uint32_t rollup_type{};
    // subtree  height is always 0 for base.
    // so that we always pass-in two base/merge circuits of the same height into the next level of recursion
    fr rollup_subtree_height{};

    AggregationObject end_aggregation_object{};
    ConstantRollupData<NCT> constants{};

    AppendOnlyTreeSnapshot<NCT> start_private_data_tree_snapshot{};
    AppendOnlyTreeSnapshot<NCT> end_private_data_tree_snapshot{};

    AppendOnlyTreeSnapshot<NCT> start_nullifier_tree_snapshot{};
    AppendOnlyTreeSnapshot<NCT> end_nullifier_tree_snapshot{};

    AppendOnlyTreeSnapshot<NCT> start_contract_tree_snapshot{};
    AppendOnlyTreeSnapshot<NCT> end_contract_tree_snapshot{};

    fr start_public_data_tree_root{};
    fr end_public_data_tree_root{};

    // Hashes (probably sha256) to make public inputs constant-sized (to then be unpacked on-chain)
    // Needs to be two fields to accomodate all 256-bits of the hash
    std::array<fr, 2> calldata_hash{};

    // for serialization, update with new fields
    MSGPACK_FIELDS(rollup_type,
                   rollup_subtree_height,
                   end_aggregation_object,
                   constants,
                   start_private_data_tree_snapshot,
                   end_private_data_tree_snapshot,
                   start_nullifier_tree_snapshot,
                   end_nullifier_tree_snapshot,
                   start_contract_tree_snapshot,
                   end_contract_tree_snapshot,
                   start_public_data_tree_root,
                   end_public_data_tree_root,
                   calldata_hash);
    bool operator==(BaseOrMergeRollupPublicInputs<NCT> const&) const = default;
};

template <typename NCT> std::ostream& operator<<(std::ostream& os, BaseOrMergeRollupPublicInputs<NCT> const& obj)
{
    return os << "rollup_type:\n"
              << obj.rollup_type << "\n"
              << "rollup_subtree_height:\n"
              << obj.rollup_subtree_height << "\n"
              << "end_aggregation_object:\n"
              << obj.end_aggregation_object
              << "\n"
                 "constants:\n"
              << obj.constants
              << "\n"
                 "start_private_data_tree_snapshot:\n"
              << obj.start_private_data_tree_snapshot
              << "\n"
                 "end_private_data_tree_snapshot:\n"
              << obj.start_private_data_tree_snapshot
              << "\n"
                 "start_nullifier_tree_snapshot:\n"
              << obj.start_nullifier_tree_snapshot
              << "\n"
                 "end_nullifier_tree_snapshot:\n"
              << obj.end_nullifier_tree_snapshot
              << "\n"
                 "start_contract_tree_snapshot:\n"
              << obj.start_contract_tree_snapshot
              << "\n"
                 "end_contract_tree_snapshot:\n"
              << obj.end_contract_tree_snapshot
              << "\n"
                 "start_public_data_tree_root:\n"
              << obj.start_public_data_tree_root
              << "\n"
                 "end_public_data_tree_root:\n"
              << obj.end_public_data_tree_root
              << "\n"
                 "calldata_hash: "
              << obj.calldata_hash << "\n";
}

}  // namespace aztec3::circuits::abis