#include "c_bind.h"
#include "index.hpp"
#include "init.hpp"

#include "aztec3/circuits/abis/append_only_tree_snapshot.hpp"
#include "aztec3/circuits/abis/membership_witness.hpp"
#include "aztec3/circuits/abis/new_contract_data.hpp"
#include "aztec3/circuits/abis/previous_kernel_data.hpp"
#include "aztec3/circuits/abis/rollup/merge/previous_rollup_data.hpp"
#include "aztec3/circuits/abis/rollup/nullifier_leaf_preimage.hpp"
#include "aztec3/circuits/kernel/private/utils.hpp"
#include "aztec3/circuits/rollup/base/init.hpp"
#include "aztec3/circuits/rollup/base/native_base_rollup_circuit.hpp"
#include "aztec3/circuits/rollup/test_utils/utils.hpp"
#include "aztec3/constants.hpp"
#include "aztec3/utils/dummy_composer.hpp"
#include <aztec3/circuits/abis/call_context.hpp>
#include <aztec3/circuits/abis/call_stack_item.hpp>
#include <aztec3/circuits/abis/combined_accumulated_data.hpp>
#include <aztec3/circuits/abis/combined_constant_data.hpp>
#include <aztec3/circuits/abis/combined_historic_tree_roots.hpp>
#include <aztec3/circuits/abis/contract_deployment_data.hpp>
#include <aztec3/circuits/abis/function_data.hpp>
#include <aztec3/circuits/abis/kernel_circuit_public_inputs.hpp>
#include <aztec3/circuits/abis/private_circuit_public_inputs.hpp>
#include <aztec3/circuits/abis/private_historic_tree_roots.hpp>
#include <aztec3/circuits/abis/private_kernel/globals.hpp>
#include <aztec3/circuits/abis/private_kernel/private_inputs.hpp>
#include <aztec3/circuits/abis/signed_tx_request.hpp>
#include <aztec3/circuits/abis/tx_context.hpp>
#include <aztec3/circuits/abis/tx_request.hpp>
#include <aztec3/circuits/apps/function_execution_context.hpp>
#include <aztec3/circuits/apps/test_apps/basic_contract_deployment/basic_contract_deployment.hpp>
#include <aztec3/circuits/apps/test_apps/escrow/deposit.hpp>
#include <aztec3/circuits/mock/mock_kernel_circuit.hpp>

#include "barretenberg/crypto/sha256/sha256.hpp"
#include "barretenberg/ecc/curves/bn254/fr.hpp"
#include "barretenberg/stdlib/merkle_tree/memory_tree.hpp"
#include <barretenberg/common/map.hpp>
#include <barretenberg/common/test.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <iostream>
#include <memory>
#include <vector>

namespace {


using aztec3::circuits::abis::PreviousKernelData;


// using aztec3::circuits::mock::mock_circuit;
using aztec3::circuits::rollup::test_utils::utils::compare_field_hash_to_expected;
using aztec3::circuits::rollup::test_utils::utils::get_empty_kernel;
using aztec3::circuits::rollup::test_utils::utils::get_empty_l1_to_l2_messages;
using aztec3::circuits::rollup::test_utils::utils::get_root_rollup_inputs;
using aztec3::circuits::rollup::test_utils::utils::set_kernel_commitments;
// using aztec3::circuits::mock::mock_kernel_inputs;

using aztec3::circuits::abis::AppendOnlyTreeSnapshot;

using aztec3::circuits::rollup::native_base_rollup::BaseOrMergeRollupPublicInputs;
using aztec3::circuits::rollup::native_base_rollup::BaseRollupInputs;
using aztec3::circuits::rollup::native_base_rollup::ConstantRollupData;
using aztec3::circuits::rollup::native_base_rollup::NT;

using aztec3::circuits::rollup::native_root_rollup::RootRollupInputs;
using aztec3::circuits::rollup::native_root_rollup::RootRollupPublicInputs;

using aztec3::circuits::abis::NewContractData;

using MemoryTree = stdlib::merkle_tree::MemoryTree;
using KernelData = aztec3::circuits::abis::PreviousKernelData<NT>;
}  // namespace

