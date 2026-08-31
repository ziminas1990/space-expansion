export enum ApplicationMode {
    Run = "run",
    Freeze = "freezed",
}

export interface AdministratorOptions {
    udpPort: number;
    login: string;
    password: string;
}

export class AdministratorConfig {
    readonly udpPort: number;
    readonly login: string;
    readonly password: string;

    constructor(options: AdministratorOptions) {
        this.udpPort = options.udpPort;
        this.login = options.login;
        this.password = options.password;
    }

    verify(): void {
        assertPort(this.udpPort, "Administrator UDP port");
        if (this.login.length <= 4) {
            throw new Error(
                "Administrator login must be longer than 4 characters",
            );
        }
        if (this.password.length <= 6) {
            throw new Error(
                "Administrator password must be longer than 6 characters",
            );
        }
    }

    toPod(): Record<string, string | number> {
        this.verify();
        return {
            "udp-port": this.udpPort,
            login: this.login,
            password: this.password,
        };
    }
}

export class GlobalGrid {
    readonly gridSize: number;
    readonly cellWidthKm: number;

    constructor(gridSize: number, cellWidthKm: number) {
        this.gridSize = gridSize;
        this.cellWidthKm = cellWidthKm;
    }

    verify(): void {
        if (this.gridSize <= 0 || this.gridSize >= 255) {
            throw new Error("Global grid size must be between 1 and 254");
        }
        if (this.cellWidthKm <= 0) {
            throw new Error("Global grid cell width must be positive");
        }
    }

    toPod(): Record<string, number> {
        this.verify();
        return {
            "grid-size": this.gridSize,
            "cell-width-km": this.cellWidthKm,
        };
    }
}

export interface GeneralOptions {
    totalThreads: number;
    loginUdpPort: number;
    initialState: ApplicationMode;
    portsPool: readonly [begin: number, end: number];
    seed?: number;
    globalGrid?: GlobalGrid;
    administrator?: AdministratorConfig;
}

export class General {
    totalThreads: number;
    loginUdpPort: number;
    seed: number;
    initialState: ApplicationMode;
    portsPool: readonly [begin: number, end: number];
    globalGrid: GlobalGrid | null;
    administrator: AdministratorConfig | null;

    constructor(options: GeneralOptions) {
        this.totalThreads = options.totalThreads;
        this.loginUdpPort = options.loginUdpPort;
        this.seed = options.seed ?? 12_345;
        this.initialState = options.initialState;
        this.portsPool = options.portsPool;
        this.globalGrid = options.globalGrid ?? null;
        this.administrator = options.administrator ?? null;
    }

    setTotalThreads(totalThreads: number): this {
        this.totalThreads = totalThreads;
        return this;
    }

    setLoginPort(port: number): this {
        assertPort(port, "Login UDP port");
        this.loginUdpPort = port;
        return this;
    }

    setInitialState(state: ApplicationMode): this {
        this.initialState = state;
        return this;
    }

    setPortsPool(begin: number, end: number): this {
        this.portsPool = [begin, end];
        return this;
    }

    setGlobalGrid(gridSize: number, cellWidthKm: number): this {
        this.globalGrid = new GlobalGrid(gridSize, cellWidthKm);
        this.globalGrid.verify();
        return this;
    }

    addAdministratorInterface(
        udpPort: number,
        login: string,
        password: string,
    ): this {
        this.administrator = new AdministratorConfig({
            udpPort,
            login,
            password,
        });
        return this;
    }

    verify(): void {
        if (this.totalThreads <= 0) {
            throw new Error("Total thread count must be positive");
        }
        assertPort(this.loginUdpPort, "Login UDP port");
        if (this.seed <= 0) {
            throw new Error("Seed must be positive");
        }

        const [begin, end] = this.portsPool;
        assertPort(begin, "Ports pool begin");
        assertPort(end, "Ports pool end");
        if (begin >= end) {
            throw new Error("Ports pool begin must be less than its end");
        }
        assertOutsidePool(this.loginUdpPort, this.portsPool, "Login UDP port");

        if (this.globalGrid === null) {
            throw new Error("Global grid is not configured");
        }
        this.globalGrid.verify();

        if (this.administrator !== null) {
            this.administrator.verify();
            assertOutsidePool(
                this.administrator.udpPort,
                this.portsPool,
                "Administrator UDP port",
            );
            if (this.administrator.udpPort === this.loginUdpPort) {
                throw new Error(
                    "Administrator and login UDP ports must differ",
                );
            }
        }
    }

    toPod(): Record<string, unknown> {
        this.verify();

        const [begin, end] = this.portsPool;
        const pod: Record<string, unknown> = {
            "total-threads": this.totalThreads,
            "login-udp-port": this.loginUdpPort,
            seed: this.seed,
            "initial-state": this.initialState,
            "ports-pool": { begin, end },
            "global-grid": this.globalGrid?.toPod(),
        };

        if (this.administrator !== null) {
            pod.administrator = this.administrator.toPod();
        }
        return pod;
    }
}

export interface TestDefaults {
    gridSize?: number;
    cellWidthKm?: number;
    administrator?: AdministratorOptions;
}

export function applyTestDefaults(
    general: General,
    defaults: TestDefaults = {},
): General {
    if (general.globalGrid === null) {
        general.setGlobalGrid(
            defaults.gridSize ?? 100,
            defaults.cellWidthKm ?? 1_000,
        );
    }
    if (general.administrator === null) {
        const administrator = defaults.administrator ?? {
            udpPort: 28_000,
            login: "administrator",
            password: "iampower",
        };
        general.addAdministratorInterface(
            administrator.udpPort,
            administrator.login,
            administrator.password,
        );
    }
    return general;
}

function assertPort(port: number, name: string): void {
    if (!Number.isInteger(port) || port <= 0 || port >= 65_535) {
        throw new Error(`${name} must be an integer between 1 and 65534`);
    }
}

function assertOutsidePool(
    port: number,
    pool: readonly [number, number],
    name: string,
): void {
    if (port >= pool[0] && port <= pool[1]) {
        throw new Error(`${name} must be outside the communication ports pool`);
    }
}
