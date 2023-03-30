#define UNICODE
#if defined(UNICODE) && !defined(_UNICODE)
#define _UNICODE
#elif defined(_UNICODE) && !defined(UNICODE)
#define UNICODE
#endif

#include <stdio.h>
#include <stddef.h>
#include <tchar.h>
#include <windows.h>
#include <commctrl.h>
#include "ztable.h"

#define NOEDITSUBCLASS

LRESULT CALLBACK ZTableWindowProcedure(HWND, UINT, WPARAM, LPARAM);
#ifdef NOEDITSUBCLASS
LRESULT CALLBACK ZTableEditWindowProcedure(HWND, UINT, WPARAM, LPARAM);
#else
LRESULT CALLBACK ZTableEditSubclassProc(
    HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
#endif // NOEDITSUBCLASS
INT CALLBACK ZTableDefaultSortProc(HWND hwnd, ZTableRow* row1, ZTableRow* row2, SHORT ic, LPARAM lParam);
LRESULT ZTable_WM_DESTROY       (ZTableData* self, WPARAM wParam, LPARAM lParam);
LRESULT ZTable_WM_HSCROLL       (ZTableData* self, WPARAM wParam, LPARAM lParam);
LRESULT ZTable_WM_KEYDOWN       (ZTableData* self, WPARAM wParam, LPARAM lParam);
LRESULT ZTable_WM_LBUTTONDBLCLK (ZTableData* self, WPARAM wParam, LPARAM lParam);
LRESULT ZTable_WM_LBUTTONDOWN   (ZTableData* self, WPARAM wParam, LPARAM lParam);
LRESULT ZTable_WM_LBUTTONUP     (ZTableData* self, WPARAM wParam, LPARAM lParam);
LRESULT ZTable_WM_MOUSELEAVE    (ZTableData* self, WPARAM wParam, LPARAM lParam);
LRESULT ZTable_WM_MOUSEMOVE     (ZTableData* self, WPARAM wParam, LPARAM lParam);
LRESULT ZTable_WM_MOUSEWHEEL    (ZTableData* self, WPARAM wParam, LPARAM lParam);
LRESULT ZTable_WM_PAINT         (ZTableData* self, WPARAM wParam, LPARAM lParam);
LRESULT ZTable_WM_RBUTTONDOWN   (ZTableData* self, WPARAM wParam, LPARAM lParam);
LRESULT ZTable_WM_SIZE          (ZTableData* self, WPARAM wParam, LPARAM lParam);
LRESULT ZTable_WM_VSCROLL       (ZTableData* self, WPARAM wParam, LPARAM lParam);

HCURSOR ARROW = NULL;
HCURSOR COLRESIZE = NULL;
HANDLE processHeap = NULL;

extern BOOL SetupClass()
{
    if (processHeap != NULL) return TRUE; /* already called */
    if ((processHeap = GetProcessHeap())==NULL) return FALSE;
    WNDCLASSEX wc;
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_DBLCLKS | CS_GLOBALCLASS;
    wc.lpfnWndProc = ZTableWindowProcedure;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = sizeof(ZTableData*);
    wc.hInstance = GetModuleHandle(NULL);
    wc.hIcon = NULL;
    wc.hCursor = NULL;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = _T("ZTable");
    wc.hIconSm = NULL;
    if (!RegisterClassEx(&wc)) return FALSE;
    ARROW = LoadCursor(NULL, IDC_ARROW);
    if (ARROW==NULL) return FALSE;
    COLRESIZE = LoadCursor(NULL, IDC_SIZEWE);
    if (COLRESIZE==NULL) return FALSE;
    return TRUE;
}

inline LRESULT ZTableNotifyParent(
        ZTableData* self, UINT code, SHORT ir, SHORT ic, LONG x, LONG y, LPVOID data) {
    self->nm.hdr.hwndFrom = self->hwnd;
    int myID = GetDlgCtrlID(self->hwnd);
    self->nm.hdr.idFrom = myID;
    self->nm.hdr.code = code;
    self->nm.ir = ir;
    self->nm.ic = ic;
    self->nm.location.x = x;
    self->nm.location.y = y;
    self->nm.data = data;
    return SendMessage(self->parentHwnd, WM_NOTIFY, (WPARAM)myID, (LPARAM)(&(self->nm)));
}

#define BREAKPOINT(n) MessageBox(self->hwnd, _T("Breakpoint " # n), _T("Debug"), MB_OK)
#define SELF ((ZTableData*) GetWindowLongPtr(hwnd, 0))
LRESULT CALLBACK ZTableWindowProcedure (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    ZTableData* self;
    switch (message) {
    case WM_CREATE:
#define CHECK(h) {if ((h) == NULL) return -1;}
        CHECK(self = (ZTableData*)HeapAlloc(
                         processHeap, HEAP_ZERO_MEMORY, sizeof(ZTableData)));
        self->hwnd = hwnd;
        self->parentHwnd = GetParent(hwnd);
        INT fontHeight = MulDiv(GetDeviceCaps(GetDC(hwnd), LOGPIXELSY), 9, 72);
        self->lineSizeX = fontHeight*2;
        self->selectedRow = -1;
        self->lastSortColumn = -1;
        self->headerHeight = fontHeight*2;
        CHECK(self->headerFont = CreateFont(
            -fontHeight, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, _T("Segoe UI")));
        self->textPadX = MulDiv(fontHeight, 2, 9);
        CHECK(self->headerFill = GetSysColorBrush(COLOR_BTNFACE));
        CHECK(self->headerFillHover = GetSysColorBrush(COLOR_3DLIGHT));
        CHECK(self->headerFillPressed = GetSysColorBrush(COLOR_3DHIGHLIGHT));
        self->pressedHeader = -1;
        self->hoverHeader = -1;
        /**self->columns = NULL;**/
        /**self->rows = NULL;**/
        /**self->nRows = 0;**/
        /**self->nRowsFiltered = 0;**/
        /**self->nCols = 0;**/
        self->rowHeight = self->lineSizeY = MulDiv(fontHeight, 5, 3);
        CHECK(self->rowFont = CreateFont(
            -fontHeight, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, _T("Segoe UI")));
        CHECK(self->rowFill = GetSysColorBrush(COLOR_WINDOW));
        CHECK(self->rowFillAlternate = GetSysColorBrush(COLOR_BTNFACE));
        CHECK(self->rowFillSelected = GetSysColorBrush(COLOR_HIGHLIGHT));
        // the void* cast is to silence the ptr/int comparison warning
        CHECK((void*)ZTableUpdateColors(self));
        CHECK(self->emptyText = HeapAlloc(processHeap, HEAP_ZERO_MEMORY, 1));
        self->emptyTextLength = 0;
        CHECK((void*)ZTableSetEmptyText(self, _T("No items to show"), 16));
        SetWindowLongPtr(hwnd, 0, (LONG)self);
        ZTableUpdateScrollInfo(self);
        return 0;
#undef CHECK
    case WM_DESTROY:
        return ZTable_WM_DESTROY(SELF, wParam, lParam);
    case WM_GETDLGCODE:
        return DLGC_WANTARROWS;
    case WM_HSCROLL:
        return ZTable_WM_HSCROLL(SELF, wParam, lParam);
    case WM_KEYDOWN:
        return ZTable_WM_KEYDOWN(SELF, wParam, lParam);
    case WM_LBUTTONDBLCLK:
        return ZTable_WM_LBUTTONDBLCLK(SELF, wParam, lParam);
    case WM_LBUTTONDOWN:
        return ZTable_WM_LBUTTONDOWN(SELF, wParam, lParam);
    case WM_LBUTTONUP:
        return ZTable_WM_LBUTTONUP(SELF, wParam, lParam);
    case WM_MOUSELEAVE:
        return ZTable_WM_MOUSELEAVE(SELF, wParam, lParam);
    case WM_MOUSEMOVE:
        return ZTable_WM_MOUSEMOVE(SELF, wParam, lParam);
    case WM_MOUSEWHEEL:
        return ZTable_WM_MOUSEWHEEL(SELF, wParam, lParam);
    case WM_PAINT:
        return ZTable_WM_PAINT(SELF, wParam, lParam);
    case WM_RBUTTONDOWN:
        return ZTable_WM_RBUTTONDOWN(SELF, wParam, lParam);
    case WM_SIZE:
        return ZTable_WM_SIZE(SELF, wParam, lParam);
    case WM_SYSCOLORCHANGE:
        ZTableUpdateColors(SELF);
        InvalidateRect(hwnd, NULL, TRUE);
        break;
    case WM_VSCROLL:
        return ZTable_WM_VSCROLL(SELF, wParam, lParam);
    /******************************************************************************/
    case ZTM_BEGINEDITING:
        return ZTableBeginEditing(SELF, LOWORD(lParam), HIWORD(lParam));
    case ZTM_CANCELEDITING:
        return ZTableCancelEditing(SELF);
    case ZTM_DELETEROW:
        return ZTableDeleteRow(SELF, (SHORT)lParam);
    case ZTM_ENDEDITING:
        return ZTableEndEditing(SELF);
    case ZTM_GETCELLAT:
        SHORT ir, ic;
        LRESULT res = ZTableGetCellAt(SELF, LOWORD(lParam), HIWORD(lParam), &ir, &ic);
        *((LONG*)wParam) = MAKEWPARAM(ir, ic);
        return res;
    case ZTM_GETCELLRECT:
        return ZTableGetCellRect(SELF, (RECT*)wParam, LOWORD(lParam), HIWORD(lParam));
    case ZTM_GETINEDITING:
        return SELF->inEditing;
    case ZTM_GETITEMPTR:
        return ZTableGetItemPtr(SELF, LOWORD(lParam), HIWORD(lParam), (ZTableItem**)wParam);
    case ZTM_GETNROWS:
        return SELF->nRows;
    case ZTM_GETSELECTEDROW:
        return SELF->selectedRow;
    case ZTM_INSERTROW:
        return ZTableInsertRow(SELF, (SHORT)lParam, (ZTableRow*)wParam);
    case ZTM_SELECT:
        return ZTableSelect(SELF, (SHORT)lParam);
    case ZTM_SETCOLUMNS:
        return ZTableSetColumns(SELF, (ZTableColumn*)wParam, (SHORT)lParam);
    case ZTM_SETCOLUMNWIDTH:
        return ZTableSetColumnWidth(SELF, (SHORT)wParam, (USHORT)lParam);
    case ZTM_SETEMPTYTEXT:
        return ZTableSetEmptyText(SELF, (LPCWSTR)wParam, (SHORT)lParam);
    case ZTM_SETITEMTEXT:
        return ZTableSetItemText(SELF, LOWORD(lParam), HIWORD(lParam), (LPCTSTR)wParam, -1);
    case ZTM_GETITEMPARAM:
        return ZTableGetItemParam(SELF, LOWORD(wParam), HIWORD(wParam), (LPARAM*)lParam);
    case ZTM_SETITEMPARAM:
        return ZTableSetItemParam(SELF, LOWORD(wParam), HIWORD(wParam), (LPARAM)lParam);
    case ZTM_SETROWCOLORS:
        return ZTableSetRowColors(SELF, (SHORT)lParam, (ZTableRow*)wParam);
    case ZTM_SETROWFILTERED:
        return ZTableSetRowFiltered(SELF, (SHORT)lParam, (BOOL)wParam);
    case ZTM_SETROWHEIGHT:
        return ZTableSetRowHeight(SELF, (USHORT)wParam);
    case ZTM_SORT:
        return ZTableSort(SELF, (ZTableSortInfo*)wParam);
    default:
        return DefWindowProc(hwnd, message, wParam, lParam);
    }
    return 0;
}
#undef SELF

#define CHECK(v) if (!(v)) {printf("CHECK("  #v ") failed\n"); return FALSE;}
//#define CHECK_E(v, err) if (!(v)) {printf("CHECK("  #v ") failed\n"); self->lastError=err; return FALSE;}

BOOL ZTableBeginEditing(ZTableData* self, SHORT ir, SHORT ic)
{
    CHECK(self->columns[ic].flags & ZTC_EDITABLE)
    CHECK(ZTableSelect(self, ir));
    CHECK(ZTableSeeColumn(self, ic));
    RECT rect;
    CHECK(ZTableGetCellRect(self, &rect, ir, ic));
    if (self->editWin==NULL) {
        CHECK(self->editWin = CreateWindowEx(
                                  WS_EX_CLIENTEDGE, WC_EDIT, NULL,
                                  WS_CHILD|WS_VISIBLE|WS_TABSTOP|ES_AUTOHSCROLL,
                                  0, 0, 0, 0, self->hwnd, NULL, NULL, NULL));
#ifdef NOEDITSUBCLASS
        LONG oldWndProc = SetWindowLongPtr(
                              self->editWin, GWLP_WNDPROC, (LONG)ZTableEditWindowProcedure);
        CHECK(oldWndProc);
        CHECK(SetProp(self->editWin, _T("OLDWP"), (HANDLE)oldWndProc));
        CHECK(SetProp(self->editWin, _T("ZTABLE"), self->hwnd));
#else
        CHECK(SetWindowSubclass(self->hwnd, ZTableEditSubclassProc, 0, (DWORD_PTR)self->hwnd));
#endif // NOEDITSUBCLASS
        SendMessage(self->editWin, WM_SETFONT, (WPARAM)self->rowFont, 0);
        SendMessage(self->editWin, EM_SETMARGINS,
                    EC_LEFTMARGIN|EC_RIGHTMARGIN,
                    MAKELPARAM(self->textPadX-3,self->textPadX-3));
    }
    LONG style = GetWindowLong(self->editWin, GWL_STYLE);
    style = (style & ~0x3) | (self->columns[ic].flags & 0x3);
    SetWindowLong(self->editWin, GWL_STYLE, style);
    CHECK(SetWindowText(self->editWin, self->rows[ir].items[ic].text));
    CHECK(MoveWindow(
              self->editWin,
              rect.left,
              rect.top,
              rect.right-rect.left-1,
              rect.bottom-rect.top-1,
              FALSE));
    CHECK(PostMessage(self->editWin, EM_SETMODIFY, FALSE, 0));
    ZTableNotifyParent(self, ZTN_EDITSTART, ir, ic, 0, 0, (LPVOID)self->editWin);
    CHECK(PostMessage(self->editWin, EM_SETSEL, 0, -1));
    ShowWindow(self->editWin, SW_SHOW);
    SetFocus(self->editWin);
    //ShowCaret(self->editWin);
    self->editRow = ir;
    self->editCol = ic;
    self->inEditing = TRUE;
    return TRUE;
}

BOOL ZTableCancelEditing(ZTableData* self)
{
    if (self->inEditing) {
        self->inEditing = FALSE;
        ShowWindow(self->editWin, SW_HIDE);
    }
    return TRUE;
}

BOOL ZTableDeleteRow(ZTableData* self, SHORT index)
{
    CHECK(index >= 0 && index < self->nRows); // also ensures (self->nRows > 0)
    SHORT nRows = self->nRows-1;
    /* Memory error protection */
    self->nRows = index;
    SIZE_T newSize = nRows*sizeof(ZTableRow);
    SIZE_T oldSize = HeapSize(processHeap, 0, self->rows);
    if (oldSize == 0xFFFFFFFF || (oldSize-newSize) > 0x100) {
        ZTableRow* newArray;
        CHECK(newArray = (ZTableRow*) HeapReAlloc(
                                processHeap,
                                HEAP_ZERO_MEMORY,
                                self->rows,
                                newSize));
        self->rows = newArray;
    }
    for (SHORT i = index; i < nRows; i++) {
        self->rows[i] = self->rows[i+1];
    }
    if (self->selectedRow == -1 || nRows == 0) {
        self->selectedRow = -1;
    } else if (index < self->selectedRow) {
        // guaranteed: self->selectedRow > 0
        self->selectedRow = self->selectedRow-1;
    } else {
        // keep the same or the last one
        self->selectedRow = min(self->selectedRow, nRows-1);
    }
    self->nRows = nRows;
    CHECK(ZTableUpdateFilteredRows(self));
    CHECK(ZTableUpdateScrollInfo(self));
    RECT rect;
    rect.left = 0;
    rect.top = self->headerHeight;
    rect.right = self->cWidth;
    rect.bottom = self->cHeight;
    CHECK(InvalidateRect(self->hwnd, &rect, TRUE));
    return TRUE;
}

BOOL ZTableEndEditing(ZTableData* self)
{
    if (self->inEditing == FALSE) return TRUE;
    self->inEditing = FALSE;
    if (SendMessage(self->editWin, EM_GETMODIFY, 0, 0)) {
        //if (SendMessage(self->parentHwnd, ZTN_EDITEND,
        //                (WPARAM)(self->editWin), MAKELPARAM(self->editRow, self->editCol))) {
        if (ZTableNotifyParent(self, ZTN_EDITEND, self->editRow, self->editCol, 0, 0, (LPVOID)self->editWin)) {
            SetFocus(self->editWin);
            CHECK(PostMessage(self->editWin, EM_SETSEL, 0, -1));
            //ShowCaret(self->editWin);
            self->inEditing = TRUE;
            return TRUE;
        } else {
            LPTSTR string = self->rows[self->editRow].items[self->editCol].text;
            INT ln = GetWindowTextLength(self->editWin);
            CHECK(string = HeapReAlloc(processHeap, HEAP_ZERO_MEMORY, string, (ln+1)*sizeof(TCHAR)));
            CHECK(GetWindowText(self->editWin, string, ln+1) || (GetLastError() == 0));
            self->rows[self->editRow].items[self->editCol].text = string;
            self->rows[self->editRow].items[self->editCol].textLength = ln;
            RECT rect;
            CHECK(ZTableGetCellRect(self, &rect, self->editRow, self->editCol));
            CHECK(InvalidateRect(self->hwnd, &rect, FALSE));
        }
    }
    ShowWindow(self->editWin, SW_HIDE);
    return TRUE;
}

BOOL ZTableGetCellAt(ZTableData* self, LONG x, LONG y, SHORT* ir, SHORT* ic)
{
    SHORT row, col;
    if (y < 0) {
        row = -2;
    } else if (y <= self->headerHeight) {
        row = -1;
    } else {
        LONG cy = self->headerHeight-self->scrollPosY;
        row = -2;
        for (SHORT row_ = 0; row_ < self->nRowsFiltered; row_++) {
            cy += self->rowHeight;
            if (cy >= y) {
                row = self->rowsFiltered[row_]->index;
                break;
            }
        }
    }
    col = -1;
    if (x >= 0) {
        LONG cx = -self->scrollPosX;
        for (SHORT col_ = 0; col_ < self->nCols; col_++) {
            cx += self->columns[col_].width;
            if (cx >= x) {
                col = col_;
                break;
            }
        }
    }
    *ic = col;
    *ir = row;
    return TRUE;
}

BOOL ZTableGetCellRect(ZTableData* self, RECT* rect, SHORT ir, SHORT ic)
{
    if (ir < -1 || ir >= self->nRows || ic < -1 || ic >= self->nCols) return FALSE;
    if (!self->rows[ir].filtered) {
        rect->left = rect->top = rect->right = rect->bottom = -1;
        return TRUE;
    }
    rect->left = -self->scrollPosX;
    if (ic == -1) { /* full row rect */
        rect->right = rect->left + self->scrollRegionX;
    } else {
        for (SHORT col = 0; col < ic; col++) {
            rect->left += self->columns[col].width;
        }
        rect->right = rect->left + self->columns[ic].width;
    }
    if (ir==-1) { /* header cell rect */
        rect->top = 0;
        rect->bottom = self->headerHeight;
    } else {
        rect->top = self->rows[ir].filteredIndex*self->rowHeight + self->headerHeight - self->scrollPosY;
        rect->bottom = rect->top + self->rowHeight;
    }
    return TRUE;
}

BOOL ZTableGetItemParam(ZTableData* self, SHORT ir, SHORT ic, LPARAM* pLParam) {
    CHECK(ir >= 0 && ir < self->nRows);
    CHECK(ic >= 0 && ic < self->nCols);
    *pLParam = self->rows[ir].items[ic].lParam;
    return TRUE;
}

BOOL ZTableGetItemPtr(ZTableData* self, SHORT ir, SHORT ic, ZTableItem** pItem) {
    CHECK(ir >= 0 && ir < self->nRows);
    CHECK(ic >= 0 && ic < self->nCols);
    *pItem = &(self->rows[ir].items[ic]);
    return TRUE;
}

BOOL ZTableGetNextEditable(ZTableData* self, SHORT ir, SHORT ic, SHORT* nir, SHORT* nic)
{
    for (SHORT col = ic+1; col < self->nCols; col++) {
        if (self->columns[col].flags & ZTC_EDITABLE) {
            *nir = ir;
            *nic = col;
            return TRUE;
        }
    }
    for (ir = ir+1; ir < self->nRows; ir++) {
        if (!self->rows[ir].filtered) continue;
        for (SHORT col = 0; col <= ic; col++) {
            if (self->columns[col].flags & ZTC_EDITABLE) {
                *nir = ir;
                *nic = col;
                return TRUE;
            }
        }
    }
    return FALSE;
}

BOOL ZTableGetNextVisibleRow(ZTableData* self, SHORT ir, SHORT* nir)
{
    for (ir = ir+1; ir < self->nRows; ir++) {
        if (!self->rows[ir].filtered) continue;
        *nir = ir;
        return TRUE;
    }
    return FALSE;
}

BOOL ZTableGetPrevEditable(ZTableData* self, SHORT ir, SHORT ic, SHORT* nir, SHORT* nic)
{
    for (SHORT col = ic-1; col > -1; col--) {
        if (self->columns[col].flags & ZTC_EDITABLE) {
            *nir = ir;
            *nic = col;
            return TRUE;
        }
    }
    for (ir = ir-1; ir >= 0; ir--) {
        if (!self->rows[ir].filtered) continue;
        for (SHORT col = self->nCols-1; col >= ic; col--) {
            if (self->columns[col].flags & ZTC_EDITABLE) {
                *nir = ir;
                *nic = col;
                return TRUE;
            }
        }
    }
    return FALSE;
}

BOOL ZTableGetPrevVisibleRow(ZTableData* self, SHORT ir, SHORT* nir)
{
    for (ir = ir-1; ir >= 0; ir--) {
        if (!self->rows[ir].filtered) continue;
        *nir = ir;
        return TRUE;
    }
    return FALSE;
}


BOOL ZTableInsertRow(ZTableData* self, SHORT index, ZTableRow* row)
{
    CHECK(self->columns != NULL);
    CHECK(self->nRows < 0xffff);
    if (index < 0) index += self->nRows;
    index = max(0, min(index, self->nRows));
    ZTableRow* newarray;
    SHORT nRows = (self->nRows)+1;
    /* Memory error protection */
    self->nRows = index;
    SIZE_T newSize = nRows*sizeof(ZTableRow);
    if (self->rows == NULL) {
        CHECK(newarray = (ZTableRow*) HeapAlloc(processHeap, HEAP_ZERO_MEMORY, newSize));
        self->rows = newarray;
    } else {
        SIZE_T oldSize = HeapSize(processHeap, 0, self->rows);
        if (oldSize < newSize || oldSize == 0xFFFFFFFF || (oldSize-newSize) > 0x100) {
            CHECK(newarray = (ZTableRow*) HeapReAlloc(
                                 processHeap,
                                 HEAP_ZERO_MEMORY,
                                 self->rows,
                                 newSize));
            self->rows = newarray;
        }
    }
    for (SHORT i = nRows-1; i > index; i--) {
        self->rows[i] = self->rows[i-1];
    }
    ZTableItem* items;
    CHECK(self->rows[index].items = items = (ZTableItem*)LocalAlloc(LPTR, sizeof(ZTableItem)*(self->nCols)));
    for (SHORT ic = 0; ic < self->nCols; ic++) {
        CHECK(items[ic].text = (LPTSTR)LocalAlloc(LPTR, sizeof(TCHAR)*(row->items[ic].textLength+1)));
        lstrcpyn(items[ic].text, row->items[ic].text, row->items[ic].textLength+1);
        items[ic].textLength = row->items[ic].textLength;
        items[ic].lParam = row->items[ic].lParam;
    }
    self->rows[index].hFill = row->hFill;
    self->rows[index].textColor = row->textColor;
    self->rows[index].filtered = TRUE;
    self->nRows = nRows;
    CHECK(ZTableUpdateFilteredRows(self));
    CHECK(ZTableUpdateScrollInfo(self));
    RECT rect;
    rect.left = 0;
    rect.top = self->headerHeight;
    rect.right = self->cWidth;
    rect.bottom = self->cHeight;
    CHECK(InvalidateRect(self->hwnd, &rect, FALSE));
    if (self->nRows == 1) {
        // Erase the empty text
        rect.bottom = rect.top+self->headerHeight;
        CHECK(InvalidateRect(self->hwnd, &rect, TRUE));
    }
    return TRUE;
}

BOOL ZTableSeeColumn(ZTableData* self, SHORT ic)
{
    CHECK(ic >= 0 && ic < self->nCols);
    LONG old = self->scrollPosX;
    LONG left = 0;
    for (SHORT col = 0; col < ic; col++) {
        left += self->columns[col].width;
    }
    LONG right = left + self->columns[ic].width;
    if (right > self->scrollPosX+self->pageSizeX) {
        self->scrollPosX = left - self->pageSizeX + (self->columns[ic].width);
    }
    if (left < self->scrollPosX) {
        self->scrollPosX = left;
    }
    if (self->scrollPosX != old) {
        CHECK(SetScrollPos(self->hwnd, SB_HORZ, self->scrollPosX+1, TRUE));
        CHECK(InvalidateRect(self->hwnd, NULL, FALSE));
    }
    return TRUE;
}

BOOL ZTableSeeRow(ZTableData* self, SHORT ir)
{
    CHECK(self->rows[ir].filtered);
    LONG old = self->scrollPosY;
    LONG top = self->rows[ir].filteredIndex*self->rowHeight;
    LONG bottom = top+self->rowHeight;
    if (bottom > self->scrollPosY + self->pageSizeY) {
        self->scrollPosY = top - self->pageSizeY + self->rowHeight;
    }
    if (top < self->scrollPosY) {
        self->scrollPosY = top;
    }
    if (self->scrollPosY != old) {
        CHECK(SetScrollPos(self->hwnd, SB_VERT, self->scrollPosY+1, TRUE));
        RECT rect;
        rect.left = 0;
        rect.top = self->headerHeight;
        rect.right = self->cWidth;
        rect.bottom = self->cHeight;
        CHECK(InvalidateRect(self->hwnd, &rect, FALSE));
    }
    return TRUE;
}
BOOL ZTableSelect(ZTableData* self, SHORT ir)
{
    CHECK(self->rows[ir].filtered);
    RECT rect;
    if (ir != self->selectedRow) {
        CHECK(ZTableGetCellRect(self, &rect, self->selectedRow, -1));
        CHECK(InvalidateRect(self->hwnd, &rect, FALSE));
        self->selectedRow = ir;
        CHECK(ZTableGetCellRect(self, &rect, self->selectedRow, -1));
        CHECK(InvalidateRect(self->hwnd, &rect, FALSE));
    }
    CHECK(ZTableSeeRow(self, ir));
    return TRUE;
}

BOOL ZTableSetColumns(ZTableData* self, ZTableColumn* cols, SHORT nCols)
{
    ZTableColumn* newarray;
    SIZE_T newSize = nCols*sizeof(ZTableColumn);
    if (self->columns == NULL) {
        CHECK(newarray = (ZTableColumn*) HeapAlloc(
                             processHeap,
                             HEAP_ZERO_MEMORY,
                             newSize));
        self->columns = newarray;
    } else {
        CHECK(self->nRows == 0);
        SIZE_T oldSize = HeapSize(processHeap, 0, self->columns);
        if (oldSize == 0xFFFFFFFF || oldSize < newSize || (oldSize-newSize) > 0x100) {
            CHECK(newarray = (ZTableColumn*) HeapReAlloc(
                                 processHeap,
                                 HEAP_ZERO_MEMORY,
                                 self->columns,
                                 newSize));
            self->columns = newarray;
        }
    }
    for (SHORT ic = 0; ic < nCols; ic++) {
        /* Memory error protection */
        self->nCols = ic;
        ZTableColumn* col = &(cols[ic]);
        if (self->columns[ic].headerText != NULL) {
            CHECK(self->columns[ic].headerText = (LPTSTR) LocalReAlloc(
                    (HLOCAL)self->columns[ic].headerText,
                    sizeof(TCHAR)*(col->headerTextLength+1),
                    0));
        } else {
            CHECK(self->columns[ic].headerText = (LPTSTR) LocalAlloc(
                    LPTR,
                    sizeof(TCHAR)*(col->headerTextLength+1)));
        }
        CHECK(lstrcpyn(
                  self->columns[ic].headerText,
                  cols[ic].headerText,
                  col->headerTextLength + 1));
        self->columns[ic].headerTextLength = col->headerTextLength;
        self->columns[ic].width = col->width;
        self->columns[ic].minWidth = col->minWidth;
        self->columns[ic].maxWidth = col->maxWidth;
        self->columns[ic].defaultWidth = col->defaultWidth;
        self->columns[ic].flags = col->flags;
    }
    self->nCols = nCols;
    CHECK(InvalidateRect(self->hwnd, NULL, TRUE));
    return TRUE;
}

BOOL ZTableSetColumnWidth(ZTableData* self, SHORT ic, USHORT width) {
    CHECK(0 <= ic && ic < self->nCols);
    if (width == (USHORT)-1) width = self->columns[ic].defaultWidth;
    RECT rect;
    CHECK(ZTableGetCellRect(self, &rect, -1, ic));
    rect.bottom = self->cHeight;
    rect.right = self->cWidth;
    InvalidateRect(self->hwnd, &rect, FALSE);
    if (width < self->columns[ic].width) {
        CHECK(ZTableGetCellRect(self, &rect, -1, self->nCols-1));
        rect.left = rect.right;
        rect.top = 0;
        rect.right = self->cWidth;
        rect.bottom = self->cHeight;
        CHECK(InvalidateRect(self->hwnd, &rect, TRUE))
    }
    self->columns[ic].width = width;
    ZTableUpdateScrollInfo(self);
    return TRUE;
}

BOOL ZTableSetEmptyText(ZTableData* self, LPCTSTR text, SHORT textLength) {
    LPTSTR buf = self->emptyText;
    CHECK(buf = HeapReAlloc(processHeap, HEAP_ZERO_MEMORY, buf, (textLength+1)*sizeof(TCHAR)));
    CHECK(lstrcpyn(buf, text, textLength + 1));
    self->emptyText = buf;
    self->emptyTextLength = textLength;
    return TRUE;
}

BOOL ZTableSetItemParam(ZTableData* self, SHORT ir, SHORT ic, LPARAM lParam)
{
    CHECK(ir >= 0 && ir < self->nRows);
    CHECK(ic >= 0 && ic < self->nCols);
    self->rows[ir].items[ic].lParam = lParam;
    RECT rect;
    CHECK(ZTableGetCellRect(self, &rect, ir, ic));
    CHECK(InvalidateRect(self->hwnd, &rect, FALSE));
    return TRUE;
}

BOOL ZTableSetItemText(ZTableData* self, SHORT ir, SHORT ic, LPCWSTR text, UINT textLength)
{
    CHECK(ir >= 0 && ir < self->nRows);
    CHECK(ic >= 0 && ic < self->nCols);
    if (textLength == -1) textLength = lstrlen(text);
    ZTableItem* item = &(self->rows[ir].items[ic]);
    LPTSTR string = item->text;
    CHECK(string = HeapReAlloc(processHeap, HEAP_ZERO_MEMORY, string, (textLength+1)*sizeof(TCHAR)));
    CHECK(lstrcpyn(string, text, textLength + 1));
    item->text = string;
    item->textLength = textLength;
    RECT rect;
    CHECK(ZTableGetCellRect(self, &rect, ir, ic));
    CHECK(InvalidateRect(self->hwnd, &rect, FALSE));
    return TRUE;
}

BOOL ZTableSetRowColors(ZTableData* self, SHORT ir, ZTableRow* row) {
    CHECK((0 <= ir) && (ir < self->nRows));
    self->rows[ir].hFill = row->hFill;
    self->rows[ir].textColor = row->textColor;
    RECT rect;
    CHECK(ZTableGetCellRect(self, &rect, ir, -1));
    CHECK(InvalidateRect(self->hwnd, &rect, FALSE));
    return TRUE;
}

BOOL ZTableSetRowFiltered(ZTableData* self, SHORT ir, BOOL filtered) {
    CHECK((0 <= ir) && (ir < self->nRows));
    if (filtered != self->rows[ir].filtered) {
        self->rows[ir].filtered = filtered;
        CHECK(ZTableUpdateFilteredRows(self));
        CHECK(ZTableUpdateScrollInfo(self));
        CHECK(InvalidateRect(self->hwnd, NULL, !filtered));
    }
    return TRUE;
}

BOOL ZTableSetRowHeight(ZTableData* self, USHORT height) {
    CHECK(0 < height && height < 0x100);
    self->rowHeight = height*self->lineSizeY;
    CHECK(ZTableUpdateScrollInfo(self));
    CHECK(InvalidateRect(self->hwnd, NULL, FALSE));
    return TRUE;
}

BOOL ZTableSort(ZTableData* self, ZTableSortInfo* info)
{
    CHECK(info->column < self->nCols && info->column >= -1);
    if (info->column != -1) {
        if (info->reversed == -1) {
            info->reversed = self->columns[info->column].flags & ZTC_REVERSED;
        }
        if (info->column == self->lastSortColumn) {
            info->reversed = !(info->reversed);
        }
        if (info->reversed) {
            self->columns[info->column].flags |= ZTC_REVERSED;
        } else {
            self->columns[info->column].flags &= ~ZTC_REVERSED;
        }
    }
    if (self->nRows > 1) {
        INT reverse_multiplier = (info->reversed ? -1 : 1);
        for (SHORT next_to_insert = 1; next_to_insert < self->nRows; next_to_insert++) {
            // We always have a sorted array below next_to_insert
            // Each item that we come to is floated down to its proper place in the sorted array
            // Extract the value so we can use its space
            ZTableRow value_being_inserted = self->rows[next_to_insert];
            // Start comparing with the value next below this
            SHORT am_i_bigger = next_to_insert-1;
            // Move everything bigger than this up a space
            // to make a hole for value_being_inserted
            // If we got to the bottom end of the array,
            // then value_being_inserted is the smallest one yet
            BYTE selected_row_may_be_moved = TRUE;
            while (am_i_bigger >= 0) {
                BOOL cmp = info->sortproc(
                    self->hwnd, &value_being_inserted, &(self->rows[am_i_bigger]),
                    info->column, info->lParam );
                if (cmp == -2) {
                    self->rows[am_i_bigger+1] = value_being_inserted;
                    return FALSE;
                }
                // If i_am_not_bigger, then to my right is
                // the proper place for value_being_inserted
                if ((cmp*reverse_multiplier) >= 0) break;
                // If i_am_bigger than value_being_inserted, shift me up one space
                self->rows[am_i_bigger+1] = self->rows[am_i_bigger];
                if (self->selectedRow == am_i_bigger) {
                    self->selectedRow += 1;
                    // We need this catch to prevent selectedRow from being
                    // moved because of equaling next_to_insert
                    // when it was merely bumped up a space to there
                    if (self->selectedRow == next_to_insert) {
                        selected_row_may_be_moved = FALSE;
                    }
                }
                // Now compare with the next one below me
                am_i_bigger -= 1;

            }
            // Insert the value in the space it belongs in the sorted sub-array
            self->rows[am_i_bigger+1] = value_being_inserted;
            if (self->selectedRow == next_to_insert && selected_row_may_be_moved) {
                self->selectedRow = am_i_bigger+1;
            }
        }
    }
    self->lastSortColumn = info->column;
    CHECK(ZTableUpdateFilteredRows(self));
    if (self->selectedRow != -1) {
        CHECK(ZTableSeeRow(self, self->selectedRow));
    }
    if (self->nRows > 1) {
        CHECK(InvalidateRect(self->hwnd, NULL, FALSE));
    }
    return TRUE;
}

BOOL ZTableUpdateColors(ZTableData* self) {
    self->headerTextColor = GetSysColor(COLOR_BTNTEXT);
    //DeleteObject(self->headerFillHover);
    //CHECK(self->headerFillHover = CreateSolidBrush(0xf9ebd9));
    //DeleteObject(self->headerFillPressed);
    //CHECK(self->headerFillPressed = CreateSolidBrush(0xf4dcbc));
    DeleteObject(self->headerOutline);
    CHECK(self->headerOutline = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_BTNSHADOW)));
    DeleteObject(self->headerArrowPen);
    CHECK(self->headerArrowPen = CreatePen(PS_SOLID, 2, GetSysColor(COLOR_GRAYTEXT)));
    self->rowTextColor = GetSysColor(COLOR_WINDOWTEXT);
    self->rowTextColorSelected = GetSysColor(COLOR_HIGHLIGHTTEXT);
    DeleteObject(self->rowOutline);
    CHECK(self->rowOutline = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_ACTIVEBORDER)));
    return TRUE;
}

