/* eslint-disable */
// GENERATED FILE DO NOT EDIT
import { Buffer } from 'buffer';
import { callCbind } from './cbind.js';
import { CircuitsWasm } from '../wasm/index.js';
type MsgpackAddress = Buffer;
type MsgpackFr = Buffer;
type MsgpackFq = Buffer;
type MsgpackProof = Buffer;
import {
  Address,
  Fr,
  Fq,
  AffineElement,
  NativeAggregationState,
  NewContractData,
  FunctionData,
  OptionallyRevealedData,
  AccumulatedData,
  HistoricTreeRoots,
  ContractDeploymentData,
  TxContext,
  ConstantData,
  PublicInputs,
  Proof,
  VerificationKeyData,
  PreviousKernelData,
} from './types.js';
import { TupleOf, mapTuple, mapValues } from '../structs/types.js';
export interface MsgpackAffineElement {
  x: Buffer;
  y: Buffer;
}
export interface IAffineElement {
  x: Fq;
  y: Fq;
}

export function toAffineElement(o: MsgpackAffineElement): AffineElement {
  if (o.x === undefined) {
    throw new Error('Expected x in AffineElement deserialization');
  }
  if (o.y === undefined) {
    throw new Error('Expected y in AffineElement deserialization');
  }
  return new AffineElement(Fq.fromBuffer(o.x), Fq.fromBuffer(o.y));
}

export function fromAffineElement(o: AffineElement): MsgpackAffineElement {
  if (o.x === undefined) {
    throw new Error('Expected x in AffineElement serialization');
  }
  if (o.y === undefined) {
    throw new Error('Expected y in AffineElement serialization');
  }
  return {
    x: o.x.toBuffer(),
    y: o.y.toBuffer(),
  };
}

export interface MsgpackNativeAggregationState {
  P0: MsgpackAffineElement;
  P1: MsgpackAffineElement;
  public_inputs: Buffer[];
  proof_witness_indices: number[];
  has_data: boolean;
}
export interface INativeAggregationState {
  p0: AffineElement;
  p1: AffineElement;
  publicInputs: Fr[];
  proofWitnessIndices: number[];
  hasData: boolean;
}

export function toNativeAggregationState(o: MsgpackNativeAggregationState): NativeAggregationState {
  if (o.P0 === undefined) {
    throw new Error('Expected P0 in NativeAggregationState deserialization');
  }
  if (o.P1 === undefined) {
    throw new Error('Expected P1 in NativeAggregationState deserialization');
  }
  if (o.public_inputs === undefined) {
    throw new Error('Expected public_inputs in NativeAggregationState deserialization');
  }
  if (o.proof_witness_indices === undefined) {
    throw new Error('Expected proof_witness_indices in NativeAggregationState deserialization');
  }
  if (o.has_data === undefined) {
    throw new Error('Expected has_data in NativeAggregationState deserialization');
  }
  return new NativeAggregationState(
    toAffineElement(o.P0),
    toAffineElement(o.P1),
    o.public_inputs.map((v: Buffer) => Fr.fromBuffer(v)),
    o.proof_witness_indices.map((v: number) => v),
    o.has_data,
  );
}

export function fromNativeAggregationState(o: NativeAggregationState): MsgpackNativeAggregationState {
  if (o.p0 === undefined) {
    throw new Error('Expected p0 in NativeAggregationState serialization');
  }
  if (o.p1 === undefined) {
    throw new Error('Expected p1 in NativeAggregationState serialization');
  }
  if (o.publicInputs === undefined) {
    throw new Error('Expected publicInputs in NativeAggregationState serialization');
  }
  if (o.proofWitnessIndices === undefined) {
    throw new Error('Expected proofWitnessIndices in NativeAggregationState serialization');
  }
  if (o.hasData === undefined) {
    throw new Error('Expected hasData in NativeAggregationState serialization');
  }
  return {
    P0: fromAffineElement(o.p0),
    P1: fromAffineElement(o.p1),
    public_inputs: o.publicInputs.map((v: Fr) => v.toBuffer()),
    proof_witness_indices: o.proofWitnessIndices.map((v: number) => v),
    has_data: o.hasData,
  };
}

