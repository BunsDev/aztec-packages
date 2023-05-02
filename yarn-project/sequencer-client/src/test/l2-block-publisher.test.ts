import {
  AppendOnlyTreeSnapshot,
  BaseOrMergeRollupPublicInputs,
  CircuitsWasm,
  Fr,
  RootRollupPublicInputs,
  UInt8Vector,
} from '@aztec/circuits.js';
import { computeContractLeaf } from '@aztec/circuits.js/abis';
import {
  makeBaseRollupPublicInputs,
  makeKernelPublicInputs,
  makeRootRollupPublicInputs,
} from '@aztec/circuits.js/factories';
import { EthereumRpc } from '@aztec/ethereum.js/eth_rpc';
import { WalletProvider } from '@aztec/ethereum.js/provider';
import { DecoderHelper, Rollup, UnverifiedDataEmitter } from '@aztec/l1-contracts';
import { Tx } from '@aztec/types';
import { MerkleTreeId, MerkleTreeOperations, MerkleTrees } from '@aztec/world-state';
import { beforeAll, describe, expect, it } from '@jest/globals';
import { default as levelup } from 'levelup';
import flatMap from 'lodash.flatmap';
import { CircuitBlockBuilder } from '../block_builder/circuit_block_builder.js';
import { createMemDown } from '../block_builder/circuit_block_builder.test.js';
import { getVerificationKeys, makeEmptyUnverifiedData } from '../index.js';
import { EmptyRollupProver } from '../prover/empty.js';
import { EthereumjsTxSender } from '../publisher/ethereumjs-tx-sender.js';
import { L1Publisher } from '../publisher/l1-publisher.js';
import {
  ProcessedTx,
  makeEmptyProcessedTx as makeEmptyProcessedTxFromHistoricTreeRoots,
  makeProcessedTx,
} from '../sequencer/processed_tx.js';
import { getCombinedHistoricTreeRoots } from '../sequencer/utils.js';
import { WasmRollupCircuitSimulator } from '../simulator/rollup.js';
import { hexStringToBuffer } from '../utils.js';

// Accounts 4 and 5 of Anvil default startup with mnemonic: 'test test test test test test test test test test test junk'
const sequencerPK = '0x47e179ec197488593b187f80a00eb0da91f1b9d0b13f8733639f19c30a34926a';
const deployerPK = '0x8b3a350cf5c34c9194ca85829a2df0ec3153be0318b5e2d3348e872092edffba';
const anvilHost = process.env.ANVIL_HOST ?? 'http://127.0.0.1:8545';
const chainId = 31337;

