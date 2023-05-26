#include "c_bind.h"
#include "testing_harness.hpp"

#include "aztec3/circuits/apps/test_apps/basic_contract_deployment/basic_contract_deployment.hpp"
#include "aztec3/circuits/apps/test_apps/escrow/deposit.hpp"
#include "aztec3/utils/circuit_errors.hpp"

#include "barretenberg/serialize/test_helper.hpp"
#include <barretenberg/common/test.hpp>

#include <gtest/gtest.h>

#include <cstdint>

namespace {

using aztec3::circuits::apps::test_apps::basic_contract_deployment::constructor;
using aztec3::circuits::apps::test_apps::escrow::deposit;

using aztec3::circuits::kernel::private_kernel::testing_harness::do_private_call_get_kernel_inputs_init;
using aztec3::circuits::kernel::private_kernel::testing_harness::do_private_call_get_kernel_inputs_inner;
using aztec3::circuits::kernel::private_kernel::testing_harness::validate_deployed_contract_address;
using aztec3::utils::CircuitErrorCode;

}  // namespace

namespace aztec3::circuits::kernel::private_kernel {

/**
 * @brief Some private circuit simulation (`deposit`, in this case)
 */
TEST(native_private_kernel_tests, deposit)
{
    NT::fr const amount = 5;
    NT::fr const asset_id = 1;
    NT::fr const memo = 999;

    auto const& private_inputs = do_private_call_get_kernel_inputs_init(false, deposit, { amount, asset_id, memo });
    DummyComposer composer = DummyComposer("private_kernel_tests__native_deposit");
    auto const& public_inputs = native_private_kernel_circuit_initial(composer, private_inputs);

    EXPECT_TRUE(validate_deployed_contract_address(private_inputs, public_inputs));
}

/**
 * @brief Some private circuit simulation (`constructor`, in this case)
 */
TEST(native_private_kernel_tests, basic_contract_deployment)
{
    NT::fr const arg0 = 5;
    NT::fr const arg1 = 1;
    NT::fr const arg2 = 999;

    auto const& private_inputs = do_private_call_get_kernel_inputs_init(true, constructor, { arg0, arg1, arg2 });
    DummyComposer composer = DummyComposer("private_kernel_tests__native_basic_contract_deployment");
    auto const& public_inputs = native_private_kernel_circuit_initial(composer, private_inputs);

    EXPECT_TRUE(validate_deployed_contract_address(private_inputs, public_inputs));
}

TEST(native_private_kernel_tests, contract_deployment_incorrect_constructor_vk_hash_fails)
{
    NT::fr const arg0 = 5;
    NT::fr const arg1 = 1;
    NT::fr const arg2 = 999;

    auto private_inputs = do_private_call_get_kernel_inputs_init(true, constructor, { arg0, arg1, arg2 });

    // Pollute the constructor vk hash in the tx_request.
    private_inputs.signed_tx_request.tx_request.tx_context.contract_deployment_data.constructor_vk_hash =
        NT::fr::random_element();

    DummyComposer composer =
        DummyComposer("private_kernel_tests__contract_deployment_incorrect_constructor_vk_hash_fails");
    native_private_kernel_circuit_initial(composer, private_inputs);

    EXPECT_EQ(composer.failed(), true);
    EXPECT_EQ(composer.get_first_failure().code, CircuitErrorCode::PRIVATE_KERNEL__INVALID_CONSTRUCTOR_VK_HASH);
    EXPECT_EQ(composer.get_first_failure().message, "constructor_vk_hash doesn't match private_call_vk_hash");
}

TEST(native_private_kernel_tests, contract_deployment_incorrect_storage_contract_address_fails)
{
    NT::fr const arg0 = 5;
    NT::fr const arg1 = 1;
    NT::fr const arg2 = 999;

    auto private_inputs = do_private_call_get_kernel_inputs_inner(true, constructor, { arg0, arg1, arg2 });

    // Modify the storage_contract_address.
    const auto random_contract_address = NT::fr::random_element();
    private_inputs.private_call.call_stack_item.public_inputs.call_context.storage_contract_address =
        random_contract_address;
    private_inputs.private_call.call_stack_item.contract_address = random_contract_address;

    // Modify the call stack item's hash with the newly added contract address.
    private_inputs.previous_kernel.public_inputs.end.private_call_stack[0] =
        private_inputs.private_call.call_stack_item.hash();

    // Invoke the native private kernel circuit
    DummyComposer composer =
        DummyComposer("private_kernel_tests__contract_deployment_incorrect_storage_contract_address_fails");
    native_private_kernel_circuit_inner(composer, private_inputs);

    // Assertion checks
    EXPECT_TRUE(composer.failed());
    EXPECT_EQ(composer.get_first_failure().code, CircuitErrorCode::PRIVATE_KERNEL__INVALID_CONTRACT_ADDRESS);
    EXPECT_EQ(composer.get_first_failure().message, "contract address supplied doesn't match derived address");
}

TEST(native_private_kernel_tests, private_function_zero_storage_contract_address_fails)
{
    NT::fr const arg0 = 5;
    NT::fr const arg1 = 1;
    NT::fr const arg2 = 999;

    auto private_inputs = do_private_call_get_kernel_inputs_inner(false, deposit, { arg0, arg1, arg2 });

    // Set storage_contract_address to 0
    private_inputs.private_call.call_stack_item.public_inputs.call_context.storage_contract_address = 0;
    private_inputs.private_call.call_stack_item.contract_address = 0;

    // Modify the call stack item's hash with the newly added contract address.
    private_inputs.previous_kernel.public_inputs.end.private_call_stack[0] =
        private_inputs.private_call.call_stack_item.hash();

    // Invoke the native private kernel circuit
    DummyComposer composer =
        DummyComposer("private_kernel_tests__private_function_zero_storage_contract_address_fails");
    native_private_kernel_circuit_inner(composer, private_inputs);

    // Assertion checks
    EXPECT_TRUE(composer.failed());
    EXPECT_EQ(composer.get_first_failure().code, CircuitErrorCode::PRIVATE_KERNEL__INVALID_CONTRACT_ADDRESS);
    EXPECT_EQ(composer.get_first_failure().message,
              "contract address can't be 0 for non-contract deployment related transactions");
}

TEST(native_private_kernel_tests, private_function_incorrect_contract_tree_root_fails)
{
    NT::fr const arg0 = 5;
    NT::fr const arg1 = 1;
    NT::fr const arg2 = 999;

    auto private_inputs = do_private_call_get_kernel_inputs_inner(false, deposit, { arg0, arg1, arg2 });

    // Set private_historic_tree_roots to a random scalar.
    private_inputs.previous_kernel.public_inputs.constants.historic_tree_roots.private_historic_tree_roots
        .contract_tree_root = NT::fr::random_element();

    // Invoke the native private kernel circuit
    DummyComposer composer = DummyComposer("private_kernel_tests__private_function_incorrect_contract_tree_root_fails");
    native_private_kernel_circuit_inner(composer, private_inputs);

    // Assertion checks
    EXPECT_TRUE(composer.failed());
    EXPECT_EQ(
        composer.get_first_failure().code,
        CircuitErrorCode::PRIVATE_KERNEL__PURPORTED_CONTRACT_TREE_ROOT_AND_PREVIOUS_KERNEL_CONTRACT_TREE_ROOT_MISMATCH);
    EXPECT_EQ(composer.get_first_failure().message,
              "purported_contract_tree_root doesn't match previous_kernel_contract_tree_root");
}

TEST(native_private_kernel_tests, private_function_incorrect_contract_leaf_index_fails)
{
    NT::fr const arg0 = 5;
    NT::fr const arg1 = 1;
    NT::fr const arg2 = 999;

    auto private_inputs = do_private_call_get_kernel_inputs_inner(false, deposit, { arg0, arg1, arg2 });

    // Set the leaf index of the contract leaf to 20 (the correct value is 1).
    NT::fr const wrong_idx = 20;
    private_inputs.private_call.contract_leaf_membership_witness.leaf_index = wrong_idx;

    // Invoke the native private kernel circuit
    DummyComposer composer =
        DummyComposer("private_kernel_tests__private_function_incorrect_contract_leaf_index_fails");
    native_private_kernel_circuit_inner(composer, private_inputs);

    // Assertion checks
    EXPECT_TRUE(composer.failed());
    EXPECT_EQ(composer.get_first_failure().code,
              CircuitErrorCode::PRIVATE_KERNEL__COMPUTED_CONTRACT_TREE_ROOT_AND_PURPORTED_CONTRACT_TREE_ROOT_MISMATCH);
    EXPECT_EQ(composer.get_first_failure().message,
              "computed_contract_tree_root doesn't match purported_contract_tree_root");
}

TEST(native_private_kernel_tests, private_function_incorrect_contract_leaf_sibling_path_fails)
{
    NT::fr const arg0 = 5;
    NT::fr const arg1 = 1;
    NT::fr const arg2 = 999;

    auto private_inputs = do_private_call_get_kernel_inputs_inner(false, deposit, { arg0, arg1, arg2 });

    // Change the contract leaf's membership proof.
    private_inputs.private_call.contract_leaf_membership_witness.sibling_path[0] = fr::random_element();

    // Invoke the native private kernel circuit
    DummyComposer composer =
        DummyComposer("private_kernel_tests__private_function_incorrect_contract_leaf_sibling_path_fails");
    native_private_kernel_circuit_inner(composer, private_inputs);

    // Assertion checks
    EXPECT_TRUE(composer.failed());
    EXPECT_EQ(composer.get_first_failure().code,
              CircuitErrorCode::PRIVATE_KERNEL__COMPUTED_CONTRACT_TREE_ROOT_AND_PURPORTED_CONTRACT_TREE_ROOT_MISMATCH);
    EXPECT_EQ(composer.get_first_failure().message,
              "computed_contract_tree_root doesn't match purported_contract_tree_root");
}

TEST(native_private_kernel_tests, private_function_incorrect_function_leaf_index_fails)
{
    NT::fr const arg0 = 5;
    NT::fr const arg1 = 1;
    NT::fr const arg2 = 999;

    auto private_inputs = do_private_call_get_kernel_inputs_inner(false, deposit, { arg0, arg1, arg2 });

    // Set the leaf index of the function leaf to 10 (the correct value is 1).
    NT::fr const wrong_idx = 10;
    private_inputs.private_call.function_leaf_membership_witness.leaf_index = wrong_idx;

    // Invoke the native private kernel circuit
    DummyComposer composer =
        DummyComposer("private_kernel_tests__private_function_incorrect_contract_leaf_index_fails");
    native_private_kernel_circuit_inner(composer, private_inputs);

    // Assertion checks
    EXPECT_TRUE(composer.failed());
    EXPECT_EQ(composer.get_first_failure().code,
              CircuitErrorCode::PRIVATE_KERNEL__COMPUTED_CONTRACT_TREE_ROOT_AND_PURPORTED_CONTRACT_TREE_ROOT_MISMATCH);
    EXPECT_EQ(composer.get_first_failure().message,
              "computed_contract_tree_root doesn't match purported_contract_tree_root");
}

TEST(native_private_kernel_tests, private_function_incorrect_function_leaf_sibling_path_fails)
{
    NT::fr const arg0 = 5;
    NT::fr const arg1 = 1;
    NT::fr const arg2 = 999;

    auto private_inputs = do_private_call_get_kernel_inputs_inner(false, deposit, { arg0, arg1, arg2 });

    // Change the function leaf's membership proof.
    private_inputs.private_call.function_leaf_membership_witness.sibling_path[0] = fr::random_element();

    // Invoke the native private kernel circuit
    DummyComposer composer =
        DummyComposer("private_kernel_tests__private_function_incorrect_contract_leaf_sibling_path_fails");
    native_private_kernel_circuit_inner(composer, private_inputs);

    // Assertion checks
    EXPECT_TRUE(composer.failed());
    EXPECT_EQ(composer.get_first_failure().code,
              CircuitErrorCode::PRIVATE_KERNEL__COMPUTED_CONTRACT_TREE_ROOT_AND_PURPORTED_CONTRACT_TREE_ROOT_MISMATCH);
    EXPECT_EQ(composer.get_first_failure().message,
              "computed_contract_tree_root doesn't match purported_contract_tree_root");
}

TEST(native_private_kernel_tests, private_function_incorrect_call_stack_item_hash_fails)
{
    NT::fr const arg0 = 5;
    NT::fr const arg1 = 1;
    NT::fr const arg2 = 999;

    auto private_inputs = do_private_call_get_kernel_inputs_inner(false, deposit, { arg0, arg1, arg2 });

    // Set the first call stack member corresponding to the `deposit` function to random scalar.
    private_inputs.private_call.call_stack_item.public_inputs.private_call_stack[0] = NT::fr::random_element();

    // Invoke the native private kernel circuit
    DummyComposer composer =
        DummyComposer("private_kernel_tests__private_function_incorrect_call_stack_item_hash_fails");
    native_private_kernel_circuit_inner(composer, private_inputs);

    // Assertion checks
    EXPECT_TRUE(composer.failed());
    EXPECT_EQ(composer.get_first_failure().code,
              CircuitErrorCode::PRIVATE_KERNEL__CALCULATED_PRIVATE_CALL_HASH_AND_PROVIDED_PRIVATE_CALL_HASH_MISMATCH);
    EXPECT_EQ(composer.get_first_failure().message,
              "calculated private_call_hash does not match provided private_call_hash at the top of the call stack");
}


TEST(native_private_kernel_tests, private_function_is_private_false_fails)
{
    NT::fr const arg0 = 5;
    NT::fr const arg1 = 1;
    NT::fr const arg2 = 999;

    auto private_inputs = do_private_call_get_kernel_inputs_inner(false, deposit, { arg0, arg1, arg2 });

    // Set is_private in function data to false.
    private_inputs.private_call.call_stack_item.function_data.is_private = false;

    // Invoke the native private kernel circuit
    DummyComposer composer = DummyComposer("private_kernel_tests__private_function_is_private_false_fails");
    native_private_kernel_circuit_inner(composer, private_inputs);

    // Assertion checks
    EXPECT_TRUE(composer.failed());
    EXPECT_EQ(composer.get_first_failure().code,
              CircuitErrorCode::PRIVATE_KERNEL__NON_PRIVATE_FUNCTION_EXECUTED_WITH_PRIVATE_KERNEL);
    EXPECT_EQ(composer.get_first_failure().message,
              "Cannot execute a non-private function with the private kernel circuit");
}

TEST(native_private_kernel_tests, private_function_incorrect_private_call_length_fails)
{
    NT::fr const arg0 = 5;
    NT::fr const arg1 = 1;
    NT::fr const arg2 = 999;

    auto private_inputs = do_private_call_get_kernel_inputs_inner(false, deposit, { arg0, arg1, arg2 });

    // Set the second element in the private call stack to a non-zero scalar.
    private_inputs.previous_kernel.public_inputs.end.private_call_stack[1] = fr::random_element();
    // private_inputs.previous_kernel.public_inputs.end.private_call_count = 2;

    // Invoke the native private kernel circuit
    DummyComposer composer =
        DummyComposer("private_kernel_tests__private_function_incorrect_private_call_length_fails");
    native_private_kernel_circuit_inner(composer, private_inputs);

    // Assertion checks
    EXPECT_TRUE(composer.failed());
    EXPECT_EQ(composer.get_first_failure().code, CircuitErrorCode::PRIVATE_KERNEL__PRIVATE_CALL_STACK_LENGTH_MISMATCH);
    EXPECT_EQ(composer.get_first_failure().message, "Private call stack must be length 1");
}

TEST(native_private_kernel_tests, private_function_non_empty_public_call_stack_fails)
{
    NT::fr const arg0 = 5;
    NT::fr const arg1 = 1;
    NT::fr const arg2 = 999;

    auto private_inputs = do_private_call_get_kernel_inputs_inner(false, deposit, { arg0, arg1, arg2 });

    // Set the first element in the public call stack to a non-zero scalar.
    private_inputs.previous_kernel.public_inputs.end.public_call_stack[0] = fr::random_element();

    // Invoke the native private kernel circuit
    DummyComposer composer = DummyComposer("private_kernel_tests__private_function_non_empty_public_call_stack_fails");
    native_private_kernel_circuit_inner(composer, private_inputs);

    // Assertion checks
    EXPECT_TRUE(composer.failed());
    EXPECT_EQ(composer.get_first_failure().code, CircuitErrorCode::PRIVATE_KERNEL__UNSUPPORTED_OP);
    EXPECT_EQ(composer.get_first_failure().message, "Public call stack must be empty");
}

TEST(native_private_kernel_tests, private_function_non_empty_new_l1_l2_msg_fails)
{
    NT::fr const arg0 = 5;
    NT::fr const arg1 = 1;
    NT::fr const arg2 = 999;

    auto private_inputs = do_private_call_get_kernel_inputs_inner(false, deposit, { arg0, arg1, arg2 });

    // Set the first element in the l2 to l1 message array to a non-zero scalar.
    private_inputs.previous_kernel.public_inputs.end.new_l2_to_l1_msgs[0] = fr::random_element();

    // Invoke the native private kernel circuit
    DummyComposer composer = DummyComposer("private_kernel_tests__private_function_non_empty_new_l1_l2_msg_fails");
    native_private_kernel_circuit_inner(composer, private_inputs);

    // Assertion checks
    EXPECT_TRUE(composer.failed());
    EXPECT_EQ(composer.get_first_failure().code, CircuitErrorCode::PRIVATE_KERNEL__UNSUPPORTED_OP);
    EXPECT_EQ(composer.get_first_failure().message, "L2 to L1 msgs must be empty");
}

TEST(native_private_kernel_tests, private_function_static_call_fails)
{
    NT::fr const arg0 = 5;
    NT::fr const arg1 = 1;
    NT::fr const arg2 = 999;

    auto private_inputs = do_private_call_get_kernel_inputs_inner(false, deposit, { arg0, arg1, arg2 });

    // Set is_static_call to true.
    private_inputs.private_call.call_stack_item.public_inputs.call_context.is_static_call = true;

    // Invoke the native private kernel circuit
    DummyComposer composer = DummyComposer("private_kernel_tests__private_function_static_call_fails");
    native_private_kernel_circuit_inner(composer, private_inputs);

    // Assertion checks
    EXPECT_TRUE(composer.failed());
    EXPECT_EQ(composer.get_first_failure().code, CircuitErrorCode::PRIVATE_KERNEL__UNSUPPORTED_OP);
    EXPECT_EQ(composer.get_first_failure().message, "Users cannot make a static call");
}

TEST(native_private_kernel_tests, private_function_delegate_call_fails)
{
    NT::fr const arg0 = 5;
    NT::fr const arg1 = 1;
    NT::fr const arg2 = 999;

    auto private_inputs = do_private_call_get_kernel_inputs_inner(false, deposit, { arg0, arg1, arg2 });

    // Set is_delegate_call to true.
    private_inputs.private_call.call_stack_item.public_inputs.call_context.is_delegate_call = true;

    // Invoke the native private kernel circuit
    DummyComposer composer = DummyComposer("private_kernel_tests__private_function_delegate_call_fails");
    native_private_kernel_circuit_inner(composer, private_inputs);

    // Assertion checks
    EXPECT_TRUE(composer.failed());
    EXPECT_EQ(composer.get_first_failure().code, CircuitErrorCode::PRIVATE_KERNEL__UNSUPPORTED_OP);
    EXPECT_EQ(composer.get_first_failure().message, "Users cannot make a delegatecall");
}

TEST(native_private_kernel_tests, private_function_incorrect_storage_contract_address_fails)
{
    NT::fr const arg0 = 5;
    NT::fr const arg1 = 1;
    NT::fr const arg2 = 999;

    auto private_inputs = do_private_call_get_kernel_inputs_inner(false, deposit, { arg0, arg1, arg2 });

    // Set the storage_contract_address to a random scalar.
    private_inputs.private_call.call_stack_item.public_inputs.call_context.storage_contract_address =
        NT::fr::random_element();

    // Invoke the native private kernel circuit
    DummyComposer composer =
        DummyComposer("private_kernel_tests__private_function_incorrect_storage_contract_address_fails");
    native_private_kernel_circuit_inner(composer, private_inputs);

    // Assertion checks
    EXPECT_TRUE(composer.failed());
    EXPECT_EQ(composer.get_first_failure().code, CircuitErrorCode::PRIVATE_KERNEL__CONTRACT_ADDRESS_MISMATCH);
    EXPECT_EQ(composer.get_first_failure().message, "Storage contract address must be that of the called contract");
}

TEST(native_private_kernel_tests, private_function_recursive_mock_kernel_proof_fails)
{
    NT::fr const arg0 = 5;
    NT::fr const arg1 = 1;
    NT::fr const arg2 = 999;

    auto private_inputs = do_private_call_get_kernel_inputs_inner(false, deposit, { arg0, arg1, arg2 });

    // Make the mock kernel proof in the first iteration to contain a recursive proof.
    // Generally, its okay to include a valid "mock" aggregated state in the mock proof but it'd just add
    // more gates. Anyway mock kernel proof is going to be removed so this test is temporary.
    private_inputs.previous_kernel.vk->contains_recursive_proof = true;

    // Invoke the native private kernel circuit
    DummyComposer composer = DummyComposer("private_kernel_tests__private_function_recursive_mock_kernel_proof_fails");
    native_private_kernel_circuit_inner(composer, private_inputs);

    // Assertion checks
    EXPECT_TRUE(composer.failed());
    EXPECT_EQ(composer.get_first_failure().code,
              CircuitErrorCode::PRIVATE_KERNEL__KERNEL_PROOF_CONTAINS_RECURSIVE_PROOF);
    EXPECT_EQ(composer.get_first_failure().message, "Mock kernel proof must not contain a recursive proof");
}

/*
TEST(native_private_kernel_tests, private_function_with_arbitrary_private_call_count)
{
    NT::fr const arg0 = 5;
    NT::fr const arg1 = 1;
    NT::fr const arg2 = 999;

    auto private_inputs = do_private_call_get_kernel_inputs_inner(false, deposit, { arg0, arg1, arg2 });

    // Set the private call count > 0.
    NT::fr const priv_call_count = 10;
    private_inputs.previous_kernel.public_inputs.end.private_call_count = priv_call_count;

    // Invoke the native private kernel circuit
    DummyComposer composer = DummyComposer("private_kernel_tests__private_function_with_arbitrary_private_call_count");
    auto public_inputs = native_private_kernel_circuit_inner(composer, private_inputs);

    // Assertion checks
    EXPECT_TRUE(validate_deployed_contract_address(private_inputs, public_inputs));
}

TEST(native_private_kernel_tests, private_function_non_base_case_previous_kernel_public_fails)
{
    NT::fr const arg0 = 5;
    NT::fr const arg1 = 1;
    NT::fr const arg2 = 999;

    auto private_inputs = do_private_call_get_kernel_inputs_inner(false, deposit, { arg0, arg1, arg2 });

    // Set the private call count > 0 and mark previous kernel proof as public.
    private_inputs.previous_kernel.public_inputs.end.private_call_count = 1;
    private_inputs.previous_kernel.public_inputs.is_private = false;

    // Invoke the native private kernel circuit
    DummyComposer composer =
        DummyComposer("private_kernel_tests__private_function_non_base_case_previous_kernel_public_fails");
    native_private_kernel_circuit_inner(composer, private_inputs);

    // Assertion checks
    EXPECT_TRUE(composer.failed());
    EXPECT_EQ(composer.get_first_failure().code,
              CircuitErrorCode::PRIVATE_KERNEL__NON_PRIVATE_KERNEL_VERIFIED_WITH_PRIVATE_KERNEL);
    EXPECT_EQ(composer.get_first_failure().message,
              "Cannot verify a non-private kernel snark in the private kernel circuit");
}

TEST(native_private_kernel_tests, constructor_in_non_base_case_fails)
{
    NT::fr const arg0 = 5;
    NT::fr const arg1 = 1;
    NT::fr const arg2 = 999;

    auto private_inputs = do_private_call_get_kernel_inputs_inner(true, constructor, { arg0, arg1, arg2 });

    // Set the private call count > 0 and we are verifying the constructor proof.
    private_inputs.previous_kernel.public_inputs.end.private_call_count = 1;

    // Invoke the native private kernel circuit
    DummyComposer composer = DummyComposer("private_kernel_tests__constructor_in_non_base_case_fails");
    native_private_kernel_circuit_inner(composer, private_inputs);

    // Assertion checks
    EXPECT_TRUE(composer.failed());
    EXPECT_EQ(composer.get_first_failure().code, CircuitErrorCode::PRIVATE_KERNEL__CONSTRUCTOR_EXECUTED_IN_RECURSION);
    EXPECT_EQ(composer.get_first_failure().message, "A constructor must be executed as the first tx in the recursion");
}
*/


/**
 * @brief Test this dummy cbind
 */
TEST(private_kernel_tests, cbind_private_kernel__dummy_previous_kernel)
{
    auto func = [] { return aztec3::circuits::kernel::private_kernel::utils::dummy_previous_kernel(); };
    auto [actual, expected] = call_func_and_wrapper(func, private_kernel__dummy_previous_kernel);
    // TODO(AD): investigate why direct operator== didn't work
    std::stringstream actual_ss;
    std::stringstream expected_ss;
    actual_ss << actual;
    expected_ss << expected;
    EXPECT_EQ(actual_ss.str(), expected_ss.str());
}

}  // namespace aztec3::circuits::kernel::private_kernel
