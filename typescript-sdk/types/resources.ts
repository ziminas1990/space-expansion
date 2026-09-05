import { create } from "@bufbuild/protobuf";
import * as proto from "#sdk/CommonTypes_pb.js";

export type ResourceType =
    | "unknown"
    | "metals"
    | "silicates"
    | "ice"
    | "stone"
    | "labor";

export type ResourceItem = {
    resource_type: ResourceType;
    amount: number;
};

export function resourceTypeFromProtobuf(value: proto.ResourceType): ResourceType {
    switch (value) {
        case proto.ResourceType.RESOURCE_METALS: return "metals";
        case proto.ResourceType.RESOURCE_SILICATES: return "silicates";
        case proto.ResourceType.RESOURCE_ICE: return "ice";
        case proto.ResourceType.RESOURCE_STONE: return "stone";
        case proto.ResourceType.RESOURCE_LABOR: return "labor";
        default: return "unknown";
    }
}

export function resourceTypeToProtobuf(value: ResourceType): proto.ResourceType {
    switch (value) {
        case "metals": return proto.ResourceType.RESOURCE_METALS;
        case "silicates": return proto.ResourceType.RESOURCE_SILICATES;
        case "ice": return proto.ResourceType.RESOURCE_ICE;
        case "stone": return proto.ResourceType.RESOURCE_STONE;
        case "labor": return proto.ResourceType.RESOURCE_LABOR;
        case "unknown": return proto.ResourceType.RESOURCE_UNKNOWN;
    }
}

export function resourceItemFromProtobuf(item: proto.ResourceItem): ResourceItem {
    return {
        resource_type: resourceTypeFromProtobuf(item.type),
        amount: item.amount,
    };
}

export function resourceItemToProtobuf(item: ResourceItem): proto.ResourceItem {
    return create(proto.ResourceItemSchema, {
        type: resourceTypeToProtobuf(item.resource_type),
        amount: item.amount,
    });
}

export function resourceItemsFromProtobuf(
    items: readonly proto.ResourceItem[] | undefined): ResourceItem[]
{
    return (items ?? []).map(resourceItemFromProtobuf);
}
