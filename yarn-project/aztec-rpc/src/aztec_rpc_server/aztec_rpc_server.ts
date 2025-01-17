import { encodeArguments } from '@aztec/acir-simulator';
import { AztecNode } from '@aztec/aztec-node';
import {
  AztecAddress,
  ContractDeploymentData,
  EcdsaSignature,
  EthAddress,
  FunctionData,
  SignedTxRequest,
  TxContext,
  TxRequest,
} from '@aztec/circuits.js';
import { Fr, Point } from '@aztec/foundation/fields';
import { createDebugLogger } from '@aztec/foundation/log';
import { KeyStore } from '@aztec/key-store';
import { ContractAbi, FunctionType } from '@aztec/foundation/abi';
import { Tx, TxHash } from '@aztec/types';
import { AztecRPCClient, DeployedContract } from '../aztec_rpc_client/index.js';
import { toContractDao } from '../contract_database/index.js';
import { ContractTree } from '../contract_tree/index.js';
import { Database, TxDao } from '../database/index.js';
import { Synchroniser } from '../synchroniser/index.js';
import { TxReceipt, TxStatus } from '../tx/index.js';

/**
 * A remote Aztec RPC Client implementation.
 */
export class AztecRPCServer implements AztecRPCClient {
  private synchroniser: Synchroniser;

  constructor(
    private keyStore: KeyStore,
    private node: AztecNode,
    private db: Database,
    private log = createDebugLogger('aztec:rpc_server'),
  ) {
    this.synchroniser = new Synchroniser(node, db);
  }

  /**
   * Starts the Aztec RPC server by initializing account states for each registered account and
   * begins the synchronisation process between the Aztec node and the database.
   * It logs the number of initial accounts that were started.
   *
   * @returns A promise that resolves when the server has started successfully.
   */
  public async start() {
    const accounts = await this.keyStore.getAccounts();
    for (const account of accounts) {
      await this.initAccountState(account);
    }
    await this.synchroniser.start();
    this.log(`Started. ${accounts.length} initial accounts.`);
  }

  /**
   * Stops the Aztec RPC server, halting processing of new transactions and shutting down the synchronizer.
   * This function ensures that all ongoing tasks are completed before stopping the server.
   * It is useful for gracefully shutting down the server during maintenance or restarts.
   *
   * @returns A Promise resolving once the server has been stopped successfully.
   */
  public async stop() {
    await this.synchroniser.stop();
    this.log('Stopped.');
  }

  /**
   * Adds a new account to the AztecRPCServer instance.
   *
   * @returns The AztecAddress of the newly added account.
   */
  public async addAccount() {
    const accountAddress = await this.keyStore.addAccount();
    await this.initAccountState(accountAddress);
    return accountAddress;
  }

  /**
   * Add an array of deployed contracts to the database.
   * Each contract should contain ABI, address, and portalContract information.
   *
   * @param contracts - An array of DeployedContract objects containing contract ABI, address, and portalContract.
   * @returns A Promise that resolves once all the contracts have been added to the database.
   */
  public async addContracts(contracts: DeployedContract[]) {
    const contractDaos = contracts.map(c => toContractDao(c.abi, c.address, c.portalContract));
    await Promise.all(contractDaos.map(c => this.db.addContract(c)));
  }

  /**
   * Retrieves the list of Aztec addresses associated with the current accounts in the key store.
   * The addresses are returned as a promise that resolves to an array of AztecAddress objects.
   *
   * @returns A promise that resolves to an array of AztecAddress instances.
   */
  public async getAccounts(): Promise<AztecAddress[]> {
    const accounts = this.synchroniser.getAccounts();
    return await Promise.all(accounts.map(a => a.getAddress()));
  }

  /**
   * Retrieve the public key associated with an address.
   * Throws an error if the account is not found in the key store.
   *
   * @param address - The AztecAddress instance representing the account.
   * @returns A Promise resolving to the Point instance representing the public key.
   */
  public getAccountPublicKey(address: AztecAddress): Promise<Point> {
    const account = this.ensureAccount(address);
    return Promise.resolve(account.getPublicKey());
  }

  /**
   * Retrieves the storage data at a specified contract address and storage slot.
   * The returned data is an array of note preimage items, with each item containing its value.
   *
   * @param contract - The AztecAddress of the target contract.
   * @param storageSlot - The Fr representing the storage slot to be fetched.
   * @returns A promise that resolves to an array of note preimage items, each containing its value.
   */
  public async getStorageAt(contract: AztecAddress, storageSlot: Fr) {
    const txAuxData = await this.db.getTxAuxData(contract, storageSlot);
    return txAuxData.map(d => d.notePreimage.items.map(item => item.value));
  }

  /**
   * Is an L2 contract deployed at this address?
   * @param contractAddress - The contract data address.
   * @returns Whether the contract was deployed.
   */
  public async isContractDeployed(contractAddress: AztecAddress): Promise<boolean> {
    return !!(await this.node.getContractInfo(contractAddress));
  }

