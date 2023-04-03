import random
import sys
from gui32.backend import winapi, wintypes
from gui32.button import Button
from gui32.edit import Edit
from gui32.toplevel import Toplevel
from ztable import *

red = 0x000080
green = 0x008000
blue = 0x800000
lred = 0x8080ff
lgreen = 0x80ff80
lblue = 0xff8080

redb = winapi.CreateSolidBrush(0x8080ff)
greenb = winapi.CreateSolidBrush(0x80ff80)
blueb = winapi.CreateSolidBrush(0xff8080)

people = (
    ('George Washington', red, lgreen, blueb),
    ('John Adams', blue, lred,greenb),
    ('Thomas Jefferson', green, lblue, redb),
)


class ZTableDemoWindow(Toplevel):
    def __init__(self):
        Toplevel.__init__(self, width=700, height=500, title='ZTable Demo')
        self.zt = ZTable(self)
        self.zt.SetEmptyText("This is a demo ZTable, and it currently has no items visible. :)")
        self.zt.SetColumns(
            self.zt.MakeColumn(width=20, flags=ZTC_SELECTOR),
            self.zt.MakeColumn(headerText='Name', width=150,
                               flags=ZTC_DEFAULTSORT | ZTC_NOTIFYDBLCLICKS | ZTC_ALIGN_RIGHT | ZTC_CUSTOMFG),
            self.zt.MakeColumn(headerText='Color', width=100,
                               flags=ZTC_CUSTOMBG | ZTC_ALIGN_CENTER | ZTC_EDITABLE | ZTC_SINGLECLICKEDIT),
        )
        for ir, (name, color, lcolor, bg) in enumerate(people):
            self.zt.InsertRow(ir, '', (name, color), ('#%06x' % color, bg))
        self.zt.OnDoubleClick = self.OnDoubleClickTable
        self.zt.OnEditStart = self.OnEditStart
        self.zt.OnEditEnd = self.OnEditEnd
        self.zt.OnAutoNewRow = self.OnAutoNewRow
        self.zt.SetAutoMakeNewRow()
        self.zt.AddNewRow()
        Button(self, text='Add row', command=self.AddRow, x=5, y=5, height=25, width=100)
        Button(self, text='Unhide all', command=self.UnhideAll, x=110, y=5, height=25, width=100)

    def AddRow(self):
        ir = len(self.zt)-1
        name, color, lcolor, bg = random.choice(people)
        self.zt.InsertRow(ir, '', (name, color), ('#%06x' % color, bg))

    def UnhideAll(self):
        for i in range(len(self.zt)):
            self.zt.SetRowFiltered(i, True)

    def OnDoubleClickTable(self, ir, ic, x, y):
        self.zt.SetRowFiltered(ir, False)

    def OnEditStart(self, ir, ic, edit_hwnd):
        pass

    def OnEditEnd(self, ir, ic, edit_hwnd):
        e = Edit.from_handle(edit_hwnd)
        text = e.Text.removeprefix('#')
        try:
            color = int(text, 16) % 0x1000000
        except ValueError:
            self.ShowError('Invalid color value!')
            return True
        self.zt.SetItemParam(ir, 'Name', color)
        e.Text = '#%06x' % color
        return False

    def OnAutoNewRow(self, ir):
        name, color, lcolor, bg = random.choice(people)
        self.zt.SetItemText(ir, 'Name', name)
        self.zt.SetItemParam(ir, 'Name', color)
        self.zt.SetItemText(ir, 'Color', '#%06x' % color)
        self.zt.SetItemParam(ir, 'Color', bg)

    def OnResize(self, width, height):
        try:
            self.zt.Move(5, 35, width - 10, height - 45)
        except AttributeError:
            pass


tl = ZTableDemoWindow()
tl.Show()
sys.exit(tl.Mainloop())
