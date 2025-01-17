#include "c_bind.h"
#include "index.hpp"

#include "aztec3/circuits/abis/call_context.hpp"
#include "aztec3/circuits/abis/call_stack_item.hpp"
#include "aztec3/circuits/abis/combined_accumulated_data.hpp"
#include "aztec3/circuits/abis/combined_constant_data.hpp"
#include "aztec3/circuits/abis/contract_deployment_data.hpp"
#include "aztec3/circuits/abis/function_data.hpp"
#include "aztec3/circuits/abis/kernel_circuit_public_inputs.hpp"
#include "aztec3/circuits/abis/private_circuit_public_inputs.hpp"
#include "aztec3/circuits/abis/private_historic_tree_roots.hpp"
#include "aztec3/circuits/abis/private_kernel/private_call_data.hpp"
#include "aztec3/circuits/abis/private_kernel/private_kernel_inputs_init.hpp"
#include "aztec3/circuits/abis/private_kernel/private_kernel_inputs_inner.hpp"
#include "aztec3/circuits/abis/signed_tx_request.hpp"
#include "aztec3/circuits/abis/tx_context.hpp"
#include "aztec3/circuits/abis/tx_request.hpp"
#include "aztec3/circuits/abis/types.hpp"
#include "aztec3/circuits/apps/function_execution_context.hpp"
#include "aztec3/circuits/apps/test_apps/basic_contract_deployment/basic_contract_deployment.hpp"
#include "aztec3/circuits/apps/test_apps/escrow/deposit.hpp"
#include "aztec3/circuits/hash.hpp"
#include "aztec3/circuits/kernel/private/utils.hpp"
#include "aztec3/constants.hpp"
#include "aztec3/utils/circuit_errors.hpp"

#include <barretenberg/common/map.hpp>
#include <barretenberg/common/test.hpp>
#include <barretenberg/serialize/test_helper.hpp>
#include <barretenberg/stdlib/merkle_tree/membership.hpp>

#include <gtest/gtest.h>

namespace {

using aztec3::circuits::compute_empty_sibling_path;
using aztec3::circuits::abis::CallContext;
using aztec3::circuits::abis::CallStackItem;
using aztec3::circuits::abis::CombinedAccumulatedData;
using aztec3::circuits::abis::CombinedConstantData;
using aztec3::circuits::abis::CombinedHistoricTreeRoots;
using aztec3::circuits::abis::ContractDeploymentData;
using aztec3::circuits::abis::FunctionData;
using aztec3::circuits::abis::FunctionLeafPreimage;
using aztec3::circuits::abis::KernelCircuitPublicInputs;
using aztec3::circuits::abis::NewContractData;
using aztec3::circuits::abis::OptionalPrivateCircuitPublicInputs;
using aztec3::circuits::abis::PrivateCircuitPublicInputs;
using aztec3::circuits::abis::PrivateHistoricTreeRoots;
using aztec3::circuits::abis::PrivateTypes;
using aztec3::circuits::abis::SignedTxRequest;
using aztec3::circuits::abis::TxContext;
using aztec3::circuits::abis::TxRequest;
using aztec3::circuits::abis::private_kernel::PrivateCallData;
using aztec3::circuits::abis::private_kernel::PrivateKernelInputsInner;

using aztec3::circuits::apps::test_apps::basic_contract_deployment::constructor;
using aztec3::circuits::apps::test_apps::escrow::deposit;

using DummyComposer = aztec3::utils::DummyComposer;

using CircuitErrorCode = aztec3::utils::CircuitErrorCode;

// A type representing any private circuit function
// (for now it works for deposit and constructor)
using private_function = std::function<OptionalPrivateCircuitPublicInputs<NT>(
    FunctionExecutionContext<aztec3::circuits::kernel::private_kernel::Composer>&,
    std::array<NT::fr, aztec3::ARGS_LENGTH> const&)>;

// Some helper constants for trees
constexpr size_t MAX_FUNCTION_LEAVES = 2 << (aztec3::FUNCTION_TREE_HEIGHT - 1);
const NT::fr EMPTY_FUNCTION_LEAF = FunctionLeafPreimage<NT>{}.hash();  // hash of empty/0 preimage
const NT::fr EMPTY_CONTRACT_LEAF = NewContractData<NT>{}.hash();       // hash of empty/0 preimage

const auto& get_empty_function_siblings()
{
    static auto EMPTY_FUNCTION_SIBLINGS = []() {
        const auto result = compute_empty_sibling_path<NT, aztec3::FUNCTION_TREE_HEIGHT>(EMPTY_FUNCTION_LEAF);
        return result;
    }();
    return EMPTY_FUNCTION_SIBLINGS;
}

const auto& get_empty_contract_siblings()
{
    static auto EMPTY_CONTRACT_SIBLINGS = []() {
        const auto result = compute_empty_sibling_path<NT, aztec3::CONTRACT_TREE_HEIGHT>(EMPTY_CONTRACT_LEAF);
        return result;
    }();
    return EMPTY_CONTRACT_SIBLINGS;
}

}  // namespace

