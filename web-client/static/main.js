
function set_status(status) {
  document.getElementById("status").innerHTML = status
}

function runApp() {
  timestamp = {
    "server_us": 0,
    "local_ms": 0
  }

  predict_now = function () {
    return timestamp.server_us + (Date.now() - timestamp.local_ms) * 1000
  }

  // Create components:
  items = new ItemsContainer()
  let stage = new Konva.Stage({
    container: 'plot',
    width: window.innerWidth,
    height: window.innerHeight,
  });
  let scene = new Scene(stage)

  // Create a web-socket connection
  set_status("Connecting...")
  const socket = new WebSocket('ws://127.0.0.1:5000');

  socket.addEventListener('open', function (event) {
    document.getElementById("status").innerHTML = "Connection established";
  });

  socket.addEventListener('message', function (event) {
    const update = JSON.parse(event.data);
    timestamp.server_us = update.ts
    timestamp.local_ms = Date.now()
    set_status(JSON.stringify(timestamp));
    for (let object_info of update.items) {
      item = new Item(object_info)
      item.timestamp = timestamp.server_us
      items.update_item(item)
    }
  });

  // Start scene animation
  animation = new Konva.Animation(function (frame) {
    scene.update(items, predict_now())
  });
  animation.start()
}