describe.skip('L1Publisher integration', () => {
  let decoderHelper: DecoderHelper;
  let rollup: Rollup;
  let unverifiedDataEmitter: UnverifiedDataEmitter;
  let ethRpc: EthereumRpc;
  let publisher: L1Publisher;
  let l2Proof: Buffer;

  const emptyProof = new UInt8Vector(Buffer.alloc(32, 0));
  let baseRollupOutputLeft: BaseOrMergeRollupPublicInputs;
  let baseRollupOutputRight: BaseOrMergeRollupPublicInputs;
  let rootRollupOutput: RootRollupPublicInputs;
  let builder: CircuitBlockBuilder;
  let builderDb: MerkleTreeOperations;
  let expectsDb: MerkleTreeOperations;
  let wasm: CircuitsWasm;

  beforeAll(async () => {
    ({ ethRpc, decoderHelper, rollup, unverifiedDataEmitter } = await deployRollup());

    builderDb = await MerkleTrees.new(levelup(createMemDown())).then(t => t.asLatest());
    expectsDb = await MerkleTrees.new(levelup(createMemDown())).then(t => t.asLatest());
    wasm = await CircuitsWasm.get();
    const vks = getVerificationKeys();
    const simulator = await WasmRollupCircuitSimulator.new();
    const prover = new EmptyRollupProver();
    builder = new CircuitBlockBuilder(builderDb, vks, simulator, prover);
    await expectsDb.updateRootsTrees();
    await builderDb.updateRootsTrees();

    baseRollupOutputLeft = makeBaseRollupPublicInputs();
    baseRollupOutputRight = makeBaseRollupPublicInputs();
    rootRollupOutput = makeRootRollupPublicInputs();
    l2Proof = Buffer.alloc(0);

    publisher = new L1Publisher(
      new EthereumjsTxSender({
        rpcUrl: anvilHost,
        chainId,
        requiredConfirmations: 1,
        rollupContract: rollup.address,
        unverifiedDataEmitterContract: unverifiedDataEmitter.address,
        publisherPrivateKey: hexStringToBuffer(sequencerPK),
      }),
      {
        retryIntervalMs: 100,
      },
    );
  }, 60_000);

  const makeEmptyProcessedTx = async () => {
    const historicTreeRoots = await getCombinedHistoricTreeRoots(builderDb);
    return makeEmptyProcessedTxFromHistoricTreeRoots(historicTreeRoots);
  };

  it('Build 2 blocks of 4 txs building on each other', async () => {
    const stateInRollup_ = await rollup.methods.rollupStateHash().call();
    expect(hexStringToBuffer(stateInRollup_.toString())).toEqual(Buffer.alloc(32, 0));

    for (let i = 0; i < 2; i++) {
      const tx = await makeProcessedTx(
        Tx.createPrivate(makeKernelPublicInputs(1 + i), emptyProof, makeEmptyUnverifiedData()),
      );

      const txsLeft = [tx, await makeEmptyProcessedTx()];
      const txsRight = [await makeEmptyProcessedTx(), await makeEmptyProcessedTx()];

      // Set tree roots to proper values in the tx
      await setTxHistoricTreeRoots(tx);

      // Calculate what would be the tree roots after the txs from the first base rollup land and update mock circuit output
      await updateExpectedTreesFromTxs(txsLeft);
      baseRollupOutputLeft.endContractTreeSnapshot = await getTreeSnapshot(MerkleTreeId.CONTRACT_TREE);
      baseRollupOutputLeft.endNullifierTreeSnapshot = await getTreeSnapshot(MerkleTreeId.NULLIFIER_TREE);
      baseRollupOutputLeft.endPrivateDataTreeSnapshot = await getTreeSnapshot(MerkleTreeId.PRIVATE_DATA_TREE);

      // Same for the two txs on the right
      await updateExpectedTreesFromTxs(txsRight);
      baseRollupOutputRight.endContractTreeSnapshot = await getTreeSnapshot(MerkleTreeId.CONTRACT_TREE);
      baseRollupOutputRight.endNullifierTreeSnapshot = await getTreeSnapshot(MerkleTreeId.NULLIFIER_TREE);
      baseRollupOutputRight.endPrivateDataTreeSnapshot = await getTreeSnapshot(MerkleTreeId.PRIVATE_DATA_TREE);

      // And update the root trees now to create proper output to the root rollup circuit
      await expectsDb.updateRootsTrees();
      rootRollupOutput.endContractTreeSnapshot = await getTreeSnapshot(MerkleTreeId.CONTRACT_TREE);
      rootRollupOutput.endNullifierTreeSnapshot = await getTreeSnapshot(MerkleTreeId.NULLIFIER_TREE);
      rootRollupOutput.endPrivateDataTreeSnapshot = await getTreeSnapshot(MerkleTreeId.PRIVATE_DATA_TREE);
      rootRollupOutput.endTreeOfHistoricContractTreeRootsSnapshot = await getTreeSnapshot(
        MerkleTreeId.CONTRACT_TREE_ROOTS_TREE,
      );
      rootRollupOutput.endTreeOfHistoricPrivateDataTreeRootsSnapshot = await getTreeSnapshot(
        MerkleTreeId.PRIVATE_DATA_TREE_ROOTS_TREE,
      );

      // Actually build a block!
      const txs = [tx, await makeEmptyProcessedTx(), await makeEmptyProcessedTx(), await makeEmptyProcessedTx()];
      // Here we die.
      const [block] = await builder.buildL2Block(1 + i, txs);

      // Now we can use the block we built!
      const blockNumber = await ethRpc.blockNumber();
      await publisher.processL2Block(block);
      const logs = await rollup.getLogs('L2BlockProcessed', { fromBlock: blockNumber + 1 });
      expect(logs).toHaveLength(1);
      expect(logs[0].args.blockNum).toEqual(BigInt(i + 1));

      const ethTx = await ethRpc.getTransactionByHash(logs[0].transactionHash!);
      const expectedData = rollup.methods.process(l2Proof, block.encode()).encodeABI();
      expect(ethTx.input).toEqual(expectedData);

      const decodedCalldataHash = await decoderHelper.methods.computeDiffRoot(block.encode()).call();
      const decodedRes = await decoderHelper.methods.decode(block.encode()).call();
      const stateInRollup = await rollup.methods.rollupStateHash().call();

      // @note There seems to be something wrong here. The Bytes32 returned are actually strings :(
      expect(block.number).toEqual(Number(decodedRes[0]));
      expect(block.getStartStateHash()).toEqual(hexStringToBuffer(decodedRes[1].toString()));
      expect(block.getEndStateHash()).toEqual(hexStringToBuffer(decodedRes[2].toString()));
      expect(block.getEndStateHash()).toEqual(hexStringToBuffer(stateInRollup.toString()));
      expect(block.getPublicInputsHash().toBuffer()).toEqual(hexStringToBuffer(decodedRes[3].toString()));
      expect(block.getCalldataHash()).toEqual(hexStringToBuffer(decodedCalldataHash.toString()));
    }
  }, 60_000);

  // BELOW IS FUNCTIONS STOLEN FROM `circuit_block_builder.test.ts`.

  // Updates the expectedDb trees based on the new commitments, contracts, and nullifiers from these txs
  const updateExpectedTreesFromTxs = async (txs: ProcessedTx[]) => {
    const newContracts = flatMap(txs, tx => tx.data.end.newContracts.map(n => computeContractLeaf(wasm, n)));
    for (const [tree, leaves] of [
      [MerkleTreeId.PRIVATE_DATA_TREE, flatMap(txs, tx => tx.data.end.newCommitments.map(l => l.toBuffer()))],
      [MerkleTreeId.CONTRACT_TREE, newContracts.map(x => x.toBuffer())],
      [MerkleTreeId.NULLIFIER_TREE, flatMap(txs, tx => tx.data.end.newNullifiers.map(l => l.toBuffer()))],
    ] as const) {
      await expectsDb.appendLeaves(tree, leaves);
    }
    for (const write of txs.flatMap(tx => tx.data.end.stateTransitions)) {
      await expectsDb.updateLeaf(MerkleTreeId.PUBLIC_DATA_TREE, write.newValue.toBuffer(), write.leafIndex.value);
    }
  };

  const setTxHistoricTreeRoots = async (tx: ProcessedTx) => {
    for (const [name, id] of [
      ['privateDataTreeRoot', MerkleTreeId.PRIVATE_DATA_TREE],
      ['contractTreeRoot', MerkleTreeId.CONTRACT_TREE],
      ['nullifierTreeRoot', MerkleTreeId.NULLIFIER_TREE],
    ] as const) {
      tx.data.constants.historicTreeRoots.privateHistoricTreeRoots[name] = Fr.fromBuffer(
        (await builderDb.getTreeInfo(id)).root,
      );
    }
  };

  const getTreeSnapshot = async (tree: MerkleTreeId) => {
    const treeInfo = await expectsDb.getTreeInfo(tree);
    return new AppendOnlyTreeSnapshot(Fr.fromBuffer(treeInfo.root), Number(treeInfo.size));
  };
});

