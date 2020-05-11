import numpy as np
import math
from typing import List, Dict, Optional
import tkinter as tk


class Point:
    def __init__(self, x: float, y: float):
        """Create point with the specified 'x' and 'y' logical(!) coordinates"""
        self.x: float = x
        self.y: float = y


class RectangleArea:
    def __init__(self, left: float, right: float, top: float, bottom: float):
        self.coordinates = np.array([
            [left, right],
            [top, bottom],
            [1,   1]
        ], dtype=np.float)

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


class Shape:
    def __init__(self, id: int, logical_coords: np.ndarray, physical_coords: np.ndarray):
        assert physical_coords.shape == physical_coords.shape
        assert logical_coords.shape[0] == 3

        self.id = id
        self.consistent: bool = True
        self.logical_coords: np.ndarray = logical_coords
        self.physical_coords: np.ndarray = physical_coords

    def unpack_physical_coordinates(self) -> List[float]:
        """Return list of coordinates, that can be passed to tkinter's canvas.
        This function may be overridden"""
        unpacked: List[float] = []
        columns = self.physical_coords.shape[1]
        for col in range(columns):
            unpacked.extend([self.physical_coords[0][col], self.physical_coords[1][col]])
        return unpacked


class Circle(Shape):
    @staticmethod
    def pack_coordinates(center: Point, r: float, dest: Optional[np.ndarray] = None):
        if not dest:
            dest = np.ones((3, 2))
        assert dest.shape == (3, 2)
        R = math.sqrt(r*r)
        dest[0][0] = center.x - R / 2
        dest[0][1] = center.x + R / 2
        dest[1][0] = center.y - R / 2
        dest[1][1] = center.y + R / 2
        return dest

    def __init__(self, id: int, logical_coords: np.ndarray, physical_coords: np.ndarray):
        super().__init__(id=id, logical_coords=logical_coords, physical_coords=physical_coords)
        assert logical_coords.shape == (3, 2)
        assert physical_coords.shape == (3, 2)

    def move(self, center: Point, r: float):
        Circle.pack_coordinates(center=center, r=r, dest=self.logical_coords)
        self.consistent = False


class Malevich():

    def __init__(self, logical_view: RectangleArea, canvas: tk.Canvas):
        self.logical_view: RectangleArea = logical_view
        self.canvas: tk.Canvas = canvas

        self._transform = np.identity(3)
        self._transform_inv = np.identity(3)
        self._shapes: List[Shape] = []

        self.push_x = 0
        self.push_y = 0
        self.last_x = 0
        self.last_y = 0

        self.canvas.bind("<Button-1>",  self.mouse_down_event)
        self.canvas.bind("<Button-3>",  self.mouse_down_event)
        self.canvas.bind("<B1-Motion>", self.motion)
        self.canvas.bind("<B3-Motion>", self.scroll)
        self.canvas.bind("<Configure>", self.reconfigure)

    def create_circle(self, center: Point, r: float, **kw):
        logical_coords = Circle.pack_coordinates(center, r)
        physical_coords = np.matmul(self._transform, logical_coords)
        circle_id = self.canvas.create_oval(
            physical_coords[0][0], physical_coords[1][0],
            physical_coords[0][1], physical_coords[1][1],
            **kw)
        circle = Circle(id=circle_id, logical_coords=logical_coords, physical_coords=physical_coords)
        self._shapes.append(circle)

    def update(self, all: bool = False):
        for shape in self._shapes:
            if all or not shape.consistent:
                shape.physical_coords = np.matmul(self._transform, shape.logical_coords)
                shape.consistent = True
                self.canvas.coords(shape.id, shape.unpack_physical_coordinates())

    def _recalculate_transform_matrix(self):
        physical_view: RectangleArea =\
            RectangleArea(0, self.canvas.winfo_width(), 0, self.canvas.winfo_height())
        translate = np.array([
            [1, 0, physical_view.x1() - self.logical_view.x1()],
            [0, 1, physical_view.y1() - self.logical_view.y1()],
            [0, 0, 1],
        ], dtype=np.float)

        scale_x = physical_view.width() / self.logical_view.width()
        scale_y = physical_view.height() / self.logical_view.height()
        scale = np.array([
            [scale_x, 0,       0],
            [0,       scale_y, 0],
            [0,       0,       1]
        ], dtype=np.float)
        self._transform = np.matmul(scale, translate)
        self._transform_inv = np.linalg.inv(self._transform)

    def _transform_shapes(self, shapes: List[Shape]):
        """Calculate physical coordinates for the specified 'shapes'"""
        for shape in shapes:
            shape.physical_coords = np.matmul(self._transform, shape.physical_coords)

    def _to_logical_coords(self, x: float, y: float) -> (float, float):
        logical_coordinates = np.array([
            [x],
            [y],
            [1]
        ], dtype=np.float)
        physical_coordinates = np.matmul(self._transform_inv, logical_coordinates)
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
        self.update(all=True)

    def scroll(self, event: tk.Event):
        dy = self.last_y - event.y
        factor: float = 1 + 2 * dy / self.canvas.winfo_height()
        center_x, center_y = self._to_logical_coords(self.push_x, self.push_y)
        self.logical_view.scale(center_x, center_y, factor)
        self.last_y = event.y
        self._recalculate_transform_matrix()
        self.update(all=True)

    def reconfigure(self, _event):
        self._recalculate_transform_matrix()
        self.update(all=True)

