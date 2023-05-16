#include "index.hpp"
#include "init.hpp"

#include "aztec3/circuits/kernel/private/utils.hpp"
#include "aztec3/constants.hpp"
#include <aztec3/circuits/abis/call_context.hpp>
#include <aztec3/circuits/abis/call_stack_item.hpp>
#include <aztec3/circuits/abis/combined_accumulated_data.hpp>
#include <aztec3/circuits/abis/combined_constant_data.hpp>
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
#include <aztec3/circuits/abis/types.hpp>
#include <aztec3/circuits/apps/function_execution_context.hpp>
#include <aztec3/circuits/apps/test_apps/basic_contract_deployment/basic_contract_deployment.hpp>
#include <aztec3/circuits/apps/test_apps/escrow/deposit.hpp>
#include <aztec3/circuits/hash.hpp>
#include <aztec3/circuits/mock/mock_kernel_circuit.hpp>

#include <barretenberg/common/map.hpp>
#include <barretenberg/stdlib/merkle_tree/membership.hpp>


namespace {

using aztec3::circuits::compute_empty_sibling_path;
using aztec3::circuits::abis::FunctionLeafPreimage;
using aztec3::circuits::abis::KernelCircuitPublicInputs;
using aztec3::circuits::abis::NewContractData;
using aztec3::circuits::abis::OptionalPrivateCircuitPublicInputs;
using aztec3::circuits::abis::private_kernel::PrivateInputs;

using DummyComposer = aztec3::utils::DummyComposer;

// A type representing any private circuit function
// (for now it works for deposit and constructor)
using private_function = std::function<OptionalPrivateCircuitPublicInputs<NT>(
    FunctionExecutionContext<aztec3::circuits::kernel::private_kernel::Composer>&,
    std::array<NT::fr, aztec3::ARGS_LENGTH> const&)>;

}  // namespace

namespace aztec3::circuits::kernel::private_kernel::testing_harness {

using aztec3::circuits::compute_empty_sibling_path;

// Some helper constants for trees
constexpr size_t MAX_FUNCTION_LEAVES = 2 << (aztec3::FUNCTION_TREE_HEIGHT - 1);
const NT::fr EMPTY_FUNCTION_LEAF = FunctionLeafPreimage<NT>{}.hash();  // hash of empty/0 preimage
const NT::fr EMPTY_CONTRACT_LEAF = NewContractData<NT>{}.hash();       // hash of empty/0 preimage

inline const auto& get_empty_function_siblings()
{
    static auto EMPTY_FUNCTION_SIBLINGS = []() {
        const auto result = compute_empty_sibling_path<NT, aztec3::FUNCTION_TREE_HEIGHT>(EMPTY_FUNCTION_LEAF);
        return result;
    }();
    return EMPTY_FUNCTION_SIBLINGS;
}

inline const auto& get_empty_contract_siblings()
{
    static auto EMPTY_CONTRACT_SIBLINGS = []() {
        const auto result = compute_empty_sibling_path<NT, aztec3::CONTRACT_TREE_HEIGHT>(EMPTY_CONTRACT_LEAF);
        return result;
    }();
    return EMPTY_CONTRACT_SIBLINGS;
}

/**
 * @brief Generate a verification key for a private circuit.
 *
 * @details Use some dummy inputs just to get the VK for a private circuit
 *
 * @param is_constructor Whether this private call is a constructor call
 * @param func The private circuit call to generate a VK for
 * @param num_args Number of args to that private circuit call
 * @return std::shared_ptr<NT::VK> - the generated VK
 */
std::shared_ptr<NT::VK> gen_func_vk(bool is_constructor, private_function const& func, size_t num_args);

/**
 * @brief Perform a private circuit call and generate the inputs to private kernel
 *
 * @param is_constructor whether this private circuit call is a constructor
 * @param func the private circuit call being validated by this kernel iteration
 * @param args_vec the private call's args
 * @return PrivateInputs<NT> - the inputs to the private call circuit
 */
PrivateInputs<NT> do_private_call_get_kernel_inputs(bool is_constructor,
                                                    private_function const& func,
                                                    std::vector<NT::fr> const& args_vec,
                                                    bool real_kernel_circuit = false);

/**
 * @brief Validate that the deployed contract address is correct.
 *
 * @details Compare the public inputs new contract address
 * with one manually computed from private inputs.
 * @param private_inputs to be used in manual computation
 * @param public_inputs that contain the expected new contract address
 */
bool validate_deployed_contract_address_(PrivateInputs<NT> const& private_inputs,
                                         KernelCircuitPublicInputs<NT> const& public_inputs);

}  // namespace aztec3::circuits::kernel::private_kernel::testing_harness
