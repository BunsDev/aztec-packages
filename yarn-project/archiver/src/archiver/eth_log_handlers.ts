import { Hex, Log, PublicClient, decodeFunctionData, getAbiItem, getAddress, hexToBytes } from 'viem';
import { InboxAbi, RollupAbi, UnverifiedDataEmitterAbi } from '@aztec/l1-artifacts';
import { Fr } from '@aztec/foundation/fields';
import {
  BufferReader,
  ContractData,
  ContractPublicData,
  EncodedContractFunction,
  L1ToL2Message,
  L1Actor,
  L2Actor,
  L2Block,
  UnverifiedData,
} from '@aztec/types';
import { EthAddress } from '@aztec/foundation/eth-address';
import { AztecAddress } from '@aztec/foundation/aztec-address';

/**
 * Processes newly received MessageAdded (L1 to L2) logs.
 * @param logs - MessageAdded logs.
 * @returns Array of all Pending L1 to L2 messages that were processed
 */
export function processPendingL1ToL2MessageAddedLogs(
  logs: Log<bigint, number, undefined, typeof InboxAbi, 'MessageAdded'>[],
): L1ToL2Message[] {
  const l1ToL2Messages: L1ToL2Message[] = [];
  for (const log of logs) {
    const { sender, senderChainId, recipient, recipientVersion, content, secretHash, deadline, fee, entryKey } =
      log.args;
    l1ToL2Messages.push(
      new L1ToL2Message(
        new L1Actor(EthAddress.fromString(sender), Number(senderChainId)),
        new L2Actor(AztecAddress.fromString(recipient), Number(recipientVersion)),
        Fr.fromString(content),
        Fr.fromString(secretHash),
        deadline,
        Number(fee),
        Fr.fromString(entryKey),
      ),
    );
  }
  return l1ToL2Messages;
}
/**
 * Processes newly received UnverifiedData logs.
 * @param blockHashMapping - A mapping from block number to relevant block hash.
 * @param logs - ContractDeployment logs.
 * @returns The set of retrieved contract public data items.
 */
export function processContractDeploymentLogs(
  blockHashMapping: { [key: number]: Buffer | undefined },
  logs: Log<bigint, number, undefined, typeof UnverifiedDataEmitterAbi, 'ContractDeployment'>[],
): [ContractPublicData[], number][] {
  const contractPublicData: [ContractPublicData[], number][] = [];
  for (let i = 0; i < logs.length; i++) {
    const log = logs[i];
    const l2BlockNum = Number(log.args.l2BlockNum);
    const blockHash = Buffer.from(hexToBytes(log.args.l2BlockHash));
    const expectedBlockHash = blockHashMapping[l2BlockNum];
    if (expectedBlockHash === undefined || !blockHash.equals(expectedBlockHash)) {
      continue;
    }
    const publicFnsReader = BufferReader.asReader(Buffer.from(log.args.acir.slice(2), 'hex'));
    const contractData = new ContractPublicData(
      new ContractData(AztecAddress.fromString(log.args.aztecAddress), EthAddress.fromString(log.args.portalAddress)),
      publicFnsReader.readVector(EncodedContractFunction),
    );
    if (contractPublicData[i]) {
      contractPublicData[i][0].push(contractData);
    } else {
      contractPublicData[i] = [[contractData], l2BlockNum];
    }
  }
  return contractPublicData;
}

/**
 * Processes newly received UnverifiedData logs.
 * @param expectedRollupNumber - The next expected rollup number.
 * @param blockHashMapping - A mapping from block number to relevant block hash.
 * @param logs - UnverifiedData logs.
 */
export function processUnverifiedDataLogs(
  expectedRollupNumber: bigint,
  blockHashMapping: { [key: number]: Buffer | undefined },
  logs: Log<bigint, number, undefined, typeof UnverifiedDataEmitterAbi, 'UnverifiedData'>[],
) {
  const unverifiedDataChunks: UnverifiedData[] = [];
  for (const log of logs) {
    const l2BlockNum = log.args.l2BlockNum;
    const blockHash = Buffer.from(hexToBytes(log.args.l2BlockHash));
    const expectedBlockHash = blockHashMapping[Number(l2BlockNum)];
    if (
      l2BlockNum !== expectedRollupNumber ||
      expectedBlockHash === undefined ||
      !blockHash.equals(expectedBlockHash)
    ) {
      continue;
    }
    const unverifiedDataBuf = Buffer.from(hexToBytes(log.args.data));
    const unverifiedData = UnverifiedData.fromBuffer(unverifiedDataBuf);
    unverifiedDataChunks.push(unverifiedData);
    expectedRollupNumber++;
  }
  return unverifiedDataChunks;
}

/**
 * Processes newly received L2BlockProcessed logs.
 * @param publicClient - The viem public client to use for transaction retrieval.
 * @param expectedRollupNumber - The next expected rollup number.
 * @param logs - L2BlockProcessed logs.
 */
