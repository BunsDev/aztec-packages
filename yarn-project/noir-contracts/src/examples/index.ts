// TODO the verification keys in this contracts are mocked ATM
import TestContract from './test_contract.json';
import ZkTokenContract from './zk_token_contract.json';
import Parent from './parent.json';
import Child from './child.json';

import { ContractAbi } from '../abi.js';

export const TestContractAbi = TestContract as ContractAbi;
export const ZkTokenContractAbi = ZkTokenContract as ContractAbi;
export const ParentAbi = Parent as ContractAbi;
export const ChildAbi = Child as ContractAbi;
