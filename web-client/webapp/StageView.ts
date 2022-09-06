import Konva from 'konva';

type Point = {
  x: number,
  y: number
}

// Transform matrix
// idx:   0, 1,  2, 3,  4,  5
// val: [sx, 0, tx, 0, sy, ty]
type Matrix = [number, number, number, number, number, number]

export class StageView {
  private m: Matrix


  constructor() {
    // Transform matrix
    // idx:   0, 1,  2, 3,  4,  5
    // val: [sx, 0, tx, 0, sy, ty]
    this.m = [1, 0, 0, 0, 1, 0]
  }

  translate(dx: number, dy: number) {
    this.m[2] += this.m[0] * dx  // tx += sx * dx
    this.m[5] += this.m[4] * dy  // ty += sy * dx
  }

  scale(x: number, y: number, factor: number) {
    this.translate(x, y)
    this.m[0] *= factor
    this.m[4] *= factor
    this.translate(-x, -y)
  }

  to_local(point: Point): [number, number] {
    return [(point.x - this.m[2]) / this.m[0], (point.y - this.m[5]) / this.m[4]]
  }

  to_global(point: Point) {
    return [point.x * this.m[0] + this.m[2], point.y * this.m[4] + this.m[5]]
  }

  get_pointer_position(stage: Konva.Stage): [number, number] {
    const pointer_position = stage.getPointerPosition()
    if (pointer_position != null) {
      return this.to_local(pointer_position)
    } else {
      return [0, 0]
    }
  }

  apply(layer: Konva.Layer) {
    const [, , , , current_tx, current_ty] = layer.getTransform().getMatrix()
    if (current_tx !== undefined && current_ty !== undefined) {
      layer.move({ "x": this.m[2] - current_tx, "y": this.m[5] - current_ty })
      layer.scale({ "x": this.m[0], "y": this.m[4] })
    }
  }
}


export class StageViewBinder {
  private view: StageView;
  private stage: Konva.Stage;
  private layers: Konva.Layer[];
  private mouse_down: boolean
  private prev_pointer_pos: [number, number]

  constructor(view: StageView, stage: Konva.Stage, layers: Konva.Layer[]) {
    this.view = view;
    this.stage = stage;
    this.layers = layers;
    this.mouse_down = false;
    this.prev_pointer_pos = [0, 0]
  }

  bind_events() {
    this.stage.on("mousedown", () => {
      this.mouse_down = true;
      this.prev_pointer_pos = this.view.get_pointer_position(this.stage)
    });

    this.stage.on("mouseup", () => {
      this.mouse_down = false;
    });

    this.stage.on("mousemove", () => {
      const pointer_position = this.view.get_pointer_position(this.stage)
      if (this.mouse_down) {
        this.view.translate(
          pointer_position[0] - this.prev_pointer_pos[0],
          pointer_position[1] - this.prev_pointer_pos[1])
        // Apply transform to layers
        for (let layer of this.layers) {
          this.view.apply(layer)
        }
        this.prev_pointer_pos = this.view.get_pointer_position(this.stage)
      }
    });

    this.stage.on("touchstart", () => {
      this.mouse_down = true;
      this.prev_pointer_pos = this.view.get_pointer_position(this.stage)
    });

    this.stage.on("touchend", () => {
      this.mouse_down = false;
    });

    this.stage.on("touchmove", () => {
      const pointer_position = this.view.get_pointer_position(this.stage)
      if (pointer_position !== undefined) {
        const [x, y] = pointer_position
        if (this.mouse_down) {
          this.view.translate(x - this.prev_pointer_pos[0],
                              y - this.prev_pointer_pos[1])
          // Apply transform to layers
          for (let layer of this.layers) {
            this.view.apply(layer)
          }
        }
        this.prev_pointer_pos = this.view.get_pointer_position(this.stage)
      }
    });

    this.stage.on("wheel", (e) => {
      const pointer_position = this.view.get_pointer_position(this.stage)
      if (pointer_position !== undefined) {
        const [x, y] = pointer_position
        e.evt.preventDefault();
        let scale_by = 1.1
        if (e.evt.deltaY < 0) {
          this.view.scale(x, y, scale_by)
        } else {
          this.view.scale(x, y, 1 / scale_by)
        }
        // Apply transform to layers
        for (let layer of this.layers) {
          this.view.apply(layer)
        }
      }
    });
  }
}