BOOL ZTableUpdateFilteredRows(ZTableData* self) {
    SHORT nRowsFiltered = 0;
    for (SHORT ir = 0; ir < self->nRows; ir++) {
        if (self->rows[ir].filtered) {
            self->rows[ir].index = ir;
            self->rows[ir].filteredIndex = nRowsFiltered;
            nRowsFiltered++;
        }
    }
    self->nRowsFiltered = min(self->nRowsFiltered, nRowsFiltered);
    ZTableRow** newarray;
    SIZE_T newSize = nRowsFiltered*sizeof(ZTableRow*);
    if (self->rowsFiltered == NULL) {
        CHECK(newarray = (ZTableRow**) HeapAlloc(processHeap, HEAP_ZERO_MEMORY, newSize));
        self->rowsFiltered = newarray;
    } else {
        SIZE_T oldSize = HeapSize(processHeap, 0, self->rowsFiltered);
        if (oldSize < newSize || oldSize == 0xFFFFFFFF || (oldSize-newSize) > 0x100) {
            CHECK(newarray = (ZTableRow**) HeapReAlloc(
                                 processHeap,
                                 HEAP_ZERO_MEMORY,
                                 self->rowsFiltered,
                                 newSize));
            self->rowsFiltered = newarray;
        }
    }
    for (SHORT ir = 0; ir < self->nRows; ir++) {
        if (self->rows[ir].filtered) {
            self->rowsFiltered[self->rows[ir].filteredIndex] = &(self->rows[ir]);
        }
    }
    self->nRowsFiltered = nRowsFiltered;
    return TRUE;
}

