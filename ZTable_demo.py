import ctypes
import sys
from gui32 import flags
from gui32.backend import winapi, wintypes
from gui32.edit import Edit
from gui32.toplevel import Toplevel
from ztable import *

red = 0x0000ff
green = 0x008000
blue = 0xff0000

people = (
    ('George Washington',   'None',         green),
    ('John Adams',          'None',         green),
    ('Thomas Jefferson',    'Republican',   red),
)


class ZTableDemoWindow(Toplevel):
    def __init__(self):
        self.selecting = False
        Toplevel.__init__(self, width=500, height=500, title='ZTable Demo')
        self.zt = ZTable(self)
        self.zt.SetColumns(
            self.zt.MakeColumn(headerText='Name', width=150, flags=ZTC_DEFAULTSORT),
            self.zt.MakeColumn(headerText='Party', width=100, flags=ZTC_DEFAULTSORT),
        )
        for name, address, color in people:
            self.zt.InsertRow(100, name, address, textColor=color)

    def OnResize(self, width, height):
        try:
            self.zt.Move(5, 5, width-10, height-10)
        except AttributeError:
            pass


tl = ZTableDemoWindow()
tl.Show()
sys.exit(tl.Mainloop())
