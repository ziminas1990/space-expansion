import Konva from 'konva';

import { StageView, StageViewBinder } from "./StageView"
import { Item, ItemsContainer } from "./ItemsContainer"

type Layers =  {
  asteroids: Konva.Layer
  ships: Konva.Layer
  gui: Konva.Layer
}

export class Scene {
  private stage: Konva.Stage
  private view: StageView
  private layers: Layers
  private binder: StageViewBinder
  private shapes: Map<string, Konva.Shape>

  constructor(stage: Konva.Stage) {
    this.stage = stage
    this.view = new StageView()

    this.layers = {
      asteroids: new Konva.Layer(),
      ships: new Konva.Layer(),
      gui: new Konva.Layer()
    }

    this.stage.add(this.layers.asteroids);
    this.stage.add(this.layers.ships);
    this.stage.add(this.layers.gui);

    this.binder = new StageViewBinder(
      this.view,
      this.stage,
      // Note: GUI layer should not be moved or scaled by user
      [this.layers.ships, this.layers.asteroids]
    )
    this.binder.bind_events()

    this.shapes = new Map<string, Konva.Shape>()
  }

  update(items_container: ItemsContainer, now: number) {
    for (let item of items_container.items_stream()) {
      let item_key = item.strkey
      let shape = this.shapes.get(item_key)
      if (shape == undefined) {
        const shape = this.spawn_shape(item)
        if (shape !== null) {
          this.shapes.set(item_key, shape)
        }
      }
      if (shape != null) {
        const predicted = item.predict_xy_position(now)
        shape.position(predicted)
      }
    }
  }

  spawn_shape(item: Item): Konva.Shape | null {
    let shape = null;
    if (item.item_type == "ship") {
      shape = new Konva.Circle({
        radius: item.radius,
        fill: 'red',
        stroke: 'black',
      });
      this.layers.ships.add(shape)
    } else if (item.item_type == "asteroid") {
      shape = new Konva.Circle({
        radius: item.radius,
        fill: 'blue',
        stroke: 'black',
      });
      this.layers.asteroids.add(shape)
    }
    return shape
  }
}
