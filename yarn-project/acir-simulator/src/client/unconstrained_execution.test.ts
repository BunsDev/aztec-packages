import { Grumpkin } from '@aztec/barretenberg.js/crypto';
import { BarretenbergWasm } from '@aztec/barretenberg.js/wasm';
import {
  ContractDeploymentData,
  FunctionData,
  MembershipWitness,
  PrivateHistoricTreeRoots,
  PRIVATE_DATA_TREE_HEIGHT,
  TxContext,
  TxRequest,
} from '@aztec/circuits.js';

import { ZkTokenContractAbi } from '@aztec/noir-contracts/examples';
import { mock } from 'jest-mock-extended';
import { encodeArguments } from '../abi_coder/index.js';
import { DBOracle } from './db_oracle.js';
import { AcirSimulator } from './simulator.js';
import { NoirPoint, toPublicKey } from '../utils.js';
import { Fr } from '@aztec/foundation/fields';
import { EthAddress } from '@aztec/foundation/eth-address';
import { AztecAddress } from '@aztec/foundation/aztec-address';

describe('Unconstrained Execution test suite', () => {
  let bbWasm: BarretenbergWasm;
  let oracle: ReturnType<typeof mock<DBOracle>>;
  let acirSimulator: AcirSimulator;

  beforeAll(async () => {
    bbWasm = await BarretenbergWasm.get();
  });

  beforeEach(() => {
    oracle = mock<DBOracle>();
    acirSimulator = new AcirSimulator(oracle);
  });

  describe('zk token contract', () => {
    let currentNonce = 0n;

    const contractDeploymentData = ContractDeploymentData.empty();
    const txContext = new TxContext(false, false, false, contractDeploymentData);

    let ownerPk: Buffer;
    let owner: NoirPoint;

    const buildNote = (amount: bigint, owner: NoirPoint) => {
      return [new Fr(1n), new Fr(currentNonce++), new Fr(owner.x), new Fr(owner.y), Fr.random(), new Fr(amount)];
    };

    beforeAll(() => {
      ownerPk = Buffer.from('5e30a2f886b4b6a11aea03bf4910fbd5b24e61aa27ea4d05c393b3ab592a8d33', 'hex');

      const grumpkin = new Grumpkin(bbWasm);
      owner = toPublicKey(ownerPk, grumpkin);
    });

    it('should run the getBalance function', async () => {
      const contractAddress = AztecAddress.random();
      const abi = ZkTokenContractAbi.functions.find(f => f.name === 'getBalance')!;

      const preimages = [...Array(5).fill(buildNote(1n, owner)), ...Array(2).fill(buildNote(2n, owner))];
      // TODO for this we need that noir siloes the commitment the same way as the kernel does, to do merkle membership

      const historicRoots = PrivateHistoricTreeRoots.empty();

      oracle.getNotes.mockImplementation((_, __, limit: number, offset: number) => {
        const notes = preimages.slice(offset, offset + limit);
        return Promise.resolve({
          count: preimages.length,
          notes: notes.map((preimage, index) => ({
            preimage,
            membershipWitness: MembershipWitness.empty(
              PRIVATE_DATA_TREE_HEIGHT, // pathSize
              BigInt(index), // leafIndex
            ),
          })),
        });
      });

      const txRequest = new TxRequest(
        AztecAddress.random(),
        contractAddress,
        new FunctionData(Buffer.alloc(4), true, true),
        encodeArguments(abi, [owner]),
        Fr.random(),
        txContext,
        Fr.ZERO,
      );

      const result = await acirSimulator.runUnconstrained(
        txRequest,
        abi,
        AztecAddress.random(),
        EthAddress.ZERO,
        historicRoots,
      );

      expect(result).toEqual([9n]);
    }, 30_000);
  });
});