namespace aztec3::circuits::kernel::private_kernel {

/**
 * @brief Print some debug info about a composer if in DEBUG_PRINTS mode
 *
 * @param composer
 */
void debugComposer(Composer const& composer)
{
#ifdef DEBUG_PRINTS
    info("computed witness: ", composer.computed_witness);
    // info("witness: ", private_kernel_composer.witness);
    // info("constant variables: ", private_kernel_composer.constant_variables);
    // info("variables: ", composer.variables);
    info("failed?: ", composer.failed());
    info("err: ", composer.err());
    info("n: ", composer.get_num_gates());
#else
    (void)composer;  // only used in debug mode
#endif
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
std::shared_ptr<NT::VK> gen_func_vk(bool is_constructor, private_function const& func, size_t const num_args)
{
    // Some dummy inputs to get the circuit to compile and get a VK
    FunctionData<NT> const dummy_function_data{
        .is_private = true,
        .is_constructor = is_constructor,
    };

    CallContext<NT> const dummy_call_context{
        .is_contract_deployment = is_constructor,
    };

    // Dummmy invokation of private call circuit, in order to derive its vk
    Composer dummy_composer = Composer("../barretenberg/cpp/srs_db/ignition");
    {
        DB dummy_db;
        NativeOracle dummy_oracle = is_constructor
                                        ? NativeOracle(dummy_db, 0, dummy_function_data, dummy_call_context, {}, 0)
                                        : NativeOracle(dummy_db, 0, dummy_function_data, dummy_call_context, 0);

        OracleWrapper dummy_oracle_wrapper = OracleWrapper(dummy_composer, dummy_oracle);

        FunctionExecutionContext dummy_ctx(dummy_composer, dummy_oracle_wrapper);

        std::array<NT::fr, ARGS_LENGTH> dummy_args;
        // if args are value 0, deposit circuit errors when inserting utxo notes
        dummy_args.fill(1);
        // Make call to private call circuit itself to lay down constraints
        func(dummy_ctx, dummy_args);
        // FIXME remove arg
        (void)num_args;
    }

    // Now we can derive the vk:
    return dummy_composer.compute_verification_key();
}

std::pair<PrivateCallData<NT>, ContractDeploymentData<NT>> create_private_call_deploy_data(
    bool const is_constructor,
    private_function const& func,
    std::vector<NT::fr> const& args_vec,
    NT::address const& msg_sender)
{
    //***************************************************************************
    // Initialize some inputs to private call and kernel circuits
    //***************************************************************************
    // TODO randomize inputs
    NT::address contract_address = is_constructor ? 0 : 12345;  // updated later if in constructor
    const NT::uint32 contract_leaf_index = 1;
    const NT::uint32 function_leaf_index = 1;
    const NT::fr portal_contract_address = 23456;
    const NT::fr contract_address_salt = 34567;
    const NT::fr acir_hash = 12341234;

    const NT::fr msg_sender_private_key = 123456789;

    FunctionData<NT> const function_data{
        .function_selector = 1,  // TODO: deduce this from the contract, somehow.
        .is_private = true,
        .is_constructor = is_constructor,
    };

    CallContext<NT> call_context{
        .msg_sender = msg_sender,
        .storage_contract_address = contract_address,
        .portal_contract_address = portal_contract_address,
        .is_delegate_call = false,
        .is_static_call = false,
        .is_contract_deployment = is_constructor,
    };

    // sometimes need private call args as array
    std::array<NT::fr, ARGS_LENGTH> args{};
    for (size_t i = 0; i < args_vec.size(); ++i) {
        args[i] = args_vec[i];
    }

    //***************************************************************************
    // Initialize contract related information like private call VK (and its hash),
    // function tree, contract tree, contract address for newly deployed contract,
    // etc...
    //***************************************************************************

    // generate private circuit VK and its hash using circuit with dummy inputs
    // it is needed below:
    //     for constructors - to generate the contract address, function leaf, etc
    //     for private calls - to generate the function leaf, etc
    const std::shared_ptr<NT::VK> private_circuit_vk = gen_func_vk(is_constructor, func, args_vec.size());
    const NT::fr private_circuit_vk_hash =
        stdlib::recursion::verification_key<CT::bn254>::compress_native(private_circuit_vk, GeneratorIndex::VK);

    ContractDeploymentData<NT> contract_deployment_data{};
    NT::fr contract_tree_root = 0;  // TODO set properly for constructor?
    if (is_constructor) {
        // TODO compute function tree root from leaves
        // create leaf preimage for each function and hash all into tree
        // push to array/vector
        // use variation of `compute_root_partial_left_tree` to compute the root from leaves
        // const auto& function_leaf_preimage = FunctionLeafPreimage<NT>{
        //    .function_selector = function_data.function_selector,
        //    .is_private = function_data.is_private,
        //    .vk_hash = private_circuit_vk_hash,
        //    .acir_hash = acir_hash,
        //};
        std::vector<NT::fr> const function_leaves(MAX_FUNCTION_LEAVES, EMPTY_FUNCTION_LEAF);
        // const NT::fr& function_tree_root = plonk::stdlib::merkle_tree::compute_tree_root_native(function_leaves);

        // TODO use actual function tree root computed from leaves
        // update cdd with actual info
        contract_deployment_data = {
            .constructor_vk_hash = private_circuit_vk_hash,
            .function_tree_root = plonk::stdlib::merkle_tree::compute_tree_root_native(function_leaves),
            .contract_address_salt = contract_address_salt,
            .portal_contract_address = portal_contract_address,
        };

        // Get constructor hash for use when deriving contract address
        auto constructor_hash = compute_constructor_hash<NT>(function_data, args, private_circuit_vk_hash);

        // Derive contract address so that it can be used inside the constructor itself
        contract_address = compute_contract_address<NT>(
            msg_sender, contract_address_salt, contract_deployment_data.function_tree_root, constructor_hash);
        // update the contract address in the call context now that it is known
        call_context.storage_contract_address = contract_address;
    } else {
        const NT::fr& function_tree_root = function_tree_root_from_siblings<NT>(function_data.function_selector,
                                                                                function_data.is_private,
                                                                                private_circuit_vk_hash,
                                                                                acir_hash,
                                                                                function_leaf_index,
                                                                                get_empty_function_siblings());

        // update contract_tree_root with real value
        contract_tree_root = contract_tree_root_from_siblings<NT>(function_tree_root,
                                                                  contract_address,
                                                                  portal_contract_address,
                                                                  contract_leaf_index,
                                                                  get_empty_contract_siblings());
    }

    //***************************************************************************
    // Create a private circuit/call using composer, oracles, execution context
    // Generate its proof and public inputs for submission with a TX request
    //***************************************************************************
    Composer private_circuit_composer = Composer("../barretenberg/cpp/srs_db/ignition");

    DB db;
    NativeOracle oracle =
        is_constructor
            ? NativeOracle(
                  db, contract_address, function_data, call_context, contract_deployment_data, msg_sender_private_key)
            : NativeOracle(db, contract_address, function_data, call_context, msg_sender_private_key);

    OracleWrapper oracle_wrapper = OracleWrapper(private_circuit_composer, oracle);

    FunctionExecutionContext ctx(private_circuit_composer, oracle_wrapper);

    OptionalPrivateCircuitPublicInputs<NT> const opt_private_circuit_public_inputs = func(ctx, args);
    PrivateCircuitPublicInputs<NT> private_circuit_public_inputs =
        opt_private_circuit_public_inputs.remove_optionality();
    // TODO this should likely be handled as part of the DB/Oracle/Context infrastructure
    private_circuit_public_inputs.historic_contract_tree_root = contract_tree_root;

    auto private_circuit_prover = private_circuit_composer.create_prover();
    NT::Proof const private_circuit_proof = private_circuit_prover.construct_proof();
    // info("\nproof: ", private_circuit_proof.proof_data);


    //***************************************************************************
    // We mock a kernel circuit proof for the base case of kernel recursion (because even the first iteration of the
    // kernel circuit expects to verify some previous kernel circuit).
    //***************************************************************************
    // TODO: we have a choice to make:
    // Either the `end` state of the mock kernel's public inputs can be set equal to the public call we _want_ to
    // verify in the first round of recursion, OR, we have some fiddly conditional logic in the circuit to ignore
    // certain checks if we're handling the 'base case' of the recursion.
    // I've chosen the former, for now.
    const CallStackItem<NT, PrivateTypes> call_stack_item{
        .contract_address = contract_address,
        .function_data = function_data,
        .public_inputs = private_circuit_public_inputs,
    };

    //***************************************************************************
    // Now we can construct the full private inputs to the kernel circuit
    //***************************************************************************

    return std::pair<PrivateCallData<NT>, ContractDeploymentData<NT>>(
    PrivateCallData<NT>{
        .call_stack_item = call_stack_item,
        .private_call_stack_preimages = ctx.get_private_call_stack_items(),

        .proof = private_circuit_proof,
        .vk = private_circuit_vk,

        .function_leaf_membership_witness = {
            .leaf_index = function_leaf_index,
            .sibling_path = get_empty_function_siblings(),
        },

        .contract_leaf_membership_witness = {
            .leaf_index = contract_leaf_index,
            .sibling_path = get_empty_contract_siblings(),
        },

        .portal_contract_address = portal_contract_address,

        .acir_hash = acir_hash
    },
    contract_deployment_data);
}

// JEAMON: Most of the current tests should call this variant (init case)
PrivateKernelInputsInit<NT> do_private_call_get_kernel_inputs_init(bool const is_constructor,
                                                                   private_function const& func,
                                                                   std::vector<NT::fr> const& args_vec)
{
    //***************************************************************************
    // Initialize some inputs to private call and kernel circuits
    //***************************************************************************
    // TODO randomize inputs

    const NT::address msg_sender =
        NT::fr(uint256_t(0x01071e9a23e0f7edULL, 0x5d77b35d1830fa3eULL, 0xc6ba3660bb1f0c0bULL, 0x2ef9f7f09867fd6eULL));
    const NT::address& tx_origin = msg_sender;

    auto const& [private_call_data, contract_deployment_data] =
        create_private_call_deploy_data(is_constructor, func, args_vec, msg_sender);

    //***************************************************************************
    // We can create a TxRequest from some of the above data. Users must sign a TxRequest in order to give permission
    // for a tx to take place - creating a SignedTxRequest.
    //***************************************************************************
    auto const tx_request = TxRequest<NT>{
        .from = tx_origin,
        .to = private_call_data.call_stack_item.contract_address,
        .function_data = private_call_data.call_stack_item.function_data,
        .args = private_call_data.call_stack_item.public_inputs.args,
        .nonce = 0,
        .tx_context =
            TxContext<NT>{
                .is_fee_payment_tx = false,
                .is_rebate_payment_tx = false,
                .is_contract_deployment_tx = is_constructor,
                .contract_deployment_data = contract_deployment_data,
            },
        .chain_id = 1,
    };

    auto const signed_tx_request = SignedTxRequest<NT>{
        .tx_request = tx_request,

        //.signature = TODO: need a method for signing a TxRequest.
    };

    //***************************************************************************
    // Now we can construct the full private inputs to the kernel circuit
    //***************************************************************************
    PrivateKernelInputsInit<NT> kernel_private_inputs = PrivateKernelInputsInit<NT>{
        .signed_tx_request = signed_tx_request,
        .private_call = private_call_data,
    };

    return kernel_private_inputs;
}


/**
 * @brief Perform a private circuit call and generate the inputs to private kernel
 *
 * @param is_constructor whether this private circuit call is a constructor
 * @param func the private circuit call being validated by this kernel iteration
 * @param args_vec the private call's args
 * @return PrivateInputs<NT> - the inputs to the private call circuit
 */
PrivateKernelInputsInner<NT> do_private_call_get_kernel_inputs_inner(bool const is_constructor,
                                                                     private_function const& func,
                                                                     std::vector<NT::fr> const& args_vec,
                                                                     bool real_kernel_circuit = false)
{
    //***************************************************************************
    // Initialize some inputs to private call and kernel circuits
    //***************************************************************************
    // TODO randomize inputs

    const NT::address msg_sender =
        NT::fr(uint256_t(0x01071e9a23e0f7edULL, 0x5d77b35d1830fa3eULL, 0xc6ba3660bb1f0c0bULL, 0x2ef9f7f09867fd6eULL));

    auto const& [private_call_data, contract_deployment_data] =
        create_private_call_deploy_data(is_constructor, func, args_vec, msg_sender);

    const TxContext<NT> tx_context = TxContext<NT>{
        .is_fee_payment_tx = false,
        .is_rebate_payment_tx = false,
        .is_contract_deployment_tx = is_constructor,
        .contract_deployment_data = contract_deployment_data,
    };

    //***************************************************************************
    // We mock a kernel circuit proof for the base case of kernel recursion (because even the first iteration of the
    // kernel circuit expects to verify some previous kernel circuit).
    //***************************************************************************
    // TODO: we have a choice to make:
    // Either the `end` state of the mock kernel's public inputs can be set equal to the public call we _want_ to
    // verify in the first round of recursion, OR, we have some fiddly conditional logic in the circuit to ignore
    // certain checks if we're handling the 'base case' of the recursion.
    // I've chosen the former, for now.

    std::array<NT::fr, KERNEL_PRIVATE_CALL_STACK_LENGTH> initial_kernel_private_call_stack{};
    initial_kernel_private_call_stack[0] = private_call_data.call_stack_item.hash();

    auto const& private_circuit_public_inputs = private_call_data.call_stack_item.public_inputs;
    // Get dummy previous kernel
    auto mock_previous_kernel = utils::dummy_previous_kernel(real_kernel_circuit);
    // Fill in some important fields in public inputs
    mock_previous_kernel.public_inputs.end.private_call_stack = initial_kernel_private_call_stack;
    mock_previous_kernel.public_inputs.constants = CombinedConstantData<NT>{
        .historic_tree_roots =
            CombinedHistoricTreeRoots<NT>{
                .private_historic_tree_roots =
                    PrivateHistoricTreeRoots<NT>{
                        .private_data_tree_root = private_circuit_public_inputs.historic_private_data_tree_root,
                        .contract_tree_root = private_circuit_public_inputs.historic_contract_tree_root,
                    },
            },
        .tx_context = tx_context,
    };
    mock_previous_kernel.public_inputs.is_private = true;

    //***************************************************************************
    // Now we can construct the full private inputs to the kernel circuit
    //***************************************************************************
    PrivateKernelInputsInner<NT> kernel_private_inputs = PrivateKernelInputsInner<NT>{
        .previous_kernel = mock_previous_kernel,
        .private_call = private_call_data,
    };

    return kernel_private_inputs;
}

/**
 * @brief Validate that the deployed contract address is correct.
 *
 * @details Compare the public inputs new contract address
 * with one manually computed from private inputs.
 * @param private_inputs to be used in manual computation
 * @param public_inputs that contain the expected new contract address
 */
void validate_deployed_contract_address(PrivateKernelInputsInit<NT> const& private_inputs,
                                        KernelCircuitPublicInputs<NT> const& public_inputs)
{
    auto tx_request = private_inputs.signed_tx_request.tx_request;
    auto cdd = private_inputs.signed_tx_request.tx_request.tx_context.contract_deployment_data;

    auto private_circuit_vk_hash = stdlib::recursion::verification_key<CT::bn254>::compress_native(
        private_inputs.private_call.vk, GeneratorIndex::VK);
    auto expected_constructor_hash = NT::compress({ private_inputs.private_call.call_stack_item.function_data.hash(),
                                                    NT::compress<ARGS_LENGTH>(tx_request.args, CONSTRUCTOR_ARGS),
                                                    private_circuit_vk_hash },
                                                  CONSTRUCTOR);
    NT::fr const expected_contract_address =
        NT::compress({ tx_request.from, cdd.contract_address_salt, cdd.function_tree_root, expected_constructor_hash },
                     CONTRACT_ADDRESS);
    EXPECT_EQ(public_inputs.end.new_contracts[0].contract_address.to_field(), expected_contract_address);
}

void validate_no_new_deployed_contract(KernelCircuitPublicInputs<NT> const& public_inputs)
{
    ASSERT_TRUE(public_inputs.end.new_contracts == CombinedAccumulatedData<NT>{}.new_contracts);
}

/**
 * @brief Some private circuit proof (`deposit`, in this case)
 */
TEST(private_kernel_tests, circuit_deposit)
{
    NT::fr const& amount = 5;
    NT::fr const& asset_id = 1;
    NT::fr const& memo = 999;

    auto const& private_inputs =
        do_private_call_get_kernel_inputs_inner(false, deposit, { amount, asset_id, memo }, true);

    // Execute and prove the first kernel iteration
    Composer private_kernel_composer("../barretenberg/cpp/srs_db/ignition");
    auto const& public_inputs = private_kernel_circuit(private_kernel_composer, private_inputs, true);

    // TODO(jeanmon): this is a temporary hack until we have private_kernel_circuit init and inner
    // variant. Once this is supported, we will be able to generate public_inputs with
    // a call to private_kernel_circuit_init(private_inputs_init, ...)
    auto const& private_inputs_init =
        do_private_call_get_kernel_inputs_init(false, deposit, { amount, asset_id, memo });

    // TODO(jeanmon): Once we have an inner/init private kernel circuit,
    // there should not be any new deployed contract address in public_inputs
    // and the following assertion can be uncommented:
    // validate_no_new_deployed_contract(public_inputs);

    // TODO(jeanmon): Remove once we have an inner/innit private kernel circuit
    // Check contract address was correctly computed by the circuit
    validate_deployed_contract_address(private_inputs_init, public_inputs);

    // Create the final kernel proof and verify it natively.
    auto final_kernel_prover = private_kernel_composer.create_prover();
    auto const& final_kernel_proof = final_kernel_prover.construct_proof();

    auto final_kernel_verifier = private_kernel_composer.create_verifier();
    auto const& final_result = final_kernel_verifier.verify_proof(final_kernel_proof);
    EXPECT_EQ(final_result, true);

    debugComposer(private_kernel_composer);
}

/**
 * @brief Some private circuit simulation (`deposit`, in this case)
 */
TEST(private_kernel_tests, native_deposit)
{
    NT::fr const& amount = 5;
    NT::fr const& asset_id = 1;
    NT::fr const& memo = 999;

    auto const& private_inputs = do_private_call_get_kernel_inputs_init(false, deposit, { amount, asset_id, memo });
    DummyComposer composer = DummyComposer("private_kernel_tests__native_deposit");
    auto const& public_inputs = native_private_kernel_circuit_initial(composer, private_inputs);

    validate_no_new_deployed_contract(public_inputs);

    // Check the first nullifier is hash of the signed tx request
    ASSERT_EQ(public_inputs.end.new_nullifiers[0], private_inputs.signed_tx_request.hash());
}

/**
 * @brief Some private circuit proof (`constructor`, in this case)
 */
TEST(private_kernel_tests, circuit_basic_contract_deployment)
{
    NT::fr const& arg0 = 5;
    NT::fr const& arg1 = 1;
    NT::fr const& arg2 = 999;

    auto const& private_inputs = do_private_call_get_kernel_inputs_inner(true, constructor, { arg0, arg1, arg2 }, true);

    // Execute and prove the first kernel iteration
    Composer private_kernel_composer("../barretenberg/cpp/srs_db/ignition");
    auto const& public_inputs = private_kernel_circuit(private_kernel_composer, private_inputs, true);

    // TODO(jeanmon): this is a temporary hack until we have private_kernel_circuit init and inner
    // variant. Once this is supported, we will be able to generate public_inputs with
    // a call to private_kernel_circuit_init(private_inputs_init, ...)
    auto const& private_inputs_init = do_private_call_get_kernel_inputs_init(true, constructor, { arg0, arg1, arg2 });

    // Check contract address was correctly computed by the circuit
    validate_deployed_contract_address(private_inputs_init, public_inputs);

    // Create the final kernel proof and verify it natively.
    auto final_kernel_prover = private_kernel_composer.create_prover();
    auto const& final_kernel_proof = final_kernel_prover.construct_proof();

    auto final_kernel_verifier = private_kernel_composer.create_verifier();
    auto const& final_result = final_kernel_verifier.verify_proof(final_kernel_proof);
    EXPECT_EQ(final_result, true);

    debugComposer(private_kernel_composer);
}

/**
 * @brief Some private circuit simulation (`constructor`, in this case)
 */
TEST(private_kernel_tests, native_basic_contract_deployment)
{
    NT::fr const& arg0 = 5;
    NT::fr const& arg1 = 1;
    NT::fr const& arg2 = 999;

    auto const& private_inputs = do_private_call_get_kernel_inputs_init(true, constructor, { arg0, arg1, arg2 });
    DummyComposer composer = DummyComposer("private_kernel_tests__native_basic_contract_deployment");
    auto const& public_inputs = native_private_kernel_circuit_initial(composer, private_inputs);

    validate_deployed_contract_address(private_inputs, public_inputs);

    // Check the first nullifier is hash of the signed tx request
    ASSERT_EQ(public_inputs.end.new_nullifiers[0], private_inputs.signed_tx_request.hash());
}

/**
 * @brief Some private circuit simulation checked against its results via cbinds
 */
TEST(private_kernel_tests, circuit_create_proof_cbinds)
{
    NT::fr const& arg0 = 5;
    NT::fr const& arg1 = 1;
    NT::fr const& arg2 = 999;

    // first run actual simulation to get public inputs
    auto const& private_inputs = do_private_call_get_kernel_inputs_init(true, constructor, { arg0, arg1, arg2 });
    DummyComposer composer = DummyComposer("private_kernel_tests__circuit_create_proof_cbinds");
    auto const& public_inputs = native_private_kernel_circuit_initial(composer, private_inputs);

    // serialize expected public inputs for later comparison
    std::vector<uint8_t> expected_public_inputs_vec;
    write(expected_public_inputs_vec, public_inputs);

    //***************************************************************************
    // Now run the simulate/prove cbinds to make sure their outputs match
    //***************************************************************************
    // TODO might be able to get rid of proving key buffer
    uint8_t const* pk_buf = nullptr;
    private_kernel__init_proving_key(&pk_buf);
    // info("Proving key size: ", pk_size);

    // TODO might be able to get rid of verification key buffer
    // uint8_t const* vk_buf;
    // size_t vk_size = private_kernel__init_verification_key(pk_buf, &vk_buf);
    // info("Verification key size: ", vk_size);

    std::vector<uint8_t> signed_constructor_tx_request_vec;
    write(signed_constructor_tx_request_vec, private_inputs.signed_tx_request);

    std::vector<uint8_t> private_constructor_call_vec;
    write(private_constructor_call_vec, private_inputs.private_call);

    uint8_t const* proof_data_buf = nullptr;
    uint8_t const* public_inputs_buf = nullptr;
    size_t public_inputs_size = 0;
    // info("Simulating to generate public inputs...");
    uint8_t* const circuit_failure_ptr = private_kernel__sim(signed_constructor_tx_request_vec.data(),
                                                             nullptr,  // no previous kernel on first iteration
                                                             private_constructor_call_vec.data(),
                                                             true,  // first iteration
                                                             &public_inputs_size,
                                                             &public_inputs_buf);
    ASSERT_TRUE(circuit_failure_ptr == nullptr);

    // TODO better equality check
    // for (size_t i = 0; i < public_inputs_size; i++)
    for (size_t i = 0; i < 10; i++) {
        ASSERT_EQ(public_inputs_buf[i], expected_public_inputs_vec[i]);
    }
    (void)public_inputs_size;
    // info("Proving");
    size_t const proof_data_size = private_kernel__prove(signed_constructor_tx_request_vec.data(),
                                                         nullptr,  // no previous kernel on first iteration
                                                         private_constructor_call_vec.data(),
                                                         pk_buf,
                                                         true,  // first iteration
                                                         &proof_data_buf);
    (void)proof_data_size;
    // info("Proof size: ", proof_data_size);
    // info("PublicInputs size: ", public_inputs_size);

    free((void*)pk_buf);
    // free((void*)vk_buf);
    free((void*)proof_data_buf);
    free((void*)public_inputs_buf);
}

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
