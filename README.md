
# ZTable - the better Windows table widget

ZTable is a easy-to-use Win32 table window with a more data-centric focus than Windows' builtin ListView.
Written in pure C.

## Win32 Usage

Just call the `SetupClass` function to register the window class. 
The class name of the table is "ZTable". 
Message and notification constants are in `ztable.h`

## Python Usage

ZTable is geared toward Python usage and therefore includes a Python class wrapper.

### Note:

The Python class uses an unpublished home-made Win32 ctypes wrapper,
but it probably could be made to work with pywin32
with some tweaking.

Example usage:

```python
from gui32.toplevel import Toplevel
from ztable import *

class YourWindow(Toplevel):
    def __init__(self):
        ...
        self.table = ZTable(self)
        self.table.SetColumns(
            self.table.MakeColumn(headerText="Column One"),
            self.table.MakeColumn(headerText="Column Two"),
            ...
        )
        for i in range(10):
            self.table.InsertRow(-1, 'value one', 'value two', ...)
        ...
```

## Features

### Column alignment

Text can be aligned left, right, or center per column, except for the header text, which is always centered.

### Resizable  columns

If a column does not have `ZTC_NORESIZE`, it can be resized by dragging their dividers with the mouse.

Or you can set the width programmatically:

```python
table.SetColumnWidth(column_index, 157)
```

The width of a column can be specified on creation, along with a minimum and maximum width:

```python
table.SetColumns(
    table.MakeColumn(headerText='157 pixels wide', width=157, minWidth=20, maxWidth=350),
    ...
)
```

If a column has `ZTC_DEFSIZEONRCLICK`, it will snap to its default size when its header is right-clicked.
The default size is set on creation:

```python
table.SetColumns(
    table.MakeColumn(headerText='Snaps to 100px width', defaultWidth=100),
    ...
)
```

### Table notifications

The table notifies of right-clicks anywhere on the window.

```python
class YourWindow(Toplevel):
    def __init__(self):
        ...
        self.table.OnRightClick = self.OnTableRightClicked
        ...

    def OnTableRightClicked(self, row_index, column_index, x, y):
        # Post a popup menu at (x, y)
        ...
```

### Column notifications

Columns can be set to report double-click notifications with `ZTC_NOTIFYDBLCLICKS`.
Columns that do not have `ZTC_DEFAULTSORT` report header clicks.

```python
class YourWindow(Toplevel):
    def __init__(self):
        ...
        self.table.OnDoubleClick = self.OnCellDoubleClicked
        self.table.OnHeaderClicked = self.OnTableHeaderClicked
        ...
    
    def OnTableHeaderClicked(self, x, y, column_index):
        # Post a menu to select options
        ...

    def OnCellDoubleClicked(self, row_index, column_index, x, y):
        # Open a dialog to show more details
        ...
```

### Sorting
Columns with `ZTC_DEFAULTSORT` automatically sort the table when their header is clicked.
Otherwise, you can implement custom sorting by setting the `OnHeaderClicked` callback to a function that calls `Sort`, as shown:
```python
class YourWindow(Toplevel):
    def __init__(self):
        ...
        self.table.OnHeaderClicked = self.OnTableHeaderClicked
        ...
    
    def OnTableHeaderClicked(self, row_index, column_index):
        lParam = 0xc0ffee
        reverse = -1 # Use opposite of current column reverse state
        self.table.Sort(self.ComparisonCallback, column_index, reverse, lParam)

    def ComparisonCallback(self, hwnd_table, itemptr_1, itemptr_2, column_index, lParam):
        if lParam != 0xc0ffee:
            print("I ordered coffee!")
        text1 = itemptr_1.contents.items[column_index].text.value
        text2 = itemptr_2.contents.items[column_index].text.value
        if text1 == text2:
            return 0
        elif text1 > text2:
            return 1
        else:
            return -1
```

### Editing

Columns with `ZTC_EDITABLE` can edited by double-clicking a cell in that column. 
If the editing notifications are not handled, simple string editing is performed.
To customize the editing process, set the `OnEditStart` and/or `OnEditEnd` callbacks, as shown:

```python
from gui32.edit import Edit

class YourWindow(Toplevel):
    def __init__(self):
        ...
        self.table.OnEditStart = self.OnEditStart
        self.table.OnEditEnd = self.OnEditEnd
        ...
    
    def OnEditStart(self, row_index, column_index, edit_window_handle):
        # The edit control currently has the text of the cell we are editing
        edit = Edit.from_handle(edit_window_handle)
        text = edit.Text
        # We can set the text that appears in the edit control
        edit.Text = "Text being edited: " + text.strip()

    def OnEditEnd(self, row_index, column_index, edit_window_handle):
        # The edit control currently has the text that the user entered
        edit = Edit.from_handle(edit_window_handle)
        text = edit.Text
        # We can set the text that is set to the cell being edited
        edit.Text = text.removeprefix("Text being edited: ")
        # If we succeeded, return False
        # Had we decided the text was invalid, we could return True 
        # to select the contents of the edit control and continue editing
        return False
```

