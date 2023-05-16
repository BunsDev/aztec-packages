#include "c_bind.h"
#include "testing_harness.hpp"

#include "aztec3/circuits/apps/test_apps/basic_contract_deployment/basic_contract_deployment.hpp"
#include "aztec3/circuits/apps/test_apps/escrow/deposit.hpp"

#include <barretenberg/common/test.hpp>

#include <gtest/gtest.h>

#include <cstdint>

namespace {

using aztec3::circuits::apps::test_apps::basic_contract_deployment::constructor;
using aztec3::circuits::apps::test_apps::escrow::deposit;

using aztec3::circuits::kernel::private_kernel::testing_harness::do_private_call_get_kernel_inputs_init;
using aztec3::circuits::kernel::private_kernel::testing_harness::do_private_call_get_kernel_inputs_inner;
using aztec3::circuits::kernel::private_kernel::testing_harness::validate_deployed_contract_address;

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
    EXPECT_EQ(composer.get_first_failure().code,
              ::aztec3::utils::CircuitErrorCode::PRIVATE_KERNEL__INVALID_CONSTRUCTOR_VK_HASH);
    EXPECT_EQ(composer.get_first_failure().message, "constructor_vk_hash doesn't match private_call_vk_hash");
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
