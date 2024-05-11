import * as msg from "../Protocol_pb.js";
import { Status } from "../types/status.js";
import { Session } from "./session.js";


export type ModuleInfo = {
    slot_id: number;
    module_type: string;
    module_name: string;
}

export type Update = {
    module_attached?: ModuleInfo;
    module_detached?: number;
}

export class Commutator {

    constructor(private session: Session) {}

    async send_total_slots_request(): Promise<Status> {
        const request = new msg.ICommutator();
        request.choice.case = "totalSlotsReq";
        request.choice.value = true;
        return await this.send(request);
    }

    async send_module_info_request(slot_id: number): Promise<Status> {
        const request = new msg.ICommutator();
        request.choice.case = "moduleInfoReq";
        request.choice.value = slot_id;
        return await this.send(request);
    }

    async send_all_modules_info_request(): Promise<Status> {
        const request = new msg.ICommutator();
        request.choice.case = "allModulesInfoReq";
        request.choice.value = true;
        return await this.send(request);
    }

    async send_open_tunnel_request(slot_id: number): Promise<Status> {
        const request = new msg.ICommutator();
        request.choice.case = "openTunnel";
        request.choice.value = slot_id;
        return await this.send(request);
    }

    async send_close_tunnel_request(slot_id: number): Promise<Status> {
        const request = new msg.ICommutator();
        request.choice.case = "closeTunnel";
        request.choice.value = slot_id;
        return await this.send(request);
    }

    async send_start_monitoring_request(): Promise<Status> {
        const request = new msg.ICommutator();
        request.choice.case = "monitor";
        request.choice.value = true;
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
        return [Status.ok(), {
            slot_id: response.choice.value.slotId,
            module_type: response.choice.value.moduleType,
            module_name: response.choice.value.moduleName
        }];
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

    async wait_update(timeout: number = 500): Promise<[Status, Update | undefined]> {
        const [status, response] = await this.wait(timeout);
        if (status.is_timeout()) {
            // Just no updates ¯\_(ツ)_/¯
            return [Status.ok(), undefined];
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
                module_attached: {
                    slot_id: update.choice.value.slotId,
                    module_type: update.choice.value.moduleType,
                    module_name: update.choice.value.moduleName
                }
            }];
        } else if (update.choice.case == "moduleDetached") {
            return [Status.ok(), { module_detached: update.choice.value }];
        } else {
            return [Status.fail(`unexpected update ${update.choice.case}`), undefined];
        }
    }

    private async send(body: msg.ICommutator) {
        const message = new msg.Message();
        message.choice.case = "commutator";
        message.choice.value = body;
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