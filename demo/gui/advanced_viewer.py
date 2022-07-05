from typing import Tuple
import logging

from PyQt5.QtWidgets import QGraphicsView
from PyQt5.QtGui import QTransform, QMouseEvent, QWheelEvent
from PyQt5.QtCore import Qt, pyqtSignal


class AdvancedViewer(QGraphicsView):

    left_click = pyqtSignal(float, float)

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.setTransformationAnchor(QGraphicsView.NoAnchor)
        self.setResizeAnchor(QGraphicsView.NoAnchor)
        self._prev_position: Tuple[int, int] = (0, 0)
        self.setHorizontalScrollBarPolicy(Qt.ScrollBarAlwaysOff)
        self.setVerticalScrollBarPolicy(Qt.ScrollBarAlwaysOff)

        self.__no_move: bool = False

    def mousePressEvent(self, event: QMouseEvent) -> None:
        self._prev_position = (event.x(), event.y())
        self.__no_move = True

    def mouseMoveEvent(self, event: QMouseEvent) -> None:
        self.__no_move = False
        dx = (event.x() - self._prev_position[0])
        dy = (event.y() - self._prev_position[1])
        scale_x = self.transform().m11()
        scale_y = self.transform().m22()
        dx /= scale_x
        dy /= scale_y
        self.translate(dx, dy)
        self._prev_position = (event.x(), event.y())

    def mouseReleaseEvent(self, event: QMouseEvent) -> None:
        if self.__no_move:
            position = self.mapToScene(int(event.x()), int(event.y()))
            self.left_click.emit(position.x(), position.y())

    def wheelEvent(self, event: QWheelEvent) -> None:
        position = self.mapToScene(int(event.position().x()),
                                   int(event.position().y()))

        self.translate(position.x(), position.y())
        if event.angleDelta().y() > 0:
            self.scale(1.2, 1.2)
        else:
            self.scale(0.8333, 0.8333)
        self.translate(-position.x(), -position.y())
