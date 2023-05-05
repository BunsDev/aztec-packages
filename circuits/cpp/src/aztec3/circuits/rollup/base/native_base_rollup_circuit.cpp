#include "init.hpp"

#include "aztec3/circuits/abis/membership_witness.hpp"
#include "aztec3/circuits/abis/public_data_read.hpp"
#include "aztec3/circuits/abis/public_data_transition.hpp"
#include "aztec3/circuits/hash.hpp"
#include "aztec3/circuits/rollup/components/components.hpp"
#include "aztec3/constants.hpp"
#include "aztec3/utils/circuit_errors.hpp"
#include <aztec3/circuits/abis/rollup/base/base_or_merge_rollup_public_inputs.hpp>
#include <aztec3/circuits/abis/rollup/base/base_rollup_inputs.hpp>
#include <aztec3/circuits/abis/rollup/nullifier_leaf_preimage.hpp>

#include "barretenberg/crypto/pedersen_hash/pedersen.hpp"
#include "barretenberg/crypto/sha256/sha256.hpp"
#include "barretenberg/ecc/curves/bn254/fr.hpp"
#include "barretenberg/stdlib/hash/pedersen/pedersen.hpp"
#include "barretenberg/stdlib/merkle_tree/membership.hpp"
#include "barretenberg/stdlib/merkle_tree/memory_tree.hpp"
#include "barretenberg/stdlib/merkle_tree/merkle_tree.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <tuple>
#include <vector>


