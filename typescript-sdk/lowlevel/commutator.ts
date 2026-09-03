import { create } from "@bufbuild/protobuf";
import * as msg from "../Protocol_pb.js";
import { Status } from "../types/status.js";
import { Session } from "./session.js";


export type ModuleInfo = {
    slot_id: number;
    module_type: string;
    module_name: string;
    blueprint_name: string;
}

export type Update = {
    module_attached?: ModuleInfo;
    module_detached?: number;
}

function parse_module_info(info: {
    slotId: number;
    moduleType: string;
    moduleName: string;
    blueprintName: string;
}): ModuleInfo {
    return {
        slot_id: info.slotId,
        module_type: info.moduleType,
        module_name: info.moduleName,
        blueprint_name: info.blueprintName,
    };
}

export class Commutator {

    constructor(private session: Session) {}

    // Binds a tunnel id from openTunnelReport to a Session on this connection.
    enable_tunnel(session_id: number): [Status, Session | undefined] {
        return this.session.register_session(session_id);
    }

    async send_total_slots_request(): Promise<Status> {
        const request = create(msg.ICommutatorSchema, {
            choice: { case: "totalSlotsReq", value: true },
        });
        return await this.send(request);
    }

    async send_module_info_request(slot_id: number): Promise<Status> {
        const request = create(msg.ICommutatorSchema, {
            choice: { case: "moduleInfoReq", value: slot_id },
        });
        return await this.send(request);
    }

    async send_all_modules_info_request(): Promise<Status> {
        const request = create(msg.ICommutatorSchema, {
            choice: { case: "allModulesInfoReq", value: true },
        });
        return await this.send(request);
    }

    async send_open_tunnel_request(slot_id: number): Promise<Status> {
        const request = create(msg.ICommutatorSchema, {
            choice: { case: "openTunnel", value: slot_id },
        });
        return await this.send(request);
    }

    async send_close_tunnel_request(slot_id: number): Promise<Status> {
        const request = create(msg.ICommutatorSchema, {
            choice: { case: "closeTunnel", value: slot_id },
        });
        return await this.send(request);
    }

    async send_start_monitoring_request(): Promise<Status> {
        const request = create(msg.ICommutatorSchema, {
            choice: { case: "monitor", value: true },
        });
        return await this.send(request);
    }

    async wait_total_slots_response(timeout: number = 500)
    : Promise<[Status, number]>
    {
        const [status, response] = await this.wait(timeout);
        if (!status.is_ok() || !response) {
            return [status.wrap("no response"), 0];
        }
        if (response.choice.case != "totalSlots") {
            return [Status.fail(`got unexpected response ${response.choice.case}`), 0];
        }
        return [Status.ok(), response.choice.value];
    }

    async wait_module_info_response(timeout: number = 500)
    : Promise<[Status, ModuleInfo | undefined]>
    {
        const [status, response] = await this.wait(timeout);
        if (!status.is_ok() || !response) {
            return [status.wrap("no response"), undefined];
        }
        if (response.choice.case != "moduleInfo") {
            return [Status.fail(`got unexpected response ${response.choice.case}`), undefined];
        }
        return [Status.ok(), parse_module_info(response.choice.value)];
    }

    async wait_open_tunnel_report(timeout: number = 500)
    : Promise<[Status, number | undefined]>
    {
        const [status, response] = await this.wait(timeout);
        if (!status.is_ok() || !response) {
            return [status.wrap("no response"), undefined];
        }
        if (response.choice.case == "openTunnelFailed") {
            return [Status.fail(`open tunnel failed: ${response.choice.value}`), undefined];
        }
        if (response.choice.case != "openTunnelReport") {
            return [Status.fail(`got unexpected response ${response.choice.case}`), undefined];
        }
        return [Status.ok(), response.choice.value];
    }

    async wait_close_tunnel_status(timeout: number = 500): Promise<Status> {
        const [status, response] = await this.wait(timeout);
        if (!status.is_ok() || !response) {
            return status.wrap("no response");
        }
        if (response.choice.case != "closeTunnelStatus") {
            return Status.fail(`got unexpected response ${response.choice.case}`);
        }
        const close_status = response.choice.value;
        return close_status == msg.ICommutator_Status.SUCCESS
            ? Status.ok()
            : Status.fail(`close tunnel failed: ${close_status}`);
    }

    async wait_monitor_ack(timeout: number = 500): Promise<Status> {
        const [status, response] = await this.wait(timeout);
        if (!status.is_ok() || !response) {
            return status.wrap("no response");
        }
        if (response.choice.case != "monitorAck") {
            return Status.fail(`got unexpected response ${response.choice.case}`);
        }
        const monitor_status = response.choice.value;
        return monitor_status == msg.ICommutator_Status.SUCCESS
            ? Status.ok()
            : Status.fail(`monitoring rejected: ${monitor_status}`);
    }

    // Waits for a commutator update. Completes on an update, session close
    // (closedInd), or timeout — all three are valid. timeout_ms <= 0 waits
    // until a message arrives or the session is closed.
    async wait_update(timeout_ms: number = 500): Promise<[Status, Update | undefined]> {
        const [status, response] = await this.wait(timeout_ms);
        if (status.is_timeout()) {
            return [status, undefined];
        }
        if (!status.is_ok() || !response) {
            return [status.wrap("no response"), undefined];
        }
        if (response.choice.case != "update") {
            return [Status.fail(`got unexpected response ${response.choice.case}`), undefined];
        }
        const update = response.choice.value;
        if (update.choice.case == "moduleAttached") {
            return [Status.ok(), {
                module_attached: parse_module_info(update.choice.value)
            }];
        } else if (update.choice.case == "moduleDetached") {
            return [Status.ok(), { module_detached: update.choice.value }];
        } else {
            return [Status.fail(`unexpected update ${update.choice.case}`), undefined];
        }
    }

    private async send(body: msg.ICommutator) {
        const message = create(msg.MessageSchema, {
            choice: { case: "commutator", value: body },
        });
        return await this.session.send(message);
    }

    private async wait(timeout_ms: number = 500):
    Promise<[Status, msg.ICommutator | undefined]>
    {
        const [status, response] = await this.session.wait(timeout_ms);
        if (!status.is_ok() || !response) {
            return [status.wrap("no response"), undefined];
        }
        if (response.choice.case != "commutator") {
            return [Status.fail(`got unexpected message ${response.choice.case}`), undefined];
        }
        return [Status.ok(), response.choice.value];
    }

}