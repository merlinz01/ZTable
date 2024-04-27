
import random
import sys
sys.path.append('../wingui')
from gui32.backend import winapi, wintypes
from gui32.button import Button, CheckBox
from gui32.colorchooser import ChooseColor
from gui32.combobox import ComboBox
from gui32.edit import Edit
from gui32.toplevel import Toplevel
from gui32.window import PyWindow
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
    ('John Adams', blue, lred, greenb),
    ('Thomas Jefferson', green, lblue, redb),
)


class ZTableDemoWindow(Toplevel):
    def __init__(self):
        Toplevel.__init__(self, width=700, height=500, title='ZTable Demo')
        self.zt = ZTable(self)
        self.zt.SetEmptyText("This is a demo ZTable, and it currently has no items visible. :)")
        self.colorchooser = Button(self.zt, command=self.OnChooseColor, text='Choose color...')
        self.checker = CheckBox(self.zt, command=self.OnCheck)
        # self.propertyEditor = PropertyEditor(self.zt)
        self.zt.SetColumns(
            self.zt.MakeColumn(width=20, flags=ZTC_SELECTOR),
            self.zt.MakeColumn(headerText='Name', width=150,
                               flags=ZTC_DEFAULTSORT | ZTC_NOTIFYDBLCLICKS | ZTC_ALIGN_RIGHT | ZTC_CUSTOMFG),
            self.zt.MakeColumn(headerText='Color', width=100,
                               flags=ZTC_CUSTOMBG | ZTC_ALIGN_CENTER | ZTC_EDITABLE | ZTC_SINGLECLICKEDIT),
            self.zt.MakeColumn(headerText='Custom editor', width=150, editWin=self.colorchooser,
                               flags=ZTC_EDITABLE | ZTC_SINGLECLICKEDIT | ZTC_CUSTOMEDITOR),
            self.zt.MakeColumn(headerText='Oh?', width=30, editWin=self.checker,
                               flags=ZTC_EDITABLE | ZTC_SINGLECLICKEDIT | ZTC_CUSTOMEDITOR),
            self.zt.MakeColumn(headerText='Edit property', width=200, editWin=self.colorchooser,
                               flags=ZTC_EDITABLE | ZTC_SINGLECLICKEDIT | ZTC_CUSTOMEDITOR | ZTC_CUSTOMEDITORROW),
        )
        for ir, (name, color, lcolor, bg) in enumerate(people):
            self.zt.InsertRow(ir, '', (name, color), ('#%06x' % color, bg), '#%06x' % lcolor, 'Yes!')
        self.zt.OnDoubleClick = self.OnDoubleClickTable
        self.zt.OnEditStart = self.OnEditStart
        self.zt.OnEditEnd = self.OnEditEnd
        self.zt.OnAutoNewRow = self.OnAutoNewRow
        self.zt.SetAutoMakeNewRow()
        self.zt.AddNewRow()
        Button(self, text='Add row', command=self.AddRow, x=5, y=5, height=25, width=100)
        Button(self, text='Unhide all', command=self.UnhideAll, x=110, y=5, height=25, width=100)

    def AddRow(self):
        ir = len(self.zt) - 1
        name, color, lcolor, bg = random.choice(people)
        self.zt.InsertRow(ir, '', (name, color), ('#%06x' % color, bg), 'Yes!', 'added')

    def UnhideAll(self):
        for i in range(len(self.zt)):
            self.zt.SetRowFiltered(i, True)

    def OnDoubleClickTable(self, ir, ic, x, y):
        self.zt.SetRowFiltered(ir, False)

    def OnEditStart(self, ir, ic, edit_hwnd):
        self.editRow = ir
        if ic == 4:
            self.checker.Checked = self.zt.GetItemText(ir, ic) == 'Yes!'

    def OnEditEnd(self, ir, ic, edit_hwnd):
        print('editend')
        if ic == 2:
            e = Edit.from_handle(edit_hwnd)
            text = e.Text.removeprefix('#')
            try:
                color = int(text, 16) % 0x1000000
            except ValueError:
                self.ShowError('Invalid color value!')
                return True
            self.zt.SetItemParam(ir, 'Name', color)
            e.Text = '#%06x' % color
        elif ic == 3:
            return False
        elif ic == 4:
            return False
        elif ic == 5:
            return False
        return False

    def OnAutoNewRow(self, ir):
        name, color, lcolor, bg = random.choice(people)
        self.zt.SetItemText(ir, 'Name', name)
        self.zt.SetItemParam(ir, 'Name', color)
        self.zt.SetItemText(ir, 'Color', '#%06x' % color)
        self.zt.SetItemParam(ir, 'Color', bg)
        self.zt.SetItemText(ir, 'Oh?', 'Yes!')
        self.zt.SetItemText(ir, 'Edit property', random.choice(('hello', 'goodbye')))
        self.zt.SetItemParam(ir, 'Edit property', random.choice((self.colorchooser, self.checker))._h)

    def OnChooseColor(self):
        color = self.zt.GetItemParam(self.editRow, 'Name')
        color = ChooseColor(self, color)
        if color is not None:
            self.zt.SetItemText(self.editRow, 'Color', '#%06x' % color)
            self.zt.SetItemParam(self.editRow, 'Name', color)
            winapi.SetProp(self.colorchooser._h, "ZTableModified", 1)

    def OnCheck(self):
        t = ('No!', 'Yes!')[self.checker.Checked]
        self.zt.SetItemText(self.editRow, 'Oh?', t)
        winapi.SetProp(self.checker._h, "ZTableModified", 1)

    def OnResize(self, width, height):
        try:
            self.zt.Move(5, 35, width - 10, height - 45)
        except AttributeError:
            pass


