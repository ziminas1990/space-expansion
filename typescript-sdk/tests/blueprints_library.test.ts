import { expect, test } from "vitest";
import {
    ApplicationMode,
    Configuration,
    DefaultBlueprints,
    General,
    makeMiner,
    Player,
    Position,
    World,
} from "./configurator/index.js";
import {
    hasServerBinary,
    integrationTimeoutMs,
    withServer,
} from "./fixture.js";
import { expectOk, getBlueprintsLibrary } from "./helpers/index.js";

function blueprintsConfiguration(): Configuration {
    return new Configuration({
        general: new General({
            totalThreads: 1,
            loginUdpPort: 7_456,
            initialState: ApplicationMode.Freeze,
            portsPool: [12_000, 12_100],
        }),
        blueprints: new DefaultBlueprints(),
        world: new World(),
        players: [
            new Player({
                login: "player",
                password: "secret",
                ships: [makeMiner("miner", new Position(0, 0))],
            }),
        ],
    });
}

test.skipIf(!hasServerBinary)(
    "Midlevel: returns the full blueprints list and filters by type",
    { timeout: integrationTimeoutMs },
    async () => {
        const db = new DefaultBlueprints();

        await withServer(blueprintsConfiguration(), async ({ login, clock }) => {
            await clock.fastForward(10);

            // 1. player logins
            const player = await login("player", "secret");

            // 2. get blueprints library
            const library = getBlueprintsLibrary(player).down_level();

            // 3. get the full blueprints list
            const blueprintsList = expectOk(
                await library.get_blueprints_list(),
                "get blueprints list",
            );

            // 4. check every configured blueprint is listed
            const typeToNames = new Map<string, string[]>();
            for (const blueprint of db.blueprints.values()) {
                const fullName = blueprint.id.toPod();
                expect(blueprintsList).toContain(fullName);

                const names = typeToNames.get(blueprint.id.type) ?? [];
                names.push(fullName);
                typeToNames.set(blueprint.id.type, names);
            }

            // 5. filter the list by each blueprint type
            for (const [blueprintType, names] of typeToNames) {
                // 5.1 get the filtered list
                const filtered = expectOk(
                    await library.get_blueprints_list(`${blueprintType}/`),
                    `get blueprints list for ${blueprintType}`,
                );

                // 5.2 check names match
                expect(new Set(filtered)).toEqual(new Set(names));
            }
        });
    },
);

test.skipIf(!hasServerBinary)(
    "Midlevel: returns each blueprint from the library",
    { timeout: integrationTimeoutMs },
    async () => {
        await withServer(blueprintsConfiguration(), async ({ login, clock }) => {
            await clock.fastForward(10);

            // 1. player logins
            const player = await login("player", "secret");

            // 2. get blueprints library
            const library = getBlueprintsLibrary(player).down_level();

            // 3. get the blueprints list
            const blueprintsList = expectOk(
                await library.get_blueprints_list(),
                "get blueprints list",
            );

            // 4. get each blueprint from the library
            for (const blueprintName of blueprintsList) {
                expectOk(
                    await library.get_blueprint(blueprintName),
                    `get blueprint ${blueprintName}`,
                );
            }
        });
    },
);