export interface MsgpackNewContractData {
  contract_address: Buffer;
  portal_contract_address: Buffer;
  function_tree_root: Buffer;
}
export interface INewContractData {
  contractAddress: Address;
  portalContractAddress: Address;
  functionTreeRoot: Fr;
}

export function toNewContractData(o: MsgpackNewContractData): NewContractData {
  if (o.contract_address === undefined) {
    throw new Error('Expected contract_address in NewContractData deserialization');
  }
  if (o.portal_contract_address === undefined) {
    throw new Error('Expected portal_contract_address in NewContractData deserialization');
  }
  if (o.function_tree_root === undefined) {
    throw new Error('Expected function_tree_root in NewContractData deserialization');
  }
  return new NewContractData(
    Address.fromBuffer(o.contract_address),
    Address.fromBuffer(o.portal_contract_address),
    Fr.fromBuffer(o.function_tree_root),
  );
}

export function fromNewContractData(o: NewContractData): MsgpackNewContractData {
  if (o.contractAddress === undefined) {
    throw new Error('Expected contractAddress in NewContractData serialization');
  }
  if (o.portalContractAddress === undefined) {
    throw new Error('Expected portalContractAddress in NewContractData serialization');
  }
  if (o.functionTreeRoot === undefined) {
    throw new Error('Expected functionTreeRoot in NewContractData serialization');
  }
  return {
    contract_address: o.contractAddress.toBuffer(),
    portal_contract_address: o.portalContractAddress.toBuffer(),
    function_tree_root: o.functionTreeRoot.toBuffer(),
  };
}

export interface MsgpackFunctionData {
  function_selector: number;
  is_private: boolean;
  is_constructor: boolean;
}
export interface IFunctionData {
  functionSelector: number;
  isPrivate: boolean;
  isConstructor: boolean;
}

export function toFunctionData(o: MsgpackFunctionData): FunctionData {
  if (o.function_selector === undefined) {
    throw new Error('Expected function_selector in FunctionData deserialization');
  }
  if (o.is_private === undefined) {
    throw new Error('Expected is_private in FunctionData deserialization');
  }
  if (o.is_constructor === undefined) {
    throw new Error('Expected is_constructor in FunctionData deserialization');
  }
  return new FunctionData(o.function_selector, o.is_private, o.is_constructor);
}

export function fromFunctionData(o: FunctionData): MsgpackFunctionData {
  if (o.functionSelector === undefined) {
    throw new Error('Expected functionSelector in FunctionData serialization');
  }
  if (o.isPrivate === undefined) {
    throw new Error('Expected isPrivate in FunctionData serialization');
  }
  if (o.isConstructor === undefined) {
    throw new Error('Expected isConstructor in FunctionData serialization');
  }
  return {
    function_selector: o.functionSelector,
    is_private: o.isPrivate,
    is_constructor: o.isConstructor,
  };
}

export interface MsgpackOptionallyRevealedData {
  call_stack_item_hash: Buffer;
  function_data: MsgpackFunctionData;
  emitted_events: TupleOf<Buffer, 4>;
  vk_hash: Buffer;
  portal_contract_address: Buffer;
  pay_fee_from_l1: boolean;
  pay_fee_from_public_l2: boolean;
  called_from_l1: boolean;
  called_from_public_l2: boolean;
}
export interface IOptionallyRevealedData {
  callStackItemHash: Fr;
  functionData: FunctionData;
  emittedEvents: TupleOf<Fr, 4>;
  vkHash: Fr;
  portalContractAddress: Address;
  payFeeFromL1: boolean;
  payFeeFromPublicL2: boolean;
  calledFromL1: boolean;
  calledFromPublicL2: boolean;
}

