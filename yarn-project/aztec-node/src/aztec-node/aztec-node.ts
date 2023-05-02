import { Archiver } from '@aztec/archiver';
import { PrimitivesWasm } from '@aztec/barretenberg.js/wasm';
import { CircuitsWasm } from '@aztec/circuits.js';
import { AztecAddress, Fr } from '@aztec/foundation';
import { SiblingPath } from '@aztec/merkle-tree';
import { P2P, P2PClient } from '@aztec/p2p';
import { SequencerClient, getCombinedHistoricTreeRoots } from '@aztec/sequencer-client';
import {
  ContractData,
  ContractDataSource,
  ContractPublicData,
  L2Block,
  L2BlockSource,
  Tx,
  TxHash,
  UnverifiedData,
  UnverifiedDataSource,
} from '@aztec/types';
import {
  MerkleTreeId,
  MerkleTrees,
  ServerWorldStateSynchroniser,
  WorldStateSynchroniser,
  computePublicDataTreeLeafIndex,
} from '@aztec/world-state';
import { default as levelup } from 'levelup';
import { MemDown, default as memdown } from 'memdown';
import { AztecNodeConfig } from './config.js';

export const createMemDown = () => (memdown as any)() as MemDown<any, any>;

/**
 * The aztec node.
 */
export class AztecNode {
  constructor(
    protected p2pClient: P2P,
    protected blockSource: L2BlockSource,
    protected unverifiedDataSource: UnverifiedDataSource,
    protected contractDataSource: ContractDataSource,
    protected merkleTreeDB: MerkleTrees,
    protected worldStateSynchroniser: WorldStateSynchroniser,
    protected sequencer: SequencerClient,
  ) {}

  /**
   * Initialises the Aztec Node, wait for component to sync.
   * @param config - The configuration to be used by the aztec node.
   * @returns - A fully synced Aztec Node for use in development/testing.
   */
  public static async createAndSync(config: AztecNodeConfig) {
    // first create and sync the archiver
    const archiver = await Archiver.createAndSync(config);

    // give the block source to the P2P network
    const p2pClient = new P2PClient(archiver);

    // now create the merkle trees and the world state syncher
    const merkleTreeDB = await MerkleTrees.new(levelup(createMemDown()), await CircuitsWasm.get());

    const worldStateSynchroniser = new ServerWorldStateSynchroniser(merkleTreeDB, archiver);

    // start both and wait for them to sync from the block source
    await Promise.all([p2pClient.start(), worldStateSynchroniser.start()]);

    // now create the sequencer
    const sequencer = await SequencerClient.new(config, p2pClient, worldStateSynchroniser, archiver);
    return new AztecNode(p2pClient, archiver, archiver, archiver, merkleTreeDB, worldStateSynchroniser, sequencer);
  }

  /**
   * Method to determine if the node is ready to accept transactions.
   * @returns - Flag indicating the readiness for tx submission.
   */
  public async isReady() {
    return (await this.p2pClient.isReady()) ?? false;
  }

  /**
   * Method to request blocks. Will attempt to return all requested blocks but will return only those available.
   * @param from - The start of the range of blocks to return.
   * @param take - The number of blocks desired.
   * @returns The blocks requested.
   */
  public async getBlocks(from: number, take: number): Promise<L2Block[]> {
    return (await this.blockSource.getL2Blocks(from, take)) ?? [];
  }

  /**
   * Method to fetch the current block height
   * @returns The block height as a number.
   */
  public async getBlockHeight(): Promise<number> {
    return await this.blockSource.getBlockHeight();
  }

  /**
   * Lookup the L2 contract data for this contract.
   * Contains the ethereum portal address and bytecode.
   * @param contractAddress - The contract data address.
   * @returns The complete contract data including portal address & bytecode (if we didn't throw an error).
   */
  public async getContractData(contractAddress: AztecAddress): Promise<ContractPublicData | undefined> {
    return await this.contractDataSource.getL2ContractPublicData(contractAddress);
  }

  /**
   * Lookup the L2 contract info for this contract.
   * Contains the ethereum portal address .
   * @param contractAddress - The contract data address.
   * @returns The contract's address & portal address.
   */
  public async getContractInfo(contractAddress: AztecAddress): Promise<ContractData | undefined> {
    return await this.contractDataSource.getL2ContractInfo(contractAddress);
  }

  /**
   * Gets the `take` amount of unverified data starting from `from`.
   * @param from - Number of the L2 block to which corresponds the first `unverifiedData` to be returned.
   * @param take - The number of `unverifiedData` to return.
   * @returns The requested `unverifiedData`.
   */
  public getUnverifiedData(from: number, take: number): Promise<UnverifiedData[]> {
    return this.unverifiedDataSource.getUnverifiedData(from, take);
  }

  /**
   * Method to submit a transaction to the p2p pool.
   * @param tx - The transaction to be submitted.
   */
  public async sendTx(tx: Tx) {
    // TODO: Patch tx to inject historic tree roots until the private kernel circuit supplies this value
    if (tx.isPrivate() && tx.data.constants.historicTreeRoots.privateHistoricTreeRoots.isEmpty()) {
      tx.data.constants.historicTreeRoots = await getCombinedHistoricTreeRoots(this.merkleTreeDB.asLatest());
    }

    await this.p2pClient!.sendTx(tx);
  }

  /**
   * Method to stop the aztec node.
   */
  public async stop() {
    await this.p2pClient.stop();
    await this.worldStateSynchroniser.stop();
    await this.merkleTreeDB.stop();
    await this.sequencer.stop();
    await this.blockSource.stop();
  }

  /**
   * Method to retrieve pending txs.
   * @returns - The pending txs.
   */
  public async getPendingTxs() {
    return await this.p2pClient!.getTxs();
  }

  /**
   * Method to retrieve a single pending tx
   * @param txHash - The transaction hash to return.
   * @returns - The pending tx if it exists
   */
  public async getPendingTxByHash(txHash: TxHash) {
    // const allTxs = await this.p2pClient.getTxs();
    const result = await this.p2pClient!.getTxByhash(txHash);
    return result;
  }

  public findContractIndex(leafValue: Buffer): Promise<bigint | undefined> {
    return this.merkleTreeDB.findLeafIndex(MerkleTreeId.CONTRACT_TREE, leafValue, false);
  }

  public getContractPath(leafIndex: bigint): Promise<SiblingPath> {
    return this.merkleTreeDB.getSiblingPath(MerkleTreeId.CONTRACT_TREE, leafIndex, false);
  }

  public getDataTreePath(leafIndex: bigint): Promise<SiblingPath> {
    return this.merkleTreeDB.getSiblingPath(MerkleTreeId.PRIVATE_DATA_TREE, leafIndex, false);
  }

  /**
   * Gets the storage value at the given contract slot.
   * @param contract - Address of the contract to query
   * @param slot - Slot to query
   * @returns Storage value at the given contract slot (or undefined if not found).
   * Note: Aztec's version of `eth_getStorageAt`
   */
  public async getStorageAt(contract: AztecAddress, slot: bigint): Promise<Buffer | undefined> {
    const leafIndex = computePublicDataTreeLeafIndex(contract, new Fr(slot), await PrimitivesWasm.get());
    return this.merkleTreeDB.getLeafValue(MerkleTreeId.PUBLIC_DATA_TREE, leafIndex, false);
  }
}