BOOL ZTableUpdateScrollInfo(ZTableData* self)
{
    LONG xrgn = 0;
    for (LONG ic = 0; ic < self->nCols; ic++) {
        xrgn += self->columns[ic].width;
    }
    self->scrollRegionX = xrgn;
    self->scrollRegionY = self->nRowsFiltered * self->rowHeight;
    self->pageSizeX = max(self->cWidth, self->lineSizeX);
    self->scrollPosX = max(0, min(self->scrollPosX, self->scrollRegionX-self->pageSizeX));
    SCROLLINFO si;
    si.cbSize = sizeof(si);
    si.fMask = SIF_ALL;
    si.nMin = 1;
    si.nMax = self->scrollRegionX;
    si.nPage = self->pageSizeX;
    si.nPos = self->scrollPosX+1;
    CHECK(SetScrollInfo(self->hwnd, SB_HORZ, &si, TRUE));
    self->pageSizeY = max(self->cHeight-self->headerHeight, self->lineSizeY);
    self->scrollPosY = max(0, min(self->scrollPosY, self->scrollRegionY-self->pageSizeY));
    si.cbSize = sizeof(si);
    si.fMask = SIF_ALL;
    si.nMin = 1;
    si.nMax = self->scrollRegionY;
    si.nPage = self->pageSizeY;
    si.nPos = self->scrollPosY+1;
    CHECK(SetScrollInfo(self->hwnd, SB_VERT, &si, TRUE));
    return TRUE;
}

