import {
  ACVMField,
  acvm,
  toACVMField,
  fromACVMField,
  ZERO_ACVM_FIELD,
  toAcvmCallPrivateStackItem,
  toAcvmNoteLoadOracleInputs,
  writeInputs,
} from './acvm/index.js';
import { AztecAddress, EthAddress, Fr } from '@aztec/foundation';
import {
  CallContext,
  OldTreeRoots,
  TxRequest,
  PrivateCallStackItem,
  FunctionData,
  PRIVATE_DATA_TREE_HEIGHT,
} from '@aztec/circuits.js';
import { DBOracle } from './db_oracle.js';
import { extractPublicInputs, frToAztecAddress, frToSelector } from './acvm/deserialize.js';
import { FunctionAbi } from '@aztec/noir-contracts';

interface NewNoteData {
  preimage: Fr[];
  storageSlot: Fr;
  owner: { x: Fr; y: Fr };
}

interface NewNullifierData {
  preimage: Fr[];
  storageSlot: Fr;
  nullifier: Fr;
}

export interface ExecutionPreimages {
  newNotes: NewNoteData[];
  nullifiedNotes: NewNullifierData[];
}

export interface ExecutionResult {
  // Needed for prover
  acir: Buffer;
  vk: Buffer;
  partialWitness: Map<number, ACVMField>;
  // Needed for the verifier (kernel)
  callStackItem: PrivateCallStackItem;
  // Needed for the user
  preimages: ExecutionPreimages;
  // Nested executions
  nestedExecutions: this[];
}

export class Execution {
  constructor(
    // Global to the tx
    private db: DBOracle,
    private request: TxRequest,
    private oldRoots: OldTreeRoots,
    // Concrete to this execution
    private abi: FunctionAbi,
    private contractAddress: AztecAddress,
    private functionData: FunctionData,
    private args: Fr[],
    private callContext: CallContext,
  ) {}

  public async run(): Promise<ExecutionResult> {
    const acir = Buffer.from(this.abi.bytecode, 'hex');
    const initialWitness = writeInputs(this.args, this.callContext, this.request.txContext, this.oldRoots);
    const newNotePreimages: NewNoteData[] = [];
    const newNullifiers: NewNullifierData[] = [];
    const nestedExecutionContexts: ExecutionResult[] = [];

    const { partialWitness } = await acvm(acir, initialWitness, {
      getSecretKey: ([address]: ACVMField[]) => {
        return this.getSecretKey(this.contractAddress, address);
      },
      getNotes2: async ([, storageSlot]: ACVMField[]) => {
        return await this.getNotes(this.contractAddress, storageSlot, 2);
      },
      getRandomField: () => Promise.resolve([toACVMField(Fr.random())]),
      notifyCreatedNote: ([storageSlot, ownerX, ownerY, ...acvmPreimage]: ACVMField[]) => {
        newNotePreimages.push({
          preimage: acvmPreimage.map(f => fromACVMField(f)),
          storageSlot: fromACVMField(storageSlot),
          owner: {
            x: fromACVMField(ownerX),
            y: fromACVMField(ownerY),
          },
        });
        return Promise.resolve([ZERO_ACVM_FIELD]);
      },
      notifyNullifiedNote: ([slot, nullifier, ...acvmPreimage]: ACVMField[]) => {
        newNullifiers.push({
          preimage: acvmPreimage.map(f => fromACVMField(f)),
          storageSlot: fromACVMField(slot),
          nullifier: fromACVMField(nullifier),
        });
        return Promise.resolve([ZERO_ACVM_FIELD]);
      },
      privateFunctionCall: async ([acvmContractAddress, acvmFunctionSelector, ...acvmArgs]) => {
        const childExecutionResult = await this.privateFunctionCall(
          frToAztecAddress(fromACVMField(acvmContractAddress)),
          frToSelector(fromACVMField(acvmFunctionSelector)),
          acvmArgs.map(f => fromACVMField(f)),
          this.callContext,
        );

        nestedExecutionContexts.push(childExecutionResult);

        return toAcvmCallPrivateStackItem(childExecutionResult.callStackItem);
      },
    });

    const publicInputs = extractPublicInputs(partialWitness, acir);

    const callStackItem = new PrivateCallStackItem(this.contractAddress, this.functionData, publicInputs);

    return {
      acir,
      partialWitness,
      callStackItem,
      preimages: {
        newNotes: newNotePreimages,
        nullifiedNotes: newNullifiers,
      },
      vk: Buffer.from(this.abi.verificationKey!, 'hex'),
      nestedExecutions: nestedExecutionContexts,
    };
  }

  private async getNotes(contractAddress: AztecAddress, storageSlot: ACVMField, count: number) {
    let notes = await this.db.getNotes(contractAddress, fromACVMField(storageSlot), count);
    if (notes.length < count) {
      const dummyCount = count - notes.length;
      const dummyNotes = Array(dummyCount)
        .fill(null)
        .map(() => this.createDummyNote());
      notes = notes.concat(dummyNotes);
    }
    const mapped = notes.flatMap(noteGetData =>
      toAcvmNoteLoadOracleInputs(noteGetData, this.oldRoots.privateDataTreeRoot),
    );
    return mapped;
  }

  // TODO this should use an unconstrained fn in the future
  private createDummyNote() {
    return {
      preimage: [new Fr(1n), new Fr(0n), new Fr(0n), new Fr(0n), new Fr(0n), new Fr(0n)],
      siblingPath: new Array(PRIVATE_DATA_TREE_HEIGHT).fill(new Fr(0n)),
      index: 0,
    };
  }

  private async getSecretKey(contractAddress: AztecAddress, address: ACVMField) {
    const key = await this.db.getSecretKey(contractAddress, frToAztecAddress(fromACVMField(address)));
    return [toACVMField(key)];
  }

  private async privateFunctionCall(
    targetContractAddress: AztecAddress,
    targetFunctionSelector: Buffer,
    targetArgs: Fr[],
    callerContext: CallContext,
  ) {
    const targetAbi = await this.db.getFunctionABI(targetContractAddress, targetFunctionSelector);
    const targetPortalContractAddress = await this.db.getPortalContractAddress(targetContractAddress);
    const targetFunctionData = new FunctionData(targetFunctionSelector, true, false);
    const derivedCallContext = this.deriveCallContext(
      callerContext,
      targetContractAddress,
      targetPortalContractAddress,
      false,
      false,
    );

    const nestedExecution = new Execution(
      this.db,
      this.request,
      this.oldRoots,
      targetAbi,
      targetContractAddress,
      targetFunctionData,
      targetArgs,
      derivedCallContext,
    );

    return nestedExecution.run();
  }

  private deriveCallContext(
    parentContext: CallContext,
    targetContractAddress: AztecAddress,
    portalContractAddress: EthAddress,
    isDelegateCall = false,
    isStaticCall = false,
  ) {
    return new CallContext(
      parentContext.storageContractAddress,
      targetContractAddress,
      portalContractAddress,
      isDelegateCall,
      isStaticCall,
      false,
    );
  }
}
