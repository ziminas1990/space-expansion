import { Status } from "../types/status.js";
import * as msg from "../Protocol_pb.js";
import { RootSession } from "./root_session.js";
import { Session } from "./session.js";


export class Commutator {
    constructor(private session: Session, private root_session: RootSession) {
        if (!this.root_session) {
            throw new Error("root_session is required");
        }
    }

    async send_total_slots_request() {
        const request = new msg.ICommutator();
        request.choice.case = "totalSlotsReq";
        request.choice.value = true;
        const message = new msg.Message();
        message.choice.case = "commutator";
        message.choice.value = request;
        return await this.session.send(message);
    }

    async wait_total_slots_response(timeout: number = 500)
    : Promise<[Status, number]>
    {
        const [status, response] = await this.session.wait(timeout);
        if (!status.is_ok() || !response) {
            return [status.wrap("no response"), 0];
        }
        if (response.choice.case != "commutator") {
            return [Status.fail(`got unexpected message ${response.choice.case}`), 0];
        }
        const commutator = response.choice.value;
        if (commutator.choice.case != "totalSlots") {
            return [Status.fail(`got unexpected response ${commutator.choice.case}`), 0];
        }
        return [Status.ok(), commutator.choice.value];
    }

}