#undef CHECK

LRESULT ZTable_WM_DESTROY(ZTableData* self, WPARAM wParam, LPARAM lParam)
{
    if (self->inEditing) ZTableCancelEditing(self);
    if (self->editWin != NULL) DestroyWindow(self->editWin);
    DeleteObject(self->headerFont);
    DeleteObject(self->headerFill);
    DeleteObject(self->headerFillHover);
    DeleteObject(self->headerFillPressed);
    DeleteObject(self->headerOutline);
    DeleteObject(self->headerArrowPen);
    DeleteObject(self->rowFont);
    DeleteObject(self->rowOutline);
    DeleteObject(self->rowFill);
    DeleteObject(self->rowFillSelected);
    DeleteObject(self->rowsClipRgn);
    for (SHORT ir = 0; ir < self->nRows; ir++) {
        for (SHORT ic = 0; ic < self->nCols; ic++) {
            HeapFree(processHeap, 0, self->rows[ir].items[ic].text);
        }
    }
    HeapFree(processHeap, 0, self->rows);
    HeapFree(processHeap, 0, self->rowsFiltered);
    for (SHORT ic = 0; ic < self->nCols; ic++) {
        HeapFree(processHeap, 0, self->columns[ic].headerText);
    }
    HeapFree(processHeap, 0, self->columns);
    HeapFree(processHeap, 0, self);
    return 0;
}

