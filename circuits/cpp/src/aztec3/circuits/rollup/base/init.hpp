
#pragma once

#include "aztec3/circuits/abis/append_only_tree_snapshot.hpp"
#include "aztec3/circuits/abis/previous_kernel_data.hpp"
#include "aztec3/circuits/abis/rollup/base/base_or_merge_rollup_public_inputs.hpp"
#include "aztec3/circuits/abis/rollup/base/base_rollup_inputs.hpp"
#include "aztec3/circuits/abis/rollup/constant_rollup_data.hpp"
#include "aztec3/utils/circuit_errors.hpp"
#include "aztec3/utils/dummy_composer.hpp"
#include <aztec3/circuits/abis/private_circuit_public_inputs.hpp>
#include <aztec3/circuits/hash.hpp>
#include <aztec3/circuits/recursion/aggregator.hpp>
#include <aztec3/utils/types/circuit_types.hpp>
#include <aztec3/utils/types/convert.hpp>
#include <aztec3/utils/types/native_types.hpp>

#include "barretenberg/stdlib/merkle_tree/memory_store.hpp"
#include "barretenberg/stdlib/merkle_tree/memory_tree.hpp"
#include "barretenberg/stdlib/merkle_tree/merkle_tree.hpp"
#include "barretenberg/stdlib/merkle_tree/nullifier_tree/nullifier_memory_tree.hpp"
#include <barretenberg/crypto/sha256/sha256.hpp>
#include <barretenberg/stdlib/merkle_tree/nullifier_tree/nullifier_leaf.hpp>
#include <barretenberg/stdlib/merkle_tree/nullifier_tree/nullifier_tree.hpp>

namespace aztec3::circuits::rollup::native_base_rollup {

using NT = aztec3::utils::types::NativeTypes;

// Params
using ConstantRollupData = abis::ConstantRollupData<NT>;
using BaseRollupInputs = abis::BaseRollupInputs<NT>;
using BaseOrMergeRollupPublicInputs = abis::BaseOrMergeRollupPublicInputs<NT>;
using DummyComposer = aztec3::utils::DummyComposer;
using CircuitErrorCode = aztec3::utils::CircuitErrorCode;

using Aggregator = aztec3::circuits::recursion::Aggregator;
using AggregationObject = utils::types::NativeTypes::AggregationObject;
using AppendOnlySnapshot = abis::AppendOnlyTreeSnapshot<NT>;

// Nullifier Tree Alias
using MerkleTree = stdlib::merkle_tree::MemoryTree;
using NullifierTree = stdlib::merkle_tree::NullifierMemoryTree;
using NullifierLeaf = stdlib::merkle_tree::nullifier_leaf;
using SparseTree = stdlib::merkle_tree::MerkleTree<stdlib::merkle_tree::MemoryStore>;

}  // namespace aztec3::circuits::rollup::native_base_rollup