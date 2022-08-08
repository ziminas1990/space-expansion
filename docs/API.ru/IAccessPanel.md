# IAccessPanel
**IAccessPanel** interface provides an authentication mechanism:
```protobuf
message IAccessPanel {

  message LoginRequest {
    string login    = 1;
    string password = 2;
  }

  message AccessGranted {
    uint32 port       = 1;
    uint32 session_id = 2;  
  }
  
  oneof choice {
    LoginRequest login     = 1;
    
    AccessGranted access_granted  = 21;
    string        access_rejected = 22;
  }
}
```
Interfaces defines a single `login` request and two possible responses: `access_granted` and `access_rejected`.

In order to login, player should send a `login` request to a **login UDP port** on server. This port is configured by server administrator. A `login` request has the following fields:
1. `login` - player's login;
2. `password` - player's password.

If login/password pair is correct, server sends a `access_granted` response, that has the following fields:

1. `port` a UDP port, on which client should send all further requests
2. `session_id` - a root session id (it will be explained later).

Note that player MUST use **the same** socket to send all further requests, since server stores `ip:port` pair, that was used to send `login` request and will ignore any messages, sent from another `ip:port`.

If the login/password pair is wrong or any other problem has occurred, an `access_rejected` response will be sent to player. It carries a string, that describes the problem.
