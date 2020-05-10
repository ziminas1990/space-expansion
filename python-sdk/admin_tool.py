from tkinter import *
from tk_widgets.malevich import Malevich, RectangleArea

lastx, lasty = 0, 0

root = Tk()
root.columnconfigure(0, weight=1)
root.rowconfigure(0, weight=1)

canvas = Malevich(logical_view=RectangleArea(-100, 100, -100, 100), master=root)
canvas.grid(column=0, row=0, sticky=(N, W, E, S))

canvas.create_circle(RectangleArea(-50, 50, -50, 50), outline="red", fill="green", width=2)

#canvas.bind("<Button-1>", xy)
#canvas.bind("<B1-Motion>", addLine)

root.mainloop()