LRESULT ZTable_WM_HSCROLL(ZTableData* self, WPARAM wParam, LPARAM lParam)
{
    LONG oldScrollPos = self->scrollPosX;
    if (self->inEditing) ZTableEndEditing(self);
    if (self->inEditing) ZTableCancelEditing(self);
    switch (LOWORD(wParam)) {
    case SB_THUMBTRACK:
    case SB_THUMBPOSITION:
        self->scrollPosX = HIWORD(wParam)-1;
        break;
    case SB_LINELEFT:
        self->scrollPosX = max(0, self->scrollPosX-self->lineSizeX);
        break;
    case SB_LINERIGHT:
        self->scrollPosX = min(
                               self->scrollPosX+self->lineSizeX,
                               self->scrollRegionX-self->pageSizeX);
        break;
    case SB_PAGELEFT:
        self->scrollPosX = max(0, self->scrollPosX-self->pageSizeX);
        break;
    case SB_PAGERIGHT:
        self->scrollPosX = min(self->scrollPosX+self->pageSizeX,
                               self->scrollRegionX-self->pageSizeX);
        break;
    default:
        return 0;
    }
    if (self->scrollPosX != oldScrollPos) {
        SetScrollPos(self->hwnd, SB_HORZ, self->scrollPosX+1, TRUE);
        InvalidateRect(self->hwnd, NULL, FALSE);
    }
    return 0;
}