You can start an editing process programmatically:

```python
table.BeginEditing(row_index, column_index)
```

You can end an editing process:

```python
# This will call table.OnEditEnd
# It may or may not actually end the editing.
table.EndEditing()
```

You can cancel an editing process:

```python
# This will not call table.OnEditEnd
# The editing will end regardless of the text entered
table.CancelEditing()
```

You can see if an editing procedure is currently active:

```python
print(table.InEditing)
```

You can combine these to make a one-shot try at ending 
the editing procedure gracefully (e.g., when closing the program):

```python
# Is there an active editing procedure?
if table.InEditing:
    # If so, try to save the value
    table.EndEditing()
    # Did it work?
    if table.InEditing:
        # If not, cancel and go on
        table.CancelEditing()
```

### Custom row colors

Any row can either have default colors, or a custom text color and background fill brush.
Set the colors as shown:

```python
# Set to default colors
table.SetRowColors(row_index)
# Set text color to a medium gray
table.SetRowColors(row_index, textColor=0x808080)
# Create a custom background brush
from gui32.backend import winapi, wintypes
red_checkered_brush = winapi.CreateBrushIndirect(
    wintypes.LOGBRUSH(2, 0x0000ff, 5))
# Set background fill to red checkers
table.SetRowColors(row_index, fillBrush=red_checkered_brush)
# Set both text color and background fill
table.SetRowColors(row_index, textColor=0x808080, fillBrush=red_checkered_brush)
# You can also set these when creating the row
table.InsertRow(-1, 'Values', ..., textColor=0x808080, fillBrush=red_checkered_brush)
```

### Custom cell colors

If a column has `ZTC_CUSTOMBG`, the user-defined parameter for each cell 
in that column is a win32 brush that is the background fill of the cell.
If the parameter is `0`, the default or row-specific background is used.

If a column has `ZTC_CUSTOMFG`, the user-defined parameter for each cell
in that column is a `COLORREF` value that is the text color of the cell.
If the parameter is `CLR_DEFAULT`, the default or row-specific text color is used.

A column cannot have both custom background and custom text color.

Example:

```python
table.SetColumns(
    table.MakeColumn(headerText='Custom text color', flags=ZTC_CUSTOMFG),
    table.MakeColumn(headerText='Custom background', flags=ZTC_CUSTOMBG),
)
from gui32.backend import winapi
colors = (
    (0x101010, winapi.CreateSolidBrush(0xf0f0f0)),
    (0x123456, winapi.CreateSolidBrush(0x654321)),
    ...
)
for color, brush in colors:
    row_index = table.InsertRow(-1, 'howdee', 'do')
    table.SetItemParam(row_index, 0, color)
    table.SetItemParam(row_index, 1, brush)
```

### Row filtering

Rows can be hidden or shown arbitrarily, as shown:

```python
# Hide the row without losing any data
table.SetRowFiltered(row_index, False)
# Show the row again
table.SetRowFiltered(row_index, True)
```

This does not affect the internal row table or a row's associated index, 
so 
```python
for row_index in range(len(table)):
    print(table.GetItemText(row_index, 0))
```
will print data for hidden rows also.

### Custom line height

You can set the height of rows in the table, in multiples of line height, as shown:

```python
# Make all the rows in the table 3 lines high
table.SetRowHeight(3)
```

### Multiline cells

If a column has `ZTC_MULTILINE`, the text in each cell will be drawn with `DT_WORDBREAK` flag set.
This means that `\r\n` line separators will cause line breaks. 
Due to the limited capabilities of the Win32 `DrawText` function, the text will not be vertically centered.
This feature is obviously only useful in conjunction with custom line height.

### New row generation

If you turn on automatic row generation:
```python
table.SetAutoMakeNewRow(True)
```
then when an editing procedure has successfully finished in the last row in the table, 
a new row is appended to the table.
The table sends a notification when it adds a row so that you can customize the contents of each new row.
To handle this notification:

```python
class YourWindow(Toplevel):
    def __init__(self):
        ...
        self.table.OnAutoNewRow = self.OnAutoNewRow
        ...

    def OnAutoNewRow(self, row_index):
        self.table.SetItemText(row_index, 0, 'This row was added.')
```

### Row selector column

If the first column has `ZTC_SELECTOR`, it is displayed without cell text, with a uniform fill color, and with an arrow in the currently selected row.
If new-row generation is enabled, it displays a `*` icon in the last row.
```python
table.SetColumns(
    table.MakeColumn(flags=ZTC_SELECTOR),
    ...
)
```

## Features not implemented

- Keybinding to start editing
- Custom editing widgets - mostly done
- Icons and bitmaps (at least check marks)? - supported sort-of for selector column
- Allow setting default row colors?
- Combined custom background and text color per cell
- Custom borders per cell
- Allow text (line numbers etc) in selector column, highlighted selection there

## License

MIT License