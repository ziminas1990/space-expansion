import * as lowlevel from "../lowlevel/index.js";
import { Status } from "../types/status.js";
import { Blueprint } from "../types/index.js";
import { BaseModule, OpenSessionCallback } from "./base_module.js";
import { ModuleType } from "./module_type.js";

export type BlueprintsLibraryStatus = lowlevel.BlueprintsLibraryStatus;

export class BlueprintsLibrary extends BaseModule<lowlevel.BlueprintsLibrary> {
    readonly type = ModuleType.BLUEPRINTS_LIBRARY;

    constructor(open_session_callback: OpenSessionCallback)
    {
        super(open_session_callback,
              async (session) => [Status.ok(), new lowlevel.BlueprintsLibrary(session)]);
    }

    async get_blueprints_list(start_with: string = "")
        : Promise<[Status, string[] | undefined]>
    {
        return await this.run(
            async (session) => this._get_blueprints_list(session, start_with));
    }

    async get_blueprint(name: string)
        : Promise<[Status, Blueprint | undefined]>
    {
        return await this.run(async (session) => this._get_blueprint(session, name));
    }

    private async _get_blueprints_list(
        session: lowlevel.BlueprintsLibrary,
        start_with: string)
        : Promise<[Status, string[] | undefined]>
    {
        const send_status = await session.send_blueprints_list_request(start_with);
        if (!send_status.is_ok()) {
            return [send_status, undefined];
        }

        const names: string[] = [];
        while (true) {
            const [status, page] = await session.wait_blueprints_list();
            if (!status.is_ok() || !page) {
                return [status.wrap("failed to get blueprints list page"), undefined];
            }
            names.push(...page.names);
            if (page.left === 0) {
                return [Status.ok(), names];
            }
        }
    }

    private async _get_blueprint(
        session: lowlevel.BlueprintsLibrary,
        name: string)
        : Promise<[Status, Blueprint | undefined]>
    {
        const send_status = await session.send_blueprint_request(name);
        if (!send_status.is_ok()) {
            return [send_status, undefined];
        }
        const [status, result] = await session.wait_blueprint();
        if (!status.is_ok() || !result) {
            return [status.wrap("failed to get blueprint"), undefined];
        }
        if (result.case === "blueprint_fail") {
            return [Status.fail(result.status), undefined];
        }
        return [Status.ok(), result.blueprint];
    }
}
