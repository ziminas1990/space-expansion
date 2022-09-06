
type Token = {
  user: string,
  token: string
}

type WebSocketEvent = keyof WebSocketEventMap;

export class Connection {
  private token?: Token = undefined
  private socket?: WebSocket = undefined

  open(host: string, established_cb: (this: WebSocket, ev: Event) => any) {
    this.token = this.retrieve_token()
    if (this.token == null) {
      return false
    }

    let url = "ws://" + host + "/ws/" + this.token.user + "/" + this.token.token
    this.socket = new WebSocket(url);
    this.socket.addEventListener("open", established_cb)
    return true;
  }

  // Send an HTTP get request to "/token" to get a token
  retrieve_token(): Token | undefined {
    let request = new XMLHttpRequest();
    request.open('GET', '/token', false);  // `false` makes the request synchronous
    request.send(null);

    if (request.status == 200) {
      return JSON.parse(request.responseText)
    } else {
      return undefined
    }
  }

  // Just a wrapper over socket API
  addEventListener<event extends keyof WebSocketEventMap>(
    event: WebSocketEvent,
    callback: (this: WebSocket, ev: WebSocketEventMap[event]) => any): boolean
  {
    if (this.socket !== undefined) {
      this.socket.addEventListener(event, callback);
      return true;
    }
    return false;
  }
}