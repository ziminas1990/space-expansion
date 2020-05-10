import numpy
from typing import List, Dict
import tkinter as tk


class RectangleArea:
    def __init__(self, left: float, right: float, top: float, bottom: float):
        self.coordinates = numpy.array([
            [left, right],
            [top, bottom],
            [1,   1]
        ], dtype=numpy.float)

    def x1(self):
        return self.coordinates[0][0]

    def y1(self):
        return self.coordinates[1][0]

    def x2(self):
        return self.coordinates[0][1]

    def y2(self):
        return self.coordinates[1][1]

    def width(self):
        return abs(self.x2() - self.x1())

    def height(self):
        return abs(self.y2() - self.y1())

    def move(self, dx: float, dy: float):
        self.coordinates[0][0] += dx
        self.coordinates[0][1] += dx
        self.coordinates[1][0] += dy
        self.coordinates[1][1] += dy

    def scale(self, center_x: float, center_y: float, factor: float):
        def _scale(value: float, base_value: float, k: float) -> float:
            return float(base_value + (value - base_value) * k)
        self.coordinates[0][0] = _scale(self.coordinates[0][0], center_x, factor)
        self.coordinates[0][1] = _scale(self.coordinates[0][1], center_x, factor)
        self.coordinates[1][0] = _scale(self.coordinates[1][0], center_y, factor)
        self.coordinates[1][1] = _scale(self.coordinates[1][1], center_y, factor)


class Circle
    def __init__(self, position: RectangleArea, owner: "Malevich", id: int):
        self.position = position
        self.owner = owner
        self.id = id


class Malevich(tk.Canvas):

    def __init__(self, logical_view: RectangleArea, master=None, **kw):
        super().__init__(master, **kw)
        self.logical_view: RectangleArea = logical_view

        self._transform = numpy.identity(3)
        self._transform_inv = numpy.identity(3)
        self._circles: List[Circle] = []

        self.push_x = 0
        self.push_y = 0
        self.last_x = 0
        self.last_y = 0

        self.bind("<Button-1>",  self.mouse_down_event)
        self.bind("<Button-3>",  self.mouse_down_event)
        self.bind("<B1-Motion>", self.motion)
        self.bind("<B3-Motion>", self.scroll)
        self.bind("<Configure>", self.reconfigure)

    def create_circle(self, position: RectangleArea, **kw):
        physical_coords = numpy.matmul(self._transform, position.coordinates)

        print(*physical_coords[1])
        circle_id = self.create_oval(
            physical_coords[0][0], physical_coords[1][0],
            physical_coords[0][1], physical_coords[1][1],
            **kw)
        circle = Circle(position, self, circle_id)
        self._circles.append(circle)

    def update(self):
        logical_coordinates = numpy.empty((3, 2 * len(self._circles)))
        for column, circle in enumerate(self._circles):
            logical_coordinates[0][2 * column] = circle.position.x1()
            logical_coordinates[1][2 * column] = circle.position.y1()
            logical_coordinates[2][2 * column] = 1
            logical_coordinates[0][2 * column + 1] = circle.position.x2()
            logical_coordinates[1][2 * column + 1] = circle.position.y2()
            logical_coordinates[2][2 * column + 1] = 1

        physical_coordinates = numpy.matmul(self._transform, logical_coordinates)
        for column, circle in enumerate(self._circles):
            self.coords(circle.id, [
                physical_coordinates[0][2 * column],
                physical_coordinates[1][2 * column],
                physical_coordinates[0][2 * column + 1],
                physical_coordinates[1][2 * column + 1]
            ])

    def _recalculate_transform_matrix(self):
        physical_view: RectangleArea = RectangleArea(0, self.winfo_width(), 0, self.winfo_height())
        translate = numpy.array([
            [1, 0, physical_view.x1() - self.logical_view.x1()],
            [0, 1, physical_view.y1() - self.logical_view.y1()],
            [0, 0, 1],
        ], dtype=numpy.float)

        scale_x = physical_view.width() / self.logical_view.width()
        scale_y = physical_view.height() / self.logical_view.height()
        scale = numpy.array([
            [scale_x, 0,       0],
            [0,       scale_y, 0],
            [0,       0,       1]
        ], dtype=numpy.float)
        self._transform = numpy.matmul(scale, translate)
        self._transform_inv = numpy.linalg.inv(self._transform)

    def _to_logical_coords(self, x: float, y: float) -> (float, float):
        logical_coordinates = numpy.array([
            [x],
            [y],
            [1]
        ], dtype=numpy.float)
        physical_coordinates = numpy.matmul(self._transform_inv, logical_coordinates)
        return physical_coordinates[0][0], physical_coordinates[1][0]

    def mouse_down_event(self, event: tk.Event):
        self.push_x = self.last_x = event.x
        self.push_y = self.last_y = event.y

    def motion(self, event: tk.Event):
        current = self._to_logical_coords(event.x, event.y)
        last = self._to_logical_coords(self.last_x, self.last_y)

        dx = last[0] - current[0]
        dy = last[1] - current[1]
        self.logical_view.move(dx, dy)

        self.last_x = event.x
        self.last_y = event.y
        self._recalculate_transform_matrix()
        self.update()

    def scroll(self, event: tk.Event):
        dy = self.last_y - event.y
        factor: float = 1 + 2 * dy / self.winfo_height()
        center_x, center_y = self._to_logical_coords(self.push_x, self.push_y)
        print(center_x, center_y)
        self.logical_view.scale(center_x, center_y, factor)
        self.last_y = event.y
        self._recalculate_transform_matrix()
        self.update()

    def reconfigure(self, _event):
        self._recalculate_transform_matrix()
        self.update()

