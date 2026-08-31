import { create } from "@bufbuild/protobuf";
import * as msg from "../Protocol_pb.js";
import * as types from "../types/index.js";
import { Session } from "./session.js";


export type PassiveScannerSpecification = {
    scanning_radius_km: number;
    max_update_time_ms: number;
}

export class PassiveScanner {

    constructor(private session: Session) {}

    async send_specification_request(): Promise<types.Status> {
        const request = create(msg.IPassiveScannerSchema, {
            choice: { case: "specificationReq", value: true },
        });
        return this.send(request);
    }

    async wait_specification(timeout: number = 500)
    : Promise<[types.Status, PassiveScannerSpecification | undefined]>
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
            scanning_radius_km: spec.scanningRadiusKm,
            max_update_time_ms: spec.maxUpdateTimeMs,
        }];
    }

    async send_monitor_request(): Promise<types.Status> {
        const request = create(msg.IPassiveScannerSchema, {
            choice: { case: "monitor", value: true },
        });
        return this.send(request);
    }

    async wait_monitor_ack(timeout: number = 500)
    : Promise<[types.Status, boolean | undefined]>
    {
        const [status, response] = await this.wait(timeout);
        if (!status.is_ok() || !response) {
            return [status.wrap("no response"), undefined];
        }
        if (response.choice.case != "monitorAck") {
            return [types.Status.fail(`got unexpected message ${response.choice.case}`),
                    undefined];
        }
        return [types.Status.ok(), response.choice.value];
    }

    async wait_update(timeout: number = 500)
    : Promise<[types.Status, types.PhysicalObject[] | undefined]>
    {
        const [status, response, timestamp] = await this.wait(timeout);
        if (status.is_timeout()) {
            return [status, undefined];
        }
        if (!status.is_ok() || !response) {
            return [status.wrap("no response"), undefined];
        }
        if (response.choice.case != "update") {
            return [types.Status.fail(`got unexpected message ${response.choice.case}`),
                    undefined];
        }
        const objects = (response.choice.value.items ?? []).map(
            (item) => types.physicalObjectFromProtobuf(item, timestamp));
        return [types.Status.ok(), objects];
    }

    private async send(request: msg.IPassiveScanner): Promise<types.Status> {
        const message = create(msg.MessageSchema, {
            choice: { case: "passiveScanner", value: request },
        });
        return this.session.send(message);
    }

    private async wait(timeout_ms: number = 500)
    : Promise<[types.Status, msg.IPassiveScanner | undefined, bigint]>
    {
        const [status, response] = await this.session.wait(timeout_ms);
        if (!status.is_ok() || !response) {
            return [status.wrap("no response"), undefined, BigInt(0)];
        }
        if (response.choice.case != "passiveScanner") {
            return [types.Status.fail(`got unexpected message ${response.choice.case}`),
                    undefined, BigInt(0)];
        }
        return [types.Status.ok(), response.choice.value, types.asUint64(response.timestamp)];
    }

}
