import { login as sdkLogin, Player as SdkPlayer } from "../highlevel/index.js";
import { Administrator, login_as_administrator } from "../midlevel/index.js";
import { applyTestDefaults, Configuration } from "./configurator/index.js";
import { IngameClock } from "./ingame_clock.js";
import { Server } from "./server.js";

export { waitFor } from "./helpers/wait.js";
export { IngameClock } from "./ingame_clock.js";

export const hasServerBinary = Boolean(process.env.SPEX_SERVER_BINARY);
export const integrationTimeoutMs = 180_000;

export interface IntegrationTestContext {
    readonly administrator: Administrator;
    readonly configuration: Configuration;
    readonly clock: IngameClock;
    readonly server: Server;
    login(login: string, password: string): Promise<SdkPlayer>;
}

export async function withServer(
    configuration: Configuration,
    test: (context: IntegrationTestContext) => Promise<void>,
): Promise<void> {
    applyTestDefaults(configuration.general);

    const server = new Server();
    const players: SdkPlayer[] = [];
    let administrator: Administrator | undefined;
    let clock: IngameClock;
    let failure: unknown;

    try {
        await server.run(configuration);

        const admin_cfg = configuration.general.administrator;
        if (admin_cfg === null) {
            throw new Error("Administrator interface is not configured");
        }
        const [status, session] = await login_as_administrator(
            "127.0.0.1",
            admin_cfg.udpPort,
            admin_cfg.login,
            admin_cfg.password,
        );
        if (!status.is_ok() || session === undefined) {
            throw new Error(status.wrap("Administrator login failed").what());
        }
        administrator = session;
        clock = new IngameClock(session.clock);

        await test({
            administrator,
            configuration,
            clock,
            server,
            login: (login, password) => loginPlayer(
                login,
                password,
                configuration.general.loginUdpPort,
                players,
            ),
        });
    } catch (error: unknown) {
        failure = error;
    }

    const cleanupErrors: unknown[] = [];
    await captureCleanupError(() => clock?.shutdown(), cleanupErrors);

    const leftoverPlayers = players.splice(0);
    await Promise.all(
        leftoverPlayers.map((player) =>
            captureCleanupError(
                () => withTimeout(player.release(), 2_000, "Player shutdown"),
                cleanupErrors,
            )
        ),
    );

    if (server.isRunning() && administrator !== undefined) {
        await captureCleanupError(
            () => administrator.clock.terminate(1_000),
            cleanupErrors,
        );
    }
    if (administrator !== undefined) {
        await captureCleanupError(() => administrator.close(), cleanupErrors);
    }
    await captureCleanupError(() => server.stop(), cleanupErrors);

    if (failure !== undefined) {
        throw withServerLogs(failure, server.logs);
    }
    if (cleanupErrors.length > 0) {
        throw new AggregateError(
            cleanupErrors,
            "Integration test cleanup failed",
        );
    }
}

async function loginPlayer(
    login: string,
    password: string,
    loginUdpPort: number,
    players: SdkPlayer[],
): Promise<SdkPlayer> {
    const [status, player] = await sdkLogin(
        "127.0.0.1",
        login,
        password,
        undefined,
        loginUdpPort,
    );
    if (!status.is_ok() || player === undefined) {
        throw new Error(status.wrap(`Failed to log in '${login}'`).what());
    }
    players.push(player);
    const release = player.release.bind(player);
    player.release = async () => {
        try {
            return await release();
        } finally {
            const index = players.indexOf(player);
            if (index >= 0) {
                players.splice(index, 1);
            }
        }
    };
    return player;
}

async function captureCleanupError(
    cleanup: () => Promise<unknown>,
    errors: unknown[],
): Promise<void> {
    try {
        await cleanup();
    } catch (error: unknown) {
        errors.push(error);
    }
}

async function withTimeout<T>(
    promise: Promise<T>,
    timeoutMs: number,
    description: string,
): Promise<T> {
    let timer: ReturnType<typeof setTimeout> | undefined;
    try {
        return await Promise.race([
            promise,
            new Promise<never>((_, reject) => {
                timer = setTimeout(
                    () => reject(new Error(`${description} timed out`)),
                    timeoutMs,
                );
            }),
        ]);
    } finally {
        if (timer !== undefined) {
            clearTimeout(timer);
        }
    }
}

function withServerLogs(error: unknown, logs: readonly string[]): Error {
    const original = error instanceof Error ? error : new Error(String(error));
    if (logs.length === 0) {
        return original;
    }

    const output = logs.slice(-100).join("\n");
    return new Error(`${original.message}\n\nServer output:\n${output}`, {
        cause: original,
    });
}
