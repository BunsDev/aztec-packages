// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 Aztec Labs.
pragma solidity >=0.8.18;

import {ERC20} from "@oz/token/ERC20/ERC20.sol";
import {DataStructures} from "@aztec/core/libraries/DataStructures.sol";
import {Constants} from "@aztec/core/libraries/Constants.sol";
import {Registry} from "@aztec/core/messagebridge/Registry.sol";

contract RollupNativeAsset is ERC20 {
  Registry public registry;
  bytes32 public aztecAddress;

  constructor() ERC20("RollupNativeAsset", "RNA") {}

  function initialize(address _registry, bytes32 _aztecAddress) external {
    registry = Registry(_registry);
    aztecAddress = _aztecAddress;
  }

  function withdraw(uint256 _amount, address _recipient) external returns (bytes32) {
    bytes memory contentBytes =
      abi.encodeWithSignature("withdraw(uint256,address)", _amount, _recipient);
    bytes32 content = bytes32(uint256(sha256(contentBytes)) % Constants.P);

    DataStructures.L2ToL1Msg memory message = DataStructures.L2ToL1Msg({
      sender: DataStructures.L2Actor(aztecAddress, 1),
      recipient: DataStructures.L1Actor(address(this), block.chainid),
      content: content
    });

    bytes32 entryKey = registry.getOutbox().consume(message);

    _mint(_recipient, _amount);

    return entryKey;
  }
}
