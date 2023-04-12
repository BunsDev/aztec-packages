import { PrivateKernelPublicInputs, UInt8Vector } from '@aztec/circuits.js';
import { Tx } from '@aztec/types';
import { UnverifiedData } from '@aztec/types';

function makeEmptyProof() {
  return new UInt8Vector(Buffer.alloc(0));
}

export function makeEmptyUnverifiedData(): UnverifiedData {
  const chunks = [Buffer.alloc(0)];
  return new UnverifiedData(chunks);
}

export function makeEmptyTx(): Tx {
  const isEmpty = true;
  return new Tx(PrivateKernelPublicInputs.makeEmpty(), makeEmptyProof(), makeEmptyUnverifiedData(), isEmpty);
}