async function deployRollup() {
  // Set up client
  const provider = WalletProvider.fromHost(anvilHost);
  provider.addAccount(hexStringToBuffer(deployerPK));
  provider.addAccount(hexStringToBuffer(sequencerPK));
  const [sequencer, deployer] = provider.getAccounts();
  const ethRpc = new EthereumRpc(provider);

  // Deploy DecodeHelper, Rollup and unverifiedDataEmitter contracts
  const decoderHelper = new DecoderHelper(ethRpc, undefined, { from: deployer, gas: 1e6 });
  await decoderHelper.deploy().send().getReceipt();

  const deployedRollup = new Rollup(ethRpc, undefined, { from: deployer, gas: 1e6 });
  await deployedRollup.deploy().send().getReceipt();

  const deployedUnverifiedDataEmitter = new UnverifiedDataEmitter(ethRpc, undefined, { from: deployer, gas: 1e6 });
  await deployedUnverifiedDataEmitter.deploy().send().getReceipt();

  // Create new instance so we can attach the sequencer as sender
  const rollup = new Rollup(ethRpc, deployedRollup.address, { from: sequencer });
  const unverifiedDataEmitter = new UnverifiedDataEmitter(ethRpc, deployedUnverifiedDataEmitter.address, {
    from: sequencer,
  });

  return { decoderHelper, rollup, deployer, unverifiedDataEmitter, sequencer, ethRpc };
}