LRESULT ZTable_WM_KEYDOWN(ZTableData* self, WPARAM wParam, LPARAM lParam)
{
    SHORT ir;
    switch (wParam) {
    case VK_UP:
        if (self->rows[self->selectedRow].filteredIndex > 0) {
            if (!ZTableGetPrevVisibleRow(self, self->selectedRow, &ir)) return 0;
            if (self->inEditing) {
                ZTableEndEditing(self);
                if (self->inEditing) return 0;
                ZTableBeginEditing(self, ir, self->editCol);
            } else ZTableSelect(self, ir);
        } else if (self->inEditing) ZTableEndEditing(self);
        break;
    case VK_DOWN:
        if (self->rows[self->selectedRow].filteredIndex+1 < self->nRowsFiltered) {
            if (!ZTableGetNextVisibleRow(self, self->selectedRow, &ir)) return 0;
            if (self->inEditing) {
                ZTableEndEditing(self);
                if (self->inEditing) return 0;
                ZTableBeginEditing(self, ir, self->editCol);
            } else ZTableSelect(self, ir);
        } else if (self->inEditing) ZTableEndEditing(self);
        break;
    }
    if (!self->inEditing) return 1;
    switch (wParam) {
    case VK_ESCAPE:
        ZTableCancelEditing(self);
        break;
    case VK_RETURN:
        ZTableEndEditing(self);
        break;
    case VK_TAB:
        ZTableEndEditing(self);
        if (self->inEditing) return 0;
        SHORT ir = -1, ic = -1;
        BOOL res;
        if (GetKeyState(VK_SHIFT) & 0x1000) {
            res = ZTableGetPrevEditable(self, self->editRow, self->editCol, &ir, &ic);
        } else {
            res = ZTableGetNextEditable(self, self->editRow, self->editCol, &ir, &ic);
        }
        if (res == TRUE) ZTableBeginEditing(self, ir, ic);
        break;
    default:
        return 1;
    }
    return 0;
}

LRESULT ZTable_WM_LBUTTONDBLCLK(ZTableData* self, WPARAM wParam, LPARAM lParam)
{
    LONG x = LOWORD(lParam), y = HIWORD(lParam);
    SHORT ir, ic;
    if (!ZTableGetCellAt(self, x, y, &ir, &ic) || ic == -1) return 0;
    if (ir < 0) return 0;
    if (self->columns[ic].flags & ZTC_EDITABLE) {
        ZTableBeginEditing(self, ir, ic);
    } else if (self->columns[ic].flags & ZTC_NOTIFYDBLCLICKS) {
        ZTableNotifyParent(self, ZTN_DBLCLICKITEM, ir, ic, x, y, NULL);
    }
    return 0;
}

LRESULT ZTable_WM_LBUTTONDOWN(ZTableData* self, WPARAM wParam, LPARAM lParam)
{
    if (self->inEditing) ZTableEndEditing(self);
    if (self->inEditing) return 0;
    SetFocus(self->hwnd);
    LONG x = LOWORD(lParam), y = HIWORD(lParam);
    SHORT ir, ic;
    if (!ZTableGetCellAt(self, x, y, &ir, &ic)) return 0;
    if (ir < -1) return 0;
    if (ir >= 0) {
        ZTableSelect(self, ir);
    } else if (ic == -1) {
        return 0;
    } else if (self->resizeCol != -1) {
        SetCapture(self->hwnd);
        self->resizeOrg = x - self->columns[self->resizeCol].width;
        self->resizing = TRUE;
    } else if (self->columns[ic].flags & ZTC_DEFAULTSORT) {
        self->pressedHeader = ic;
        ZTableSeeColumn(self, ic);
        ZTableSortInfo si;
        si.sortproc = ZTableDefaultSortProc;
        si.column = ic;
        si.reversed = -1;
        si.lParam = 0;
        ZTableSort(self, &si);
    } else {
        self->pressedHeader = ic;
        ZTableSeeColumn(self, ic);
        RECT rect;
        ZTableGetCellRect(self, &rect, -1, ic);
        InvalidateRect(self->hwnd, &rect, FALSE);
        ZTableNotifyParent(self, ZTN_HEADERCLICKED, ir, ic, x, y, NULL);
    }
    return 0;
}

