import { type ChildProcess, spawn } from "node:child_process";
import { mkdtemp, rm, writeFile } from "node:fs/promises";
import { tmpdir } from "node:os";
import { join } from "node:path";
import {
    createInterface,
    type Interface as ReadLineInterface,
} from "node:readline";

export interface ServerConfiguration {
    toPod(): unknown;
}

export class Server {
    private serverProcess: ChildProcess | null = null;
    private stdoutReader: ReadLineInterface | null = null;
    private outputQueue: string[] = [];
    private output: string[] = [];
    private configDirectory: string | null = null;

    async run(configuration: ServerConfiguration): Promise<void> {
        if (this.isRunning()) {
            throw new Error("The server is already running");
        }

        const serverBinary = process.env.SPEX_SERVER_BINARY;
        if (serverBinary === undefined || serverBinary.length === 0) {
            throw new Error("SPEX_SERVER_BINARY is not set");
        }

        this.outputQueue = [];
        this.output = [];
        this.configDirectory = await mkdtemp(
            join(tmpdir(), "space-expansion-"),
        );
        const configPath = join(this.configDirectory, "config.yaml");

        // JSON is valid YAML, so no test-only YAML dependency is required.
        await writeFile(
            configPath,
            `${JSON.stringify(configuration.toPod(), null, 2)}\n`,
            "utf8",
        );

        try {
            const child = spawn(serverBinary, [configPath], {
                stdio: ["ignore", "pipe", "inherit"],
            });
            this.serverProcess = child;

            if (child.stdout === null) {
                throw new Error("Failed to capture the server stdout");
            }

            this.stdoutReader = createInterface({ input: child.stdout });
            this.stdoutReader.on("line", (line) => {
                this.outputQueue.push(line);
                this.output.push(line);
            });

            await new Promise<void>((resolve, reject) => {
                const onSpawn = (): void => {
                    child.off("error", onError);
                    resolve();
                };
                const onError = (error: Error): void => {
                    child.off("spawn", onSpawn);
                    reject(error);
                };

                child.once("spawn", onSpawn);
                child.once("error", onError);
            });

            const [started] = await this.waitLog(
                "Server has been started",
                1_000,
            );
            if (!started) {
                throw new Error("The server did not start within 1 second");
            }
        } catch (error: unknown) {
            await this.stop();
            throw error;
        }
    }

    async waitLog(
        substr: string,
        timeoutMs = 1_000,
    ): Promise<readonly [found: boolean, line: string | null]> {
        if (this.serverProcess === null) {
            throw new Error("The server has not been run");
        }

        const stopAt = Date.now() + timeoutMs;
        while (true) {
            const line = this.outputQueue.shift();
            if (line !== undefined && line.includes(substr)) {
                return [true, line];
            }

            const remainingMs = stopAt - Date.now();
            if (remainingMs <= 0) {
                return [false, null];
            }

            await new Promise<void>((resolve) => {
                setTimeout(resolve, Math.min(10, remainingMs));
            });
        }
    }

    isRunning(): boolean {
        return (
            this.serverProcess?.pid !== undefined &&
            this.serverProcess.exitCode === null &&
            this.serverProcess.signalCode === null
        );
    }

    get logs(): readonly string[] {
        return this.output;
    }

    async stop(): Promise<void> {
        const child = this.serverProcess;

        try {
            if (child !== null && this.isRunning()) {
                const exited = new Promise<void>((resolve) => {
                    child.once("exit", () => resolve());
                });

                child.kill("SIGKILL");
                let timer: ReturnType<typeof setTimeout> | undefined;
                try {
                    await Promise.race([
                        exited,
                        new Promise<never>((_, reject) => {
                            timer = setTimeout(
                                () =>
                                    reject(
                                        new Error(
                                            "The server did not stop within 1 second",
                                        ),
                                    ),
                                1_000,
                            );
                        }),
                    ]);
                } finally {
                    if (timer !== undefined) {
                        clearTimeout(timer);
                    }
                }

                if (this.isRunning()) {
                    throw new Error("Failed to stop the server");
                }
            }
        } finally {
            this.stdoutReader?.close();
            this.stdoutReader = null;
            this.serverProcess = null;
            this.outputQueue = [];

            if (this.configDirectory !== null) {
                await rm(this.configDirectory, {
                    recursive: true,
                    force: true,
                });
                this.configDirectory = null;
            }
        }
    }
}
