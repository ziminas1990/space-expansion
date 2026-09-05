export type LogLevel = "DEBUG" | "INFO" | "WARNING" | "ERROR";

const LEVELS: Record<LogLevel, number> = {
    DEBUG: 10,
    INFO: 20,
    WARNING: 30,
    ERROR: 40,
};

let min_level = LEVELS.INFO;

export function set_log_level(level: string): void {
    const normalized = level.toUpperCase() as LogLevel;
    min_level = LEVELS[normalized] ?? LEVELS.INFO;
}

export type Logger = {
    debug: (message: string) => void;
    info: (message: string) => void;
    warning: (message: string) => void;
    error: (message: string) => void;
};

function emit(level: LogLevel, name: string, message: string): void {
    if (LEVELS[level] < min_level) {
        return;
    }
    const stream = level === "ERROR" ? process.stderr : process.stdout;
    stream.write(`${new Date().toISOString()} ${level} ${name}: ${message}\n`);
}

export function create_logger(name: string): Logger {
    return {
        debug: (message) => emit("DEBUG", name, message),
        info: (message) => emit("INFO", name, message),
        warning: (message) => emit("WARNING", name, message),
        error: (message) => emit("ERROR", name, message),
    };
}
