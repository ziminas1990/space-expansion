import Konva from 'konva';

import { Connection } from "./Connection"
import { ItemsContainer } from "./ItemsContainer"
import { Scene } from "./Scene"
import * as protocol from "./Protocol"

function set_status(status: string) {
  const label: HTMLElement | null = document.getElementById("status")
  if (label !== null) {
    label.innerHTML = status
  }
}

class Timestamp {
  server_us: number = 0;
  local_ms: number = 0;

  predict_now_us(): number {
    return this.server_us + (Date.now() - this.local_ms) * 1000
  }

  update(server_us: number) {
    this.server_us = server_us;
    this.local_ms = Date.now();
  }
};

export function runApp() {
  const timestamp: Timestamp = new Timestamp()

  // Create components:
  const connection: Connection = new Connection()
  const items: ItemsContainer = new ItemsContainer()
  const stage = new Konva.Stage({
    container: 'plot',
    width: window.innerWidth,
    height: window.innerHeight,
  });
  const scene: Scene = new Scene(stage)

  // Create a web-socket connection
  set_status("Connecting...")
  connection.open(
    window.location.host,
    function () {
      set_status("Connection established");
    }
  )

  // Add callback to listen for incoming updates
  connection.addEventListener('message', function (event: MessageEvent) {
    const update: protocol.Update = JSON.parse(event.data);
    timestamp.update(update.ts)
    set_status(JSON.stringify(timestamp));
    for (let object_info of update.items) {
      items.update_item(object_info)
    }
  });

  const animation: Konva.Animation = new Konva.Animation(function () {
    scene.update(items, timestamp.predict_now_us())
  });
  animation.start()
}

runApp();