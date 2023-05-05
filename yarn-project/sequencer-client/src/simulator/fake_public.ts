import { PublicDB, PublicExecution } from '@aztec/acir-simulator';
import { AztecAddress, CircuitsWasm, EthAddress, Fr, PublicCircuitPublicInputs, TxRequest } from '@aztec/circuits.js';
import { MerkleTreeId, MerkleTreeOperations } from '@aztec/world-state';
import { PublicCircuitSimulator } from './index.js';
import { computePublicDataTreeLeafIndex } from '@aztec/circuits.js/abis';

export class FakePublicCircuitSimulator implements PublicCircuitSimulator {
  public readonly db: WorldStatePublicDB;

  constructor(private readonly merkleTree: MerkleTreeOperations) {
    this.db = new WorldStatePublicDB(this.merkleTree);
  }

  public async publicCircuit(
    tx: TxRequest,
    functionBytecode: Buffer,
    portalAddress: EthAddress,
  ): Promise<PublicCircuitPublicInputs> {
    const publicDataTreeInfo = await this.merkleTree.getTreeInfo(MerkleTreeId.PUBLIC_DATA_TREE);
    const historicPublicDataTreeRoot = Fr.fromBuffer(publicDataTreeInfo.root);

    const execution = PublicExecution.fromTransactionRequest(this.db, tx, functionBytecode, portalAddress);
    const result = await execution.run();
    return PublicCircuitPublicInputs.from({
      args: tx.args,
      callContext: execution.callContext,
      emittedEvents: [],
      l1MsgStack: [],
      proverAddress: AztecAddress.random(),
      publicCallStack: [],
      returnValues: result.returnValues,
      stateReads: result.stateReads,
      stateTransitions: result.stateTransitions,
      historicPublicDataTreeRoot,
    });
  }
}

class WorldStatePublicDB implements PublicDB {
  constructor(private db: MerkleTreeOperations) {}

  public async storageRead(contract: AztecAddress, slot: Fr): Promise<Fr> {
    const leafIndex = computePublicDataTreeLeafIndex(await CircuitsWasm.get(), contract, slot);
    const value = await this.db.getLeafValue(MerkleTreeId.PUBLIC_DATA_TREE, leafIndex);
    return value ? Fr.fromBuffer(value) : Fr.ZERO;
  }
}
