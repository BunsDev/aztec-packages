import { PrivateKernelPublicInputs, Proof } from '@aztec/circuits.js';
import { makePrivateKernelPublicInputs } from '@aztec/circuits.js/factories';
import { PrivateTx, Tx, UnverifiedData } from '@aztec/types';

function makeEmptyProof() {
  return new Proof(Buffer.alloc(0));
}

export function makeEmptyUnverifiedData(): UnverifiedData {
  const chunks = [Buffer.alloc(0)];
  return new UnverifiedData(chunks);
}

export function makeEmptyPrivateTx(): PrivateTx {
  return Tx.createPrivate(PrivateKernelPublicInputs.makeEmpty(), makeEmptyProof(), makeEmptyUnverifiedData());
}

export function makePrivateTx(seed = 0): PrivateTx {
  return Tx.createPrivate(makePrivateKernelPublicInputs(seed), makeEmptyProof(), UnverifiedData.random(2));
}