export function toOptionallyRevealedData(o: MsgpackOptionallyRevealedData): OptionallyRevealedData {
  if (o.call_stack_item_hash === undefined) {
    throw new Error('Expected call_stack_item_hash in OptionallyRevealedData deserialization');
  }
  if (o.function_data === undefined) {
    throw new Error('Expected function_data in OptionallyRevealedData deserialization');
  }
  if (o.emitted_events === undefined) {
    throw new Error('Expected emitted_events in OptionallyRevealedData deserialization');
  }
  if (o.vk_hash === undefined) {
    throw new Error('Expected vk_hash in OptionallyRevealedData deserialization');
  }
  if (o.portal_contract_address === undefined) {
    throw new Error('Expected portal_contract_address in OptionallyRevealedData deserialization');
  }
  if (o.pay_fee_from_l1 === undefined) {
    throw new Error('Expected pay_fee_from_l1 in OptionallyRevealedData deserialization');
  }
  if (o.pay_fee_from_public_l2 === undefined) {
    throw new Error('Expected pay_fee_from_public_l2 in OptionallyRevealedData deserialization');
  }
  if (o.called_from_l1 === undefined) {
    throw new Error('Expected called_from_l1 in OptionallyRevealedData deserialization');
  }
  if (o.called_from_public_l2 === undefined) {
    throw new Error('Expected called_from_public_l2 in OptionallyRevealedData deserialization');
  }
  return new OptionallyRevealedData(
    Fr.fromBuffer(o.call_stack_item_hash),
    toFunctionData(o.function_data),
    mapTuple(o.emitted_events, (v: Buffer) => Fr.fromBuffer(v)),
    Fr.fromBuffer(o.vk_hash),
    Address.fromBuffer(o.portal_contract_address),
    o.pay_fee_from_l1,
    o.pay_fee_from_public_l2,
    o.called_from_l1,
    o.called_from_public_l2,
  );
}

export function fromOptionallyRevealedData(o: OptionallyRevealedData): MsgpackOptionallyRevealedData {
  if (o.callStackItemHash === undefined) {
    throw new Error('Expected callStackItemHash in OptionallyRevealedData serialization');
  }
  if (o.functionData === undefined) {
    throw new Error('Expected functionData in OptionallyRevealedData serialization');
  }
  if (o.emittedEvents === undefined) {
    throw new Error('Expected emittedEvents in OptionallyRevealedData serialization');
  }
  if (o.vkHash === undefined) {
    throw new Error('Expected vkHash in OptionallyRevealedData serialization');
  }
  if (o.portalContractAddress === undefined) {
    throw new Error('Expected portalContractAddress in OptionallyRevealedData serialization');
  }
  if (o.payFeeFromL1 === undefined) {
    throw new Error('Expected payFeeFromL1 in OptionallyRevealedData serialization');
  }
  if (o.payFeeFromPublicL2 === undefined) {
    throw new Error('Expected payFeeFromPublicL2 in OptionallyRevealedData serialization');
  }
  if (o.calledFromL1 === undefined) {
    throw new Error('Expected calledFromL1 in OptionallyRevealedData serialization');
  }
  if (o.calledFromPublicL2 === undefined) {
    throw new Error('Expected calledFromPublicL2 in OptionallyRevealedData serialization');
  }
  return {
    call_stack_item_hash: o.callStackItemHash.toBuffer(),
    function_data: fromFunctionData(o.functionData),
    emitted_events: mapTuple(o.emittedEvents, (v: Fr) => v.toBuffer()),
    vk_hash: o.vkHash.toBuffer(),
    portal_contract_address: o.portalContractAddress.toBuffer(),
    pay_fee_from_l1: o.payFeeFromL1,
    pay_fee_from_public_l2: o.payFeeFromPublicL2,
    called_from_l1: o.calledFromL1,
    called_from_public_l2: o.calledFromPublicL2,
  };
}

