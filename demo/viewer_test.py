import random

from PyQt5.QtWidgets import QApplication
from PyQt5.QtWidgets import QGraphicsScene, QGraphicsEllipseItem
from PyQt5.QtCore import QRectF
from PyQt5.QtGui import QBrush, QPen, QColor

from gui.advanced_viewer import AdvancedViewer


def main():
    app = QApplication([])

    viewer = AdvancedViewer()
    viewer.show()

    scene = QGraphicsScene(-1000000, -1000000, 2000000, 2000000)
    viewer.setScene(scene)

    for x in range(-200, 200, 10):
        for y in range(-200, 200, 10):
            if ((x + y) / 10) % 2 == 0:
                shape = scene.addRect(x, y, 5, 5)
            else:
                shape = scene.addEllipse(QRectF(x-1, y-1, 7, 7))
            shape.setBrush(QBrush(QColor(random.randint(0, 255),
                                         random.randint(0, 255),
                                         random.randint(0, 255))))

    app.exec()


main()