let last_items = new Map;
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

   update() {
      //считаем ускорение
      let key_update_item = this.strkey();
      let last_item = items.items.get(key_update_item); //id: 0 item_type: "asteroid" position: Position  ax: 0 ay: 0 vx: 1 vy: 1 x: 7169.459495998263 y: 7269.459495736856 [[Prototype]]: Object radius: 50 timestamp: 7297158952
      this.position.ax = (this.position.vx - last_item.position.vx)/(this.timestamp - last_item.timestamp);
      this.position.ay = (this.position.vy - last_item.position.vy)/(this.timestamp - last_item.timestamp);
      console.log(this)
      //отправляем новый переписанный item в мапу itemsContainer
      items.update_item(this);
      console.log(items);
   }


  strkey() {
    return this.item_type + "." + this.id;
  }

  predict_position(at) {
    let dt_sec = (at - this.timestamp) / 10**6
    return { 
      "x": this.position.x + (this.position.vx * dt_sec + this.position.ax * dt_sec),
      "y": this.position.y + (this.position.vy * dt_sec + this.position.ay * dt_sec),
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