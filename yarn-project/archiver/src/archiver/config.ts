import { EthAddress } from '@aztec/foundation/eth-address';
import { L1Addresses } from '@aztec/types';

/**
 * The archiver configuration.
 */
export interface ArchiverConfig extends L1Addresses {
  /**
   * The url of the Ethereum RPC node.
   */
  rpcUrl: string;

  /**
   * The key for the ethereum node.
   */
  apiKey?: string;

  /**
   * The polling interval in ms for retrieving new L2 blocks and unverified data.
   */
  archiverPollingInterval?: number;

  /**
   * Eth block from which we start scanning for L2Blocks.
   */
  searchStartBlock: number;
}

/**
 * Returns the archiver configuration from the environment variables.
 * Note: If an environment variable is not set, the default value is used.
 * @returns The archiver configuration.
 */
export function getConfigEnvVars(): ArchiverConfig {
  const {
    ETHEREUM_HOST,
    ARCHIVER_POLLING_INTERVAL,
    ROLLUP_CONTRACT_ADDRESS,
    UNVERIFIED_DATA_EMITTER_ADDRESS,
    SEARCH_START_BLOCK,
    API_KEY,
    INBOX_CONTRACT_ADDRESS,
  } = process.env;
  return {
    rpcUrl: ETHEREUM_HOST || 'http://127.0.0.1:8545/',
    archiverPollingInterval: ARCHIVER_POLLING_INTERVAL ? +ARCHIVER_POLLING_INTERVAL : 1_000,
    rollupContract: ROLLUP_CONTRACT_ADDRESS ? EthAddress.fromString(ROLLUP_CONTRACT_ADDRESS) : EthAddress.ZERO,
    inboxContract: INBOX_CONTRACT_ADDRESS ? EthAddress.fromString(INBOX_CONTRACT_ADDRESS) : EthAddress.ZERO,
    unverifiedDataEmitterContract: UNVERIFIED_DATA_EMITTER_ADDRESS
      ? EthAddress.fromString(UNVERIFIED_DATA_EMITTER_ADDRESS)
      : EthAddress.ZERO,
    searchStartBlock: SEARCH_START_BLOCK ? +SEARCH_START_BLOCK : 0,
    apiKey: API_KEY,
  };
}
