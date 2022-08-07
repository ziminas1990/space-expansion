# API in general
Space-expansion API is based on [Google Protobuf](https://developers.google.com/protocol-buffers). It has two files:
1. [CommonTypes.proto](https://github.com/ziminas1990/space-expansion/blob/master/server/CommonTypes.proto) - describes some common types
2. [Protocol.proto](https://github.com/ziminas1990/space-expansion/blob/master/Protocol.proto) - describes all interfaces

Both files use ["Proto3 language guide"](https://developers.google.com/protocol-buffers/docs/proto3) specification. [UDP](https://en.wikipedia.org/wiki/User_Datagram_Protocol) protocol is used as a transport for exchanging messages with server.

Both client and server send only messages of type `Message`. It is a top-level message in API, and here is how it is specified:
```protobuf
message Message {
  uint32 tunnelId  = 1;
  uint64 timestamp = 2;

  oneof choice {
    // All possible interfaces are listed here
    ISessionControl    session            = 10;
    IAccessPanel       accessPanel        = 13;
    ICommutator        commutator         = 14;
    IShip              ship               = 15;
    INavigation        navigation         = 16;
    IEngine            engine             = 17;
    // .... and many other interfaces
  }
}
```
So, it just a container for all other types of messages (like `IShip` or `IEngine`) and has the following fields:
1. `tunnelid` - specifies a session, to which message belongs to (sessions will be described later)
2. `timestamp` - specifies an **in-game** time (in microseconds), when the message was sent from server, or when it should be handled by server (it depends on message's direction);
3. `choice` - contains a nested message; note, that since `choice` has `oneof` type, it can't carry two or more nested messages. It always carry just one of them.

All other messages, that starts from "I" (e.g. `IShip`, `IEngine` and etc) are called **interfaces**. Each module (ship, scanner, engine and etc) has it's own interface, that describes how this module can be controlled by player. Although each interface has its own specific features, they do share some common traits. Let's take a look on a typical interface:
```protobuf
message IAsteroidMiner {
  // Most interfaces has a `Status` enumeration to return error codes
  enum Status {
    SUCCESS               = 0;
    INTERNAL_ERROR        = 1;
    ASTEROID_DOESNT_EXIST = 2;
    MINER_IS_BUSY         = 3;
    // ... and other statuses
  }

  // If request or response has a complex type, it should be declared as follow:
  message Specification {
    uint32 max_distance    = 1;
    uint32 cycle_time_ms   = 2;
    uint32 yield_per_cycle = 3;
  }

	// A container for nested message (request or response)
  oneof choice {
    // First, there are requests and commands, that can be sent by player
    bool          specification_req = 1;
    string        bind_to_cargo     = 2;
    uint32        start_mining      = 3;
    bool          stop_mining       = 4;

    // Then there are responses and notifications, that can be sent by server
    Specification specification        = 21;
    Status        bind_to_cargo_status = 22;
    Status        start_mining_status  = 23;
    Resources     mining_report        = 24;
    Status        mining_is_stopped    = 25;
    Status        stop_mining_status   = 26;
  }
}
```
As you may see, typical interface has:
1. `Status` - an enumeration, that contains all possible problems, that can occur. In additional it usually provides `SUCCESS` status to indicate, that everything is fine.
2. **Inner types** - if some request or response has a complex type, this type should be also declared inside the interface; see the `Specification` type
3. `oneof choice` - a container with nested request or response; requests and commands, that could be sent by player, are declared first, followed by possible declarations of responses and indications, that could be sent by server.

Note, that almost each module has a `specification_req` request and a corresponding `specification` response. They are used to get information about module's parameters.
