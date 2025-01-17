#pragma once

#include "aztec3/circuits/abis/function_leaf_preimage.hpp"
#include <aztec3/circuits/abis/function_data.hpp>
#include <aztec3/circuits/abis/new_contract_data.hpp>
#include <aztec3/constants.hpp>
#include <aztec3/utils/circuit_errors.hpp>

#include "barretenberg/crypto/sha256/sha256.hpp"

#include <array>

namespace aztec3::circuits {

using abis::FunctionData;
using aztec3::circuits::abis::ContractLeafPreimage;
using aztec3::circuits::abis::FunctionLeafPreimage;

template <typename NCT> typename NCT::fr compute_args_hash(std::array<typename NCT::fr, ARGS_LENGTH> args)
{
    return NCT::compress(args, CONSTRUCTOR_ARGS);
}

template <typename NCT> typename NCT::fr compute_constructor_hash(FunctionData<NCT> function_data,
                                                                  std::array<typename NCT::fr, ARGS_LENGTH> args,
                                                                  typename NCT::fr constructor_vk_hash)
{
    using fr = typename NCT::fr;

    fr const function_data_hash = function_data.hash();
    fr const args_hash = compute_args_hash<NCT>(args);

    std::vector<fr> const inputs = {
        function_data_hash,
        args_hash,
        constructor_vk_hash,
    };

    return NCT::compress(inputs, aztec3::GeneratorIndex::CONSTRUCTOR);
}

template <typename NCT> typename NCT::address compute_contract_address(typename NCT::address deployer_address,
                                                                       typename NCT::fr contract_address_salt,
                                                                       typename NCT::fr function_tree_root,
                                                                       typename NCT::fr constructor_hash)
{
    using fr = typename NCT::fr;
    using address = typename NCT::address;

    std::vector<fr> const inputs = {
        deployer_address.to_field(),
        contract_address_salt,
        function_tree_root,
        constructor_hash,
    };

    return address(NCT::compress(inputs, aztec3::GeneratorIndex::CONTRACT_ADDRESS));
}

template <typename NCT>
typename NCT::fr silo_commitment(typename NCT::address contract_address, typename NCT::fr commitment)
{
    using fr = typename NCT::fr;

    std::vector<fr> const inputs = {
        contract_address.to_field(),
        commitment,
    };

    return NCT::compress(inputs, aztec3::GeneratorIndex::OUTER_COMMITMENT);
}

template <typename NCT>
typename NCT::fr silo_nullifier(typename NCT::address contract_address, typename NCT::fr nullifier)
{
    using fr = typename NCT::fr;

    std::vector<fr> const inputs = {
        contract_address.to_field(),
        nullifier,
    };

    return NCT::compress(inputs, aztec3::GeneratorIndex::OUTER_NULLIFIER);
}

/**
 * @brief Calculate the Merkle tree root from the sibling path and leaf.
 *
 * @details The leaf is hashed with its sibling, and then the result is hashed
 * with the next sibling etc in the path. The last hash is the root.
 *
 * @tparam NCT Operate on NativeTypes or CircuitTypes
 * @tparam N The number of elements in the sibling path
 * @param leaf The leaf element of the Merkle tree
 * @param leafIndex The index of the leaf element in the Merkle tree
 * @param siblingPath The nodes representing the merkle siblings of the leaf, its parent,
 * the next parent, etc up to the sibling below the root
 * @return The computed Merkle tree root.
 *
 * TODO need to use conditional assigns instead of `ifs` for circuit version.
 *      see membership.hpp:check_subtree_membership (left/right/conditional_assign, etc)
 */
template <typename NCT, size_t N>
typename NCT::fr root_from_sibling_path(typename NCT::fr const& leaf,
                                        typename NCT::uint32 const& leafIndex,
                                        std::array<typename NCT::fr, N> const& siblingPath)
{
    auto node = leaf;
    for (size_t i = 0; i < N; i++) {
        if (leafIndex & (1 << i)) {
            node = NCT::merkle_hash(siblingPath[i], node);
        } else {
            node = NCT::merkle_hash(node, siblingPath[i]);
        }
    }
    return node;  // root
}

/**
 * @brief Calculate the Merkle tree root from the sibling path and leaf.
 *
 * @details The leaf is hashed with its sibling, and then the result is hashed
 * with the next sibling etc in the path. The last hash is the root.
 *
 * @tparam NCT Operate on NativeTypes or CircuitTypes
 * @tparam N The number of elements in the sibling path
 * @param leaf The leaf element of the Merkle tree
 * @param leafIndex The index of the leaf element in the Merkle tree
 * @param siblingPath The nodes representing the merkle siblings of the leaf, its parent,
 * the next parent, etc up to the sibling below the root
 * @return The computed Merkle tree root.
 *
 * TODO need to use conditional assigns instead of `ifs` for circuit version.
 *      see membership.hpp:check_subtree_membership (left/right/conditional_assign, etc)
 */
template <typename NCT, size_t N>
typename NCT::fr root_from_sibling_path(typename NCT::fr const& leaf,
                                        typename NCT::fr const& leafIndex,
                                        std::array<typename NCT::fr, N> const& siblingPath)
{
    auto node = leaf;
    uint256_t index = leafIndex;
    for (size_t i = 0; i < N; i++) {
        if (index & 1) {
            node = NCT::merkle_hash(siblingPath[i], node);
        } else {
            node = NCT::merkle_hash(node, siblingPath[i]);
        }
        index >>= uint256_t(1);
    }
    return node;  // root
}

template <typename NCT, typename Composer, size_t SIZE>
void check_membership(Composer& composer,
                      typename NCT::fr const& value,
                      typename NCT::fr const& index,
                      std::array<typename NCT::fr, SIZE> const& sibling_path,
                      typename NCT::fr const& root,
                      std::string const& msg)
{
    const auto calculated_root = root_from_sibling_path<NCT>(value, index, sibling_path);
    composer.do_assert(calculated_root == root,
                       std::string("Membership check failed: ") + msg,
                       aztec3::utils::CircuitErrorCode::MEMBERSHIP_CHECK_FAILED);
}

/**
 * @brief Calculate the function tree root from the sibling path and leaf preimage.
 *
 * @tparam NCT (native or circuit)
 * @param function_selector in leaf preimage
 * @param is_private in leaf preimage
 * @param vk_hash in leaf preimage
 * @param acir_hash in leaf preimage
 * @param function_leaf_index leaf index in the function tree
 * @param function_leaf_sibling_path
 * @return NCT::fr
 */
template <typename NCT> typename NCT::fr function_tree_root_from_siblings(
    typename NCT::uint32 const& function_selector,
    typename NCT::boolean const& is_private,
    typename NCT::fr const& vk_hash,
    typename NCT::fr const& acir_hash,
    typename NCT::fr const& function_leaf_index,
    std::array<typename NCT::fr, FUNCTION_TREE_HEIGHT> const& function_leaf_sibling_path)
{
    const auto function_leaf_preimage = FunctionLeafPreimage<NCT>{
        .function_selector = function_selector,
        .is_private = is_private,
        .vk_hash = vk_hash,
        .acir_hash = acir_hash,
    };

    const auto function_leaf = function_leaf_preimage.hash();

    auto function_tree_root =
        root_from_sibling_path<NCT>(function_leaf, function_leaf_index, function_leaf_sibling_path);
    return function_tree_root;
}

/**
 * @brief Calculate the contract tree root from the sibling path and leaf preimage.
 *
 * @tparam NCT (native or circuit)
 * @param function_tree_root in leaf preimage
 * @param storage_contract_address in leaf preimage
 * @param portal_contract_address in leaf preimage
 * @param contract_leaf_index leaf index in the function tree
 * @param contract_leaf_sibling_path
 * @return NCT::fr
 */
template <typename NCT> typename NCT::fr contract_tree_root_from_siblings(
    typename NCT::fr const& function_tree_root,
    typename NCT::address const& storage_contract_address,
    typename NCT::address const& portal_contract_address,
    typename NCT::fr const& contract_leaf_index,
    std::array<typename NCT::fr, CONTRACT_TREE_HEIGHT> const& contract_leaf_sibling_path)
{
    const ContractLeafPreimage<NCT> contract_leaf_preimage{ storage_contract_address,
                                                            portal_contract_address,
                                                            function_tree_root };

    const auto contract_leaf = contract_leaf_preimage.hash();

    const auto computed_contract_tree_root =
        root_from_sibling_path<NCT>(contract_leaf, contract_leaf_index, contract_leaf_sibling_path);
    return computed_contract_tree_root;
}

/**
 * @brief Compute sibling path for an empty tree.
 *
 * @tparam NCT (native or circuit)
 * @tparam TREE_HEIGHT
 * @param zero_leaf the leaf value that corresponds to a zero preimage
 * @return std::array<typename NCT::fr, TREE_HEIGHT>
 */
template <typename NCT, size_t TREE_HEIGHT>
std::array<typename NCT::fr, TREE_HEIGHT> compute_empty_sibling_path(typename NCT::fr const& zero_leaf)
{
    std::array<typename NCT::fr, TREE_HEIGHT> sibling_path = { zero_leaf };
    for (size_t i = 1; i < TREE_HEIGHT; i++) {
        // hash previous sibling with itself to get node above
        sibling_path[i] = NCT::merkle_hash(sibling_path[i - 1], sibling_path[i - 1]);
    }
    return sibling_path;
}

/**
 * @brief Compute the value to be inserted into the public data tree
 * @param value The value to be inserted into the public data tree
 * @return The hash value required for insertion into the public data tree
 */
template <typename NCT> typename NCT::fr compute_public_data_tree_value(typename NCT::fr const& value)
{
    // as it's a public value, it doesn't require hashing.
    // leaving this function here in case we decide to change this.
    return value;
}

/**
 * @brief Compute the index for inserting a value into the public data tree
 * @param contract_address The address of the contract to which the inserted element belongs
 * @param storage_slot The storage slot to which the inserted element belongs
 * @return The index for insertion into the public data tree
 */
template <typename NCT> typename NCT::fr compute_public_data_tree_index(typename NCT::fr const& contract_address,
                                                                        typename NCT::fr const& storage_slot)
{
    return NCT::compress({ contract_address, storage_slot }, GeneratorIndex::PUBLIC_LEAF_INDEX);
}

template <typename NCT> typename NCT::fr compute_l2_to_l1_hash(typename NCT::address contract_address,
                                                               typename NCT::fr rollup_version_id,
                                                               typename NCT::fr portal_contract_address,
                                                               typename NCT::fr chain_id,
                                                               typename NCT::fr content)
{
    using fr = typename NCT::fr;

    std::vector<fr> const inputs = {
        contract_address.to_field(), rollup_version_id, portal_contract_address, chain_id, content,
    };

    constexpr auto const num_bytes = 5 * 32;
    std::array<uint8_t, num_bytes> calldata_hash_inputs_bytes;
    // Convert all into a buffer, then copy into the array, then hash
    for (size_t i = 0; i < inputs.size(); i++) {
        auto as_bytes = inputs[i].to_buffer();

        auto offset = i * 32;
        std::copy(as_bytes.begin(), as_bytes.end(), calldata_hash_inputs_bytes.begin() + offset);
    }

    std::vector<uint8_t> const calldata_hash_inputs_bytes_vec(calldata_hash_inputs_bytes.begin(),
                                                              calldata_hash_inputs_bytes.end());

    // @todo @LHerskind NOTE sha to field!
    return sha256::sha256_to_field(calldata_hash_inputs_bytes_vec);
}

}  // namespace aztec3::circuits