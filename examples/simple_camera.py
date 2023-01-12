import pygridstreamer as gs
import time

print("\n---------------\nCreate new Grid\n---------------")
grid = gs.Grid("MainGrid")
print(grid)

print("\n-------------------\nChannel with Layout\n-------------------")
layout = "{ src: OpenCVCamera InputFormat='rgb24' OutputFormat='rgb24' ! dst: OpenCVWindow }"
channel = grid.allocate_channel("CamChannel", layout)
print(channel)
print(channel.cells())
print("\n-----------\nRun Channel\n-----------")
print("State:", channel.state);

channel._dump()

channel.state="running"
channel.run();

time.sleep(100)

