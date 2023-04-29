import { PUBLIC_DATA_TREE_HEIGHT, Proof, makeEmptyProof } from '@aztec/circuits.js';
import { makeKernelPublicInputs, makePublicCircuitPublicInputs } from '@aztec/circuits.js/factories';
import { SiblingPath } from '@aztec/merkle-tree';
import { ContractPublicData, ContractDataSource, EncodedContractFunction } from '@aztec/types';
import { MerkleTreeOperations, TreeInfo } from '@aztec/world-state';
import { jest } from '@jest/globals';
import { MockProxy, mock } from 'jest-mock-extended';
import pick from 'lodash.pick';
import times from 'lodash.times';
import { makePrivateTx, makePublicTx } from '../index.js';
import { PublicProver } from '../prover/index.js';
import { PublicCircuitSimulator, PublicKernelCircuitSimulator } from '../simulator/index.js';
import { WasmPublicKernelCircuitSimulator } from '../simulator/public_kernel.js';
import { PublicProcessor } from './public_processor.js';

describe('public_processor', () => {
  let db: MockProxy<MerkleTreeOperations>;
  let publicCircuit: MockProxy<PublicCircuitSimulator>;
  let publicKernel: MockProxy<PublicKernelCircuitSimulator>;
  let publicProver: MockProxy<PublicProver>;
  let contractDataSource: MockProxy<ContractDataSource>;

  let publicFunction: EncodedContractFunction;
  let contractData: ContractPublicData;
  let proof: Proof;
  let root: Buffer;

  let processor: PublicProcessor;

  beforeEach(() => {
    db = mock<MerkleTreeOperations>();
    publicCircuit = mock<PublicCircuitSimulator>();
    publicKernel = mock<PublicKernelCircuitSimulator>();
    publicProver = mock<PublicProver>();
    contractDataSource = mock<ContractDataSource>();

    contractData = ContractPublicData.random();
    publicFunction = EncodedContractFunction.random();
    proof = makeEmptyProof();
    root = Buffer.alloc(32, 5);

    publicProver.getPublicCircuitProof.mockResolvedValue(proof);
    publicProver.getPublicKernelCircuitProof.mockResolvedValue(proof);
    db.getTreeInfo.mockResolvedValue({ root } as TreeInfo);
    contractDataSource.getL2ContractPublicData.mockResolvedValue(contractData);
    contractDataSource.getPublicFunction.mockResolvedValue(publicFunction);

    processor = new PublicProcessor(db, publicCircuit, publicKernel, publicProver, contractDataSource);
  });

  it('skips non-public txs', async function () {
    const tx = makePrivateTx();
    const hash = await tx.getTxHash();
    const [processed, failed] = await processor.process([tx]);

    expect(processed).toEqual([{ hash, ...pick(tx, 'data', 'proof', 'unverifiedData') }]);
    expect(failed).toEqual([]);
  });

  it('returns failed txs without aborting entire operation', async function () {
    publicCircuit.publicCircuit.mockRejectedValue(new Error(`Failed`));

    const tx = makePublicTx();
    const [processed, failed] = await processor.process([tx]);

    expect(processed).toEqual([]);
    expect(failed).toEqual([tx]);
  });

  it('runs a public tx through mock circuits', async function () {
    const publicCircuitOutput = makePublicCircuitPublicInputs();
    publicCircuit.publicCircuit.mockResolvedValue(publicCircuitOutput);

    const path = times(PUBLIC_DATA_TREE_HEIGHT, i => Buffer.alloc(32, i));
    db.getSiblingPath.mockResolvedValue(new SiblingPath(path));

    const output = makeKernelPublicInputs();
    publicKernel.publicKernelCircuitNoInput.mockResolvedValue(output);

    const tx = makePublicTx();
    const hash = await tx.getTxHash();
    const [processed, failed] = await processor.process([tx]);

    expect(processed).toEqual([{ hash, data: output, proof, ...pick(tx, 'txRequest') }]);
    expect(failed).toEqual([]);

    expect(publicCircuit.publicCircuit).toHaveBeenCalled();
    expect(publicKernel.publicKernelCircuitNoInput).toHaveBeenCalled();
  });

  it('runs a public tx through the actual public kernel circuit', async function () {
    const publicKernel = new WasmPublicKernelCircuitSimulator();
    const publicKernelSpy = jest.spyOn(publicKernel, 'publicKernelCircuitNoInput');
    processor = new PublicProcessor(db, publicCircuit, publicKernel, publicProver, contractDataSource);

    const publicCircuitOutput = makePublicCircuitPublicInputs();
    publicCircuit.publicCircuit.mockResolvedValue(publicCircuitOutput);

    const path = times(PUBLIC_DATA_TREE_HEIGHT, i => Buffer.alloc(32, i));
    db.getSiblingPath.mockResolvedValue(new SiblingPath(path));

    const tx = makePublicTx();
    const hash = await tx.getTxHash();
    const [processed, failed] = await processor.process([tx]);

    expect(processed[0].data.isPrivate).toBeFalsy();
    expect(processed).toEqual([expect.objectContaining({ hash, proof, ...pick(tx, 'txRequest') })]);
    expect(failed).toEqual([]);

    expect(publicCircuit.publicCircuit).toHaveBeenCalled();
    expect(publicKernelSpy).toHaveBeenCalled();
  });
});
