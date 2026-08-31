export enum ResourceType {
    Metals = "metals",
    Silicates = "silicates",
    Ice = "ice",
    Stones = "stones",
    Labor = "labor",
}

export type ResourceAmounts = Partial<Record<ResourceType, number>>;

export class ResourcesList {
    readonly resources: ResourceAmounts;

    constructor(resources: ResourceAmounts = {}) {
        this.resources = { ...resources };
    }

    set(resourceType: ResourceType, value: number): this {
        this.resources[resourceType] = value;
        return this;
    }

    add(other: ResourcesList): this {
        for (const [resourceType, amount] of other.entries()) {
            this.resources[resourceType] = (this.resources[resourceType] ?? 0) +
                amount;
        }
        return this;
    }

    multiply(multiplier: number): ResourcesList {
        const result = new ResourcesList();
        for (const [resourceType, amount] of this.entries()) {
            result.set(resourceType, amount * multiplier);
        }
        return result;
    }

    contains(other: ResourcesList): boolean {
        for (const [resourceType, amount] of other.entries()) {
            if (resourceType === ResourceType.Labor) {
                continue;
            }
            if ((this.resources[resourceType] ?? 0) < amount) {
                return false;
            }
        }
        return true;
    }

    verify(): void {}

    toPod(): Record<string, number> {
        this.verify();
        return Object.fromEntries(
            this.entries().filter(([, amount]) => amount > 0),
        );
    }

    protected entries(): Array<[ResourceType, number]> {
        return Object.entries(this.resources) as Array<[ResourceType, number]>;
    }
}

export class PhysicalResources extends ResourcesList {
    override verify(): void {
        if (this.resources[ResourceType.Labor] !== undefined) {
            throw new Error("Physical resources cannot contain labor");
        }
    }
}