  /**
   * Create a deployment transaction request for deploying a new contract.
   * The function generates ContractDeploymentData and a TxRequest instance containing
   * the constructor function data, flat arguments, nonce, and other necessary information.
   * This TxRequest can then be signed and sent to deploy the contract on the Aztec network.
   *
   * @param abi - The contract ABI containing function definitions.
   * @param args - The arguments required for the constructor function of the contract.
   * @param portalContract - The Ethereum address of the portal contract.
   * @param contractAddressSalt - (Optional) Salt value used to generate the contract address.
   * @param from - (Optional) The Aztec address of the account that deploys the contract.
   * @returns A TxRequest instance containing all necessary information for contract deployment.
   */
  public async createDeploymentTxRequest(
    abi: ContractAbi,
    args: any[],
    portalContract: EthAddress,
    contractAddressSalt = Fr.random(),
    from?: AztecAddress,
  ) {
    const fromAddress = this.ensureAccountOrDefault(from);

    const constructorAbi = abi.functions.find(f => f.name === 'constructor');
    if (!constructorAbi) {
      throw new Error('Cannot find constructor in the ABI.');
    }

    const flatArgs = encodeArguments(constructorAbi, args);
    const contractTree = await ContractTree.new(
      abi,
      flatArgs,
      portalContract,
      contractAddressSalt,
      fromAddress,
      this.node,
    );
    const { functionData, vkHash } = contractTree.newContractConstructor!;
    const functionTreeRoot = await contractTree.getFunctionTreeRoot();
    const contractDeploymentData = new ContractDeploymentData(
      Fr.fromBuffer(vkHash),
      functionTreeRoot,
      contractAddressSalt,
      portalContract,
    );
    const txContext = new TxContext(false, false, true, contractDeploymentData);

    const contract = contractTree.contract;
    await this.db.addContract(contract);

    return new TxRequest(
      fromAddress,
      contract.address,
      functionData,
      flatArgs,
      Fr.random(), // nonce
      txContext,
      Fr.ZERO, // chainId
    );
  }

  /**
   * Create a transaction request for a contract function call with the provided arguments.
   * Retrieves necessary information about the contract from the database and constructs a TxRequest object.
   * Throws an error if the contract or function is unknown.
   *
   * @param functionName - Name of the function to be invoked in the contract.
   * @param args - Array of input arguments for the function.
   * @param to - Address of the target contract.
   * @param from - (Optional) Address of the sender (defaults to first available account).
   * @returns A TxRequest instance representing the contract function call and its context.
   */
  public async createTxRequest(functionName: string, args: any[], to: AztecAddress, from?: AztecAddress) {
    const fromAddress = this.ensureAccountOrDefault(from);

    const contract = await this.db.getContract(to);
    if (!contract) {
      throw new Error('Unknown contract.');
    }

    const functionDao = contract.functions.find(f => f.name === functionName);
    if (!functionDao) {
      throw new Error('Unknown function.');
    }

    const flatArgs = encodeArguments(functionDao, args);

    const functionData = new FunctionData(
      functionDao.selector,
      functionDao.functionType === FunctionType.SECRET,
      false,
    );

    const txContext = new TxContext(
      false,
      false,
      false,
      new ContractDeploymentData(Fr.ZERO, Fr.ZERO, Fr.ZERO, new EthAddress(Buffer.alloc(EthAddress.SIZE_IN_BYTES))),
    );

    return new TxRequest(
      fromAddress,
      to,
      functionData,
      flatArgs,
      Fr.random(), // nonce
      txContext,
      Fr.ZERO, // chainId
    );
  }

  /**
   * Sign a TxRequest with the creator's private key.
   * This function retrieves the private key of the account specified in the TxRequest 'from' field
   * and signs it, generating an EcdsaSignature which can be used to create a valid kernel proof.
   *
   * @param txRequest - The TxRequest instance containing necessary information for signing.
   * @returns An EcdsaSignature instance representing the signed transaction.
   */
  public signTxRequest(txRequest: TxRequest) {
    this.ensureAccount(txRequest.from);
    return this.keyStore.signTxRequest(txRequest);
  }

  /**
   * Creates a new transaction object from a given signed transaction request.
   * If the transaction is private, it simulates and proves the transaction request using accountState.
   * If it is public, it creates a public transaction without the need for simulation.
   * The resulting transaction object can then be sent to the network for execution using sendTx method.
   *
   * @param txRequest - The signed transaction request containing all necessary details for executing the transaction.
   * @param signature - The ECDSA signature of the transaction request.
   * @returns A transaction object that can be sent to the network.
   */
  public async createTx(txRequest: TxRequest, signature: EcdsaSignature) {
    let toContract: AztecAddress | undefined;
    let newContract: AztecAddress | undefined;
    const accountState = this.ensureAccount(txRequest.from);

    const contractAddress = txRequest.to;
    let tx: Tx;
    if (!txRequest.functionData.isPrivate) {
      // Note: there is no simulation being performed client-side for public functions execution.
      tx = Tx.createPublic(new SignedTxRequest(txRequest, signature));
    } else if (txRequest.functionData.isConstructor) {
      newContract = contractAddress;

      tx = await accountState.simulateAndProve(txRequest, signature, contractAddress);
    } else {
      toContract = contractAddress;
      tx = await accountState.simulateAndProve(txRequest, signature);
    }

    const dao = new TxDao(await tx.getTxHash(), undefined, undefined, txRequest.from, toContract, newContract, '');
    await this.db.addTx(dao);

    return tx;
  }