namespace aztec3::circuits::rollup::root::native_root_rollup_circuit {

class root_rollup_tests : public ::testing::Test {
  protected:
    static void run_cbind(RootRollupInputs& root_rollup_inputs,
                          RootRollupPublicInputs& expected_public_inputs,
                          bool compare_pubins = true)
    {
        info("Retesting via cbinds....");
        // TODO might be able to get rid of proving key buffer
        uint8_t const* pk_buf = nullptr;
        size_t const pk_size = root_rollup__init_proving_key(&pk_buf);
        (void)pk_size;
        // info("Proving key size: ", pk_size);

        // TODO might be able to get rid of verification key buffer
        uint8_t const* vk_buf = nullptr;
        size_t const vk_size = root_rollup__init_verification_key(pk_buf, &vk_buf);
        (void)vk_size;
        // info("Verification key size: ", vk_size);

        std::vector<uint8_t> root_rollup_inputs_vec;
        write(root_rollup_inputs_vec, root_rollup_inputs);

        // uint8_t const* proof_data;
        // size_t proof_data_size;
        uint8_t const* public_inputs_buf = nullptr;
        size_t public_inputs_size = 0;
        // info("simulating circuit via cbind");
        uint8_t* const circuit_failure_ptr =
            root_rollup__sim(root_rollup_inputs_vec.data(), &public_inputs_size, &public_inputs_buf);
        ASSERT_TRUE(circuit_failure_ptr == nullptr);
        // info("Proof size: ", proof_data_size);
        // info("PublicInputs size: ", public_inputs_size);

        if (compare_pubins) {
            RootRollupPublicInputs public_inputs;
            uint8_t const* public_inputs_buf_tmp = public_inputs_buf;
            read(public_inputs_buf_tmp, public_inputs);
            ASSERT_EQ(public_inputs.calldata_hash.size(), expected_public_inputs.calldata_hash.size());
            for (size_t i = 0; i < public_inputs.calldata_hash.size(); i++) {
                ASSERT_EQ(public_inputs.calldata_hash[i], expected_public_inputs.calldata_hash[i]);
            }

            std::vector<uint8_t> expected_public_inputs_vec;
            write(expected_public_inputs_vec, expected_public_inputs);

            ASSERT_EQ(public_inputs_size, expected_public_inputs_vec.size());
            // Just compare the first 10 bytes of the serialized public outputs
            if (public_inputs_size > 10) {
                // for (size_t 0; i < public_inputs_size; i++) {
                for (size_t i = 0; i < 10; i++) {
                    ASSERT_EQ(public_inputs_buf[i], expected_public_inputs_vec[i]);
                }
            }
        }

        free((void*)pk_buf);
        free((void*)vk_buf);
        // free((void*)proof_data);
        free((void*)public_inputs_buf);
    }
};

TEST_F(root_rollup_tests, native_check_block_hashes_empty_blocks)
{
    MemoryTree const data_tree = MemoryTree(PRIVATE_DATA_TREE_HEIGHT);

    // calculate calldata hash
    std::vector<uint8_t> const zero_bytes_vec(704, 0);
    auto call_data_hash_inner = sha256::sha256(zero_bytes_vec);

    std::array<uint8_t, 64> hash_input;
    for (uint8_t i = 0; i < 32; ++i) {
        hash_input[i] = call_data_hash_inner[i];
        hash_input[32 + i] = call_data_hash_inner[i];
    }

    std::vector<uint8_t> const calldata_hash_input_bytes_vec(hash_input.begin(), hash_input.end());

    auto calldata_hash = sha256::sha256(calldata_hash_input_bytes_vec);

    // get messages
    std::array<fr, NUMBER_OF_L1_L2_MESSAGES_PER_ROLLUP> const l1_to_l2_messages = get_empty_l1_to_l2_messages();

    // hash messages
    std::vector<uint8_t> const messages_hash_input_bytes_vec(NUMBER_OF_L1_L2_MESSAGES_PER_ROLLUP * 32, 0);
    auto messages_hash = sha256::sha256(messages_hash_input_bytes_vec);

    utils::DummyComposer composer = utils::DummyComposer();
    std::array<KernelData, 4> const kernels = {
        get_empty_kernel(), get_empty_kernel(), get_empty_kernel(), get_empty_kernel()
    };

    RootRollupInputs inputs = get_root_rollup_inputs(composer, kernels, l1_to_l2_messages);
    RootRollupPublicInputs outputs =
        aztec3::circuits::rollup::native_root_rollup::root_rollup_circuit(composer, inputs);

    // check calldata hash
    ASSERT_TRUE(compare_field_hash_to_expected(outputs.calldata_hash, calldata_hash));
    // Check messages hash
    ASSERT_TRUE(compare_field_hash_to_expected(outputs.l1_to_l2_messages_hash, messages_hash));

    EXPECT_FALSE(composer.failed());

    // Expected hash of public inputs for an empty L2 block. Also used in the contract tests.
    fr const expected_hash = uint256_t("11840efc30e9fcbdd0aae30da2a5b441132420b4f0cc4ffd6bdc41888845f775");
    ASSERT_EQ(outputs.hash(), expected_hash);

    run_cbind(inputs, outputs, true);
}

TEST_F(root_rollup_tests, native_root_missing_nullifier_logic)
{
    utils::DummyComposer composer = utils::DummyComposer();

    MemoryTree data_tree = MemoryTree(PRIVATE_DATA_TREE_HEIGHT);
    MemoryTree contract_tree = MemoryTree(CONTRACT_TREE_HEIGHT);
    MemoryTree historic_data_tree = MemoryTree(PRIVATE_DATA_TREE_ROOTS_TREE_HEIGHT);
    MemoryTree historic_contract_tree = MemoryTree(CONTRACT_TREE_ROOTS_TREE_HEIGHT);
    MemoryTree l1_to_l2_messages_tree = MemoryTree(L1_TO_L2_MSG_TREE_HEIGHT);
    MemoryTree historic_l1_to_l2_tree = MemoryTree(L1_TO_L2_MSG_TREE_ROOTS_TREE_HEIGHT);

    // Historic trees are initialised with an empty root at position 0.
    historic_data_tree.update_element(0, data_tree.root());
    historic_contract_tree.update_element(0, contract_tree.root());
    historic_l1_to_l2_tree.update_element(0, l1_to_l2_messages_tree.root());

    std::array<KernelData, 4> kernels = {
        get_empty_kernel(), get_empty_kernel(), get_empty_kernel(), get_empty_kernel()
    };
    std::array<fr, NUMBER_OF_L1_L2_MESSAGES_PER_ROLLUP> l1_to_l2_messages = get_empty_l1_to_l2_messages();

    // Create commitments
    for (uint8_t kernel_j = 0; kernel_j < 4; kernel_j++) {
        std::array<fr, KERNEL_NEW_COMMITMENTS_LENGTH> new_commitments;
        for (uint8_t commitment_k = 0; commitment_k < KERNEL_NEW_COMMITMENTS_LENGTH; commitment_k++) {
            auto val = fr(kernel_j * KERNEL_NEW_COMMITMENTS_LENGTH + commitment_k + 1);
            new_commitments[commitment_k] = val;
            data_tree.update_element(kernel_j * KERNEL_NEW_COMMITMENTS_LENGTH + commitment_k, val);
        }
        set_kernel_commitments(kernels[kernel_j], new_commitments);
    }

    // TODO: Add nullifiers

    // Contract tree
    NewContractData<NT> new_contract = {
        .contract_address = fr(1),
        .portal_contract_address = fr(3),
        .function_tree_root = fr(2),
    };
    auto contract_leaf = crypto::pedersen_commitment::compress_native(
        {
            new_contract.contract_address,
            new_contract.portal_contract_address,
            new_contract.function_tree_root,
        },
        GeneratorIndex::CONTRACT_LEAF);

    // Update contract tree
    contract_tree.update_element(2, contract_leaf);
    kernels[2].public_inputs.end.new_contracts[0] = new_contract;

    // l1 to l2 messages snapshot
    AppendOnlyTreeSnapshot<NT> const start_l1_to_l2_messages_tree_snapshot = { .root = l1_to_l2_messages_tree.root(),
                                                                               .next_available_leaf_index = 0 };

    // The start historic data snapshots
    AppendOnlyTreeSnapshot<NT> const start_historic_data_tree_snapshot = { .root = historic_data_tree.root(),
                                                                           .next_available_leaf_index = 1 };
    AppendOnlyTreeSnapshot<NT> const start_historic_contract_tree_snapshot = { .root = historic_contract_tree.root(),
                                                                               .next_available_leaf_index = 1 };
    AppendOnlyTreeSnapshot<NT> const start_historic_l1_to_l2_tree_snapshot = { .root = historic_l1_to_l2_tree.root(),
                                                                               .next_available_leaf_index = 1 };

    // Create 16 empty l1 to l2 messages, and update the l1_to_l2 messages tree
    for (size_t i = 0; i < l1_to_l2_messages.size(); i++) {
        l1_to_l2_messages_tree.update_element(i, l1_to_l2_messages[i]);
    }

    // Insert the newest data root into the historic tree
    historic_data_tree.update_element(1, data_tree.root());
    historic_contract_tree.update_element(1, contract_tree.root());
    historic_l1_to_l2_tree.update_element(1, l1_to_l2_messages_tree.root());

    // Compute the end snapshot
    AppendOnlyTreeSnapshot<NT> const end_historic_data_tree_snapshot = { .root = historic_data_tree.root(),
                                                                         .next_available_leaf_index = 2 };
    AppendOnlyTreeSnapshot<NT> const end_historic_contract_tree_snapshot = { .root = historic_contract_tree.root(),
                                                                             .next_available_leaf_index = 2 };
    AppendOnlyTreeSnapshot<NT> const end_historic_l1_to_l2_tree_snapshot = { .root = historic_l1_to_l2_tree.root(),
                                                                             .next_available_leaf_index = 2 };
    AppendOnlyTreeSnapshot<NT> const end_l1_to_l2_messages_tree_snapshot = { .root = l1_to_l2_messages_tree.root(),
                                                                             .next_available_leaf_index =
                                                                                 NUMBER_OF_L1_L2_MESSAGES_PER_ROLLUP };

    RootRollupInputs rootRollupInputs = get_root_rollup_inputs(composer, kernels, l1_to_l2_messages);
    RootRollupPublicInputs outputs =
        aztec3::circuits::rollup::native_root_rollup::root_rollup_circuit(composer, rootRollupInputs);

    // Check private data trees
    ASSERT_EQ(
        outputs.start_private_data_tree_snapshot,
        rootRollupInputs.previous_rollup_data[0].base_or_merge_rollup_public_inputs.start_private_data_tree_snapshot);
    ASSERT_EQ(
        outputs.end_private_data_tree_snapshot,
        rootRollupInputs.previous_rollup_data[1].base_or_merge_rollup_public_inputs.end_private_data_tree_snapshot);
    AppendOnlyTreeSnapshot<NT> const expected_private_data_tree_snapshot = { .root = data_tree.root(),
                                                                             .next_available_leaf_index = 16 };
    ASSERT_EQ(outputs.end_private_data_tree_snapshot, expected_private_data_tree_snapshot);

    // Check public data trees
    ASSERT_EQ(outputs.start_public_data_tree_root,
              rootRollupInputs.previous_rollup_data[0].base_or_merge_rollup_public_inputs.start_public_data_tree_root);
    ASSERT_EQ(outputs.end_public_data_tree_root,
              rootRollupInputs.previous_rollup_data[1].base_or_merge_rollup_public_inputs.end_public_data_tree_root);

    // check contract trees
    ASSERT_EQ(outputs.start_contract_tree_snapshot,
              rootRollupInputs.previous_rollup_data[0].base_or_merge_rollup_public_inputs.start_contract_tree_snapshot);
    ASSERT_EQ(outputs.end_contract_tree_snapshot,
              rootRollupInputs.previous_rollup_data[1].base_or_merge_rollup_public_inputs.end_contract_tree_snapshot);
    AppendOnlyTreeSnapshot<NT> const expected_contract_tree_snapshot{ .root = contract_tree.root(),
                                                                      .next_available_leaf_index = 4 };
    ASSERT_EQ(outputs.end_contract_tree_snapshot, expected_contract_tree_snapshot);

    // TODO: Check nullifier trees

    // Check historic data trees
    ASSERT_EQ(outputs.start_tree_of_historic_private_data_tree_roots_snapshot, start_historic_data_tree_snapshot);
    ASSERT_EQ(outputs.end_tree_of_historic_private_data_tree_roots_snapshot, end_historic_data_tree_snapshot);

    // Check historic contract trees
    ASSERT_EQ(outputs.start_tree_of_historic_contract_tree_roots_snapshot, start_historic_contract_tree_snapshot);
    ASSERT_EQ(outputs.end_tree_of_historic_contract_tree_roots_snapshot, end_historic_contract_tree_snapshot);

    // Check historic l1 to l2 messages trees
    ASSERT_EQ(outputs.start_tree_of_historic_l1_to_l2_messages_tree_roots_snapshot,
              start_historic_l1_to_l2_tree_snapshot);
    ASSERT_EQ(outputs.end_tree_of_historic_l1_to_l2_messages_tree_roots_snapshot, end_historic_l1_to_l2_tree_snapshot);

    // Check l1 to l2 messages trees
    ASSERT_EQ(outputs.start_l1_to_l2_messages_tree_snapshot, start_l1_to_l2_messages_tree_snapshot);
    ASSERT_EQ(outputs.end_l1_to_l2_messages_tree_snapshot, end_l1_to_l2_messages_tree_snapshot);

    EXPECT_FALSE(composer.failed());

    run_cbind(rootRollupInputs, outputs, true);
}

}  // namespace aztec3::circuits::rollup::root::native_root_rollup_circuit