export interface MsgpackAccumulatedData {
  aggregation_object: MsgpackNativeAggregationState;
  private_call_count: Buffer;
  new_commitments: TupleOf<Buffer, 4>;
  new_nullifiers: TupleOf<Buffer, 4>;
  private_call_stack: TupleOf<Buffer, 8>;
  public_call_stack: TupleOf<Buffer, 8>;
  l1_msg_stack: TupleOf<Buffer, 4>;
  new_contracts: TupleOf<MsgpackNewContractData, 1>;
  optionally_revealed_data: TupleOf<MsgpackOptionallyRevealedData, 4>;
}
export interface IAccumulatedData {
  aggregationObject: NativeAggregationState;
  privateCallCount: Fr;
  newCommitments: TupleOf<Fr, 4>;
  newNullifiers: TupleOf<Fr, 4>;
  privateCallStack: TupleOf<Fr, 8>;
  publicCallStack: TupleOf<Fr, 8>;
  l1MsgStack: TupleOf<Fr, 4>;
  newContracts: TupleOf<NewContractData, 1>;
  optionallyRevealedData: TupleOf<OptionallyRevealedData, 4>;
}

export function toAccumulatedData(o: MsgpackAccumulatedData): AccumulatedData {
  if (o.aggregation_object === undefined) {
    throw new Error('Expected aggregation_object in AccumulatedData deserialization');
  }
  if (o.private_call_count === undefined) {
    throw new Error('Expected private_call_count in AccumulatedData deserialization');
  }
  if (o.new_commitments === undefined) {
    throw new Error('Expected new_commitments in AccumulatedData deserialization');
  }
  if (o.new_nullifiers === undefined) {
    throw new Error('Expected new_nullifiers in AccumulatedData deserialization');
  }
  if (o.private_call_stack === undefined) {
    throw new Error('Expected private_call_stack in AccumulatedData deserialization');
  }
  if (o.public_call_stack === undefined) {
    throw new Error('Expected public_call_stack in AccumulatedData deserialization');
  }
  if (o.l1_msg_stack === undefined) {
    throw new Error('Expected l1_msg_stack in AccumulatedData deserialization');
  }
  if (o.new_contracts === undefined) {
    throw new Error('Expected new_contracts in AccumulatedData deserialization');
  }
  if (o.optionally_revealed_data === undefined) {
    throw new Error('Expected optionally_revealed_data in AccumulatedData deserialization');
  }
  return new AccumulatedData(
    toNativeAggregationState(o.aggregation_object),
    Fr.fromBuffer(o.private_call_count),
    mapTuple(o.new_commitments, (v: Buffer) => Fr.fromBuffer(v)),
    mapTuple(o.new_nullifiers, (v: Buffer) => Fr.fromBuffer(v)),
    mapTuple(o.private_call_stack, (v: Buffer) => Fr.fromBuffer(v)),
    mapTuple(o.public_call_stack, (v: Buffer) => Fr.fromBuffer(v)),
    mapTuple(o.l1_msg_stack, (v: Buffer) => Fr.fromBuffer(v)),
    mapTuple(o.new_contracts, (v: MsgpackNewContractData) => toNewContractData(v)),
    mapTuple(o.optionally_revealed_data, (v: MsgpackOptionallyRevealedData) => toOptionallyRevealedData(v)),
  );
}

export function fromAccumulatedData(o: AccumulatedData): MsgpackAccumulatedData {
  if (o.aggregationObject === undefined) {
    throw new Error('Expected aggregationObject in AccumulatedData serialization');
  }
  if (o.privateCallCount === undefined) {
    throw new Error('Expected privateCallCount in AccumulatedData serialization');
  }
  if (o.newCommitments === undefined) {
    throw new Error('Expected newCommitments in AccumulatedData serialization');
  }
  if (o.newNullifiers === undefined) {
    throw new Error('Expected newNullifiers in AccumulatedData serialization');
  }
  if (o.privateCallStack === undefined) {
    throw new Error('Expected privateCallStack in AccumulatedData serialization');
  }
  if (o.publicCallStack === undefined) {
    throw new Error('Expected publicCallStack in AccumulatedData serialization');
  }
  if (o.l1MsgStack === undefined) {
    throw new Error('Expected l1MsgStack in AccumulatedData serialization');
  }
  if (o.newContracts === undefined) {
    throw new Error('Expected newContracts in AccumulatedData serialization');
  }
  if (o.optionallyRevealedData === undefined) {
    throw new Error('Expected optionallyRevealedData in AccumulatedData serialization');
  }
  return {
    aggregation_object: fromNativeAggregationState(o.aggregationObject),
    private_call_count: o.privateCallCount.toBuffer(),
    new_commitments: mapTuple(o.newCommitments, (v: Fr) => v.toBuffer()),
    new_nullifiers: mapTuple(o.newNullifiers, (v: Fr) => v.toBuffer()),
    private_call_stack: mapTuple(o.privateCallStack, (v: Fr) => v.toBuffer()),
    public_call_stack: mapTuple(o.publicCallStack, (v: Fr) => v.toBuffer()),
    l1_msg_stack: mapTuple(o.l1MsgStack, (v: Fr) => v.toBuffer()),
    new_contracts: mapTuple(o.newContracts, (v: NewContractData) => fromNewContractData(v)),
    optionally_revealed_data: mapTuple(o.optionallyRevealedData, (v: OptionallyRevealedData) =>
      fromOptionallyRevealedData(v),
    ),
  };
}

