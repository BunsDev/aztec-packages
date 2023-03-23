/**
 * Type for a logger.
 */
export type DebugLogger = (...args: any[]) => void;
/**
 * Creates a logger.
 * TODO port aztec2 logger over.
 * @param moduleName - Name of module for logging or filtering purposes.
 */
export declare function createDebugLogger(moduleName: string): DebugLogger;
//# sourceMappingURL=index.d.ts.map