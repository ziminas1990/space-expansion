import { create } from "@bufbuild/protobuf";
import * as msg from "../Protocol_pb.js";
import * as types from "../types/index.js";
import { Session } from "./session.js";


export type ShipyardStatus =
    | "SUCCESS"
    | "INTERNAL_ERROR"
    | "CARGO_NOT_FOUND"
    | "SHIPYARD_IS_BUSY"
    | "BUILD_STARTED"
    | "BUILD_IN_PROGRESS"
    | "BUILD_COMPLETE"
    | "BUILD_CANCELED"
    | "BUILD_FROZEN"
    | "BUILD_FAILED"
    | "BLUEPRINT_NOT_FOUND";

export type ShipyardSpecification = {
    labor_per_sec: number;
}

export type ShipyardBuildingReport = {
    status: ShipyardStatus;
    progress: number;
}

export type ShipyardShipBuilt = {
    ship_name: string;
    slot_id: number;
}

export type ShipyardBuildingEvent =
    | { case: "building_report"; report: ShipyardBuildingReport }
    | { case: "building_complete"; ship: ShipyardShipBuilt };

export class Shipyard {

    constructor(private session: Session) {}

    async send_specification_request(): Promise<types.Status> {
        const request = create(msg.IShipyardSchema, {
            choice: { case: "specificationReq", value: true },
        });
        return this.send(request);
    }

    async wait_specification(timeout: number = 500)
    : Promise<[types.Status, ShipyardSpecification | undefined]>
    {
        const [status, response] = await this.wait(timeout);
        if (!status.is_ok() || !response) {
            return [status.wrap("no response"), undefined];
        }
        if (response.choice.case != "specification") {
            return [types.Status.fail(`got unexpected message ${response.choice.case}`),
                    undefined];
        }
        return [types.Status.ok(), {
            labor_per_sec: response.choice.value.laborPerSec,
        }];
    }

    async send_bind_to_cargo(cargo_name: string): Promise<types.Status> {
        const request = create(msg.IShipyardSchema, {
            choice: { case: "bindToCargo", value: cargo_name },
        });
        return this.send(request);
    }

    async wait_bind_to_cargo_status(timeout: number = 500)
    : Promise<[types.Status, ShipyardStatus | undefined]>
    {
        const [status, response] = await this.wait(timeout);
        if (!status.is_ok() || !response) {
            return [status.wrap("no response"), undefined];
        }
        if (response.choice.case != "bindToCargoStatus") {
            return [types.Status.fail(`got unexpected message ${response.choice.case}`),
                    undefined];
        }
        const server_status = shipyardStatusFromProtobuf(response.choice.value);
        if (!server_status) {
            return [types.Status.fail(`got unexpected status ${response.choice.value}`),
                    undefined];
        }
        return [types.Status.ok(), server_status];
    }

    async send_start_build(blueprint_name: string, ship_name: string): Promise<types.Status> {
        const request = create(msg.IShipyardSchema, {
            choice: {
                case: "startBuild",
                value: {
                    blueprintName: blueprint_name,
                    shipName: ship_name,
                },
            },
        });
        return this.send(request);
    }

    async send_cancel_build(): Promise<types.Status> {
        const request = create(msg.IShipyardSchema, {
            choice: { case: "cancelBuild", value: true },
        });
        return this.send(request);
    }

    async wait_building_event(timeout: number = 1000)
    : Promise<[types.Status, ShipyardBuildingEvent | undefined]>
    {
        const [status, response] = await this.wait(timeout);
        if (!status.is_ok() || !response) {
            return [status.wrap("no response"), undefined];
        }
        if (response.choice.case == "buildingComplete") {
            const ship = response.choice.value;
            return [types.Status.ok(), {
                case: "building_complete",
                ship: {
                    ship_name: ship.shipName,
                    slot_id: ship.slotId,
                },
            }];
        }
        if (response.choice.case != "buildingReport") {
            return [types.Status.fail(`got unexpected message ${response.choice.case}`),
                    undefined];
        }
        const report = response.choice.value;
        const server_status = shipyardStatusFromProtobuf(report.status);
        if (!server_status) {
            return [types.Status.fail(`got unexpected status ${report.status}`),
                    undefined];
        }
        return [types.Status.ok(), {
            case: "building_report",
            report: {
                status: server_status,
                progress: report.progress,
            },
        }];
    }

    private async send(request: msg.IShipyard): Promise<types.Status> {
        const message = create(msg.MessageSchema, {
            choice: { case: "shipyard", value: request },
        });
        return this.session.send(message);
    }

    private async wait(timeout_ms: number = 500)
    : Promise<[types.Status, msg.IShipyard | undefined]>
    {
        const [status, response] = await this.session.wait(timeout_ms);
        if (!status.is_ok() || !response) {
            return [status.wrap("no response"), undefined];
        }
        if (response.choice.case != "shipyard") {
            return [types.Status.fail(`got unexpected message ${response.choice.case}`),
                    undefined];
        }
        return [types.Status.ok(), response.choice.value];
    }

}

function shipyardStatusFromProtobuf(
    value: msg.IShipyard_Status): ShipyardStatus | undefined
{
    switch (value) {
        case msg.IShipyard_Status.SUCCESS: return "SUCCESS";
        case msg.IShipyard_Status.INTERNAL_ERROR: return "INTERNAL_ERROR";
        case msg.IShipyard_Status.CARGO_NOT_FOUND: return "CARGO_NOT_FOUND";
        case msg.IShipyard_Status.SHIPYARD_IS_BUSY: return "SHIPYARD_IS_BUSY";
        case msg.IShipyard_Status.BUILD_STARTED: return "BUILD_STARTED";
        case msg.IShipyard_Status.BUILD_IN_PROGRESS: return "BUILD_IN_PROGRESS";
        case msg.IShipyard_Status.BUILD_COMPLETE: return "BUILD_COMPLETE";
        case msg.IShipyard_Status.BUILD_CANCELED: return "BUILD_CANCELED";
        case msg.IShipyard_Status.BUILD_FROZEN: return "BUILD_FROZEN";
        case msg.IShipyard_Status.BUILD_FAILED: return "BUILD_FAILED";
        case msg.IShipyard_Status.BLUEPRINT_NOT_FOUND: return "BLUEPRINT_NOT_FOUND";
        default: return undefined;
    }
}
