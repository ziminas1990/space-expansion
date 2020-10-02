import numpy as np
import enum
import abc
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

    def center(self) -> (float, float):
        cx = (self.coordinates[0][0] + self.coordinates[0][1]) / 2
        cy = (self.coordinates[1][0] + self.coordinates[1][1]) / 2
        return (cx, cy)

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


def unpack_coordinates(coordinates: np.ndarray,
                       columns: Optional[List[int]] = None) -> List[float]:
    """Return coordinates from the specified 'columns' of the specified
    'coordinates' matrix as list: [x1, y1, x2, y2 ...]. This list can be
    passed to tk.Canvas.coords() call"""
    unpacked: List[float] = []

    if not columns:
        columns = [i for i in range(coordinates.shape[1])]

    for col in columns:
        unpacked.extend([coordinates[0][col], coordinates[1][col]])
    return unpacked


class Shape(abc.ABC):

    def __init__(self, logical_coords: np.ndarray):
        assert logical_coords.shape[0] == 3
        self.logical_coords: np.ndarray = logical_coords
        self.physical_coords: np.ndarray = np.zeros(self.logical_coords.shape)
        self.canvas: Optional[tk.Canvas] = None

    def transform(self, transform: np.ndarray):
        """Update shape on attached canvas. The specified 'transform' 3x3
        matrix should be used to calculate physical coordinates on canvas"""
        np.matmul(transform, self.logical_coords, out=self.physical_coords)
        if self.canvas:
            self.update(self.canvas)

    def attach(self, canvas: tk.Canvas, transform: np.ndarray):
        """
        Create shape on the specified 'canvas'. The specified 'transform'
        matrix will be used to calculate shape's physical coordinates.
        """
        self.detach()
        self.transform(transform)
        self.canvas = canvas
        self.create(self.canvas)

    def detach(self):
        """
        If object has been attached to canvas, release all canvas
        resources, related with this shape. Otherwise function
        has no affect
        """
        if self.canvas:
            self.destroy(self.canvas)
        self.canvas = None

    @abc.abstractmethod
    def create(self, canvas: tk.Canvas):
        """Create shape on the specified 'canvas'"""
        pass

    @abc.abstractmethod
    def update(self, canvas: tk.Canvas):
        """Update shape on the specified 'canvas'. Canvas will be
        the same, as was passed to 'create' call"""
        pass

    @abc.abstractmethod
    def destroy(self, canvas: tk.Canvas):
        """Destroy shape on the specified 'canvas'. Canvas will be
        the same, as was passed to 'create' call"""
        pass


class Circle(Shape):
    @staticmethod
    def pack_coordinates(center: Point, r: float, dest: Optional[np.ndarray] = None):
        if dest is None:
            dest = np.ones((3, 2))
        assert dest.shape == (3, 2)
        R = math.sqrt(r*r)
        dest[0][0] = center.x - R / 2
        dest[0][1] = center.x + R / 2
        dest[1][0] = center.y - R / 2
        dest[1][1] = center.y + R / 2
        return dest

    def __init__(self, center: Point, r: float, **tkparams):
        super().__init__(logical_coords=Circle.pack_coordinates(center=center, r=r))
        self.tk_params = tkparams
        self.circle_id: Optional[int] = None
        # id, assigned to circle object by tk.Canvas

    def change_position(self, center: Point, r: float):
        Circle.pack_coordinates(center=center, r=r, dest=self.logical_coords)

    def create(self, canvas: tk.Canvas):
        self.circle_id = self.canvas.create_oval(
            self.physical_coords[0][0], self.physical_coords[1][0],
            self.physical_coords[0][1], self.physical_coords[1][1],
            self.tk_params)

    def update(self, canvas: tk.Canvas):
        canvas.coords(self.circle_id, unpack_coordinates(self.physical_coords))

    def destroy(self, canvas: tk.Canvas):
        assert self.circle_id is not None
        canvas.delete(self.circle_id)


class SquareMark(Shape):
    @staticmethod
    def pack_coordinates(position: Point, dest: Optional[np.ndarray] = None):
        if dest is None:
            dest = np.ones((3, 1))
        assert dest.shape == (3, 1)
        dest[0][0] = position.x
        dest[1][0] = position.y
        return dest

    def unpack_coordinates(self) -> List[float]:
        """Return list: [x1, y1, x2, y2]"""
        return [
            self.physical_coords[0][0] - self.half_size,
            self.physical_coords[1][0] - self.half_size,
            self.physical_coords[0][0] + self.half_size,
            self.physical_coords[1][0] + self.half_size,
        ]

    def __init__(self, position: Point, size: float, **tkparams):
        super().__init__(logical_coords=SquareMark.pack_coordinates(position=position))
        self.tk_params = tkparams
        self.square_id: Optional[int] = None
        self.half_size: float = size/2

    def change_position(self, position: Point):
        SquareMark.pack_coordinates(position=position, dest=self.logical_coords)

    def create(self, canvas: tk.Canvas):
        tk_position: List[float] = self.unpack_coordinates()
        self.square_id = self.canvas.create_rectangle(*tk_position, self.tk_params)

    def update(self, canvas: tk.Canvas):
        canvas.coords(self.square_id, self.unpack_coordinates())

    def destroy(self, canvas: tk.Canvas):
        assert self.square_id is not None
        canvas.delete(self.square_id)


class AdvancedCanvas():
    def __init__(self, logical_view: RectangleArea, canvas: tk.Canvas):
        self.logical_view: RectangleArea = logical_view
        self.canvas: tk.Canvas = canvas

        self._transform = np.identity(3)
        self._transform_inv = np.identity(3)
        self._shapes: Dict[int, Shape] = {}

        self.push_x = 0
        self.push_y = 0
        self.last_x = 0
        self.last_y = 0

        self.canvas.bind("<Button-1>",  self.mouse_down_event)
        self.canvas.bind("<Button-3>",  self.mouse_down_event)
        self.canvas.bind("<B1-Motion>", self.motion)
        self.canvas.bind("<B3-Motion>", self.scroll)
        self.canvas.bind("<Configure>", self.reconfigure)

    def create_circle(self, shape_id: int, center: Point, r: float, **kw) -> Optional[Circle]:
        assert shape_id not in self._shapes, f"Shape with id={shape_id} already exists"

        circle = Circle(center=center, r=r, **kw)
        self._shapes.update({shape_id: circle})
        circle.attach(self.canvas, self._transform)
        return circle

    def create_square_mark(self, shape_id: int, position: Point, size: float, **kw) \
            -> Optional[SquareMark]:
        assert shape_id not in self._shapes, f"Shape with id={shape_id} already exists"

        mark = SquareMark(position=position, size=size, **kw)
        self._shapes.update({shape_id: mark})
        mark.attach(self.canvas, self._transform)
        return mark

    def remove(self, shape_id: int):
        assert shape_id in self._shapes, f"No shape with id={shape_id}"
        shape = self._shapes[shape_id]
        shape.detach()

    def update(self):
        for shape in self._shapes.values():
            shape.transform(self._transform)

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
        self.update()

    def scroll(self, event: tk.Event):
        dy = self.last_y - event.y
        factor: float = 1 + 2 * dy / self.canvas.winfo_height()
        center_x, center_y = self._to_logical_coords(self.push_x, self.push_y)
        self.logical_view.scale(center_x, center_y, factor)
        self.last_y = event.y
        self._recalculate_transform_matrix()
        self.update()

    def reconfigure(self, _event):
        self._recalculate_transform_matrix()
        self.update()

