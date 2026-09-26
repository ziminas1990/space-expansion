import { create } from "@bufbuild/protobuf";
import { expect, test } from "vitest";
import * as msg from "../Protocol_pb.js";
import { Shipyard as LowlevelShipyard } from "../lowlevel/shipyard.js";
import type { Session } from "../lowlevel/session.js";
import { Shipyard as MidlevelShipyard } from "../midlevel/shipyard.js";
import { Status } from "../types/status.js";

function shipyard_message(choice: msg.IShipyard["choice"]) {
    return create(msg.MessageSchema, {
        choice: { case: "shipyard", value: create(msg.IShipyardSchema, { choice }) },
    });
}

function fake_session(responses: Array<msg.Message | null>) {
    const sent: msg.Message[] = [];
    let closed = false;
    const session = {
        send: async (message: msg.Message) => {
            sent.push(message);
            return Status.ok();
        },
        wait: async () => {
            const message = responses.shift();
            return message === null
                ? [Status.timeout(), undefined] as const
                : message === undefined
                ? [Status.fail("no response"), undefined] as const
                : [Status.ok(), message] as const;
        },
        close: async () => {
            closed = true;
            return Status.ok();
        },
        is_active: () => !closed,
    } as unknown as Session;
    return { session, sent, is_closed: () => closed };
}

test("lowlevel Shipyard decodes the monitoring handshake and reused build messages", async () => {
    const { session, sent } = fake_session([
        shipyard_message({ case: "monitoringAck", value: true }),
        shipyard_message({
            case: "buildStarted",
            value: { blueprintName: "Ship/Miner", shipName: "Ore One" },
        }),
        shipyard_message({
            case: "buildingReport",
            value: { status: msg.IShipyard_Status.BUILD_FROZEN, progress: 0.4 },
        }),
        shipyard_message({
            case: "buildingComplete",
            value: { shipName: "Ore One", slotId: 7 },
        }),
    ]);
    const shipyard = new LowlevelShipyard(session);

    expect((await shipyard.send_monitoring_request()).is_ok()).toBe(true);
    expect(sent[0]?.choice.case).toBe("shipyard");
    if (sent[0]?.choice.case === "shipyard") {
        expect(sent[0].choice.value.choice.case).toBe("monitoring");
    }
    expect(await shipyard.wait_monitoring_ack()).toEqual([Status.ok(), true]);
    expect((await shipyard.wait_building_event())[1]).toEqual({
        case: "build_started",
        build: { blueprint_name: "Ship/Miner", ship_name: "Ore One" },
    });
    expect((await shipyard.wait_building_event())[1]).toEqual({
        case: "building_report",
        report: { status: "BUILD_FROZEN", progress: 0.4 },
    });
    expect((await shipyard.wait_building_event())[1]).toEqual({
        case: "building_complete",
        ship: { ship_name: "Ore One", slot_id: 7 },
    });
});

test("midlevel monitoring reports the current build without a transient idle state", async () => {
    const { session, is_closed } = fake_session([
        shipyard_message({ case: "monitoringAck", value: true }),
        shipyard_message({
            case: "buildStarted",
            value: { blueprintName: "Ship/Scout", shipName: "Eye" },
        }),
        shipyard_message({
            case: "buildingReport",
            value: { status: msg.IShipyard_Status.BUILD_IN_PROGRESS, progress: 0.6 },
        }),
        shipyard_message({
            case: "buildingComplete",
            value: { shipName: "Eye", slotId: 9 },
        }),
    ]);
    const shipyard = new MidlevelShipyard(async () => [Status.ok(), session]);
    const events: unknown[] = [];

    const status = await shipyard.monitoring(async (event) => {
        events.push(event);
        return event?.case !== "building_complete";
    });
    expect(status.is_ok()).toBe(true);
    expect(events).toEqual([
        { case: "build_started", build: {
            blueprint_name: "Ship/Scout", ship_name: "Eye",
        } },
        { case: "building_report", report: {
            status: "BUILD_IN_PROGRESS", progress: 0.6,
        } },
        { case: "building_complete", ship: { ship_name: "Eye", slot_id: 9 } },
    ]);
    expect(is_closed()).toBe(true);
});

test("midlevel monitoring reports idle when no build follows the acknowledgment", async () => {
    const { session } = fake_session([
        shipyard_message({ case: "monitoringAck", value: true }),
        null,
    ]);
    const shipyard = new MidlevelShipyard(async () => [Status.ok(), session]);
    const events: unknown[] = [];
    const status = await shipyard.monitoring(async (event) => {
        events.push(event);
        return false;
    });
    expect(status.is_ok()).toBe(true);
    expect(events).toEqual([{ case: "idle" }]);
});

test("build_ship accepts BuildStarted as the successful first response", async () => {
    const { session } = fake_session([
        shipyard_message({
            case: "buildStarted",
            value: { blueprintName: "Ship/Scout", shipName: "Eye" },
        }),
        shipyard_message({
            case: "buildingReport",
            value: { status: msg.IShipyard_Status.BUILD_IN_PROGRESS, progress: 0.5 },
        }),
        shipyard_message({
            case: "buildingComplete",
            value: { shipName: "Eye", slotId: 9 },
        }),
    ]);
    const shipyard = new MidlevelShipyard(async () => [Status.ok(), session]);
    const progress: number[] = [];

    const [status, ship] = await shipyard.build_ship("Ship/Scout", "Eye", async (_, value) => {
        progress.push(value);
    });
    expect(status.is_ok()).toBe(true);
    expect(ship).toEqual({ ship_name: "Eye", slot_id: 9 });
    expect(progress).toEqual([0.5]);
});
