import { create } from "@bufbuild/protobuf";
import * as msg from "#sdk/Protocol_pb.js";
import * as types from "#sdk/types/index.js";
import { Session } from "./session.js";


export type AsteroidScannerStatus =
    | "IN_PROGRESS"
    | "SCANNER_BUSY"
    | "ASTEROID_TOO_FAR";

export type AsteroidScannerSpecification = {
    max_distance: number;
    scanning_time_ms: number;
}

export type AsteroidScanResult = {
    asteroid_id: number;
    weight: number;
    metals_percent: number;
    ice_percent: number;
    silicates_percent: number;
}

export class AsteroidScanner {

    constructor(private session: Session) {}

    async send_specification_request(): Promise<types.Status> {
        const request = create(msg.IAsteroidScannerSchema, {
            choice: { case: "specificationReq", value: true },
        });
        return this.send(request);
    }

    async wait_specification(timeout: number = 500)
    : Promise<[types.Status, AsteroidScannerSpecification | undefined]>
    {
        const [status, response] = await this.wait(timeout);
        if (!status.is_ok() || !response) {
            return [status.wrap("no response"), undefined];
        }
        if (response.choice.case != "specification") {
            return [types.Status.fail(`got unexpected message ${response.choice.case}`),
                    undefined];
        }
        const spec = response.choice.value;
        return [types.Status.ok(), {
            max_distance: spec.maxDistance,
            scanning_time_ms: spec.scanningTimeMs,
        }];
    }

    async send_scan_asteroid(asteroid_id: number): Promise<types.Status> {
        const request = create(msg.IAsteroidScannerSchema, {
            choice: { case: "scanAsteroid", value: asteroid_id },
        });
        return this.send(request);
    }

    async wait_scanning_status(timeout: number = 500)
    : Promise<[types.Status, AsteroidScannerStatus | undefined]>
    {
        const [status, response] = await this.wait(timeout);
        if (!status.is_ok() || !response) {
            return [status.wrap("no response"), undefined];
        }
        if (response.choice.case != "scanningStatus") {
            return [types.Status.fail(`got unexpected message ${response.choice.case}`),
                    undefined];
        }
        const server_status = asteroidScannerStatusFromProtobuf(response.choice.value);
        if (!server_status) {
            return [types.Status.fail(`got unexpected status ${response.choice.value}`),
                    undefined];
        }
        return [types.Status.ok(), server_status];
    }

    async wait_scanning_finished(timeout: number = 500)
    : Promise<[types.Status, AsteroidScanResult | undefined]>
    {
        const [status, response] = await this.wait(timeout);
        if (!status.is_ok() || !response) {
            return [status.wrap("no response"), undefined];
        }
        if (response.choice.case != "scanningFinished") {
            return [types.Status.fail(`got unexpected message ${response.choice.case}`),
                    undefined];
        }
        const result = response.choice.value;
        return [types.Status.ok(), {
            asteroid_id: result.asteroidId,
            weight: result.weight,
            metals_percent: result.metalsPercent,
            ice_percent: result.icePercent,
            silicates_percent: result.silicatesPercent,
        }];
    }

    private async send(request: msg.IAsteroidScanner): Promise<types.Status> {
        const message = create(msg.MessageSchema, {
            choice: { case: "asteroidScanner", value: request },
        });
        return this.session.send(message);
    }

    private async wait(timeout_ms: number = 500)
    : Promise<[types.Status, msg.IAsteroidScanner | undefined]>
    {
        const [status, response] = await this.session.wait(timeout_ms);
        if (!status.is_ok() || !response) {
            return [status.wrap("no response"), undefined];
        }
        if (response.choice.case != "asteroidScanner") {
            return [types.Status.fail(`got unexpected message ${response.choice.case}`),
                    undefined];
        }
        return [types.Status.ok(), response.choice.value];
    }

}

function asteroidScannerStatusFromProtobuf(
    value: msg.IAsteroidScanner_Status): AsteroidScannerStatus | undefined
{
    switch (value) {
        case msg.IAsteroidScanner_Status.IN_PROGRESS: return "IN_PROGRESS";
        case msg.IAsteroidScanner_Status.SCANNER_BUSY: return "SCANNER_BUSY";
        case msg.IAsteroidScanner_Status.ASTEROID_TOO_FAR: return "ASTEROID_TOO_FAR";
        default: return undefined;
    }
}
