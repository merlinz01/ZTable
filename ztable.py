
import ctypes
# noinspection PyUnresolvedReferences
from gui32.backend import winapi, wintypes
# noinspection PyUnresolvedReferences
from gui32.window import ChildWindow

# For whatever crazy reason:
ctypes.windll.kernel32.LoadLibraryW('ZTable')
ZTable = ctypes.WinDLL('ZTable')
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
        ('textColor', wintypes.COLORREF),
        ('index', wintypes.SHORT),
        ('indexFiltered', wintypes.SHORT))


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
ZTN_AUTONEWROW = ZTN_FIRST - 6

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
ZTC_CUSTOMFG = 0x0400
ZTC_SELECTOR = 0x0800
ZTC_SINGLECLICKEDIT = 0x1000
ZTC_SLOWDCLICKEDIT = 0x2000
ZTC_DOUBLECLICKEDIT = 0x4000

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
    'ZTC_CUSTOMFG',
    'ZTC_SELECTOR',
    'ZTC_SINGLECLICKEDIT',
    'ZTC_DOUBLECLICKEDIT',
    'ZTC_SLOWDCLICKEDIT',
]


class ZTable(ChildWindow):
    _class = 'ZTable'
    _style = 0x50310000
    _exstyle = 0x200

    def __init__(self, parent, *args, **kw):
        """ZTable main class.
            ZTable is a homemade Win32 table window with the following features:
                - Columns aligned left, right, or center
                - Resizable or fixed-width columns
                - Double and right click and header click notifications
                - Sorting by column, with custom comparisons or default string comparisons
                - Value editing, with before and after notifications
                - Custom text and fill colors in any row
                - Custom fill color or text color for cells in a column
                - Rows can be hidden and shown without creating/deleting
                - Custom height of all rows in multiples of line height
                - Multiline cells per column setting
        """
        ChildWindow.__init__(self, parent, *args, **kw)
        self.parent.attach_notify(self._id, self._on_parent_notify)
        self.OnDoubleClick = None
        self.OnRightClick = None
        self.OnEditStart = None
        self.OnEditEnd = None
        self.OnHeaderClicked = None
        self.OnAutoNewRow = None

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
        elif code == ZTN_AUTONEWROW:
            if self.OnAutoNewRow:
                self.OnAutoNewRow(nm.ir)
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
        """Returns the number of rows (including hidden) in the table"""
        return self._msg(0x723, 0, 0, False)

    def __bool__(self):
        return True

    def AddNewRow(self):
        self._msg(0x728, 0, 0)

    def BeginEditing(self, ir, ic):
        self._msg(0x732, 0, winapi.MAKELONG(ir, ic))

    def CancelEditing(self):
        """Cancel the current editing procedure without saving contents of the edit control"""
        self._msg(0x733, 0, 0)

    def DeleteRow(self, index):
        """Delete the specified row"""
        self._msg(0x724, 0, index)

    def EndEditing(self):
        """End the current editing procedure, calling `OnEditEnd()`"""
        return self._msg(0x731, 0, 0)

    def GetCellAt(self, x, y):
        """Get the row and column indices of the cell visible at point `(x, y)`.
            If in header, `row` == -1. If not under any column, `column` == -1.
            If not anywhere, `column` == -1 and `row` == -2."""
        cell = wintypes.LONG()
        self._msg(0x741, ctypes.addressof(cell), winapi.MAKELONG(x, y))
        return winapi.LOWORD(cell.value, True), winapi.HIWORD(cell.value, True)

    def GetItem(self, row, col):
        """Returns a `ctypes` structure for the internal item struct for the specified cell"""
        iptr = LPZTableItem()
        col = self._col(col)
        self._msg(0x744, ctypes.addressof(iptr), winapi.MAKELONG(row, col))
        return iptr.contents

    def GetItemParam(self, row, col):
        """Returns the user-defined value for the specified cell"""
        p = wintypes.LPARAM()
        col = self._col(col)
        self._msg(0x744, winapi.MAKELONG(row, col), ctypes.addressof(p))
        return p.value
    GetItemLParam = GetItemParam

    def GetItemText(self, row, col):
        """Returns the text for the specified cell"""
        col = self._col(col)
        return self.GetItem(row, col).text

    @property
    def InEditing(self):
        """Returns `True` if there is a current editing procedure"""
        return bool(self._msg(0x734, 0, 0, False))

    def InsertRow(self, index, *items, fillBrush=None, textColor=0xff000000):
        """Inserts a row at the specified index.
            `items` should be either `str` or `tuple(str, int)`,
                where `str` is the text for the cell, and `int` is the user-defined parameter for the cell.
                The count of `items` must equal the number of columns in the table.
            `fillBrush` is a handle to a gdi32 brush for the row background, or `None` for default background.
            `textColor` is a `COLORREF` value for the row text color, or `CLR_DEFAULT` for default text color.
        """
        if index == -1:
            index = len(self)-1
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
        return index

    @staticmethod
    def MakeColumn(headerText='', width=100, minWidth=10, maxWidth=0xffff, defaultWidth=None, flags=0):
        """Returns a column structure for passing to `SetColumns()`.
            `headerText` is the text to display on the column header.
            `width` is the initial column width in pixels.
            `minWidth` is the minimum width in pixels for resizable columns.
            `maxWidth` is the maximum width in pixels for resizable columns.
            `defaultWidth` is the width in pixels that columns with `ZTC_DEFSIZEONRCLICK`
                are set to when the header is right-clicked.
            `flags` is a combination of `ZTC_*` constants
        """
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
        """Index of the selected row, or `None` for no selection"""
        res = self._msg(0x725, 0, 0, False)
        if res == -1:
            return None
        return res

    @Selection.setter
    def Selection(self, row):
        if row is None:
            row = -1
        self._msg(0x721, 0, row)

    def SetAutoMakeNewRow(self, make=True):
        self._msg(0x704, int(bool(make)), 0)

    def SetColumns(self, *cols):
        """Set the array of columns for the table.
            `cols` are column structures created with `MakeColumn()`
            Can only be called with no rows in the table to prevent undefined behavior.
        """
        assert not len(self), "setting columns with existing rows is not allowed"
        self._ncols = n = len(cols)
        array = (ZTableColumn * n)(*cols)
        self._msg(0x711, ctypes.addressof(array), n)
        self.col = {c.headerText: i for i, c in enumerate(array)}

    def SetColumnWidth(self, col, width):
        """Set the width of the specified column in pixels"""
        col = self._col(col)
        self._msg(0x712, col, width)

    def SetEmptyText(self, text):
        """Set the text displayed when no rows are visible"""
        s = wintypes.LPCWSTR(text)
        self._msg(0x701, ctypes.cast(s, wintypes.LPVOID).value, len(text))

    def SetItemParam(self, row, col, lParam):
        """Set the user-defined data for the specified cell"""
        col = self._col(col)
        self._msg(0x746, winapi.MAKELONG(row, col), lParam)

    def SetItemText(self, row, col, text):
        """Set the text for the specified cell"""
        col = self._col(col)
        cell = winapi.MAKELONG(row, col)
        text = wintypes.LPCWSTR(text)
        self._msg(0x743, ctypes.cast(text, wintypes.LPVOID).value, cell)

    def SetRowColors(self, index, fillBrush=None, textColor=-0x1000000):
        """Set the background brush and text color for the specified row"""
        row = ZTableRow(None, fillBrush, textColor)
        self._msg(0x726, ctypes.addressof(row), index)

    def SetRowFiltered(self, index, filtered):
        """Set whether the specified row is shown (`True` for visible, `False` for hidden)"""
        self._msg(0x727, int(bool(filtered)), index)

    def SetRowHeight(self, height):
        """Set the height in lines for all the rows in the table"""
        self._msg(0x703, height, 0)

    def Sort(self, key, col, reverse=-1, lparam=0):
        """Sort the table using insertion sort algorithm.
            `key` is a function with five arguments that compares
            two `ZTableItem` structs from the column being sorted.
                def key(hwnd_of_table, item_1, item_2, column, lparam_given):
                    return 0 if item_1 == item_2 else -1 if item_1 < item_2 else 1
            `col` is the index of the column to sort.
            `reverse` specifies whether to reverse the sorting. `-1` means opposite of current.
            `lparam` is the user-defined data to pass to the key function as the third argument.
        """
        col = self._col(col)
        func = SORTPROC(key)
        info = ZTableSortInfo(func, col, reverse, lparam)
        self._msg(0x702, ctypes.addressof(info), 0)
