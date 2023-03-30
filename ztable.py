
import ctypes
from gui32.backend import winapi, wintypes
from gui32.window import ChildWindow

ZTable = ctypes.windll.ZTable
winapi.function(ZTable, 'SetupClass', wintypes.BOOL, errcheck=winapi.ERROR_NULL)()


class ZTableColumn(ctypes.Structure):
    _fields_ = (
        ('headerText', wintypes.LPWSTR),
        ('headerTextLength', wintypes.SHORT),
        ('width', wintypes.USHORT),
        ('minWidth', wintypes.USHORT),
        ('maxWidth', wintypes.USHORT),
        ('defaultWidth', wintypes.USHORT),
        ('flags', wintypes.USHORT))


class ZTableItem(ctypes.Structure):
    _fields_ = (
        ('text', wintypes.LPCWSTR),
        ('textLength', wintypes.SHORT),
        ('lParam', wintypes.LPARAM))


LPZTableItem = ctypes.POINTER(ZTableItem)


class ZTableRow(ctypes.Structure):
    _fields_ = (
        ('items', LPZTableItem),
        ('hFill', wintypes.HBRUSH),
        ('textColor', wintypes.COLORREF))


LPZTableRow = ctypes.POINTER(ZTableRow)
SORTPROC = ctypes.WINFUNCTYPE(wintypes.INT, wintypes.HWND, LPZTableRow, LPZTableRow, wintypes.SHORT, wintypes.LPARAM)


class ZTableSortInfo(ctypes.Structure):
    _fields_ = (
        ('sortproc', SORTPROC),
        ('column', wintypes.SHORT),
        ('reversed', wintypes.BOOL),
        ('lParam', wintypes.LPARAM))


class NM_ZTABLE(ctypes.Structure):
    _fields_ = (
        ('hdr', wintypes.NMHDR),
        ('ir', wintypes.SHORT),
        ('ic', wintypes.SHORT),
        ('location', wintypes.POINT),
        ('data', wintypes.LPVOID))


LPNM_ZTABLE = ctypes.POINTER(NM_ZTABLE)

ZTN_FIRST = 0xfffffc18
ZTN_EDITEND = ZTN_FIRST - 1
ZTN_EDITSTART = ZTN_FIRST - 2
ZTN_HEADERCLICKED = ZTN_FIRST - 3
ZTN_DBLCLICKITEM = ZTN_FIRST - 4
ZTN_RCLICK = ZTN_FIRST - 5

ZTC_ALIGN_LEFT = 0x0000
ZTC_ALIGN_CENTER = 0x0001
ZTC_ALIGN_RIGHT = 0x0002
ZTC_EDITABLE = 0x0004
ZTC_DEFAULTSORT = 0x0008
ZTC_REVERSED = 0x0010
ZTC_NOTIFYDBLCLICKS = 0x0020
ZTC_NORESIZE = 0x0040
ZTC_DEFSIZEONRCLICK = 0x0080
ZTC_CUSTOMBG = 0x0100
ZTC_MULTILINE = 0x0200

__all__ = [
    'ZTable',
    'ZTN_FIRST',
    'ZTN_EDITEND',
    'ZTN_EDITSTART',
    'ZTN_HEADERCLICKED',
    'ZTN_DBLCLICKITEM',
    'ZTN_RCLICK',
    'ZTC_ALIGN_LEFT',
    'ZTC_ALIGN_CENTER',
    'ZTC_ALIGN_RIGHT',
    'ZTC_EDITABLE',
    'ZTC_DEFAULTSORT',
    'ZTC_REVERSED',
    'ZTC_NORESIZE',
    'ZTC_NOTIFYDBLCLICKS',
    'ZTC_DEFSIZEONRCLICK',
    'ZTC_CUSTOMBG',
    'ZTC_MULTILINE',
]