LRESULT ZTable_WM_LBUTTONUP(ZTableData* self, WPARAM wParam, LPARAM lParam)
{
    if (self->pressedHeader != -1) {
        self->pressedHeader = -1;
        RECT rect;
        ZTableGetCellRect(self, &rect, -1, self->pressedHeader);
        InvalidateRect(self->hwnd, &rect, FALSE);
    }
    if (self->resizing) {
        ReleaseCapture();
        self->resizing = FALSE;
        ZTableUpdateScrollInfo(self);
        InvalidateRect(self->hwnd, NULL, TRUE);
    }
    return 0;
}

LRESULT ZTable_WM_MOUSELEAVE(ZTableData* self, WPARAM wParam, LPARAM lParam)
{
    if (self->hoverHeader != -1) {
        RECT rect;
        ZTableGetCellRect(self, &rect, -1, self->hoverHeader);
        InvalidateRect(self->hwnd, &rect, FALSE);
        self->hoverHeader = -1;
    }
    if (self->pressedHeader != -1) {
        RECT rect;
        ZTableGetCellRect(self, &rect, -1, self->pressedHeader);
        InvalidateRect(self->hwnd, &rect, FALSE);
        self->pressedHeader = -1;
    }
    return 0;
}

LRESULT ZTable_WM_MOUSEMOVE(ZTableData* self, WPARAM wParam, LPARAM lParam)
{
    RECT rect;
    SHORT x = LOWORD(lParam), y = HIWORD(lParam);
    if (self->resizing) {
        self->columns[self->resizeCol].width = min(max(x - self->resizeOrg,
                               self->columns[self->resizeCol].minWidth),
                           self->columns[self->resizeCol].maxWidth);
        rect.left = self->resizeOrg;
        rect.top = 0;
        rect.right = self->cWidth;
        rect.bottom = (self->nRows > 0 ? self->cHeight : self->headerHeight);
        InvalidateRect(self->hwnd, &rect, FALSE);
        SetCursor(COLRESIZE);
        return 0;
    }
    SHORT ir, ic;
    if (!ZTableGetCellAt(self, x, y, &ir, &ic) || ic == -1 || ir >= 0) {
        SetCursor(ARROW);
        if (self->hoverHeader != -1) {
            ZTableGetCellRect(self, &rect, -1, self->hoverHeader);
            InvalidateRect(self->hwnd, &rect, FALSE);
            self->hoverHeader = -1;
        }
        return 0;
    }
    ZTableGetCellRect(self, &rect, -1, ic);
    if (!(self->columns[ic].flags & ZTC_NORESIZE) && abs(x-rect.right) < 5) {
        SetCursor(COLRESIZE);
        self->resizeCol = ic;
    } else {
        SetCursor(ARROW);
        self->resizeCol = -1;
    }
    if (ic != self->hoverHeader) {
        InvalidateRect(self->hwnd, &rect, FALSE);
        if (self->hoverHeader != -1) {
            ZTableGetCellRect(self, &rect, -1, self->hoverHeader);
            InvalidateRect(self->hwnd, &rect, FALSE);
        }
        self->hoverHeader = ic;
        TRACKMOUSEEVENT tme;
        tme.cbSize = sizeof(TRACKMOUSEEVENT);
        tme.dwFlags = TME_LEAVE;
        tme.hwndTrack = self->hwnd;
        TrackMouseEvent(&tme);
    }
    return 0;
}

LRESULT ZTable_WM_MOUSEWHEEL(ZTableData* self, WPARAM wParam, LPARAM lParam)
{
    if ((self->scrollRegionY) <= (self->pageSizeY)) return 0;
    if (self->inEditing) ZTableEndEditing(self);
    if (self->inEditing) return 0;
    LONG old = self->scrollPosY;
    SHORT delta = self->lineSizeY * (SHORT)HIWORD(wParam) / -120;
    if (delta < 0) {
        self->scrollPosY = max(0, self->scrollPosY+delta);
    } else {
        self->scrollPosY = min(
                               self->scrollPosY+delta,
                               self->scrollRegionY-self->pageSizeY);
    }
    if (self->scrollPosY != old) {
        SetScrollPos(self->hwnd, SB_VERT, self->scrollPosY+1, TRUE);
        RECT rect;
        rect.left = 0;
        rect.top = self->headerHeight;
        rect.right = self->cWidth;
        rect.bottom = self->cHeight;
        InvalidateRect(self->hwnd, &rect, FALSE);
    }
    return 0;
}

LRESULT ZTable_WM_PAINT(ZTableData* self, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC dc = BeginPaint(self->hwnd, &ps);
    SetBkMode(dc, TRANSPARENT);
    HPEN oldp = SelectObject(dc, self->rowOutline);
    HFONT oldf = SelectObject(dc, self->rowFont);
    HBRUSH oldb = SelectObject(dc, self->rowFill);
    RECT cell;
    cell.top = self->headerHeight - self->scrollPosY;
    SelectClipRgn(dc, self->rowsClipRgn);
    SHORT ir;
    for (ir = 0; ir < self->nRowsFiltered; ir++) {
        cell.bottom = cell.top + self->rowHeight;
        if (cell.bottom < self->headerHeight) {
            cell.top = cell.bottom;
            continue;
        }
        if (cell.top > self->cHeight) break;
        ZTableRow* row = self->rowsFiltered[ir];
        HBRUSH rowBackground;
        if (row->index == self->selectedRow) {
            rowBackground = self->rowFillSelected;
            SetTextColor(dc, self->rowTextColorSelected);
        } else {
            if (row->hFill == NULL) {
                if (self->rowFillAlternate != NULL && ir % 2 == 1) {
                    rowBackground = self->rowFillAlternate;
                } else {
                    rowBackground = self->rowFill;
                }
            } else {
                rowBackground = row->hFill;
            }
            if (row->textColor == CLR_DEFAULT) {
                SetTextColor(dc, self->rowTextColor);
            } else {
                SetTextColor(dc, row->textColor);
            }
        }
        cell.left = -self->scrollPosX;
        for (SHORT ic = 0; ic < self->nCols; ic++) {
            cell.right = cell.left + self->columns[ic].width;
            if (cell.left-self->scrollPosX <= self->cWidth && cell.right >= -self->scrollPosX) {
                if (self->columns[ic].flags & ZTC_CUSTOMBG && row->items[ic].lParam != 0) {
                    SelectObject(dc, (HBRUSH)(row->items[ic].lParam));
                } else {
                    SelectObject(dc, rowBackground);
                }
                Rectangle(dc,
                          cell.left-1,
                          cell.top-1,
                          cell.right,
                          cell.bottom);
                RECT tRect = cell;
                tRect.left += self->textPadX;
                //tRect.top += self->rowTextPadY;
                tRect.right -= self->textPadX;
                //tRect.bottom -= self->rowTextPadY;
                DrawText(
                    dc,
                    row->items[ic].text,
                    row->items[ic].textLength,
                    &tRect,
                    ((self->columns[ic].flags & ZTC_MULTILINE ? DT_WORDBREAK : DT_SINGLELINE)
                         | DT_WORD_ELLIPSIS | DT_VCENTER
                         | (self->columns[ic].flags & 0x3)));
            }
            cell.left = cell.right;
        }
        cell.top = cell.bottom;
    }
    if (ir == 0) {
        SetTextColor(dc, self->rowTextColor);
        cell.left = 0;
        cell.top = self->headerHeight;
        cell.right = self->cWidth;
        cell.bottom = cell.top+self->headerHeight;
        DrawText(
            dc,
            self->emptyText,
            self->emptyTextLength,
            &cell,
            DT_SINGLELINE | DT_WORD_ELLIPSIS | DT_CENTER | DT_VCENTER);
    }
    SelectClipRgn(dc, NULL);
    //BREAKPOINT(WM_PAINT_rows_done);
    SelectObject(dc, self->headerOutline);
    SelectObject(dc, self->headerFill);
    SelectObject(dc, self->headerFont);
    SetTextColor(dc, self->headerTextColor);
    cell.left = -self->scrollPosX;
    for (SHORT ic = 0; ic < self->nCols; ic++) {
        //BREAKPOINT(WM_PAINT_painting_a_header);
        cell.right = cell.left + self->columns[ic].width;
        if (cell.left-self->scrollPosX <= self->cWidth && cell.right >= -self->scrollPosX) {
            //BREAKPOINT(WM_PAINT_a_header_is_visible);
            BYTE xd = 0, yd = 0;
            BYTE reselFill = FALSE;
            if (ic == self->pressedHeader) {
                //if (self->columns[ic].flags & ZTC_SORTABLE) {
                xd = 1;
                yd = 1;
                //}
                SelectObject(dc, self->headerFillPressed);
                reselFill = TRUE;
            } else if (ic == self->hoverHeader) {
                SelectObject(dc, self->headerFillHover);
                reselFill = TRUE;
            }
            Rectangle(dc,
                      cell.left-1,
                      -1,
                      cell.right,
                      self->headerHeight);
            RECT rect;
            rect.left = cell.left+self->textPadX+xd;
            rect.top = yd;
            rect.right = cell.right-self->textPadX+xd;
            rect.bottom = self->headerHeight+yd;
            DrawText(dc,
                     self->columns[ic].headerText,
                     self->columns[ic].headerTextLength,
                     &rect,
                     DT_SINGLELINE | DT_WORD_ELLIPSIS | DT_CENTER | DT_VCENTER);
            if (reselFill) {
                SelectObject(dc, self->headerFill);
            }
            if (ic == self->lastSortColumn) {
                SelectObject(dc, self->headerArrowPen);
                BYTE e = 5, m = 1;
                if (self->columns[ic].flags & ZTC_REVERSED) {
                    m = 5;
                    e = 1;
                }
                LONG center = cell.left + self->columns[ic].width/2;
                MoveToEx(dc, center-4, e, NULL);
                LineTo(dc, center, m);
                LineTo(dc, center+4, e);
                SelectObject(dc, self->headerOutline);
            }
            //BREAKPOINT(WM_PAINT_done_painting_visible_header);
        }
        cell.left = cell.right;
        //BREAKPOINT(WM_PAINT_done_painting_a_header);
    }
    if (cell.left < self->cWidth) {
        Rectangle(dc,
                  cell.left-1,
                  -1,
                  self->cWidth+1,
                  self->headerHeight);
        if (self->resizing && self->nRows) {
            RECT rect;
            rect.left = cell.left;
            rect.top = self->headerHeight;
            rect.right = self->cWidth;
            rect.bottom = self->cHeight;
            FillRect(dc, &rect, (HBRUSH)GetClassLongPtr(self->hwnd, GCLP_HBRBACKGROUND));
        }
    }
    SelectObject(dc, oldp);
    SelectObject(dc, oldb);
    SelectObject(dc, oldf);
    EndPaint(self->hwnd, &ps);
    //BREAKPOINT(WM_PAINT_end);
    return 0;
}

