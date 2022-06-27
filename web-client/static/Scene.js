
class Scene {
  constructor(stage) {
    this.stage = stage
    this.view = new StageView()

    this.asteroids_layer = new Konva.Layer();
    this.ships_layer = new Konva.Layer();
    this.gui_layer = new Konva.Layer();
    this.stage.add(this.asteroids_layer);
    this.stage.add(this.ships_layer);
    this.stage.add(this.gui_layer);

    this.binding = new StageViewBinder(
      this.view, 
      this.stage,
      // Note: GUI layer should not be moved or scaled by user
      [this.ships_layer, this.asteroids_layer]
    )

    this.shapes = new Map()
  }

  update(items_container, now) {
    for (let item of items_container.items_stream()) {
      let item_key = item.strkey()
      let shape = this.shapes.get(item_key)
      if (shape == undefined) {
        shape = this.spawn_shape(item)
        this.shapes.set(item_key, shape)
      }
      if (shape != null) {
        let predicted = item.predict_position(now)
        shape.position(predicted)
      }
    }
  }

  spawn_shape(item) {
    let shape = null
    if (item.item_type == "ship") {
      shape = new Konva.Circle({
        radius: item.radius,
        fill: 'red',
        stroke: 'black',
      });
      this.ships_layer.add(shape)
    } else if (item.item_type == "asteroid") {
      shape = new Konva.Circle({
        radius: item.radius,
        fill: 'blue',
        stroke: 'black',
      });
      this.asteroids_layer.add(shape)
    }
    return shape
  }
}