  /**
   * Send a transaction.
   * @param tx - The transaction.
   * @returns A hash of the transaction, used to identify it.
   */
  public async sendTx(tx: Tx): Promise<TxHash> {
    await this.node.sendTx(tx);
    return tx.getTxHash();
  }

  /**
   * Simulate the execution of a view (read-only) function on a deployed contract without actually modifying state.
   * This is useful to inspect contract state, for example fetching a variable value or calling a getter function.
   * The function takes function name and arguments as parameters, along with the contract address
   * and optionally the sender's address.
   *
   * @param functionName - The name of the function to be called in the contract.
   * @param args - An array of arguments to be passed into the function call.
   * @param to - The address of the contract on which the function will be called.
   * @param from - (Optional) The caller of the transaction.
   * @returns The result of the view function call, structured based on the function ABI.
   */
  public async viewTx(functionName: string, args: any[], to: AztecAddress, from?: AztecAddress) {
    const txRequest = await this.createTxRequest(functionName, args, to, from);
    const accountState = this.ensureAccount(txRequest.from);
    const executionResult = await accountState.simulateUnconstrained(txRequest);

    // TODO - Return typed result based on the function abi.
    return executionResult;
  }

  /**
   * Fetchs a transaction receipt for a tx.
   * @param txHash - The transaction hash.
   * @returns A recipt of the transaction.
   */
  public async getTxReceipt(txHash: TxHash): Promise<TxReceipt> {
    const localTx = await this.synchroniser.getTxByHash(txHash);
    const partialReceipt = {
      txHash: txHash,
      blockHash: localTx?.blockHash,
      blockNumber: localTx?.blockNumber,
      from: localTx?.from,
      to: localTx?.to,
      contractAddress: localTx?.contractAddress,
      error: '',
    };

    if (localTx?.blockHash) {
      return {
        ...partialReceipt,
        status: TxStatus.MINED,
      };
    }

    const pendingTx = await this.node.getPendingTxByHash(txHash);
    if (pendingTx) {
      return {
        ...partialReceipt,
        status: TxStatus.PENDING,
      };
    }

    // if the transaction mined it will be removed from the pending pool and there is a race condition here as the synchroniser will not have the tx as mined yet, so it will appear dropped
    // until the synchroniser picks this up

    const accountState = this.synchroniser.getAccount(localTx.from);
    if (accountState && !(await accountState?.isSynchronised())) {
      // there is a pending L2 block, which means the transaction will not be in the tx pool but may be awaiting mine on L1
      return {
        ...partialReceipt,
        status: TxStatus.PENDING,
      };
    }

    // TODO we should refactor this once the node can store transactions. At that point we should query the node and not deal with block heights.

    return {
      ...partialReceipt,
      status: TxStatus.DROPPED,
      error: 'Tx dropped by P2P node.',
    };
  }

  /**
   * Initializes the account state for a given address.
   * It retrieves the private key from the key store and adds the account to the synchroniser.
   * This function is called for all existing accounts during the server start, or when a new account is added afterwards.
   *
   * @param address - The address of the account to initialize.
   */
  private async initAccountState(address: AztecAddress) {
    const accountPrivateKey = await this.keyStore.getAccountPrivateKey(address);
    await this.synchroniser.addAccount(accountPrivateKey);
    this.log(`Account added: ${address.toString()}`);
  }

  /**
   * Retrieves an existing account or the default one if none is provided.
   * Ensures that the given account address exists in the synchroniser, otherwise throws an error.
   * If no account address is provided, it returns the first account from the synchroniser.
   * Throws an error if there are no accounts available in the key store.
   *
   * @param account - (Optional) Address of the account to ensure its existence.
   * @returns The ensured AztecAddress instance.
   */
  private ensureAccountOrDefault(account?: AztecAddress) {
    const address = account || this.synchroniser.getAccounts()[0]?.getAddress();
    if (!address) {
      throw new Error('No accounts available in the key store.');
    }

    this.ensureAccount(address);

    return address;
  }

  /**
   * Ensures the given account address exists in the synchroniser.
   * Retrieves the account state for the provided address and throws an error if the account is not found.
   *
   * @param account - The account address.
   * @returns The account state associated with the given address.
   * @throws If the account is unknown or not found in the synchroniser.
   */
  private ensureAccount(account: AztecAddress) {
    const accountState = this.synchroniser.getAccount(account);
    if (!accountState) {
      throw new Error(`Unknown account: ${account.toShortString()}.`);
    }

    return accountState;
  }
}
