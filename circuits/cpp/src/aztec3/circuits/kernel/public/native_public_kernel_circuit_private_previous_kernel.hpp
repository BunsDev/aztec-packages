#pragma once

#include "init.hpp"
#include "common.hpp"

#include <aztec3/circuits/abis/public_kernel/public_kernel_inputs.hpp>
#include <aztec3/circuits/abis/kernel_circuit_public_inputs.hpp>
#include <aztec3/utils/dummy_composer.hpp>

namespace aztec3::circuits::kernel::public_kernel {

using aztec3::circuits::abis::KernelCircuitPublicInputs;
using aztec3::circuits::abis::public_kernel::PublicKernelInputs;
using DummyComposer = aztec3::utils::DummyComposer;

// TODO: decide what to return.
KernelCircuitPublicInputs<NT> native_public_kernel_circuit_private_previous_kernel(
    DummyComposer& composer, PublicKernelInputs<NT> const& public_kernel_inputs);
} // namespace aztec3::circuits::kernel::public_kernel