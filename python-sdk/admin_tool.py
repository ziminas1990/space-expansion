import tkinter as tk
from typing import Dict, Set
import asyncio
import logging.config
import time

from transport.udp_channel import UdpChannel
from transport.protobuf_channel import ProtobufChannel
from tk_widgets import malevich
from protocol.Privileged_pb2 import Message as PrivilegedMessage
from interfaces.privileged.access import Access as PrivilegedAccess
from interfaces.privileged.screen import Screen as PrivilegedScreen
import interfaces.privileged.types as privileged_types


async def screen_update_loop(advanced_canvas: malevich.Malevich,
                             remote_screen: PrivilegedScreen):
    """
    This task periodically requests all objects, that are covered by
    logical view area of the specified 'canvas' and update them. The specified
    'remote_screen" will be used to retrieve data from server
    """
    try:
        circles: Dict[int, malevich.Circle] = {}

        while True:
            # Get all asteroids
            cx, cy = advanced_canvas.logical_view.center()
            width = advanced_canvas.logical_view.width()
            height = advanced_canvas.logical_view.height()
            await remote_screen.set_position(center_x=cx, center_y=cy, width=width, height=height)
            objects = await remote_screen.show(privileged_types.ObjectType.ASTEROID)
            for object in objects:
                circle = circles[object.id] if object.id in circles else None
                if circle is None:
                    circle = advanced_canvas.create_circle(
                        center=malevich.Point(object.x, object.y),
                        r=object.r,
                        outline="red", fill="green", width=3
                    )
                    circles.update({circle.id: circle})
                else:
                    circle.change(
                        center=malevich.Point(object.x, object.y),
                        r=object.r
                    )
            advanced_canvas.update()
            await asyncio.sleep(len(objects) * 0.001)
    except Exception as ex:
        print(f"screen_update_loop() crashed: {ex}")


async def main():

    # Creating channels
    udp_channel = UdpChannel(on_closed_cb=lambda: print("Closed!"),
                             channel_name="UDP")
    if not await udp_channel.open("127.0.0.1", 17392):
        print("Failed to open UDP connection!")
        return

    privileged_channel = ProtobufChannel(name="Player",
                                         toplevel_message_type=PrivilegedMessage)
    privileged_channel.attach_to_channel(udp_channel)

    # Creating components:
    access_panel = PrivilegedAccess()
    access_panel.attach_to_channel(privileged_channel)

    # Logging in
    status, token = await access_panel.login("admin", "admin")
    screen = PrivilegedScreen()
    screen.attach_to_channel(channel=privileged_channel, token=token)

    if not await screen.set_position(center_x=0, center_y=0, width=300000, height=300000):
        return

    # Creating Malevich object for drawing
    root = tk.Tk()
    root.columnconfigure(0, weight=1)
    root.rowconfigure(0, weight=1)

    canvas = tk.Canvas(master=root)
    canvas.grid(column=0, row=0, sticky=(tk.N, tk.W, tk.E, tk.S))

    objects_map = malevich.Malevich(logical_view=malevich.RectangleArea(-150000, 150000, -150000, 150000),
                                    canvas=canvas)

    asyncio.create_task(
        screen_update_loop(objects_map, screen)
    )

    while True:
        root.update_idletasks()
        root.update()
        await asyncio.sleep(0)


asyncio.run(main())