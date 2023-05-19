#pragma once
#include "aztec3/utils/types/circuit_types.hpp"

#include <barretenberg/serialize/msgpack.hpp>

#include <variant>

namespace aztec3::utils {

enum CircuitErrorCode : uint16_t {
    NO_ERROR = 0,
    UNINITIALIZED_RESULT = 1,  // Default for CircuitResult
    // Private kernel related errors
    PRIVATE_KERNEL_CIRCUIT_FAILED = 2000,
    PRIVATE_KERNEL__INVALID_CONSTRUCTOR_VK_HASH = 2001,
    PRIVATE_KERNEL__INVALID_CONTRACT_ADDRESS = 2002,
    PRIVATE_KERNEL__PURPORTED_CONTRACT_TREE_ROOT_AND_PREVIOUS_KERNEL_CONTRACT_TREE_ROOT_MISMATCH = 2003,
    PRIVATE_KERNEL__COMPUTED_CONTRACT_TREE_ROOT_AND_PURPORTED_CONTRACT_TREE_ROOT_MISMATCH = 2004,
    PRIVATE_KERNEL__NEW_COMMITMENTS_NOT_EMPTY_FOR_STATIC_CALL = 2005,
    PRIVATE_KERNEL__NEW_NULLIFIERS_NOT_EMPTY_FOR_STATIC_CALL = 2006,
    PRIVATE_KERNEL__CALCULATED_PRIVATE_CALL_HASH_AND_PROVIDED_PRIVATE_CALL_HASH_MISMATCH = 2007,
    PRIVATE_KERNEL__PRIVATE_CALL_STACK_ITEM_HASH_MISMATCH = 2008,
    PRIVATE_KERNEL__NON_PRIVATE_FUNCTION_EXECUTED_WITH_PRIVATE_KERNEL = 2009,
    PRIVATE_KERNEL__PRIVATE_CALL_STACK_LENGTH_MISMATCH = 2010,
    PRIVATE_KERNEL__UNSUPPORTED_OP = 2011,
    PRIVATE_KERNEL__CONTRACT_ADDRESS_MISMATCH = 2012,
    PRIVATE_KERNEL__NON_PRIVATE_KERNEL_VERIFIED_WITH_PRIVATE_KERNEL = 2013,
    PRIVATE_KERNEL__CONSTRUCTOR_EXECUTED_IN_RECURSION = 2014,
    PRIVATE_KERNEL__PRIVATE_CALL_STACK_EMPTY = 2015,
    PRIVATE_KERNEL__KERNEL_PROOF_CONTAINS_RECURSIVE_PROOF = 2016,
    PRIVATE_KERNEL__NEW_NULLIFIERS_NOT_EMPTY_IN_FIRST_ITERATION = 2017,

    // Public kernel related errors
    PUBLIC_KERNEL_CIRCUIT_FAILED = 3000,
    PUBLIC_KERNEL__UNSUPPORTED_OP = 3001,
    PUBLIC_KERNEL__PRIVATE_FUNCTION_NOT_ALLOWED = 3002,
    PUBLIC_KERNEL__CONTRACT_ADDRESS_MISMATCH = 3003,
    PUBLIC_KERNEL__EMPTY_PUBLIC_CALL_STACK = 3004,
    PUBLIC_KERNEL__NON_EMPTY_PRIVATE_CALL_STACK = 3005,
    PUBLIC_KERNEL__PREVIOUS_KERNEL_NOT_PRIVATE = 3009,
    PUBLIC_KERNEL__PREVIOUS_KERNEL_NOT_PUBLIC = 3010,
    PUBLIC_KERNEL__CALCULATED_PRIVATE_CALL_HASH_AND_PROVIDED_PRIVATE_CALL_HASH_MISMATCH = 3011,
    PUBLIC_KERNEL__PUBLIC_CALL_STACK_MISMATCH = 3012,
    PUBLIC_KERNEL__CONTRACT_DEPLOYMENT_NOT_ALLOWED = 3013,
    PUBLIC_KERNEL__CONSTRUCTOR_NOT_ALLOWED = 3014,
    PUBLIC_KERNEL__CONTRACT_ADDRESS_INVALID = 3015,
    PUBLIC_KERNEL__FUNCTION_SIGNATURE_INVALID = 3016,
    PUBLIC_KERNEL__BYTECODE_HASH_INVALID = 3017,
    PUBLIC_KERNEL__CALL_CONTEXT_INVALID = 3018,
    PUBLIC_KERNEL__DELEGATE_CALL_PROHIBITED_BY_USER = 3019,
    PUBLIC_KERNEL__STATIC_CALL_PROHIBITED_BY_USER = 3019,
    PUBLIC_KERNEL__PUBLIC_CALL_STACK_INVALID_MSG_SENDER = 3020,
    PUBLIC_KERNEL__PUBLIC_CALL_STACK_INVALID_STORAGE_ADDRESS = 3021,
    PUBLIC_KERNEL__PUBLIC_CALL_STACK_INVALID_PORTAL_ADDRESS = 3022,
    PUBLIC_KERNEL__PUBLIC_CALL_STACK_CONTRACT_STORAGE_UPDATES_PROHIBITED_FOR_STATIC_CALL = 3022,
    PUBLIC_KERNEL__CALL_CONTEXT_INVALID_STORAGE_ADDRESS_FOR_DELEGATE_CALL = 3023,
    PUBLIC_KERNEL__CALL_CONTEXT_CONTRACT_STORAGE_UPDATE_REQUESTS_PROHIBITED_FOR_STATIC_CALL = 3024,
    PUBLIC_KERNEL__NEW_NULLIFIERS_NOT_EMPTY_IN_FIRST_ITERATION = 3025,

