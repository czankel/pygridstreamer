import pygridstreamer as gs
import time

print("-------------\nGet cell list\n-------------")
cells = gs.celltypes()
print(cells)

print("\n---------------\nCreate new Grid\n---------------")
grid = gs.Grid("MainGrid")
print(grid)

print("\n--------------------\nChannel reassignment\n--------------------")
channel_tmp = grid.allocate_channel("main-channel")
channel_tmp = grid.allocate_channel("second-channel")
print(channel_tmp)

print("\n-------------------\nChannel with Layout\n-------------------")
layout = "{ src: TestImage ! dst: TestImageVerify }"
channel = grid.allocate_channel("other-channel", layout=layout)
print ("Channel:", channel)

print("\n-------------\nShow Channels\n-------------")
channels = grid.channels()
print ("Channels:", channels)

print("\n-------------\nShow Pipeline\n-------------")
pipeline_list = channel.cells()
print("Pipelines:", pipeline_list)

pipeline = pipeline_list["BasePipeline"]
print("Type:", pipeline.type())

cell_list = pipeline.cells()
print("Cells:", cell_list)

cell = cell_list["src"]
print("Source:", cell)

print("\n---------------\nShow Parameters\n----------------")
param_list = cell.parameters()
print("Parameters:", param_list)

dim = cell.dimension
print("Dimension:", dim)
print("Format:", dim.format)
print("Value:", dim.value)

dim.value = "1024x512"
print("Set 1024 x 512:", dim.value)

dim.value = [640, 480]
print("Set 640 x 480:", dim.value)

print("\n--------\nCallback\n--------\n")

def on_verify(*args):
    print("Frequency:", args[1])

dstcell = cell_list["dst"]
dstcell.on_verify.connect(on_verify)

print("\n-----------\nRun Channel\n-----------")
print("State:", channel.state);
channel.run()
print("State:", channel.state);

time.sleep(10)

print("\n-------------\nClose Channel\n-------------")

print("State:", channel.state);
channel.close()
time.sleep(1)
print("State:", channel.state);