export interface MsgpackHistoricTreeRoots {
  private_data_tree_root: Buffer;
  nullifier_tree_root: Buffer;
  contract_tree_root: Buffer;
  private_kernel_vk_tree_root: Buffer;
}
export interface IHistoricTreeRoots {
  privateDataTreeRoot: Fr;
  nullifierTreeRoot: Fr;
  contractTreeRoot: Fr;
  privateKernelVkTreeRoot: Fr;
}

export function toHistoricTreeRoots(o: MsgpackHistoricTreeRoots): HistoricTreeRoots {
  if (o.private_data_tree_root === undefined) {
    throw new Error('Expected private_data_tree_root in HistoricTreeRoots deserialization');
  }
  if (o.nullifier_tree_root === undefined) {
    throw new Error('Expected nullifier_tree_root in HistoricTreeRoots deserialization');
  }
  if (o.contract_tree_root === undefined) {
    throw new Error('Expected contract_tree_root in HistoricTreeRoots deserialization');
  }
  if (o.private_kernel_vk_tree_root === undefined) {
    throw new Error('Expected private_kernel_vk_tree_root in HistoricTreeRoots deserialization');
  }
  return new HistoricTreeRoots(
    Fr.fromBuffer(o.private_data_tree_root),
    Fr.fromBuffer(o.nullifier_tree_root),
    Fr.fromBuffer(o.contract_tree_root),
    Fr.fromBuffer(o.private_kernel_vk_tree_root),
  );
}

export function fromHistoricTreeRoots(o: HistoricTreeRoots): MsgpackHistoricTreeRoots {
  if (o.privateDataTreeRoot === undefined) {
    throw new Error('Expected privateDataTreeRoot in HistoricTreeRoots serialization');
  }
  if (o.nullifierTreeRoot === undefined) {
    throw new Error('Expected nullifierTreeRoot in HistoricTreeRoots serialization');
  }
  if (o.contractTreeRoot === undefined) {
    throw new Error('Expected contractTreeRoot in HistoricTreeRoots serialization');
  }
  if (o.privateKernelVkTreeRoot === undefined) {
    throw new Error('Expected privateKernelVkTreeRoot in HistoricTreeRoots serialization');
  }
  return {
    private_data_tree_root: o.privateDataTreeRoot.toBuffer(),
    nullifier_tree_root: o.nullifierTreeRoot.toBuffer(),
    contract_tree_root: o.contractTreeRoot.toBuffer(),
    private_kernel_vk_tree_root: o.privateKernelVkTreeRoot.toBuffer(),
  };
}

export interface MsgpackContractDeploymentData {
  constructor_vk_hash: Buffer;
  function_tree_root: Buffer;
  contract_address_salt: Buffer;
  portal_contract_address: Buffer;
}
export interface IContractDeploymentData {
  constructorVkHash: Fr;
  functionTreeRoot: Fr;
  contractAddressSalt: Fr;
  portalContractAddress: Address;
}