    BASE_FAILED = 4000,
    BASE__KERNEL_PROOF_VERIFICATION_FAILED = 4001,
    BASE__INCORRECT_NUM_OF_NEW_COMMITMENTS = 4002,
    BASE__INVALID_NULLIFIER_SUBTREE = 4003,
    BASE__INVALID_NULLIFIER_RANGE = 4004,
    BASE__INVALID_PUBLIC_DATA_READS = 4005,
    BASE__INVALID_PUBLIC_DATA_UPDATE_REQUESTS = 4006,

    MERGE_CIRCUIT_FAILED = 6000,

    // Component errors (used by all circuits)
    ROLLUP_TYPE_MISMATCH = 7001,
    ROLLUP_HEIGHT_MISMATCH = 7002,
    CONSTANTS_MISMATCH = 7003,
    PRIVATE_DATA_TREE_SNAPSHOT_MISMATCH = 7004,
    NULLIFIER_TREE_SNAPSHOT_MISMATCH = 7005,
    CONTRACT_TREE_SNAPSHOT_MISMATCH = 7006,
    PUBLIC_DATA_TREE_ROOT_MISMATCH = 7007,
    MEMBERSHIP_CHECK_FAILED = 7008,

    ROOT_CIRCUIT_FAILED = 8000,
};

struct CircuitError {
    // using int for easy serialization
    uint16_t code = CircuitErrorCode::NO_ERROR;
    std::string message;

    // for serialization, update with new fields
    MSGPACK_FIELDS(code, message);
    static CircuitError no_error() { return { CircuitErrorCode::NO_ERROR, "" }; }
};

// Define CircuitResult<T> as a std::variant T + error union type
// We do not use std::variant directly as we need default-constructible types for msgpack
template <typename T> struct CircuitResult {
    CircuitResult() : result(CircuitError{ UNINITIALIZED_RESULT, "" }) {}
    CircuitResult(const T& value) : result(value) {}
    CircuitResult(const CircuitError& value) : result(value) {}
    std::variant<CircuitError, T> result;

    // for serialization: delegate to msgpack std::variant support
    void msgpack_pack(auto& packer) const { packer.pack(result); }
    void msgpack_unpack(auto obj) { result = obj; }
};

// help our msgpack schema compiler with this struct
// Alias CircuitResult as std::variant<CircuitError, T>
template <typename T> inline void msgpack_schema_pack(auto& packer, CircuitResult<T> const& result)
{
    msgpack_schema_pack(packer, result.result);
}

inline void read(uint8_t const*& it, CircuitError& obj)
{
    using serialize::read;
    read(it, obj.code);
    read(it, obj.message);
};

inline void write(std::vector<uint8_t>& buf, CircuitError const& obj)
{
    using serialize::write;

    write(buf, obj.code);
    write(buf, obj.message);
};

inline std::ostream& operator<<(std::ostream& os, CircuitError const& obj)
{
    return os << "code: " << obj.code << "\n"
              << "message: " << obj.message << "\n";
}
}  // namespace aztec3::utils