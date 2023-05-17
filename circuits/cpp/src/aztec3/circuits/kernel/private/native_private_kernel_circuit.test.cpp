#include "c_bind.h"
#include "testing_harness.hpp"

#include "aztec3/circuits/apps/test_apps/basic_contract_deployment/basic_contract_deployment.hpp"
#include "aztec3/circuits/apps/test_apps/escrow/deposit.hpp"
#include "aztec3/utils/circuit_errors.hpp"

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
TEST(native_private_kernel_tests, native_deposit)
{
    NT::fr const& amount = 5;
    NT::fr const& asset_id = 1;
    NT::fr const& memo = 999;

    auto const& private_inputs = do_private_call_get_kernel_inputs(false, deposit, { amount, asset_id, memo });
    DummyComposer composer = DummyComposer("private_kernel_tests__native_deposit");
    auto const& public_inputs = native_private_kernel_circuit(composer, private_inputs);

    EXPECT_TRUE(validate_deployed_contract_address(private_inputs, public_inputs));
}

/**
 * @brief Some private circuit simulation (`constructor`, in this case)
 */
TEST(native_private_kernel_tests, native_basic_contract_deployment)
{
    NT::fr const& arg0 = 5;
    NT::fr const& arg1 = 1;
    NT::fr const& arg2 = 999;

    auto const& private_inputs = do_private_call_get_kernel_inputs(true, constructor, { arg0, arg1, arg2 });
    DummyComposer composer = DummyComposer("private_kernel_tests__native_basic_contract_deployment");
    auto const& public_inputs = native_private_kernel_circuit(composer, private_inputs);

    EXPECT_TRUE(validate_deployed_contract_address(private_inputs, public_inputs));
}

TEST(native_private_kernel_tests, native_contract_deployment_incorrect_constructor_vk_hash_fails)
{
    NT::fr const& arg0 = 5;
    NT::fr const& arg1 = 1;
    NT::fr const& arg2 = 999;

    auto private_inputs = do_private_call_get_kernel_inputs(true, constructor, { arg0, arg1, arg2 });

    // Pollute the constructor vk hash in the tx_request.
    private_inputs.signed_tx_request.tx_request.tx_context.contract_deployment_data.constructor_vk_hash =
        NT::fr::random_element();

    DummyComposer composer =
        DummyComposer("private_kernel_tests__native_contract_deployment_incorrect_constructor_vk_hash_fails");
    native_private_kernel_circuit(composer, private_inputs);

    EXPECT_EQ(composer.failed(), true);
    EXPECT_EQ(composer.get_first_failure().code, CircuitErrorCode::PRIVATE_KERNEL__INVALID_CONSTRUCTOR_VK_HASH);
    EXPECT_EQ(composer.get_first_failure().message, "constructor_vk_hash doesn't match private_call_vk_hash");
}

TEST(native_private_kernel_tests, native_contract_deployment_incorrect_storage_contract_address_fails)
{
    NT::fr const& arg0 = 5;
    NT::fr const& arg1 = 1;
    NT::fr const& arg2 = 999;

    auto private_inputs = do_private_call_get_kernel_inputs(true, constructor, { arg0, arg1, arg2 });

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
        DummyComposer("private_kernel_tests__native_contract_deployment_incorrect_storage_contract_address_fails");
    native_private_kernel_circuit(composer, private_inputs);

    // Assertion checks
    EXPECT_TRUE(composer.failed());
    EXPECT_EQ(composer.get_first_failure().code, CircuitErrorCode::PRIVATE_KERNEL__INVALID_CONTRACT_ADDRESS);
    EXPECT_EQ(composer.get_first_failure().message, "contract address supplied doesn't match derived address");
}