LRESULT ZTable_WM_RBUTTONDOWN(ZTableData* self, WPARAM wParam, LPARAM lParam) {
    if (self->resizing) {
        ReleaseCapture();
        self->resizing = FALSE;
        ZTableUpdateScrollInfo(self);
        InvalidateRect(self->hwnd, NULL, TRUE);
        return 0;
    }
    if (self->inEditing) ZTableEndEditing(self);
    if (self->inEditing) return 0;
    SetFocus(self->hwnd);
    LONG x = LOWORD(lParam), y = HIWORD(lParam);
    SHORT ir, ic;
    ZTableGetCellAt(self, x, y, &ir, &ic);
    if (ic != -1 && ir == -1 && self->columns[ic].flags & ZTC_DEFSIZEONRCLICK) {
        ZTableSetColumnWidth(self, ic, (USHORT)-1);
    }
    ZTableNotifyParent(self, ZTN_RCLICK, ir, ic, x, y, NULL);
    return 0;
}

LRESULT ZTable_WM_SIZE(ZTableData* self, WPARAM wParam, LPARAM lParam)
{
    if (self->inEditing) ZTableCancelEditing(self);
    self->cWidth = (LONG) LOWORD(lParam);
    self->cHeight = (LONG) HIWORD(lParam);
    DeleteObject(self->rowsClipRgn);
    self->rowsClipRgn = CreateRectRgn(0, self->headerHeight, self->cWidth, self->cHeight);
    ZTableUpdateScrollInfo(self);
    InvalidateRect(self->hwnd, NULL, FALSE);
    return 0;
}

LRESULT ZTable_WM_VSCROLL(ZTableData* self, WPARAM wParam, LPARAM lParam)
{
    LONG oldScrollPos = self->scrollPosY;
    if (self->inEditing) ZTableEndEditing(self);
    if (self->inEditing) ZTableCancelEditing(self);
    switch (LOWORD(wParam)) {
    case SB_THUMBPOSITION:
    case SB_THUMBTRACK:
        self->scrollPosY = HIWORD(wParam)-1;
        break;
    case SB_LINEUP:
        self->scrollPosY = max(0, self->scrollPosY-self->lineSizeY);
        break;
    case SB_LINEDOWN:
        self->scrollPosY = min(self->scrollPosY+self->lineSizeY,
                               self->scrollRegionY-self->pageSizeY);
        break;
    case SB_PAGEUP:
        self->scrollPosY = max(0, self->scrollPosY-self->pageSizeY);
        break;
    case SB_PAGEDOWN:
        self->scrollPosY = min(self->scrollPosY+self->pageSizeY,
                               self->scrollRegionY-self->pageSizeY);
        break;
    default:
        return 0;
    }
    if (self->scrollPosY != oldScrollPos) {
        SetScrollPos(self->hwnd, SB_VERT, self->scrollPosY+1, TRUE);
        RECT rect;
        rect.left = 0;
        rect.top = self->headerHeight;
        rect.right = self->cWidth;
        rect.bottom = self->cHeight;
        InvalidateRect(self->hwnd, &rect, FALSE);
    }
    return 0;
}

#ifdef NOEDITSUBCLASS
LRESULT CALLBACK ZTableEditWindowProcedure(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
#else
LRESULT CALLBACK ZTableEditSubclassProc(
    HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
#endif // NOEDITSUBCLASS
{
    switch (uMsg) {
    case WM_KILLFOCUS:
#ifdef NOEDITSUBCLASS
        SendMessage((HWND)GetProp(hwnd, _T("ZTABLE")), ZTM_ENDEDITING, 0, 0);
#else
        SendMessage((HWND)dwRefData, ZTM_ENDEDITING, 0, 0);
#endif // NOEDITSUBCLASS
        break;
    case WM_GETDLGCODE:
        return DLGC_WANTALLKEYS | DLGC_WANTTAB | DLGC_WANTARROWS;
    case WM_KEYDOWN:
        switch (wParam) {
        case VK_TAB:
        case VK_ESCAPE:
        case VK_RETURN:
        case VK_UP:
        case VK_DOWN:
#ifdef NOEDITSUBCLASS
            if (!SendMessage((HWND)GetProp(hwnd, _T("ZTABLE")), WM_KEYDOWN, wParam, lParam)) return 0;
#else
            if (!SendMessage((HWND)dwRefData, uMsg, wParam, lParam)) return 0;
#endif // NOEDITSUBCLASS
            break;
        }
        break;
    }
#ifdef NOEDITSUBCLASS
    return CallWindowProc((WNDPROC)GetProp(hwnd, _T("OLDWP")), hwnd, uMsg, wParam, lParam);
#else
    return DefSubclassProc(hwnd, uMsg, wParam, lParam);
#endif // NOEDITSUBCLASS
}

INT CALLBACK ZTableDefaultSortProc(HWND hwnd, ZTableRow* row1, ZTableRow* row2, SHORT ic, LPARAM lParam)
{
    return CompareString(
        LOCALE_USER_DEFAULT,
        SORT_STRINGSORT,
        row1->items[ic].text,
        row1->items[ic].textLength,
        row2->items[ic].text,
        row2->items[ic].textLength)-2;
}
