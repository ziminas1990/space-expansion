
class Connection {
  constructor() {
    this.token = null
    this.socket = null
  }

  open(host, established_cb) {
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
  retrieve_token() {
    let request = new XMLHttpRequest();
    request.open('GET', '/token', false);  // `false` makes the request synchronous
    request.send(null);

    if (request.status == 200) {
      return JSON.parse(request.responseText)
    } else {
      return null
    }
  }

  // Just a wrapper over socket API
  addEventListener(event, callback) {
    this.socket.addEventListener(event, callback)
  }
}