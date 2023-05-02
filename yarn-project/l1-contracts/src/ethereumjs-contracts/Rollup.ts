// THIS IS GENERATED CODE, DO NOT EDIT!
/* eslint-disable */
import { EthAddress } from '@aztec/foundation';
import { EthereumRpc } from '@aztec/ethereum.js/eth_rpc';
import { Contract, ContractTxReceipt, EventLog, Options, TxCall, TxSend } from '@aztec/ethereum.js/contract';
import * as Bytes from '@aztec/ethereum.js/contract/bytes.js';
import abi from './RollupAbi.js';
export type L2BlockProcessedEvent = {
  blockNum: bigint;
};
export interface L2BlockProcessedEventLog extends EventLog<L2BlockProcessedEvent, 'L2BlockProcessed'> {}
interface RollupEvents {
  L2BlockProcessed: L2BlockProcessedEvent;
}
interface RollupEventLogs {
  L2BlockProcessed: L2BlockProcessedEventLog;
}
interface RollupTxEventLogs {
  L2BlockProcessed: L2BlockProcessedEventLog[];
}
export interface RollupTransactionReceipt extends ContractTxReceipt<RollupTxEventLogs> {}
interface RollupMethods {
  VERIFIER(): TxCall<EthAddress>;
  process(_proof: Bytes.Bytes, _l2Block: Bytes.Bytes): TxSend<RollupTransactionReceipt>;
  rollupStateHash(): TxCall<Bytes.Bytes32>;
}
export interface RollupDefinition {
  methods: RollupMethods;
  events: RollupEvents;
  eventLogs: RollupEventLogs;
}
export class Rollup extends Contract<RollupDefinition> {
  constructor(eth: EthereumRpc, address?: EthAddress, options?: Options) {
    super(eth, abi, address, options);
  }
  deploy(): TxSend<RollupTransactionReceipt> {
    return super.deployBytecode(
      '0x60a060405234801561001057600080fd5b5060405161001d9061004b565b604051809103906000f080158015610039573d6000803e3d6000fd5b506001600160a01b0316608052610058565b61019d80610d5483390190565b608051610cdb61007960003960008181604b015261016b0152610cdb6000f3fe608060405234801561001057600080fd5b50600436106100415760003560e01c806308c84e70146100465780631ab9c6031461008a5780637c39d130146100a1575b600080fd5b61006d7f000000000000000000000000000000000000000000000000000000000000000081565b6040516001600160a01b0390911681526020015b60405180910390f35b61009360005481565b604051908152602001610081565b6100b46100af36600461091e565b6100b6565b005b6000806000806100c68686610237565b60005493975091955093509150158015906100e357508260005414155b1561011357600054604051632d2ef59f60e11b815260048101919091526024810184905260440160405180910390fd5b604080516001808252818301909252600091602080830190803683370190505090508181600081518110610149576101496109f9565b6020908102919091010152604051633a94343960e21b81526001600160a01b037f0000000000000000000000000000000000000000000000000000000000000000169063ea50d0e4906101a2908b908590600401610a33565b602060405180830381865afa1580156101bf573d6000803e3d6000fd5b505050506040513d601f19601f820116820180604052508101906101e39190610aad565b610200576040516309bde33960e01b815260040160405180910390fd5b600083815560405186917f655779015b9b95c7fd18f01ea4619ab4c31289bbe134ba85c5b20bcdeb1dabf391a25050505050505050565b813560e01c6000808061025761024e600186610aec565b6004888861027d565b92506102668460d8888861027d565b91506102728686610328565b905092959194509250565b6040805160d880825261010082019092526000918291906020820181803683370190505090508560181c60208201538560101c60218201538560081c602282015385602382015360d485850160248301376002816040516102de9190610aff565b602060405180830381855afa1580156102fb573d6000803e3d6000fd5b5050506040513d601f19601f8201168201806040525081019061031e9190610b1b565b9695505050505050565b604080516101c880825261020082019092526000919082908260208201818036833701905050905081600486016020830137600061036686866103fc565b90508060046101ac036020018301527f30644e72e131a029b85045b68181585d2833e84879b9709143e1f593f00000016002836040516103a69190610aff565b602060405180830381855afa1580156103c3573d6000803e3d6000fd5b5050506040513d601f19601f820116820180604052508101906103e69190610b1b565b6103f09190610b4a565b93505050505b92915050565b60006104296040518060800160405280600081526020016000815260200160008152602001600081525090565b6101ac84013560e090811c808352602090810286016101b0810135831c848301819052918202016101b4810135831c604085810191909152909102016101b80135901c6060820152600061047f60046002610b5e565b825161048b9190610b75565b67ffffffffffffffff8111156104a3576104a36108bf565b6040519080825280602002602001820160405280156104cc578160200160208202803683370190505b5082519091506101b0906000906104e4906020610b5e565b6104ef836004610b89565b6104f99190610b89565b905060008460200151602061050e9190610b5e565b610519836004610b89565b6105239190610b89565b90506000856040015160406105389190610b5e565b610543836004610b89565b61054d9190610b89565b90506000866060015160206105629190610b5e565b61056c9083610b89565b905060005b865181101561072f57604080516104c08082526104e0820190925260009160208201818036833701905050905060206101008d890182840137610100818101918e890190840182013761010081810191610200918f89019185010137610200818101916040918f880191850101376020600202810190506020848e0182840137602c808201916014918f870160200191850101376020810190506020603485018e0182840137602c808201916014918f870160540191850101375061063860046002610b5e565b610643906020610b5e565b61064d9088610b89565b965061065b60046002610b5e565b610666906020610b5e565b6106709087610b89565b955061067e60046002610b5e565b610689906040610b5e565b6106939086610b89565b94506106a0604085610b89565b93506106ad606884610b89565b92506002816040516106bf9190610aff565b602060405180830381855afa1580156106dc573d6000803e3d6000fd5b5050506040513d601f19601f820116820180604052508101906106ff9190610b1b565b888381518110610711576107116109f9565b6020908102919091010152508061072781610b9c565b915050610571565b5061073986610747565b9a9950505050505050505050565b6000805b8251610758826002610c99565b1015610770578061076881610b9c565b91505061074b565b600061077d826002610c99565b905080845260005b8281101561089a5760005b828110156108875760028682815181106107ac576107ac6109f9565b6020026020010151878360016107c29190610b89565b815181106107d2576107d26109f9565b60200260200101516040516020016107f4929190918252602082015260400190565b60408051601f198184030181529082905261080e91610aff565b602060405180830381855afa15801561082b573d6000803e3d6000fd5b5050506040513d601f19601f8201168201806040525081019061084e9190610b1b565b8661085a600284610b75565b8151811061086a5761086a6109f9565b6020908102919091010152610880600282610b89565b9050610790565b508061089281610b9c565b915050610785565b50836000815181106108ae576108ae6109f9565b602002602001015192505050919050565b634e487b7160e01b600052604160045260246000fd5b60008083601f8401126108e757600080fd5b50813567ffffffffffffffff8111156108ff57600080fd5b60208301915083602082850101111561091757600080fd5b9250929050565b60008060006040848603121561093357600080fd5b833567ffffffffffffffff8082111561094b57600080fd5b818601915086601f83011261095f57600080fd5b813581811115610971576109716108bf565b604051601f8201601f19908116603f01168101908382118183101715610999576109996108bf565b816040528281528960208487010111156109b257600080fd5b8260208601602083013760006020848301015280975050505060208601359150808211156109df57600080fd5b506109ec868287016108d5565b9497909650939450505050565b634e487b7160e01b600052603260045260246000fd5b60005b83811015610a2a578181015183820152602001610a12565b50506000910152565b60408152600083518060408401526020610a538260608601838901610a0f565b6060601f19601f93909301929092168401848103830185830152855192810183905285820192600091608001905b80831015610aa15784518252938301936001929092019190830190610a81565b50979650505050505050565b600060208284031215610abf57600080fd5b81518015158114610acf57600080fd5b9392505050565b634e487b7160e01b600052601160045260246000fd5b818103818111156103f6576103f6610ad6565b60008251610b11818460208701610a0f565b9190910192915050565b600060208284031215610b2d57600080fd5b5051919050565b634e487b7160e01b600052601260045260246000fd5b600082610b5957610b59610b34565b500690565b80820281158282048414176103f6576103f6610ad6565b600082610b8457610b84610b34565b500490565b808201808211156103f6576103f6610ad6565b600060018201610bae57610bae610ad6565b5060010190565b600181815b80851115610bf0578160001904821115610bd657610bd6610ad6565b80851615610be357918102915b93841c9390800290610bba565b509250929050565b600082610c07575060016103f6565b81610c14575060006103f6565b8160018114610c2a5760028114610c3457610c50565b60019150506103f6565b60ff841115610c4557610c45610ad6565b50506001821b6103f6565b5060208310610133831016604e8410600b8410161715610c73575081810a6103f6565b610c7d8383610bb5565b8060001904821115610c9157610c91610ad6565b029392505050565b6000610acf8383610bf856fea2646970667358221220ddf248ba6894d5466329c62aa3e8d071d039ec5f5594b21af88ea780d45fefb364736f6c63430008120033608060405234801561001057600080fd5b5061017d806100206000396000f3fe608060405234801561001057600080fd5b50600436106100365760003560e01c8063937f6a101461003b578063ea50d0e41461005a575b600080fd5b60405168496d2061206d6f636b60b81b81526020015b60405180910390f35b610072610068366004610082565b6001949350505050565b6040519015158152602001610051565b6000806000806040858703121561009857600080fd5b843567ffffffffffffffff808211156100b057600080fd5b818701915087601f8301126100c457600080fd5b8135818111156100d357600080fd5b8860208285010111156100e557600080fd5b60209283019650945090860135908082111561010057600080fd5b818701915087601f83011261011457600080fd5b81358181111561012357600080fd5b8860208260051b850101111561013857600080fd5b9598949750506020019450505056fea264697066735822122079065ece8684a52b261c9ab9f34e06d3a4e53f346b36185587fd3fe29e5c9cc764736f6c63430008120033',
    ) as any;
  }
}
export var RollupAbi = abi;
