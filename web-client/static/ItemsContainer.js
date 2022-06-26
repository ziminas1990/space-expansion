
class Position {
  constructor([x, y, vx, vy]) {
    this.x = x;
    this.y = y;
    this.vx = vx;
    this.vy = vy;
  }
}

class Item {
  constructor({type: item_type, id, pos: position, radius=10, ts: timestamp = undefined}) {
    this.item_type = item_type;
    this.id        = id;
    this.position  = new Position(position);
    this.radius    = radius;
    this.timestamp = timestamp;
  }

  strkey() {
    return this.item_type + "." + this.id;
  }

  predict_position(at) {
    let dt_sec = (at - this.timestamp) / 10**6
    return {
      "x": this.position.x + this.position.vx * dt_sec,
      "y": this.position.y + this.position.vy * dt_sec
    }
  }
};

class ItemsContainer {
  constructor() {
    this.items = new Map();
  }

  update_item(item) {
    this.items.set(item.strkey(), item)
  }

  items_stream() {
    return this.items.values()
  }

  print() {
    let items_stream = "";
    for (let entry of this.items.entries()) {
      items_stream += "<p>" + JSON.stringify(entry) + "</p>"
    }
    return items_stream;
  }

};