export function toContractDeploymentData(o: MsgpackContractDeploymentData): ContractDeploymentData {
  if (o.constructor_vk_hash === undefined) {
    throw new Error('Expected constructor_vk_hash in ContractDeploymentData deserialization');
  }
  if (o.function_tree_root === undefined) {
    throw new Error('Expected function_tree_root in ContractDeploymentData deserialization');
  }
  if (o.contract_address_salt === undefined) {
    throw new Error('Expected contract_address_salt in ContractDeploymentData deserialization');
  }
  if (o.portal_contract_address === undefined) {
    throw new Error('Expected portal_contract_address in ContractDeploymentData deserialization');
  }
  return new ContractDeploymentData(
    Fr.fromBuffer(o.constructor_vk_hash),
    Fr.fromBuffer(o.function_tree_root),
    Fr.fromBuffer(o.contract_address_salt),
    Address.fromBuffer(o.portal_contract_address),
  );
}

export function fromContractDeploymentData(o: ContractDeploymentData): MsgpackContractDeploymentData {
  if (o.constructorVkHash === undefined) {
    throw new Error('Expected constructorVkHash in ContractDeploymentData serialization');
  }
  if (o.functionTreeRoot === undefined) {
    throw new Error('Expected functionTreeRoot in ContractDeploymentData serialization');
  }
  if (o.contractAddressSalt === undefined) {
    throw new Error('Expected contractAddressSalt in ContractDeploymentData serialization');
  }
  if (o.portalContractAddress === undefined) {
    throw new Error('Expected portalContractAddress in ContractDeploymentData serialization');
  }
  return {
    constructor_vk_hash: o.constructorVkHash.toBuffer(),
    function_tree_root: o.functionTreeRoot.toBuffer(),
    contract_address_salt: o.contractAddressSalt.toBuffer(),
    portal_contract_address: o.portalContractAddress.toBuffer(),
  };
}

export interface MsgpackTxContext {
  is_fee_payment_tx: boolean;
  is_rebate_payment_tx: boolean;
  is_contract_deployment_tx: boolean;
  contract_deployment_data: MsgpackContractDeploymentData;
}
export interface ITxContext {
  isFeePaymentTx: boolean;
  isRebatePaymentTx: boolean;
  isContractDeploymentTx: boolean;
  contractDeploymentData: ContractDeploymentData;
}

export function toTxContext(o: MsgpackTxContext): TxContext {
  if (o.is_fee_payment_tx === undefined) {
    throw new Error('Expected is_fee_payment_tx in TxContext deserialization');
  }
  if (o.is_rebate_payment_tx === undefined) {
    throw new Error('Expected is_rebate_payment_tx in TxContext deserialization');
  }
  if (o.is_contract_deployment_tx === undefined) {
    throw new Error('Expected is_contract_deployment_tx in TxContext deserialization');
  }
  if (o.contract_deployment_data === undefined) {
    throw new Error('Expected contract_deployment_data in TxContext deserialization');
  }
  return new TxContext(
    o.is_fee_payment_tx,
    o.is_rebate_payment_tx,
    o.is_contract_deployment_tx,
    toContractDeploymentData(o.contract_deployment_data),
  );
}

export function fromTxContext(o: TxContext): MsgpackTxContext {
  if (o.isFeePaymentTx === undefined) {
    throw new Error('Expected isFeePaymentTx in TxContext serialization');
  }
  if (o.isRebatePaymentTx === undefined) {
    throw new Error('Expected isRebatePaymentTx in TxContext serialization');
  }
  if (o.isContractDeploymentTx === undefined) {
    throw new Error('Expected isContractDeploymentTx in TxContext serialization');
  }
  if (o.contractDeploymentData === undefined) {
    throw new Error('Expected contractDeploymentData in TxContext serialization');
  }
  return {
    is_fee_payment_tx: o.isFeePaymentTx,
    is_rebate_payment_tx: o.isRebatePaymentTx,
    is_contract_deployment_tx: o.isContractDeploymentTx,
    contract_deployment_data: fromContractDeploymentData(o.contractDeploymentData),
  };
}

export interface MsgpackConstantData {
  historic_tree_roots: MsgpackHistoricTreeRoots;
  tx_context: MsgpackTxContext;
}
export interface IConstantData {
  historicTreeRoots: HistoricTreeRoots;
  txContext: TxContext;
}