TEST(native_private_kernel_tests, native_private_function_zero_storage_contract_address_fails)
{
    NT::fr const& arg0 = 5;
    NT::fr const& arg1 = 1;
    NT::fr const& arg2 = 999;

    auto private_inputs = do_private_call_get_kernel_inputs(false, deposit, { arg0, arg1, arg2 });

    // Set storage_contract_address to 0
    private_inputs.private_call.call_stack_item.public_inputs.call_context.storage_contract_address = 0;
    private_inputs.private_call.call_stack_item.contract_address = 0;

    // Modify the call stack item's hash with the newly added contract address.
    private_inputs.previous_kernel.public_inputs.end.private_call_stack[0] =
        private_inputs.private_call.call_stack_item.hash();

    // Invoke the native private kernel circuit
    DummyComposer composer =
        DummyComposer("private_kernel_tests__native_private_function_zero_storage_contract_address_fails");
    native_private_kernel_circuit(composer, private_inputs);

    // Assertion checks
    EXPECT_TRUE(composer.failed());
    EXPECT_EQ(composer.get_first_failure().code, CircuitErrorCode::PRIVATE_KERNEL__INVALID_CONTRACT_ADDRESS);
    EXPECT_EQ(composer.get_first_failure().message,
              "contract address can't be 0 for non-contract deployment related transactions");
}

TEST(native_private_kernel_tests, native_private_function_incorrect_contract_tree_root_fails)
{
    NT::fr const& arg0 = 5;
    NT::fr const& arg1 = 1;
    NT::fr const& arg2 = 999;

    auto private_inputs = do_private_call_get_kernel_inputs(false, deposit, { arg0, arg1, arg2 });

    // Set private_historic_tree_roots to a random scalar.
    private_inputs.previous_kernel.public_inputs.constants.historic_tree_roots.private_historic_tree_roots
        .contract_tree_root = NT::fr::random_element();

    // Invoke the native private kernel circuit
    DummyComposer composer =
        DummyComposer("private_kernel_tests__native_private_function_incorrect_contract_tree_root_fails");
    native_private_kernel_circuit(composer, private_inputs);

    // Assertion checks
    EXPECT_TRUE(composer.failed());
    EXPECT_EQ(
        composer.get_first_failure().code,
        CircuitErrorCode::PRIVATE_KERNEL__PURPORTED_CONTRACT_TREE_ROOT_AND_PREVIOUS_KERNEL_CONTRACT_TREE_ROOT_MISMATCH);
    EXPECT_EQ(composer.get_first_failure().message,
              "purported_contract_tree_root doesn't match previous_kernel_contract_tree_root");
}

TEST(native_private_kernel_tests, native_private_function_incorrect_contract_leaf_index_fails)
{
    NT::fr const& arg0 = 5;
    NT::fr const& arg1 = 1;
    NT::fr const& arg2 = 999;

    auto private_inputs = do_private_call_get_kernel_inputs(false, deposit, { arg0, arg1, arg2 });

    // Set the leaf index of the contract leaf to 20 (the correct value is 1).
    private_inputs.private_call.contract_leaf_membership_witness.leaf_index = 20;

    // Invoke the native private kernel circuit
    DummyComposer composer =
        DummyComposer("private_kernel_tests__native_private_function_incorrect_contract_leaf_index_fails");
    native_private_kernel_circuit(composer, private_inputs);

    // Assertion checks
    EXPECT_TRUE(composer.failed());
    EXPECT_EQ(composer.get_first_failure().code,
              CircuitErrorCode::PRIVATE_KERNEL__COMPUTED_CONTRACT_TREE_ROOT_AND_PURPORTED_CONTRACT_TREE_ROOT_MISMATCH);
    EXPECT_EQ(composer.get_first_failure().message,
              "computed_contract_tree_root doesn't match purported_contract_tree_root");
}

/**
 * @brief Test this dummy cbind
 */
TEST(native_private_kernel_tests, native_dummy_previous_kernel_cbind)
{
    uint8_t const* cbind_previous_kernel_buf = nullptr;
    size_t const cbind_buf_size = private_kernel__dummy_previous_kernel(&cbind_previous_kernel_buf);

    auto const& previous_kernel = utils::dummy_previous_kernel();
    std::vector<uint8_t> expected_vec;
    write(expected_vec, previous_kernel);

    // Just compare the first 10 bytes of the serialized public outputs
    // TODO(david): this is not a good test as it only checks a few bytes
    // would be best if we could just check struct equality or check
    // equality of an entire memory region (same as other similar TODOs
    // in other test files)
    // TODO(david): better equality check
    // for (size_t i = 0; i < cbind_buf_size; i++) {
    for (size_t i = 0; i < 10; i++) {
        ASSERT_EQ(cbind_previous_kernel_buf[i], expected_vec[i]);
    }
    (void)cbind_buf_size;
}

}  // namespace aztec3::circuits::kernel::private_kernel
