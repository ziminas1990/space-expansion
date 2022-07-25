let last_items = new Map;
class Position {                //при первой записи у объекта класса Item можно не записывать ускорение так как оно не посчитано
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

   update(item_from_update) {                 // у итема на которое пришло обновление переопределяем значения, а новое значение передали в параметре                                      // this - это старое значение  item_from_update - это новое значение, считаем ускорение
      this.position.ax = (item_from_update.pos[2] - this.position.vx)/(item_from_update.ts - this.timestamp); 
      this.position.ay = (item_from_update.pos[3] - this.position.vy)/(item_from_update.ts - this.timestamp);
      this.timestamp = item_from_update.ts;   // перезаписываем время с последнего обновления 
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

  update_item(item) {                             // один итем из массива пришедшего с сервера
   let str_key = item.type + "." + item.id;       // записываем его ключ в строку 
      if (this.items.has(str_key)) {              // если такой итем уже есть  
         let last_item = this.items.get(str_key); // записываем его прошлое значение (это item и через него имеем  доступ к методу класса Item)
         last_item.update(item);                  // обновляем его передавая новое значение распарсенного итема
      } else {                                    // если же такого итема еще нет, то
         let new_item = new Item(item);           // создаем новый инстенс  класса ITEM (не считаем ему ускорение так как в 1 раз у него нет данных о прошлой скорости)
         this.items.set(str_key, new_item);       // добавляем его в мапу                       
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