export function toConstantData(o: MsgpackConstantData): ConstantData {
  if (o.historic_tree_roots === undefined) {
    throw new Error('Expected historic_tree_roots in ConstantData deserialization');
  }
  if (o.tx_context === undefined) {
    throw new Error('Expected tx_context in ConstantData deserialization');
  }
  return new ConstantData(toHistoricTreeRoots(o.historic_tree_roots), toTxContext(o.tx_context));
}

export function fromConstantData(o: ConstantData): MsgpackConstantData {
  if (o.historicTreeRoots === undefined) {
    throw new Error('Expected historicTreeRoots in ConstantData serialization');
  }
  if (o.txContext === undefined) {
    throw new Error('Expected txContext in ConstantData serialization');
  }
  return {
    historic_tree_roots: fromHistoricTreeRoots(o.historicTreeRoots),
    tx_context: fromTxContext(o.txContext),
  };
}

export interface MsgpackPublicInputs {
  end: MsgpackAccumulatedData;
  constants: MsgpackConstantData;
  is_private: boolean;
}
export interface IPublicInputs {
  end: AccumulatedData;
  constants: ConstantData;
  isPrivate: boolean;
}

export function toPublicInputs(o: MsgpackPublicInputs): PublicInputs {
  if (o.end === undefined) {
    throw new Error('Expected end in PublicInputs deserialization');
  }
  if (o.constants === undefined) {
    throw new Error('Expected constants in PublicInputs deserialization');
  }
  if (o.is_private === undefined) {
    throw new Error('Expected is_private in PublicInputs deserialization');
  }
  return new PublicInputs(toAccumulatedData(o.end), toConstantData(o.constants), o.is_private);
}

export function fromPublicInputs(o: PublicInputs): MsgpackPublicInputs {
  if (o.end === undefined) {
    throw new Error('Expected end in PublicInputs serialization');
  }
  if (o.constants === undefined) {
    throw new Error('Expected constants in PublicInputs serialization');
  }
  if (o.isPrivate === undefined) {
    throw new Error('Expected isPrivate in PublicInputs serialization');
  }
  return {
    end: fromAccumulatedData(o.end),
    constants: fromConstantData(o.constants),
    is_private: o.isPrivate,
  };
}

export interface MsgpackVerificationKeyData {
  composer_type: number;
  circuit_size: number;
  num_public_inputs: number;
  commitments: Record<string, MsgpackAffineElement>;
  contains_recursive_proof: boolean;
  recursive_proof_public_input_indices: number[];
}
export interface IVerificationKeyData {
  composerType: number;
  circuitSize: number;
  numPublicInputs: number;
  commitments: Record<string, AffineElement>;
  containsRecursiveProof: boolean;
  recursiveProofPublicInputIndices: number[];
}

export function toVerificationKeyData(o: MsgpackVerificationKeyData): VerificationKeyData {
  if (o.composer_type === undefined) {
    throw new Error('Expected composer_type in VerificationKeyData deserialization');
  }
  if (o.circuit_size === undefined) {
    throw new Error('Expected circuit_size in VerificationKeyData deserialization');
  }
  if (o.num_public_inputs === undefined) {
    throw new Error('Expected num_public_inputs in VerificationKeyData deserialization');
  }
  if (o.commitments === undefined) {
    throw new Error('Expected commitments in VerificationKeyData deserialization');
  }
  if (o.contains_recursive_proof === undefined) {
    throw new Error('Expected contains_recursive_proof in VerificationKeyData deserialization');
  }
  if (o.recursive_proof_public_input_indices === undefined) {
    throw new Error('Expected recursive_proof_public_input_indices in VerificationKeyData deserialization');
  }
  return new VerificationKeyData(
    o.composer_type,
    o.circuit_size,
    o.num_public_inputs,
    mapValues(o.commitments, (v: AffineElement) => toAffineElement(v)),
    o.contains_recursive_proof,
    o.recursive_proof_public_input_indices.map((v: number) => v),
  );
}

