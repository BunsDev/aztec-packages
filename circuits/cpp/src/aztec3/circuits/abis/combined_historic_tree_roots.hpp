#pragma once
#include "private_historic_tree_roots.hpp"

#include <aztec3/utils/types/circuit_types.hpp>
#include <aztec3/utils/types/convert.hpp>
#include <aztec3/utils/types/native_types.hpp>

#include <barretenberg/stdlib/primitives/witness/witness.hpp>

namespace aztec3::circuits::abis {

using aztec3::circuits::abis::PrivateHistoricTreeRoots;
using aztec3::utils::types::CircuitTypes;
using aztec3::utils::types::NativeTypes;
using plonk::stdlib::witness_t;
using std::is_same;

template <typename NCT> struct CombinedHistoricTreeRoots {
    using fr = typename NCT::fr;
    using boolean = typename NCT::boolean;

    PrivateHistoricTreeRoots<NCT> private_historic_tree_roots{};

    boolean operator==(CombinedHistoricTreeRoots<NCT> const& other) const
    {
        return private_historic_tree_roots == other.private_historic_tree_roots;
    };

    template <typename Composer>
    CombinedHistoricTreeRoots<CircuitTypes<Composer>> to_circuit_type(Composer& composer) const
    {
        static_assert((std::is_same<NativeTypes, NCT>::value));

        // Capture the composer:
        auto to_circuit_type = [&](auto& e) { return e.to_circuit_type(composer); };

        CombinedHistoricTreeRoots<CircuitTypes<Composer>> data = {
            to_circuit_type(private_historic_tree_roots),
        };

        return data;
    };

    template <typename Composer> CombinedHistoricTreeRoots<NativeTypes> to_native_type() const
    {
        static_assert(std::is_same<CircuitTypes<Composer>, NCT>::value);
        auto to_native_type = [&]<typename T>(T& e) { return e.template to_native_type<Composer>(); };

        CombinedHistoricTreeRoots<NativeTypes> data = {
            to_native_type(private_historic_tree_roots),
        };

        return data;
    };

    void set_public()
    {
        static_assert(!(std::is_same<NativeTypes, NCT>::value));

        private_historic_tree_roots.set_public();
    }
};

template <typename NCT> void read(uint8_t const*& it, CombinedHistoricTreeRoots<NCT>& historic_tree_roots)
{
    using serialize::read;

    read(it, historic_tree_roots.private_historic_tree_roots);
};

template <typename NCT> void write(std::vector<uint8_t>& buf, CombinedHistoricTreeRoots<NCT> const& historic_tree_roots)
{
    using serialize::write;

    write(buf, historic_tree_roots.private_historic_tree_roots);
};

template <typename NCT>
std::ostream& operator<<(std::ostream& os, CombinedHistoricTreeRoots<NCT> const& historic_tree_roots)
{
    return os << "private_historic_tree_roots: " << historic_tree_roots.private_historic_tree_roots << "\n";
}

}  // namespace aztec3::circuits::abis