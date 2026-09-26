import type { ResourceItem } from "@spx/sdk/types";
import { InstalledModule, type InstalledModulePacked } from "./module.js";

export type ContainerContent = {
    volume: number;
    used: number;
    resources: ResourceItem[];
};

function copy_content(content: ContainerContent): ContainerContent {
    return {
        volume: content.volume,
        used: content.used,
        resources: content.resources.map((resource) => ({ ...resource })),
    };
}

export class ResourceContainer extends InstalledModule {
    private content?: ContainerContent;

    constructor(slot_id: number, name: string) {
        super(slot_id, "ResourceContainer", name);
    }

    get_content(): ContainerContent | undefined {
        return this.content === undefined ? undefined : copy_content(this.content);
    }

    update_content(content: ContainerContent): void {
        this.content = copy_content(content);
    }

    override pack(): InstalledModulePacked {
        return this.content === undefined
            ? super.pack()
            : [this.slot_id, this.type, this.name, copy_content(this.content)];
    }
}
