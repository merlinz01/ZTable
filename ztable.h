#ifndef _ZTABLE_H_
#define _ZTABLE_H_

/*** Types ***/

typedef struct _ZTableColumn {
    LPWSTR headerText;
    SHORT headerTextLength;
    USHORT width;
    USHORT minWidth;
    USHORT maxWidth;
    USHORT defaultWidth;
    USHORT flags;
} ZTableColumn;

typedef struct _ZTableItem {
    LPWSTR text;
    SHORT textLength;
    LPARAM lParam;
} ZTableItem;

typedef struct _ZTableRow {
    ZTableItem* items;
    HBRUSH hFill;
    COLORREF textColor;
    BOOL filtered; // TRUE if shown, FALSE if hidden
    SHORT index;
    SHORT filteredIndex;
} ZTableRow;

typedef struct _NM_ZTABLE {
    NMHDR hdr;
    SHORT ir;
    SHORT ic;
    POINT location;
    LPVOID data;
} NM_ZTABLE;

typedef struct _ZTableData {
    HWND hwnd;
    HWND parentHwnd;
    /* Client area size */
    LONG cWidth;
    LONG cHeight;
    HRGN rowsClipRgn;
    /* Edit window */
    HWND editWin;
    BYTE inEditing;
    SHORT editRow;
    SHORT editCol;
    /* Array sizes */
    SHORT nCols;
    SHORT nRows;
    SHORT nRowsFiltered;
    /* Scroll data */
    LONG scrollPosX;
    LONG scrollPosY;
    LONG scrollRegionX;
    LONG scrollRegionY;
    SHORT lineSizeX;
    SHORT lineSizeY;
    LONG pageSizeX;
    LONG pageSizeY;
    /* Selection */
    SHORT selectedRow;
    SHORT lastSortColumn;
    /* Header data */
    SHORT headerHeight;
    BYTE textPadX;
    COLORREF headerTextColor;
    HFONT headerFont;
    HBRUSH headerFill;
    HBRUSH headerFillHover;
    HBRUSH headerFillPressed;
    HPEN headerOutline;
    SHORT pressedHeader;
    SHORT hoverHeader;
    HPEN headerArrowPen;
    /* Column resizing */
    BYTE resizing;
    SHORT resizeCol;
    LONG resizeOrg;
    /* column array */
    ZTableColumn* columns;
    /* Row array */
    ZTableRow* rows;
    ZTableRow** rowsFiltered;
    /* Row data */
    USHORT rowHeight;
    COLORREF rowTextColor;
    COLORREF rowTextColorSelected;
    HFONT rowFont;
    HPEN rowOutline;
    HBRUSH rowFill;
    HBRUSH rowFillAlternate;
    HBRUSH rowFillSelected;
    LPWSTR emptyText;
    SHORT emptyTextLength;
    /* Notification struct */
    NM_ZTABLE nm;
    /* Last-error storage */
    //INT lastError;
} ZTableData;

typedef INT (CALLBACK SORTPROC)(HWND, ZTableRow*, ZTableRow*, SHORT, LPARAM);

typedef struct _ZTableSortInfo {
    SORTPROC* sortproc;
    SHORT column;
    BOOL reversed;
    LPARAM lParam;
} ZTableSortInfo;