export function fromVerificationKeyData(o: VerificationKeyData): MsgpackVerificationKeyData {
  if (o.composerType === undefined) {
    throw new Error('Expected composerType in VerificationKeyData serialization');
  }
  if (o.circuitSize === undefined) {
    throw new Error('Expected circuitSize in VerificationKeyData serialization');
  }
  if (o.numPublicInputs === undefined) {
    throw new Error('Expected numPublicInputs in VerificationKeyData serialization');
  }
  if (o.commitments === undefined) {
    throw new Error('Expected commitments in VerificationKeyData serialization');
  }
  if (o.containsRecursiveProof === undefined) {
    throw new Error('Expected containsRecursiveProof in VerificationKeyData serialization');
  }
  if (o.recursiveProofPublicInputIndices === undefined) {
    throw new Error('Expected recursiveProofPublicInputIndices in VerificationKeyData serialization');
  }
  return {
    composer_type: o.composerType,
    circuit_size: o.circuitSize,
    num_public_inputs: o.numPublicInputs,
    commitments: mapValues(o.commitments, (v: AffineElement) => fromAffineElement(v)),
    contains_recursive_proof: o.containsRecursiveProof,
    recursive_proof_public_input_indices: o.recursiveProofPublicInputIndices.map((v: number) => v),
  };
}

export interface MsgpackPreviousKernelData {
  public_inputs: MsgpackPublicInputs;
  proof: Buffer;
  vk: MsgpackVerificationKeyData;
  vk_index: number;
  vk_path: TupleOf<Buffer, 3>;
}
export interface IPreviousKernelData {
  publicInputs: PublicInputs;
  proof: Proof;
  vk: VerificationKeyData;
  vkIndex: number;
  vkPath: TupleOf<Fr, 3>;
}

export function toPreviousKernelData(o: MsgpackPreviousKernelData): PreviousKernelData {
  if (o.public_inputs === undefined) {
    throw new Error('Expected public_inputs in PreviousKernelData deserialization');
  }
  if (o.proof === undefined) {
    throw new Error('Expected proof in PreviousKernelData deserialization');
  }
  if (o.vk === undefined) {
    throw new Error('Expected vk in PreviousKernelData deserialization');
  }
  if (o.vk_index === undefined) {
    throw new Error('Expected vk_index in PreviousKernelData deserialization');
  }
  if (o.vk_path === undefined) {
    throw new Error('Expected vk_path in PreviousKernelData deserialization');
  }
  return new PreviousKernelData(
    toPublicInputs(o.public_inputs),
    Proof.fromBuffer(o.proof),
    toVerificationKeyData(o.vk),
    o.vk_index,
    mapTuple(o.vk_path, (v: Buffer) => Fr.fromBuffer(v)),
  );
}

export function fromPreviousKernelData(o: PreviousKernelData): MsgpackPreviousKernelData {
  if (o.publicInputs === undefined) {
    throw new Error('Expected publicInputs in PreviousKernelData serialization');
  }
  if (o.proof === undefined) {
    throw new Error('Expected proof in PreviousKernelData serialization');
  }
  if (o.vk === undefined) {
    throw new Error('Expected vk in PreviousKernelData serialization');
  }
  if (o.vkIndex === undefined) {
    throw new Error('Expected vkIndex in PreviousKernelData serialization');
  }
  if (o.vkPath === undefined) {
    throw new Error('Expected vkPath in PreviousKernelData serialization');
  }
  return {
    public_inputs: fromPublicInputs(o.publicInputs),
    proof: o.proof.toBuffer(),
    vk: fromVerificationKeyData(o.vk),
    vk_index: o.vkIndex,
    vk_path: mapTuple(o.vkPath, (v: Fr) => v.toBuffer()),
  };
}

export function abisComputeContractAddress(
  wasm: CircuitsWasm,
  arg0: Address,
  arg1: Fr,
  arg2: Fr,
  arg3: Fr,
): Promise<Address> {
  return callCbind(wasm, 'abis__compute_contract_address', [arg0, arg1, arg2, arg3]);
}
export function privateKernelDummyPreviousKernel(wasm: CircuitsWasm): Promise<PreviousKernelData> {
  return callCbind(wasm, 'private_kernel__dummy_previous_kernel', []);
}