class ZTable(ChildWindow):
    _class = 'ZTable'
    _style = 0x50310000
    _exstyle = 0x200

    def __init__(self, *args, **kw):
        ChildWindow.__init__(self, *args, **kw)
        self.parent.attach_notify(self._id, self._on_parent_notify)
        self.OnDoubleClick = None
        self.OnRightClick = None
        self.OnEditStart = None
        self.OnEditEnd = None
        self.OnHeaderClicked = None

    def _on_parent_notify(self, pnmh):
        nm = ctypes.cast(pnmh, LPNM_ZTABLE).contents
        code = nm.hdr.code
        if code == ZTN_DBLCLICKITEM:
            if self.OnDoubleClick:
                self.OnDoubleClick(nm.ir, nm.ic, nm.location.x, nm.location.y)
        elif code == ZTN_RCLICK:
            if self.OnRightClick:
                self.OnRightClick(nm.ir, nm.ic, nm.location.x, nm.location.y)
        elif code == ZTN_EDITSTART:
            if self.OnEditStart:
                self.OnEditStart(nm.ir, nm.ic, nm.data)
        elif code == ZTN_EDITEND:
            if self.OnEditEnd:
                return self.OnEditEnd(nm.ir, nm.ic, nm.data)
        elif code == ZTN_HEADERCLICKED:
            if self.OnHeaderClicked:
                self.OnHeaderClicked(nm.location.x, nm.location.y, nm.ic)
        return 0

    def _msg(self, msg, wParam, lParam, errcheck=True):
        res = self.SendMessage(msg, wParam, lParam)
        if errcheck and not res:
            raise ctypes.WinError()
        return res

    def _col(self, c):
        if isinstance(c, int):
            return c
        if isinstance(c, str):
            return self.col[c]
        raise TypeError('Columns must be str or int')

    def __len__(self):
        return self._msg(0x723, 0, 0, False)

    def __bool__(self):
        return True

    def CancelEditing(self):
        self._msg(0x733, 0, 0)

    def DeleteRow(self, index):
        self._msg(0x724, 0, index)

    def EndEditing(self):
        return self._msg(0x731, 0, 0)

    def GetCellAt(self, x, y):
        cell = wintypes.LONG()
        self._msg(0x741, ctypes.addressof(cell), winapi.MAKELONG(x, y))
        return winapi.LOWORD(cell.value, True), winapi.HIWORD(cell.value, True)

    def GetItem(self, row, col):
        iptr = LPZTableItem()
        col = self._col(col)
        self._msg(0x744, ctypes.addressof(iptr), winapi.MAKELONG(row, col))
        return iptr.contents

    def GetItemLParam(self, row, col):
        col = self._col(col)
        return self.GetItem(row, col).lParam

    def GetItemText(self, row, col):
        col = self._col(col)
        return self.GetItem(row, col).text

    @property
    def InEditing(self):
        return self._msg(0x734, 0, 0, False)

    def InsertRow(self, index, *items, fillBrush=None, textColor=-0x1000000):
        if not items:
            items = ['']*self._ncols
        array = (ZTableItem*len(items))()
        for i, item in enumerate(items):
            if isinstance(item, (list, tuple)):
                item, lparam = item
            else:
                lparam = 0
            array[i].text = item
            array[i].textLength = len(item)
            array[i].lParam = lparam
        row = ZTableRow(ctypes.cast(ctypes.byref(array), LPZTableItem), fillBrush, textColor)
        self._msg(0x722, ctypes.addressof(row), index)

    @staticmethod
    def MakeColumn(headerText='', width=100, minWidth=10, maxWidth=0xffff, defaultWidth=None, flags=0):
        col = ZTableColumn()
        col.headerText = headerText
        col.headerTextLength = len(headerText)
        col.width = width
        col.minWidth = minWidth
        col.maxWidth = maxWidth
        col.defaultWidth = defaultWidth or width
        col.flags = flags
        return col

    @property
    def Selection(self):
        return self._msg(0x725, 0, 0, False)

    @Selection.setter
    def Selection(self, row):
        self._msg(0x721, 0, row)

    def SetColumns(self, *cols):
        self._ncols = n = len(cols)
        array = (ZTableColumn * n)(*cols)
        self._msg(0x711, ctypes.addressof(array), n)
        self.col = {c.headerText: i for i, c in enumerate(array)}

    def SetColumnWidth(self, col, width):
        col = self._col(col)
        self._msg(0x712, col, width)

    def SetEmptyText(self, text):
        s = wintypes.LPCWSTR(text)
        self._msg(0x701, ctypes.cast(s, wintypes.LPVOID).value, len(text))

    def SetItemParam(self, row, col, lParam):
        col = self._col(col)
        self._msg(0x746, winapi.MAKELONG(row, col), lParam)

    def SetItemText(self, row, col, text):
        col = self._col(col)
        cell = winapi.MAKELONG(row, col)
        text = wintypes.LPCWSTR(text)
        self._msg(0x743, ctypes.cast(text, wintypes.LPVOID).value, cell)

    def SetRowColors(self, index, fillBrush=None, textColor=-0x1000000):
        row = ZTableRow(None, fillBrush, textColor)
        self._msg(0x726, ctypes.addressof(row), index)

    def SetRowFiltered(self, index, filtered):
        self._msg(0x727, int(bool(filtered)), index)

    def SetRowHeight(self, height):
        self._msg(0x703, height, 0)

    def Sort(self, key, col, reverse=-1, lparam=0):
        col = self._col(col)
        func = SORTPROC(key)
        info = ZTableSortInfo(func, col, reverse, lparam)
        self._msg(0x702, ctypes.addressof(info), 0)
