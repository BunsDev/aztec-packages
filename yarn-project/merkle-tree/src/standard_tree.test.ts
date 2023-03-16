import { default as levelup } from 'levelup';
import { default as memdown } from 'memdown';
import { Hasher, Pedersen, SiblingPath, StandardMerkleTree } from './index.js';
import { merkleTreeTestSuite } from './test_suite.js';

const createDb = async (levelUp: levelup.LevelUp, hasher: Hasher, name: string, depth: number) => {
  return await StandardMerkleTree.new(levelUp, hasher, name, depth);
};

const createFromName = async (levelUp: levelup.LevelUp, hasher: Hasher, name: string) => {
  return await StandardMerkleTree.fromName(levelUp, hasher, name);
};

merkleTreeTestSuite('StandardMerkleTree', createDb, createFromName);

describe('StandardMerkleTreeSpecific', () => {
  const pedersen = new Pedersen();
  const values: Buffer[] = [];

  beforeAll(() => {
    for (let i = 0; i < 4; ++i) {
      const v = Buffer.alloc(32, i + 1);
      v.writeUInt32BE(i, 28);
      values[i] = v;
    }
  });
  it('should have correct root and sibling paths', async () => {
    const db = levelup(memdown());
    const tree = await createDb(db, pedersen, 'test', 2);

    const zeroTreeLeafHash = StandardMerkleTree.ZERO_ELEMENT;
    const level1ZeroHash = pedersen.compress(zeroTreeLeafHash, zeroTreeLeafHash);
    expect(tree.getNumLeaves()).toEqual(0n);
    expect(tree.getRoot()).toEqual(pedersen.compress(level1ZeroHash, level1ZeroHash));
    expect(await tree.getSiblingPath(0n)).toEqual(new SiblingPath([zeroTreeLeafHash, level1ZeroHash]));

    await tree.appendLeaves([values[0]]);
    expect(tree.getNumLeaves()).toEqual(1n);
    expect(tree.getRoot()).toEqual(pedersen.compress(pedersen.compress(values[0], zeroTreeLeafHash), level1ZeroHash));
    expect(await tree.getSiblingPath(0n)).toEqual(new SiblingPath([zeroTreeLeafHash, level1ZeroHash]));

    await tree.appendLeaves([values[1]]);
    expect(tree.getNumLeaves()).toEqual(2n);
    expect(tree.getRoot()).toEqual(pedersen.compress(pedersen.compress(values[0], values[1]), level1ZeroHash));
    expect(await tree.getSiblingPath(1n)).toEqual(new SiblingPath([values[0], level1ZeroHash]));

    await tree.appendLeaves([values[2]]);
    expect(tree.getNumLeaves()).toEqual(3n);
    expect(tree.getRoot()).toEqual(
      pedersen.compress(pedersen.compress(values[0], values[1]), pedersen.compress(values[2], zeroTreeLeafHash)),
    );
    expect(await tree.getSiblingPath(2n)).toEqual(
      new SiblingPath([zeroTreeLeafHash, pedersen.compress(values[0], values[1])]),
    );

    await tree.appendLeaves([values[3]]);
    expect(tree.getNumLeaves()).toEqual(4n);
    expect(tree.getRoot()).toEqual(
      pedersen.compress(pedersen.compress(values[0], values[1]), pedersen.compress(values[2], values[3])),
    );
    expect(await tree.getSiblingPath(3n)).toEqual(
      new SiblingPath([values[2], pedersen.compress(values[0], values[1])]),
    );
    // Lifted from memory_tree.test.cpp to ensure consistency.
    //expect(root.toString('hex')).toEqual('0bf2e78afd70f72b0e6eafb03c41faef167a82441b05e517cdf35d813302061f');
    expect(await tree.getSiblingPath(0n)).toEqual(
      new SiblingPath([values[1], pedersen.compress(values[2], values[3])]),
    );
    expect(await tree.getSiblingPath(1n)).toEqual(
      new SiblingPath([values[0], pedersen.compress(values[2], values[3])]),
    );
    expect(await tree.getSiblingPath(2n)).toEqual(
      new SiblingPath([values[3], pedersen.compress(values[0], values[1])]),
    );
    expect(await tree.getSiblingPath(3n)).toEqual(
      new SiblingPath([values[2], pedersen.compress(values[0], values[1])]),
    );
  });
});
