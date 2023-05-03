#pragma once
#include <stddef.h>

namespace aztec3 {

// Note: must be kept in sync with ts/structs/constants.ts
constexpr size_t ARGS_LENGTH = 8;
constexpr size_t RETURN_VALUES_LENGTH = 4;
constexpr size_t EMITTED_EVENTS_LENGTH = 4;

constexpr size_t NEW_COMMITMENTS_LENGTH = 4;
constexpr size_t NEW_NULLIFIERS_LENGTH = 4;

constexpr size_t STATE_TRANSITIONS_LENGTH = 4;
constexpr size_t STATE_READS_LENGTH = 4;

constexpr size_t PRIVATE_CALL_STACK_LENGTH = 4;
constexpr size_t PUBLIC_CALL_STACK_LENGTH = 4;
constexpr size_t NEW_L2_TO_L1_MSGS_LENGTH = 2;

constexpr size_t KERNEL_NEW_COMMITMENTS_LENGTH = 4;
constexpr size_t KERNEL_NEW_NULLIFIERS_LENGTH = 4;
constexpr size_t KERNEL_NEW_CONTRACTS_LENGTH = 1;
constexpr size_t KERNEL_PRIVATE_CALL_STACK_LENGTH = 8;
constexpr size_t KERNEL_PUBLIC_CALL_STACK_LENGTH = 8;
constexpr size_t KERNEL_NEW_L2_TO_L1_MSGS_LENGTH = 4;
constexpr size_t KERNEL_OPTIONALLY_REVEALED_DATA_LENGTH = 4;

constexpr size_t VK_TREE_HEIGHT = 3;
constexpr size_t FUNCTION_TREE_HEIGHT = 4;
constexpr size_t CONTRACT_TREE_HEIGHT = 4;
constexpr size_t PRIVATE_DATA_TREE_HEIGHT = 8;
constexpr size_t NULLIFIER_TREE_HEIGHT = 8;
constexpr size_t PUBLIC_DATA_TREE_HEIGHT = 254;
constexpr size_t L1_TO_L2_MSG_TREE_HEIGHT = 8;

constexpr size_t CONTRACT_SUBTREE_DEPTH = 1;
constexpr size_t CONTRACT_SUBTREE_INCLUSION_CHECK_DEPTH = CONTRACT_TREE_HEIGHT - CONTRACT_SUBTREE_DEPTH;

constexpr size_t PRIVATE_DATA_SUBTREE_DEPTH = 3;
constexpr size_t PRIVATE_DATA_SUBTREE_INCLUSION_CHECK_DEPTH = PRIVATE_DATA_TREE_HEIGHT - PRIVATE_DATA_SUBTREE_DEPTH;

constexpr size_t NULLIFIER_SUBTREE_DEPTH = 3;
constexpr size_t NULLIFIER_SUBTREE_INCLUSION_CHECK_DEPTH = NULLIFIER_TREE_HEIGHT - NULLIFIER_SUBTREE_DEPTH;

// NUMBER_OF_L1_L2_MESSAGES_PER_ROLLUP must equal 2^L1_TO_L2_MSG_SUBTREE_DEPTH for subtree insertions.
constexpr size_t L1_TO_L2_MSG_SUBTREE_DEPTH = 4;
constexpr size_t NUMBER_OF_L1_L2_MESSAGES_PER_ROLLUP = 16;
constexpr size_t L1_TO_L2_MSG_SUBTREE_INCLUSION_CHECK_DEPTH = L1_TO_L2_MSG_TREE_HEIGHT - L1_TO_L2_MSG_SUBTREE_DEPTH;

constexpr size_t PRIVATE_DATA_TREE_ROOTS_TREE_HEIGHT = 8;
constexpr size_t CONTRACT_TREE_ROOTS_TREE_HEIGHT = 8;
constexpr size_t L1_TO_L2_MSG_TREE_ROOTS_TREE_HEIGHT = 8;
constexpr size_t ROLLUP_VK_TREE_HEIGHT = 8;  // TODO: update

constexpr size_t FUNCTION_SELECTOR_NUM_BYTES = 4;  // must be <= 31


// Enumerate the hash_indices which are used for pedersen hashing
// Start from 1 to avoid the default generators.
enum GeneratorIndex {
    /**
     * Indices with size ≤ 8
     */
    COMMITMENT = 1,                // Size = 7 (unused)
    COMMITMENT_PLACEHOLDER,        // size = 1 (unused), for omitting some elements of commitment when partially comm
    OUTER_COMMITMENT,              // Size = 2
    NULLIFIER_HASHED_PRIVATE_KEY,  // Size = 1 (unused)
    NULLIFIER,                     // Size = 4 (unused)
    INITIALISATION_NULLIFIER,      // Size = 2 (unused)
    OUTER_NULLIFIER,               // Size = 2
    STATE_READ,                    // Size = 2 (unused)
    STATE_TRANSITION,              // Size = 3 (unused)
    FUNCTION_DATA,                 // Size = 3
    FUNCTION_LEAF,                 // Size = 4
    CONTRACT_DEPLOYMENT_DATA,      // Size = 4
    CONSTRUCTOR,                   // Size = 3
    CONSTRUCTOR_ARGS,              // Size = 8
    CONTRACT_ADDRESS,              // Size = 4
    CONTRACT_LEAF,                 // Size = 3
    CALL_CONTEXT,                  // Size = 6
    CALL_STACK_ITEM,               // Size = 3
    CALL_STACK_ITEM_2,             // Size = ? (unused), // TODO see function where it's used for explanation
    L2_TO_L1_MSG,                  // Size = 2 (unused)
    TX_CONTEXT,                    // Size = 4
    PUBLIC_LEAF_INDEX,             // Size = 2 (unused)
    PUBLIC_DATA_LEAF,              // Size = ? (unused) // TODO what's the expected size? Assuming ≤ 8
    SIGN_TX_REQUEST,               // Size = 7
    /**
     * Indices with size ≤ 16
     */
    TX_REQUEST = 33,  // Size = 14
    /**
     * Indices with size ≤ 40
     */
    VK = 41,                        // Size = 35
    PRIVATE_CIRCUIT_PUBLIC_INPUTS,  // Size = 39
    PUBLIC_CIRCUIT_PUBLIC_INPUTS,   // Size = 32 (unused)
};

enum StorageSlotGeneratorIndex {
    BASE_SLOT,
    MAPPING_SLOT,
    MAPPING_SLOT_PLACEHOLDER,
};

// Enumerate the hash_sub_indices which are used for committing to private state note preimages.
// Start from 1.
enum PrivateStateNoteGeneratorIndex {
    VALUE = 1,
    OWNER,
    CREATOR,
    SALT,
    NONCE,
    MEMO,
    IS_DUMMY,
};

enum PrivateStateType { PARTITIONED = 1, WHOLE };

}  // namespace aztec3