class PropertyEditor(PyWindow):
    _style = 0x50010000
    #_exstyle = 0x10000

    def __init__(self, parent):
        PyWindow.__init__(self, parent)
        self.attach_message(7, self.OnSetFocus)
        self.button = Button(self, command=self.OnButtonClicked, text='Edit externally')
        self.checkbutton = CheckBox(self, command=self.OnCheckbuttonClicked)
        self.combo = ComboBox(self)
        self.combo.Add('Maine', 'New Hampshire', 'Vermont')
        self.combo.OnSelectionChange = self.OnComboSelectionChange
        self.edit = Edit(self)
        self.e = None

    def OnEditStart(self, ir, ic, text):
        self.button.Hide()
        self.combo.Hide()
        self.edit.Hide()
        self.checkbutton.Hide()
        self.e = random.choice((self.button, self.combo, self.edit, self.checkbutton))
        self.SetEditorText(text)
        width, height = self.GetClientArea()
        x = -1
        if self.e is self.checkbutton:
            x = 2
        self.e.Move(x, -2, width+2, height+4)
        self.e.Show()

    def SetEditorText(self, text):
        if self.e is self.edit:
            self.edit.Text = text
            self.edit.Modified = False
            winapi.SetProp(self._h, "ZTableModified", 1)
        elif self.e is self.button or self.e is self.checkbutton:
            self.button.Text = text
        elif self.e is self.checkbutton:
            print(text)
            self.checkbutton.Text = ('No', 'Yes')[text and text[0].lower() in 'yt1']

    def OnSetFocus(self, msg, wp, lp):
        self.e.Focus()

    def OnResize(self, width, height):
        if self.e:
            self.e.Move(-1, -2, width+2, height+4)

    def OnEditEnd(self, ir, ic):
        if self.e is self.edit and not self.edit.Modified:
            return False
        self.parent.SetItemText(ir, ic, self.e.Text)
        return False

    def OnComboSelectionChange(self, si):
        winapi.SetProp(self._h, "ZTableModified", 1)

    def OnButtonClicked(self):
        res = self.ExternalEditor()
        if res is not None:
            winapi.SetProp(self._h, "ZTableModified", 1)
        self.SetEditorText(res)

    def OnCheckbuttonClicked(self):
        self.checkbutton.Text = ('No', 'Yes')[self.checkbutton.Checked]
        winapi.SetProp(self._h, "ZTableModified", 1)

    def ExternalEditor(self):
        self.parent.parent.ShowInfo('This is, was, or may will be an external editor.')
        return 'edited in external editor'


if __name__ == '__main__':
    tl = ZTableDemoWindow()
    tl.Show()
    sys.exit(tl.Mainloop())