/*** Function definitions ***/
extern BOOL SetupClass();
BOOL ZTableBeginEditing(ZTableData* self, SHORT ir, SHORT ic);
BOOL ZTableCancelEditing(ZTableData* self);
BOOL ZTableDeleteRow(ZTableData* self, SHORT index);
BOOL ZTableEndEditing(ZTableData* self);
BOOL ZTableGetCellAt(ZTableData* self, LONG x, LONG y, SHORT* ir, SHORT* ic);
BOOL ZTableGetCellRect(ZTableData* self, RECT* rect, SHORT ir, SHORT ic);
BOOL ZTableGetItemParam(ZTableData* self, SHORT ir, SHORT ic, LPARAM* pLParam);
BOOL ZTableGetItemPtr(ZTableData* self, SHORT ir, SHORT ic, ZTableItem** pItem);
BOOL ZTableGetNextEditable(ZTableData* self, SHORT ir, SHORT ic, SHORT* nir, SHORT* nic);
BOOL ZTableGetNextVisibleRow(ZTableData* self, SHORT ir, SHORT* nir);
BOOL ZTableGetPrevEditable(ZTableData* self, SHORT ir, SHORT ic, SHORT* nir, SHORT* nic);
BOOL ZTableGetPrevVisibleRow(ZTableData* self, SHORT ir, SHORT* nir);
BOOL ZTableInsertRow(ZTableData* self, SHORT index, ZTableRow* row);
BOOL ZTableSeeColumn(ZTableData* self, SHORT ic);
BOOL ZTableSeeRow(ZTableData* self, SHORT ir);
BOOL ZTableSelect(ZTableData* self, SHORT ir);
BOOL ZTableSetColumns(ZTableData* self, ZTableColumn* cols, SHORT nCols);
BOOL ZTableSetColumnWidth(ZTableData* self, SHORT ic, USHORT width);
BOOL ZTableSetEmptyText(ZTableData* self, LPCTSTR text, SHORT textLength);
BOOL ZTableSetItemText(ZTableData* self, SHORT ir, SHORT ic, LPCWSTR text, UINT textLength);
BOOL ZTableSetItemParam(ZTableData* self, SHORT ir, SHORT ic, LPARAM lParam);
BOOL ZTableSetRowColors(ZTableData* self, SHORT ir, ZTableRow* row);
BOOL ZTableSetRowFiltered(ZTableData* self, SHORT ir, BOOL filtered);
BOOL ZTableSetRowHeight(ZTableData* self, USHORT height);
BOOL ZTableSort(ZTableData* self, ZTableSortInfo* info);
BOOL ZTableUpdateColors(ZTableData*);
BOOL ZTableUpdateFilteredRows(ZTableData*);
BOOL ZTableUpdateScrollInfo(ZTableData*);

/*** Defines ***/
/* Window messages */
#define ZTM_FIRST WM_USER+0x300

#define ZTM_SETEMPTYTEXT ZTM_FIRST+0x01
#define ZTM_SORT ZTM_FIRST+0x02
#define ZTM_SETROWHEIGHT ZTM_FIRST+0x03

#define ZTM_SETCOLUMNS ZTM_FIRST+0x11
#define ZTM_SETCOLUMNWIDTH ZTM_FIRST+0x12

#define ZTM_SELECT ZTM_FIRST+0x21
#define ZTM_INSERTROW ZTM_FIRST+0x22
#define ZTM_GETNROWS ZTM_FIRST+0x23
#define ZTM_DELETEROW ZTM_FIRST+0x24
#define ZTM_GETSELECTEDROW ZTM_FIRST+0x25
#define ZTM_SETROWCOLORS ZTM_FIRST+0x26
#define ZTM_SETROWFILTERED ZTM_FIRST+0x27

#define ZTM_ENDEDITING ZTM_FIRST+0x31
#define ZTM_BEGINEDITING ZTM_FIRST+0x32
#define ZTM_CANCELEDITING ZTM_FIRST+0x33
#define ZTM_GETINEDITING ZTM_FIRST+0x34

#define ZTM_GETCELLAT ZTM_FIRST+0x41
#define ZTM_GETCELLRECT ZTM_FIRST+0x42
#define ZTM_SETITEMTEXT ZTM_FIRST+0x43
#define ZTM_GETITEMPTR ZTM_FIRST+0x44
#define ZTM_GETITEMPARAM ZTM_FIRST+0x45
#define ZTM_SETITEMPARAM ZTM_FIRST+0x46

/* Window notifications */
#define ZTN_FIRST (0U-1000U)
#define ZTN_LAST (0U-1025U)
#define ZTN_EDITEND (ZTN_FIRST-1)
#define ZTN_EDITSTART (ZTN_FIRST-2)
#define ZTN_HEADERCLICKED (ZTN_FIRST-3)
#define ZTN_DBLCLICKITEM (ZTN_FIRST-4)
#define ZTN_RCLICK (ZTN_FIRST-5)
/* Column flags */
#define ZTC_ALIGN_LEFT      0x0000
#define ZTC_ALIGN_CENTER    0x0001
#define ZTC_ALIGN_RIGHT     0x0002
#define ZTC_EDITABLE        0x0004
#define ZTC_DEFAULTSORT     0x0008
#define ZTC_REVERSED        0x0010
#define ZTC_NOTIFYDBLCLICKS 0x0020
#define ZTC_NORESIZE        0x0040
#define ZTC_DEFSIZEONRCLICK 0x0080
#define ZTC_CUSTOMBG        0x0100
#define ZTC_MULTILINE       0x0200

/*** Error enum - not implemented ***/
/*enum ZTableError {
    NO_ERROR_ = 0,
    SYSTEM_OR_OTHER_ERROR,
    COLUMN_NOT_EDITABLE,
};*/

#endif // _ZTABLE_H_