namespace aztec3::circuits::rollup::native_base_rollup {

NT::fr calculate_empty_tree_root(const size_t depth)
{
    MerkleTree const empty_tree = MerkleTree(depth);
    return empty_tree.root();
}

// TODO: can we aggregate proofs if we do not have a working circuit impl

bool verify_kernel_proof(NT::Proof const& kernel_proof)
{
    (void)kernel_proof;
    return true;
}

/**
 * @brief Create an aggregation object for the proofs that are provided
 *          - We add points P0 for each of our proofs
 *          - We add points P1 for each of our proofs
 *          - We concat our public inputs
 *
 * @param baseRollupInputs
 * @return AggregationObject
 */
AggregationObject aggregate_proofs(BaseRollupInputs const& baseRollupInputs)
{
    // TODO: NOTE: for now we simply return the aggregation object from the first proof
    return baseRollupInputs.kernel_data[0].public_inputs.end.aggregation_object;
}

/** TODO: implement
 * @brief Get the prover contribution hash object
 *
 * @return NT::fr
 */
NT::fr get_prover_contribution_hash()
{
    return NT::fr(0);
}

std::vector<NT::fr> calculate_contract_leaves(BaseRollupInputs const& baseRollupInputs)
{
    std::vector<NT::fr> contract_leaves;

    for (size_t i = 0; i < 2; i++) {
        auto new_contacts = baseRollupInputs.kernel_data[i].public_inputs.end.new_contracts;

        // loop over the new contracts
        // TODO: NOTE: we are currently assuming that there is only going to be one
        for (auto& leaf_preimage : new_contacts) {
            // When there is no contract deployment, we should insert a zero leaf into the tree and ignore the
            // member-ship check. This is to ensure that we don't hit "already deployed" errors when we are not
            // deploying contracts. e.g., when we are only calling functions on existing contracts.
            auto to_push = leaf_preimage.contract_address == NT::address(0) ? NT::fr(0) : leaf_preimage.hash();
            contract_leaves.push_back(to_push);
        }
    }

    return contract_leaves;
}

NT::fr calculate_contract_subtree(std::vector<NT::fr> contract_leaves)
{
    MerkleTree contracts_tree = MerkleTree(CONTRACT_SUBTREE_DEPTH);

    // Compute the merkle root of a contract subtree
    // Contracts subtree
    for (size_t i = 0; i < contract_leaves.size(); i++) {
        contracts_tree.update_element(i, contract_leaves[i]);
    }
    return contracts_tree.root();
}

NT::fr calculate_commitments_subtree(DummyComposer& composer, BaseRollupInputs const& baseRollupInputs)
{
    // Leaves that will be added to the new trees
    std::array<NT::fr, KERNEL_NEW_COMMITMENTS_LENGTH * 2> const commitment_leaves;

    MerkleTree commitments_tree = MerkleTree(PRIVATE_DATA_SUBTREE_DEPTH);

    for (size_t i = 0; i < 2; i++) {
        auto new_commitments = baseRollupInputs.kernel_data[i].public_inputs.end.new_commitments;

        // Our commitments size MUST be 4 to calculate our subtrees correctly
        composer.do_assert(new_commitments.size() == 4,
                           "New commitments in kernel data must be 4",
                           CircuitErrorCode::BASE__INCORRECT_NUM_OF_NEW_COMMITMENTS);

        for (size_t j = 0; j < new_commitments.size(); j++) {
            // todo: batch insert
            commitments_tree.update_element(i * KERNEL_NEW_COMMITMENTS_LENGTH + j, new_commitments[j]);
        }
    }

    // Commitments subtree
    return commitments_tree.root();
}

/**
 * @brief Check all of the provided commitments against the historical tree roots
 *
 * @param constantBaseRollupData
 * @param baseRollupInputs
 */
void perform_historical_private_data_tree_membership_checks(DummyComposer& composer,
                                                            BaseRollupInputs const& baseRollupInputs)
{
    // For each of the historic_private_data_tree_membership_checks, we need to do an inclusion proof
    // against the historical root provided in the rollup constants
    auto historic_root = baseRollupInputs.constants.start_tree_of_historic_private_data_tree_roots_snapshot.root;

    for (size_t i = 0; i < 2; i++) {
        NT::fr const leaf =
            baseRollupInputs.kernel_data[i]
                .public_inputs.constants.historic_tree_roots.private_historic_tree_roots.private_data_tree_root;
        abis::MembershipWitness<NT, PRIVATE_DATA_TREE_ROOTS_TREE_HEIGHT> const historic_root_witness =
            baseRollupInputs.historic_private_data_tree_root_membership_witnesses[i];

        check_membership<NT>(composer,
                             leaf,
                             historic_root_witness.leaf_index,
                             historic_root_witness.sibling_path,
                             historic_root,
                             format("historic private data tree roots ", i));
    }
}

void perform_historical_contract_data_tree_membership_checks(DummyComposer& composer,
                                                             BaseRollupInputs const& baseRollupInputs)
{
    auto historic_root = baseRollupInputs.constants.start_tree_of_historic_contract_tree_roots_snapshot.root;

    for (size_t i = 0; i < 2; i++) {
        NT::fr const leaf =
            baseRollupInputs.kernel_data[i]
                .public_inputs.constants.historic_tree_roots.private_historic_tree_roots.contract_tree_root;
        abis::MembershipWitness<NT, PRIVATE_DATA_TREE_ROOTS_TREE_HEIGHT> const historic_root_witness =
            baseRollupInputs.historic_contract_tree_root_membership_witnesses[i];

        check_membership<NT>(composer,
                             leaf,
                             historic_root_witness.leaf_index,
                             historic_root_witness.sibling_path,
                             historic_root,
                             format("historic contract data tree roots ", i));
    }
}

NT::fr create_nullifier_subtree(std::array<NullifierLeaf, KERNEL_NEW_NULLIFIERS_LENGTH * 2> const& nullifier_leaves)
{
    // Build a merkle tree of the nullifiers
    MerkleTree nullifier_subtree = MerkleTree(NULLIFIER_SUBTREE_DEPTH);
    for (size_t i = 0; i < nullifier_leaves.size(); i++) {
        // check if the nullifier is zero, if so dont insert
        if (uint256_t(nullifier_leaves[i].value) == uint256_t(0)) {
            nullifier_subtree.update_element(i, fr::zero());
        } else {
            nullifier_subtree.update_element(i, nullifier_leaves[i].hash());
        }
    }

    return nullifier_subtree.root();
}

/**
 * @brief Check non membership of each of the generated nullifiers in the current tree
 *
 * @returns The end nullifier tree root
 */
AppendOnlySnapshot check_nullifier_tree_non_membership_and_insert_to_tree(DummyComposer& composer,
                                                                          BaseRollupInputs const& baseRollupInputs)
{
    // LADIES AND GENTLEMEN The P L A N ( is simple )
    // 1. Get the previous nullifier set setup
    // 2. Check for the first added nullifier that it doesnt exist
    // 3. Update the nullifier set
    // 4. Calculate a new root with the sibling path
    // 5. Use that for the next nullifier check.
    // 6. Iterate for all of em
    // 7. le bosh (profit)

    // BOYS AND GIRLS THE P L A N ( once the first plan is complete )
    // GENERATE OUR NEW NULLIFIER SUBTREE
    // 1. We need to point the new nullifiers to point to the index that the previous nullifier replaced
    // 2. If we receive the 0 nullifier leaf (where all values are 0, we skip insertion and leave a sparse subtree)

    // New nullifier subtree
    std::array<NullifierLeaf, KERNEL_NEW_NULLIFIERS_LENGTH * 2> nullifier_insertion_subtree;

    // This will update on each iteration
    auto current_nullifier_tree_root = baseRollupInputs.start_nullifier_tree_snapshot.root;

    // This will increase with every insertion
    auto start_insertion_index = baseRollupInputs.start_nullifier_tree_snapshot.next_available_leaf_index;
    auto new_index = start_insertion_index;

    // For each kernel circuit
    for (size_t i = 0; i < 2; i++) {
        auto new_nullifiers = baseRollupInputs.kernel_data[i].public_inputs.end.new_nullifiers;
        // For each of our nullifiers
        for (size_t j = 0; j < KERNEL_NEW_NULLIFIERS_LENGTH; j++) {
            // Witness containing index and path
            auto nullifier_index = 4 * i + j;

            auto witness = baseRollupInputs.low_nullifier_membership_witness[nullifier_index];
            // Preimage of the lo-index required for a non-membership proof
            auto low_nullifier_preimage = baseRollupInputs.low_nullifier_leaf_preimages[nullifier_index];
            // Newly created nullifier
            auto nullifier = new_nullifiers[j];

            // TODO: reason about this more strongly, can this cause issues?
            if (nullifier != 0) {
                // Create the nullifier leaf of the new nullifier to be inserted
                NullifierLeaf new_nullifier_leaf = {
                    .value = nullifier,
                    .nextIndex = low_nullifier_preimage.next_index,
                    .nextValue = low_nullifier_preimage.next_value,
                };

                // Assuming populated premier subtree
                if (low_nullifier_preimage.leaf_value == 0 && low_nullifier_preimage.next_value == 0) {
                    // check previous nullifier leaves
                    // TODO: this is a hack, and insecure, we need to fix this
                    bool matched = false;

                    for (size_t k = 0; k < nullifier_index && !matched; k++) {
                        if (nullifier_insertion_subtree[k].value == 0) {
                            continue;
                        }

                        if ((uint256_t(nullifier_insertion_subtree[k].value) < uint256_t(nullifier)) &&
                            (uint256_t(nullifier_insertion_subtree[k].nextValue) > uint256_t(nullifier) ||
                             nullifier_insertion_subtree[k].nextValue == 0)) {
                            matched = true;
                            // Update pointers
                            new_nullifier_leaf.nextIndex = nullifier_insertion_subtree[k].nextIndex;
                            new_nullifier_leaf.nextValue = nullifier_insertion_subtree[k].nextValue;

                            // Update child
                            nullifier_insertion_subtree[k].nextIndex = new_index;
                            nullifier_insertion_subtree[k].nextValue = nullifier;
                        }
                    }

                    // if not matched, our subtree will misformed - we must reject
                    composer.do_assert(
                        matched, "Nullifier subtree is malformed", CircuitErrorCode::BASE__INVALID_NULLIFIER_SUBTREE);

                } else {
                    auto is_less_than_nullifier = uint256_t(low_nullifier_preimage.leaf_value) < uint256_t(nullifier);
                    auto is_next_greater_than = uint256_t(low_nullifier_preimage.next_value) > uint256_t(nullifier);

                    if (!(is_less_than_nullifier && is_next_greater_than)) {
                        if (low_nullifier_preimage.next_index != 0 && low_nullifier_preimage.next_value != 0) {
                            composer.do_assert(false,
                                               "Nullifier is not in the correct range",
                                               CircuitErrorCode::BASE__INVALID_NULLIFIER_RANGE);
                        }
                    }

                    // Recreate the original low nullifier from the preimage
                    auto const original_low_nullifier = NullifierLeaf{
                        .value = low_nullifier_preimage.leaf_value,
                        .nextIndex = low_nullifier_preimage.next_index,
                        .nextValue = low_nullifier_preimage.next_value,
                    };

                    // perform membership check for the low nullifier against the original root
                    check_membership<NT, DummyComposer, NULLIFIER_TREE_HEIGHT>(composer,
                                                                               original_low_nullifier.hash(),
                                                                               witness.leaf_index,
                                                                               witness.sibling_path,
                                                                               current_nullifier_tree_root,
                                                                               "low nullifier membership check");

                    // Calculate the new value of the low_nullifier_leaf
                    auto const updated_low_nullifier = NullifierLeaf{ .value = low_nullifier_preimage.leaf_value,
                                                                      .nextIndex = new_index,
                                                                      .nextValue = nullifier };

                    // We need another set of witness values for this
                    current_nullifier_tree_root = root_from_sibling_path<NT>(
                        updated_low_nullifier.hash(), witness.leaf_index, witness.sibling_path);
                }

                nullifier_insertion_subtree[nullifier_index] = new_nullifier_leaf;
            } else {
                // 0 case
                NullifierLeaf const new_nullifier_leaf = {
                    .value = 0,
                    .nextIndex = 0,
                    .nextValue = 0,
                };
                nullifier_insertion_subtree[nullifier_index] = new_nullifier_leaf;
            }

            // increment insertion index
            new_index = new_index + 1;
        }
    }

    // Check that the new subtree is to be inserted at the next location, and is empty currently
    const auto empty_nullifier_subtree_root = components::calculate_empty_tree_root(NULLIFIER_SUBTREE_DEPTH);
    auto leafIndexNullifierSubtreeDepth =
        baseRollupInputs.start_nullifier_tree_snapshot.next_available_leaf_index >> NULLIFIER_SUBTREE_DEPTH;
    check_membership<NT>(composer,
                         empty_nullifier_subtree_root,
                         leafIndexNullifierSubtreeDepth,
                         baseRollupInputs.new_nullifiers_subtree_sibling_path,
                         current_nullifier_tree_root,
                         "empty nullifier subtree membership check");

    // Create new nullifier subtree to insert into the whole nullifier tree
    auto nullifier_sibling_path = baseRollupInputs.new_nullifiers_subtree_sibling_path;
    auto nullifier_subtree_root = create_nullifier_subtree(nullifier_insertion_subtree);

    // Calculate the new root
    // We are inserting a subtree rather than a full tree here
    auto subtree_index = start_insertion_index >> (NULLIFIER_SUBTREE_DEPTH);
    auto new_root = root_from_sibling_path<NT>(nullifier_subtree_root, subtree_index, nullifier_sibling_path);

    // Return the new state of the nullifier tree
    return {
        .root = new_root,
        .next_available_leaf_index = new_index,
    };
}

fr insert_state_transitions(
    DummyComposer& composer,
    fr tree_root,
    std::array<abis::PublicDataTransition<NT>, STATE_TRANSITIONS_LENGTH> const& state_transitions,
    size_t witnesses_offset,
    std::array<abis::MembershipWitness<NT, PUBLIC_DATA_TREE_HEIGHT>, 2 * STATE_TRANSITIONS_LENGTH> const& witnesses)
{
    auto root = tree_root;

    for (size_t i = 0; i < STATE_TRANSITIONS_LENGTH; ++i) {
        const auto& state_write = state_transitions[i];
        const auto& witness = witnesses[i + witnesses_offset];

        if (state_write.is_empty()) {
            continue;
        }

        composer.do_assert(
            witness.leaf_index == state_write.leaf_index,
            format("mismatch state write ", state_write.leaf_index, " and witness leaf index ", witness.leaf_index),
            CircuitErrorCode::BASE__INVALID_PUBLIC_TRANSITIONS);

        check_membership<NT>(composer,
                             state_write.old_value,
                             state_write.leaf_index,
                             witness.sibling_path,
                             root,
                             format("validate_state_transitions index ", i));

        root = root_from_sibling_path<NT>(state_write.new_value, state_write.leaf_index, witness.sibling_path);
    }

    return root;
}

void validate_state_reads(
    DummyComposer& composer,
    fr tree_root,
    std::array<abis::PublicDataRead<NT>, STATE_READS_LENGTH> const& state_reads,
    size_t witnesses_offset,
    std::array<abis::MembershipWitness<NT, PUBLIC_DATA_TREE_HEIGHT>, 2 * STATE_READS_LENGTH> const& witnesses)
{
    for (size_t i = 0; i < STATE_READS_LENGTH; ++i) {
        const auto& state_read = state_reads[i];
        const auto& witness = witnesses[i + witnesses_offset];

        if (state_read.is_empty()) {
            continue;
        }

        composer.do_assert(
            witness.leaf_index == state_read.leaf_index,
            format("mismatch state read ", state_read.leaf_index, " and witness leaf index ", witness.leaf_index),
            CircuitErrorCode::BASE__INVALID_PUBLIC_READS);

        check_membership<NT>(composer,
                             state_read.value,
                             state_read.leaf_index,
                             witness.sibling_path,
                             tree_root,
                             format("validate_state_reads index ", i + witnesses_offset));
    }
};

fr validate_and_process_public_state(DummyComposer& composer, BaseRollupInputs const& baseRollupInputs)
{
    // Process state reads and transitions for left input
    validate_state_reads(composer,
                         baseRollupInputs.start_public_data_tree_root,
                         baseRollupInputs.kernel_data[0].public_inputs.end.state_reads,
                         0,
                         baseRollupInputs.new_state_reads_sibling_paths);

    auto mid_public_data_tree_root =
        insert_state_transitions(composer,
                                 baseRollupInputs.start_public_data_tree_root,
                                 baseRollupInputs.kernel_data[0].public_inputs.end.state_transitions,
                                 0,
                                 baseRollupInputs.new_state_transitions_sibling_paths);

    // Process state reads and transitions for right input using the resulting tree root from the left one
    validate_state_reads(composer,
                         mid_public_data_tree_root,
                         baseRollupInputs.kernel_data[1].public_inputs.end.state_reads,
                         STATE_READS_LENGTH,
                         baseRollupInputs.new_state_reads_sibling_paths);

    auto end_public_data_tree_root =
        insert_state_transitions(composer,
                                 mid_public_data_tree_root,
                                 baseRollupInputs.kernel_data[1].public_inputs.end.state_transitions,
                                 STATE_TRANSITIONS_LENGTH,
                                 baseRollupInputs.new_state_transitions_sibling_paths);

    return end_public_data_tree_root;
}

BaseOrMergeRollupPublicInputs base_rollup_circuit(DummyComposer& composer, BaseRollupInputs const& baseRollupInputs)
{
    // Verify the previous kernel proofs
    for (size_t i = 0; i < 2; i++) {
        NT::Proof const proof = baseRollupInputs.kernel_data[i].proof;
        composer.do_assert(verify_kernel_proof(proof),
                           "kernel proof verification failed",
                           CircuitErrorCode::BASE__KERNEL_PROOF_VERIFICATION_FAILED);
    }

    // First we compute the contract tree leaves
    std::vector<NT::fr> const contract_leaves = calculate_contract_leaves(baseRollupInputs);

    // Check contracts and commitments subtrees
    NT::fr const contracts_tree_subroot = calculate_contract_subtree(contract_leaves);
    NT::fr const commitments_tree_subroot = calculate_commitments_subtree(composer, baseRollupInputs);

    // Insert commitment subtrees:
    const auto empty_commitments_subtree_root = components::calculate_empty_tree_root(PRIVATE_DATA_SUBTREE_DEPTH);
    auto end_private_data_tree_snapshot =
        components::insert_subtree_to_snapshot_tree(composer,
                                                    baseRollupInputs.start_private_data_tree_snapshot,
                                                    baseRollupInputs.new_commitments_subtree_sibling_path,
                                                    empty_commitments_subtree_root,
                                                    commitments_tree_subroot,
                                                    PRIVATE_DATA_SUBTREE_DEPTH,
                                                    "empty commitment subtree membership check");

    // Insert contract subtrees:
    const auto empty_contracts_subtree_root = components::calculate_empty_tree_root(CONTRACT_SUBTREE_DEPTH);
    auto end_contract_tree_snapshot =
        components::insert_subtree_to_snapshot_tree(composer,
                                                    baseRollupInputs.start_contract_tree_snapshot,
                                                    baseRollupInputs.new_contracts_subtree_sibling_path,
                                                    empty_contracts_subtree_root,
                                                    contracts_tree_subroot,
                                                    CONTRACT_SUBTREE_DEPTH,
                                                    "empty contract subtree membership check");

    // Insert nullifiers:
    AppendOnlySnapshot const end_nullifier_tree_snapshot =
        check_nullifier_tree_non_membership_and_insert_to_tree(composer, baseRollupInputs);

    // Validate public state reads and transitions, and update public data tree
    fr const end_public_data_tree_root = validate_and_process_public_state(composer, baseRollupInputs);

    // Calculate the overall calldata hash
    std::array<NT::fr, 2> const calldata_hash = components::compute_kernels_calldata_hash(baseRollupInputs.kernel_data);

    // Perform membership checks that the notes provided exist within the historic trees data
    perform_historical_private_data_tree_membership_checks(composer, baseRollupInputs);
    perform_historical_contract_data_tree_membership_checks(composer, baseRollupInputs);

    AggregationObject const aggregation_object = aggregate_proofs(baseRollupInputs);

    BaseOrMergeRollupPublicInputs public_inputs = {
        .rollup_type = abis::BASE_ROLLUP_TYPE,
        .rollup_subtree_height = fr(0),
        .end_aggregation_object = aggregation_object,
        .constants = baseRollupInputs.constants,
        .start_private_data_tree_snapshot = baseRollupInputs.start_private_data_tree_snapshot,
        .end_private_data_tree_snapshot = end_private_data_tree_snapshot,
        .start_nullifier_tree_snapshot = baseRollupInputs.start_nullifier_tree_snapshot,
        .end_nullifier_tree_snapshot = end_nullifier_tree_snapshot,
        .start_contract_tree_snapshot = baseRollupInputs.start_contract_tree_snapshot,
        .end_contract_tree_snapshot = end_contract_tree_snapshot,
        .start_public_data_tree_root = baseRollupInputs.start_public_data_tree_root,
        .end_public_data_tree_root = end_public_data_tree_root,
        .calldata_hash = calldata_hash,
    };
    return public_inputs;
}

}  // namespace aztec3::circuits::rollup::native_base_rollup