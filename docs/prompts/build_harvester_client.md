# The Harvester Client

The Harvester client is an application that connects to the game server and
fully automates resource gathering. Its job is to collect and stockpile
resources as efficiently as possible.

To gather resources, the Harvester must take control of ships that have
modules for collecting resources from asteroids. It must also build new ships
so that resource gathering speeds up over time.

The application must read `$CWS/harvester.json`, which has the following structure:

```json
{
    "ip": "127.0.0.1",
    "port": 6842,
    "username": "Olenoid",
    "password": "admin"
}
```

These values must be used to connect to the game server.
