import * as protocol from "./Protocol"

class Position {
  public x: number;
  public y: number;
  public vx: number;
  public vy: number;
  // Acceleration will be calculated later, on updates
  public ax: number = 0;
  public ay: number = 0;

  static makeFrom(data: protocol.Position | Position): Position {
    if (data instanceof Position) {
      return new Position(data.x, data.y, data.vx, data.vy, data.ax, data.ay)
    } else {
      return new Position(data[0], data[1], data[2], data[3])
    }
  }

  constructor(x: number, y: number, vx: number, vy: number,
              ax: number = 0, ay: number = 0)
  {
    this.x = x;
    this.y = y;
    this.vx = vx;
    this.vy = vy;
    this.ax = ax;
    this.ay = ay;
  }
}

export class Item {
  public item_type: protocol.ItemType
  public id: number
  public position: Position
  public radius: number
  public timestamp: number


  constructor(item: protocol.Item) {
    this.item_type = item.type;
    this.id        = item.id;
    this.position  = Position.makeFrom(item.pos)
    this.radius    = item.radius !== undefined ? item.radius : 10;
    this.timestamp = item.ts;
  }

  update(item: protocol.Item) {
    if (item.radius !== undefined) {
      this.radius = item.radius
    }
    this.position.x = item.pos[0];
    this.position.y = item.pos[1];
    this.position.ax = (item.pos[2] - this.position.vx)/(item.ts - this.timestamp);
    this.position.ay = (item.pos[3] - this.position.vy)/(item.ts - this.timestamp);
    this.timestamp = item.ts;
  }

  strkey() {
    return this.item_type + "." + this.id;
  }

  predict_xy_position(at: number): {x: number, y: number} {
    const dt_sec = (at - this.timestamp) / 10**6
    const dt_half_sqr = dt_sec * dt_sec / 2;
    return {
      "x": this.position.x + (this.position.vx * dt_sec + this.position.ax * dt_half_sqr),
      "y": this.position.y + (this.position.vy * dt_sec + this.position.ay * dt_half_sqr),
    }
  }
};

export class ItemsContainer {
  private items: Map<string, Item>;

  constructor() {
    this.items = new Map<string, Item>();
  }

  update_item(item: protocol.Item) {
    let str_key = item.type + "." + item.id;
    const last_item = this.items.get(str_key);
    if (last_item !== undefined) {
       last_item.update(item);
    } else {
       let new_item = new Item(item);
       this.items.set(str_key, new_item);
    }
  }

  items_stream(): IterableIterator<Item> {
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
