export interface ILogger {
    debug(message: string): void;
    info(message: string): void;
    warning(message: string): void;
    error(message: string): void;
    child(name: string): ILogger;
}

export const noop_logger: ILogger = {
    debug() {},
    info() {},
    warning() {},
    error() {},
    child() {
        return noop_logger;
    },
};

export function create_console_logger(): ILogger {
    return {
        debug(message) {
            console.debug(message);
        },
        info(message) {
            console.info(message);
        },
        warning(message) {
            console.warn(message);
        },
        error(message) {
            console.error(message);
        },
        child(_child_name) {
            throw new Error("Console logger does not support child loggers");
        },
    };
}

export function crate_formatted_logger(name: string, underlying: ILogger): ILogger {
    return {
        debug(message) {
            underlying.debug(`${new Date().toISOString()} [DEBUG] ${name}: ${message}`);
        },
        info(message) {
            underlying.info(`${new Date().toISOString()} [INFO] ${name}: ${message}`);
        },
        warning(message) {
            underlying.warning(`${new Date().toISOString()} [WARNING] ${name}: ${message}`);
        },
        error(message) {
            underlying.error(`${new Date().toISOString()} [ERROR] ${name}: ${message}`);
        },
        child(child_name) {
            return crate_formatted_logger(`${name}.${child_name}`, underlying);
        },
    };
}
