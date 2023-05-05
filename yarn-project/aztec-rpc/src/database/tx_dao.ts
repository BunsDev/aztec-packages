import { AztecAddress } from '@aztec/foundation/aztec-address';
import { TxHash } from '@aztec/types';

export class TxDao {
  constructor(
    public readonly txHash: TxHash,
    public blockHash: Buffer | undefined,
    public blockNumber: number | undefined,
    public readonly from: AztecAddress,
    public readonly to: AztecAddress | undefined,
    public readonly contractAddress: AztecAddress | undefined,
    public readonly error: string,
    public readonly contractBytecoe?: Buffer,
  ) {}
}
