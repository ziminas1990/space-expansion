import { create } from "@bufbuild/protobuf";
import * as msg from "../Protocol_pb.js";
import * as types from "../types/index.js";
import { Session } from "./session.js";


export type CelestialScannerStatus = "SUCCESS" | "SCANNER_BUSY";

export type CelestialScannerSpecification = {
    max_radius_km: number;
    processing_time_us: number;
}

export type CelestialScanningReport = {
    asteroids: types.PhysicalObject[];
    left: number;
}

export type CelestialScanningResult =
    | { case: "scanning_report"; report: CelestialScanningReport }
    | { case: "scanning_failed"; status: CelestialScannerStatus };

export class CelestialScanner {

    constructor(private session: Session) {}

    async send_specification_request(): Promise<types.Status> {
        const request = create(msg.ICelestialScannerSchema, {
            choice: { case: "specificationReq", value: true },
        });
        return this.send(request);
    }

    async wait_specification(timeout: number = 500)
    : Promise<[types.Status, CelestialScannerSpecification | undefined]>
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
            max_radius_km: spec.maxRadiusKm,
            processing_time_us: spec.processingTimeUs,
        }];
    }

    async send_scan_request(
        scanning_radius_km: number,
        minimal_radius_m: number): Promise<types.Status>
    {
        const request = create(msg.ICelestialScannerSchema, {
            choice: {
                case: "scan",
                value: {
                    scanningRadiusKm: scanning_radius_km,
                    minimalRadiusM: minimal_radius_m,
                },
            },
        });
        return this.send(request);
    }

    async wait_scanning_report(timeout: number = 500)
    : Promise<[types.Status, CelestialScanningResult | undefined]>
    {
        const [status, response, timestamp] = await this.wait(timeout);
        if (!status.is_ok() || !response) {
            return [status.wrap("no response"), undefined];
        }
        if (response.choice.case == "scanningFailed") {
            const server_status = celestialScannerStatusFromProtobuf(response.choice.value);
            if (!server_status) {
                return [types.Status.fail(`got unexpected status ${response.choice.value}`),
                        undefined];
            }
            return [types.Status.ok(), {
                case: "scanning_failed",
                status: server_status,
            }];
        }
        if (response.choice.case != "scanningReport") {
            return [types.Status.fail(`got unexpected message ${response.choice.case}`),
                    undefined];
        }
        const report = response.choice.value;
        return [types.Status.ok(), {
            case: "scanning_report",
            report: {
                asteroids: (report.asteroids ?? []).map(
                    (asteroid) => physicalObjectFromAsteroidInfo(asteroid, timestamp)),
                left: report.left,
            },
        }];
    }

    private async send(request: msg.ICelestialScanner): Promise<types.Status> {
        const message = create(msg.MessageSchema, {
            choice: { case: "celestialScanner", value: request },
        });
        return this.session.send(message);
    }

    private async wait(timeout_ms: number = 500)
    : Promise<[types.Status, msg.ICelestialScanner | undefined, bigint]>
    {
        const [status, response] = await this.session.wait(timeout_ms);
        if (!status.is_ok() || !response) {
            return [status.wrap("no response"), undefined, BigInt(0)];
        }
        if (response.choice.case != "celestialScanner") {
            return [types.Status.fail(`got unexpected message ${response.choice.case}`),
                    undefined, BigInt(0)];
        }
        return [types.Status.ok(), response.choice.value, types.asUint64(response.timestamp)];
    }

}

function celestialScannerStatusFromProtobuf(
    value: msg.ICelestialScanner_Status): CelestialScannerStatus | undefined
{
    switch (value) {
        case msg.ICelestialScanner_Status.SUCCESS: return "SUCCESS";
        case msg.ICelestialScanner_Status.SCANNER_BUSY: return "SCANNER_BUSY";
        default: return undefined;
    }
}

function physicalObjectFromAsteroidInfo(
    asteroid: msg.ICelestialScanner_AsteroidInfo,
    timestamp: bigint): types.PhysicalObject
{
    return {
        object_type: "asteroid",
        object_id: asteroid.id,
        radius: asteroid.r,
        position: types.positionFromKinematics(asteroid, timestamp),
    };
}