export async function processBlockLogs(
  publicClient: PublicClient,
  expectedRollupNumber: bigint,
  logs: Log<bigint, number, undefined, typeof RollupAbi, 'L2BlockProcessed'>[],
) {
  const retrievedBlocks: L2Block[] = [];
  for (const log of logs) {
    const blockNum = log.args.blockNum;
    if (blockNum !== expectedRollupNumber) {
      throw new Error('Block number mismatch. Expected: ' + expectedRollupNumber + ' but got: ' + blockNum + '.');
    }
    // TODO: Fetch blocks from calldata in parallel
    const newBlock = await getBlockFromCallData(publicClient, log.transactionHash!, log.args.blockNum);
    retrievedBlocks.push(newBlock);
    expectedRollupNumber++;
  }
  return retrievedBlocks;
}

/**
 * Builds an L2 block out of calldata from the tx that published it.
 * Assumes that the block was published from an EOA.
 * TODO: Add retries and error management.
 * @param publicClient - The viem public client to use for transaction retrieval.
 * @param txHash - Hash of the tx that published it.
 * @param l2BlockNum - L2 block number.
 * @returns An L2 block deserialized from the calldata.
 */
async function getBlockFromCallData(
  publicClient: PublicClient,
  txHash: `0x${string}`,
  l2BlockNum: bigint,
): Promise<L2Block> {
  const { input: data } = await publicClient.getTransaction({ hash: txHash });
  // TODO: File a bug in viem who complains if we dont remove the ctor from the abi here
  const { functionName, args } = decodeFunctionData({
    abi: RollupAbi.filter(item => item.type.toString() !== 'constructor'),
    data,
  });
  if (functionName !== 'process') throw new Error(`Unexpected method called ${functionName}`);
  const [, l2BlockHex] = args! as [Hex, Hex];
  const block = L2Block.decode(Buffer.from(hexToBytes(l2BlockHex)));
  if (BigInt(block.number) !== l2BlockNum) {
    throw new Error(`Block number mismatch: expected ${l2BlockNum} but got ${block.number}`);
  }
  return block;
}

/**
 * Gets relevant `L2BlockProcessed` logs from chain.
 * @param publicClient - The viem public client to use for transaction retrieval.
 * @param rollupAddress - The address of the rollup contract.
 * @param fromBlock - First block to get logs from (inclusive).
 * @returns An array of `L2BlockProcessed` logs.
 */
export async function getL2BlockProcessedLogs(
  publicClient: PublicClient,
  rollupAddress: EthAddress,
  fromBlock: bigint,
) {
  // Note: For some reason the return type of `getLogs` would not get correctly derived if I didn't set the abiItem
  //       as a standalone constant.
  const abiItem = getAbiItem({
    abi: RollupAbi,
    name: 'L2BlockProcessed',
  });
  return await publicClient.getLogs({
    address: getAddress(rollupAddress.toString()),
    event: abiItem,
    fromBlock,
  });
}

/**
 * Gets relevant `UnverifiedData` logs from chain.
 * @param publicClient - The viem public client to use for transaction retrieval.
 * @param unverifiedDataEmitterAddress - The address of the unverified data emitter contract.
 * @param fromBlock - First block to get logs from (inclusive).
 * @returns An array of `UnverifiedData` logs.
 */
export async function getUnverifiedDataLogs(
  publicClient: PublicClient,
  unverifiedDataEmitterAddress: EthAddress,
  fromBlock: bigint,
): Promise<any[]> {
  // Note: For some reason the return type of `getLogs` would not get correctly derived if I didn't set the abiItem
  //       as a standalone constant.
  const abiItem = getAbiItem({
    abi: UnverifiedDataEmitterAbi,
    name: 'UnverifiedData',
  });
  return await publicClient.getLogs({
    address: getAddress(unverifiedDataEmitterAddress.toString()),
    event: abiItem,
    fromBlock,
  });
}

/**
 * Gets relevant `ContractDeployment` logs from chain.
 * @param publicClient - The viem public client to use for transaction retrieval.
 * @param unverifiedDataEmitterAddress - The address of the unverified data emitter contract.
 * @param fromBlock - First block to get logs from (inclusive).
 * @returns An array of `ContractDeployment` logs.
 */
export async function getContractDeploymentLogs(
  publicClient: PublicClient,
  unverifiedDataEmitterAddress: EthAddress,
  fromBlock: bigint,
): Promise<Log<bigint, number, undefined, typeof UnverifiedDataEmitterAbi, 'ContractDeployment'>[]> {
  const abiItem = getAbiItem({
    abi: UnverifiedDataEmitterAbi,
    name: 'ContractDeployment',
  });
  return await publicClient.getLogs({
    address: getAddress(unverifiedDataEmitterAddress.toString()),
    event: abiItem,
    fromBlock,
  });
}

/**
 * Get relevant `MessageAdded` logs emitted by Inbox on chain.
 * @param publicClient - The viem public client to use for transaction retrieval.
 * @param inboxAddress - The address of the inbox contract.
 * @param fromBlock - First block to get logs from (inclusive).
 * @returns An array of `MessageAdded` logs.
 */
export async function getPendingL1ToL2MessageLogs(
  publicClient: PublicClient,
  inboxAddress: EthAddress,
  fromBlock: bigint,
): Promise<Log<bigint, number, undefined, typeof InboxAbi, 'MessageAdded'>[]> {
  const abiItem = getAbiItem({
    abi: InboxAbi,
    name: 'MessageAdded',
  });
  return await publicClient.getLogs({
    address: getAddress(inboxAddress.toString()),
    event: abiItem,
    fromBlock,
  });
}
