import { create } from "@bufbuild/protobuf";
import * as msg from "../Protocol_pb.js";
import * as types from "../types/index.js";
import { Session } from "./session.js";


export type BlueprintsLibraryStatus =
    | "SUCCESS"
    | "INTERNAL_ERROR"
    | "BLUEPRINT_NOT_FOUND";

export type BlueprintsNamesPage = {
    names: string[];
    left: number;
}

export type BlueprintResult =
    | { case: "blueprint"; blueprint: types.Blueprint }
    | { case: "blueprint_fail"; status: BlueprintsLibraryStatus };

export class BlueprintsLibrary {

    constructor(private session: Session) {}

    async send_blueprints_list_request(start_with: string = ""): Promise<types.Status> {
        const request = create(msg.IBlueprintsLibrarySchema, {
            choice: { case: "blueprintsListReq", value: start_with },
        });
        return this.send(request);
    }

    async wait_blueprints_list(timeout: number = 500)
    : Promise<[types.Status, BlueprintsNamesPage | undefined]>
    {
        const [status, response] = await this.wait(timeout);
        if (!status.is_ok() || !response) {
            return [status.wrap("no response"), undefined];
        }
        if (response.choice.case != "blueprintsList") {
            return [types.Status.fail(`got unexpected message ${response.choice.case}`),
                    undefined];
        }
        const page = response.choice.value;
        return [types.Status.ok(), {
            names: page.names ?? [],
            left: page.left,
        }];
    }

    async send_blueprint_request(blueprint_name: string): Promise<types.Status> {
        const request = create(msg.IBlueprintsLibrarySchema, {
            choice: { case: "blueprintReq", value: blueprint_name },
        });
        return this.send(request);
    }

    async wait_blueprint(timeout: number = 500)
    : Promise<[types.Status, BlueprintResult | undefined]>
    {
        const [status, response] = await this.wait(timeout);
        if (!status.is_ok() || !response) {
            return [status.wrap("no response"), undefined];
        }
        if (response.choice.case == "blueprintFail") {
            const server_status = blueprintsLibraryStatusFromProtobuf(response.choice.value);
            if (!server_status) {
                return [types.Status.fail(`got unexpected status ${response.choice.value}`),
                        undefined];
            }
            return [types.Status.ok(), {
                case: "blueprint_fail",
                status: server_status,
            }];
        }
        if (response.choice.case != "blueprint") {
            return [types.Status.fail(`got unexpected message ${response.choice.case}`),
                    undefined];
        }
        return [types.Status.ok(), {
            case: "blueprint",
            blueprint: types.blueprintFromProtobuf(response.choice.value),
        }];
    }

    private async send(request: msg.IBlueprintsLibrary): Promise<types.Status> {
        const message = create(msg.MessageSchema, {
            choice: { case: "blueprintsLibrary", value: request },
        });
        return this.session.send(message);
    }

    private async wait(timeout_ms: number = 500)
    : Promise<[types.Status, msg.IBlueprintsLibrary | undefined]>
    {
        const [status, response] = await this.session.wait(timeout_ms);
        if (!status.is_ok() || !response) {
            return [status.wrap("no response"), undefined];
        }
        if (response.choice.case != "blueprintsLibrary") {
            return [types.Status.fail(`got unexpected message ${response.choice.case}`),
                    undefined];
        }
        return [types.Status.ok(), response.choice.value];
    }

}

function blueprintsLibraryStatusFromProtobuf(
    value: msg.IBlueprintsLibrary_Status): BlueprintsLibraryStatus | undefined
{
    switch (value) {
        case msg.IBlueprintsLibrary_Status.SUCCESS: return "SUCCESS";
        case msg.IBlueprintsLibrary_Status.INTERNAL_ERROR: return "INTERNAL_ERROR";
        case msg.IBlueprintsLibrary_Status.BLUEPRINT_NOT_FOUND: return "BLUEPRINT_NOT_FOUND";
        default: return undefined;
    }
}
