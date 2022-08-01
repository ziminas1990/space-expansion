class Position {          
  constructor([x, y, vx, vy]) {
    this.x = x;
    this.y = y;
    this.vx = vx;
    this.vy = vy;    
    // Acceleration will be calculated later, on updates
    this.ax = 0;
    this.ay = 0;
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

   update(item_from_update) {                
    this.position.ax = (item_from_update.pos[2] - this.position.vx)/(item_from_update.ts - this.timestamp); 
    this.position.ay = (item_from_update.pos[3] - this.position.vy)/(item_from_update.ts - this.timestamp);
    this.timestamp = item_from_update.ts;   
   }

   strkey() {
    return this.item_type + "." + this.id;
   }

   predict_position(at) {
    let dt_half_sqr = dt_sec * dt_sec / 2;
    return { 
      "x": this.position.x + (this.position.vx * dt_sec + this.position.ax * dt_half_sqr),
      "y": this.position.y + (this.position.vy * dt_sec + this.position.ay * dt_half_sqr),
    }
  }
};

class ItemsContainer {
  constructor() {
    this.items = new Map();
   }

   update_item(item) {                             
    let str_key = item.type + "." + item.id;       
      if (this.items.has(str_key)) {              
         let last_item = this.items.get(str_key); 
         last_item.update(item);                  
      } else {                                    
         let new_item = new Item(item);           
         this.items.set(str_key, new_item);                              
      }
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