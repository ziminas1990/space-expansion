
class StageView {
  constructor() {
    // Transform matrix
    // idx:   0, 1,  2, 3,  4,  5
    // val: [sx, 0, tx, 0, sy, ty]
    this.m = [1, 0, 0, 0, 1, 0]
  }

  translate(dx, dy) {
    this.m[2] += this.m[0] * dx  // tx += sx * dx
    this.m[5] += this.m[4] * dy  // ty += sy * dx
  }

  scale(x, y, factor) {
    this.translate(x, y)
    this.m[0] *= factor
    this.m[4] *= factor
    this.translate(-x, -y)
  }

  to_local({ x, y }) {
    return [(x - this.m[2]) / this.m[0], (y - this.m[5]) / this.m[4]]
  }

  to_global({ x, y }) {
    return [x * this.m[0] + this.m[2], x * this.m[4] + this.m[5]]
  }

  get_pointer_position(stage) {
    return this.to_local(stage.getPointerPosition())
  }

  apply(layer) {
    let [, , , , current_tx, current_ty] = layer.getTransform().getMatrix()
    layer.move({ "x": this.m[2] - current_tx, "y": this.m[5] - current_ty })
    layer.scale({ "x": this.m[0], "y": this.m[4] })
  }

  bind_to_stage(stage) {
    bind_view_to_stage(stage, this)
  }
}


class StageViewBinder {
  constructor(view, stage, layers) {
    let mouse_down = false;
    let prev_x = 0;
    let prev_y = 0;

    stage.on("mousedown", function (evt) {
      mouse_down = true;
    });

    stage.on("mouseup", function (evt) {
      mouse_down = false;
    });

    stage.on("mousemove", function (evt) {
      let [x, y] = view.get_pointer_position(stage)
      if (mouse_down) {
        view.translate(x - prev_x, y - prev_y)
        // Apply transform to layers
        for (let layer of layers) {
          view.apply(layer)
        }
      }
      [prev_x, prev_y] = view.get_pointer_position(stage)
    });

    stage.on("touchstart", function (evt) {
      mouse_down = true;
      [prev_x, prev_y] = view.get_pointer_position(stage)
    });

    stage.on("touchend", function (evt) {
      mouse_down = false;
    });

    stage.on("touchmove", function (evt) {
      let [x, y] = view.get_pointer_position(stage)
      if (mouse_down) {
        view.translate(x - prev_x, y - prev_y)
        // Apply transform to layers
        for (let layer of layers) {
          view.apply(layer)
        }
      }
      [prev_x, prev_y] = view.get_pointer_position(stage)
    });

    stage.on("wheel", function (e) {
      let [x, y] = view.get_pointer_position(stage)
      e.evt.preventDefault();
      let scale_by = 1.1
      if (e.evt.deltaY < 0) {
        view.scale(x, y, scale_by)
      } else {
        view.scale(x, y, 1 / scale_by)
      }
      // Apply transform to layers
      for (let layer of layers) {
        view.apply(layer)
      }
    });
  }
}