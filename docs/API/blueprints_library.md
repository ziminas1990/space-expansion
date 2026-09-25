# Blueprints Library — the IBlueprintsLibrary interface

Previous topic: [Asteroid Miner](./asteroid_miner.md)
Next topic: [Shipyard](./shipyard.md)

## What Is Blueprint

A blueprint describes an object that can be produced. It describes a single
module, including that module's parameters, or a whole ship.

Blueprints are used to build new ships. Building ships is covered in the
[Shipyard](./shipyard.md) document.

A blueprints library is a virtual player module. Through it the client can
read the blueprints available to that player: their names, the properties of
the object each blueprint produces, and the resources required to produce it.

A blueprint name has the form `Class/Type`. The class is a module type from
the [module table](./modules_table.md). The type is the name of that particular blueprint. Examples are `RCS/civilian-engine` and `Ship/Tiny-Scout`.

## How to connect to the blueprints library

A module of type `"BlueprintsLibrary"` with the name `"Central"` is always
connected to the player's [commutator](./commutator.md), and there is exactly
one. The module implements the `IBlueprintsLibrary` interface, which has two
commands:

- `blueprints_list_req` — request blueprint names
- `blueprint_req` — request one blueprint

## The blueprints_list_req command

`blueprints_list_req` requests blueprint names. The command is a string. The
server returns every blueprint whose full name begins with that string. An
empty string selects every blueprint. The comparison is case-sensitive, so
`"rcs"` does not select `"RCS/civilian-engine"`.

The server replies with one or more `blueprints_list` messages:

- `names` — blueprint names
- `left` — how many matching names are still to be sent after this message

The client keeps reading until a message whose `left` is `0`. When no name
matches, that single message has `left` equal to `0` and an empty `names`
list.

For example, the names that begin with `"RCS"`:

```json
{
  "tunnelId": 1100,
  "timestamp": 0,
  "blueprints_library": {
    "blueprints_list_req": "RCS"
  }
}
```

The server replies:

```json
{
  "tunnelId": 1100,
  "timestamp": 238221334,
  "blueprints_library": {
    "blueprints_list": {
      "left": 0,
      "names": [
        "RCS/titanic-engine",
        "RCS/civilian-engine",
        "RCS/ancient-nordic-engine"
      ]
    }
  }
}
```

## The blueprint_req command

`blueprint_req` requests one blueprint. The command is the blueprint's full
name, including the class, the `/`, and the type.

On success the server replies with one `blueprint` message:

- `name` — the same full name as in the request
- `properties` — the parameters of the object this blueprint produces
- `expenses` — the resources required to produce it

On failure the server replies with `blueprint_fail` and the status
`BLUEPRINT_NOT_FOUND`. That is the reply when the library has no blueprint
with that name, and also when the string is not a full name. A string with no
`/` has no type, so it is not a full name.

### Properties

Each property has a `name`. A single parameter also has `value`. A group of
parameters has `nested` properties instead, and each nested property has the
same shape.

`value` is always a string. A numeric parameter is sent as its decimal
representation, so a thrust of 500000 newtons is the string `"500000"`.

`expenses` is a separate field. Those resources are not repeated among
`properties`.

A module blueprint's properties are the parameters of that module. An RCS module
has `max_thrust`. A resource container has `volume`. An asteroid scanner has
`max_scanning_distance` and `scanning_time_ms`. The meaning of each parameter
is the same as in that module's documentation.

A ship blueprint has these properties:

- `radius` — the ship's size, in meters. It is the hull radius.
- `weight` — the ship's mass, in kilograms
- `max_rotation_speed` — the fastest the ship can turn, in radians per
  second.
- `modules` — the modules installed on the ship. Each nested property's `name`
  is the module's name on the ship, and its `value` is that module's blueprint
  name

### Expenses

Each item in `expenses` has `type` and `amount`.

Material resources are a mass, in kilograms. Their types and densities are
listed in [Resources](./reference_info.md#resources). `RESOURCE_LABOR` is the
unitless amount of work required to produce the object.

For a module blueprint, `expenses` is the cost of that module. For a ship
blueprint, `expenses` is the sum of the ship's own cost and the cost of every
module blueprint named in `modules`.

For example, an RCS blueprint:

```json
{
  "tunnelId": 1101,
  "timestamp": 0,
  "blueprints_library": {
    "blueprint_req": "RCS/civilian-engine"
  }
}
```

The server replies:

```json
{
  "tunnelId": 1101,
  "timestamp": 238300000,
  "blueprints_library": {
    "blueprint": {
      "name": "RCS/civilian-engine",
      "properties": [
        { "name": "max_thrust", "value": "500000" }
      ],
      "expenses": [
        { "type": "RESOURCE_LABOR", "amount": 100 },
        { "type": "RESOURCE_METALS", "amount": 1000 }
      ]
    }
  }
}
```

A ship blueprint includes its modules and their cost. `Ship/Tiny-Scout` has a
hull that costs 100 labor, an `AsteroidScanner/tiny-scanner` that costs 10
labor, and an `RCS/ancient-nordic-engine` that costs 10 labor. The reported
expenses are the sum, 120 labor:

```json
{
  "tunnelId": 1101,
  "timestamp": 238400000,
  "blueprints_library": {
    "blueprint": {
      "name": "Ship/Tiny-Scout",
      "properties": [
        { "name": "weight", "value": "10000" },
        { "name": "radius", "value": "30" },
        { "name": "max_rotation_speed", "value": "1" },
        {
          "name": "modules",
          "nested": [
            {
              "name": "asteroid-scanner",
              "value": "AsteroidScanner/tiny-scanner"
            },
            {
              "name": "rcs",
              "value": "RCS/ancient-nordic-engine"
            }
          ]
        }
      ],
      "expenses": [
        { "type": "RESOURCE_LABOR", "amount": 120 },
        { "type": "RESOURCE_METALS", "amount": 10000 }
      ]
    }
  }
}
```
