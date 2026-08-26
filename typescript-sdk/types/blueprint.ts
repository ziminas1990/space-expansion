import { create } from "@bufbuild/protobuf";
import * as proto from "../CommonTypes_pb.js";
import {
    ResourceItem,
    resourceItemFromProtobuf,
    resourceItemToProtobuf,
} from "./resources.js";

export type Property = {
    name: string;
    value: string;
    nested: Property[];
};

export type Blueprint = {
    name: string;
    properties: Property[];
    expenses: ResourceItem[];
};

export function propertyFromProtobuf(property: proto.Property): Property {
    return {
        name: property.name,
        value: property.value,
        nested: property.nested.map(propertyFromProtobuf),
    };
}

export function propertyToProtobuf(property: Property): proto.Property {
    return create(proto.PropertySchema, {
        name: property.name,
        value: property.value,
        nested: property.nested.map(propertyToProtobuf),
    });
}

export function blueprintFromProtobuf(blueprint: proto.Blueprint): Blueprint {
    return {
        name: blueprint.name,
        properties: blueprint.properties.map(propertyFromProtobuf),
        expenses: blueprint.expenses.map(resourceItemFromProtobuf),
    };
}

export function blueprintToProtobuf(blueprint: Blueprint): proto.Blueprint {
    return create(proto.BlueprintSchema, {
        name: blueprint.name,
        properties: blueprint.properties.map(propertyToProtobuf),
        expenses: blueprint.expenses.map(resourceItemToProtobuf),
    });
}
