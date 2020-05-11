import tkinter as tk
import asyncio
import logging.config

from transport.udp_channel import UdpChannel
from transport.protobuf_channel import ProtobufChannel
from tk_widgets.malevich import Malevich, RectangleArea, Point
from protocol.Privileged_pb2 import Message as PrivilegedMessage
from interfaces.privileged.access import Access as PrivilegedAccess
from interfaces.privileged.screen import Screen as PrivilegedScreen
import interfaces.privileged.types as privileged_types


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

    # Get all asteroids
    objects = await screen.show(privileged_types.ObjectType.ASTEROID)

    root = tk.Tk()
    root.columnconfigure(0, weight=1)
    root.rowconfigure(0, weight=1)

    canvas = tk.Canvas(master=root)
    canvas.grid(column=0, row=0, sticky=(tk.N, tk.W, tk.E, tk.S))

    malevich = Malevich(logical_view=RectangleArea(-150000, 150000, -150000, 150000), canvas=canvas)

    for asteroid in objects:
        malevich.create_circle(center=Point(asteroid.x, asteroid.y),
                               r=asteroid.r,
                               outline="red", fill="green", width=2)

    root.mainloop()



asyncio.run(main())