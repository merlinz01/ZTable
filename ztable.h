#ifndef ZTABLE_H_DEFINED
#define ZTABLE_H_DEFINED

 /*****************************************************************************\
|  ZTable is a homemade Win32 table window with the following features:         |
|    + Columns aligned left, right, or center                                   |
|    + Resizable or fixed-width columns                                         |
|    + Double and right click and header click notifications                    |
|    + Sorting by column, with custom comparisons or default string comparisons |
|    + Value editing, with before and after notifications                       |
|    + Custom text and fill colors in any row                                   |
|    + Custom fill color or text color for cells in a column                    |
|    + Rows can be hidden and shown without creating/deleting                   |
|    + Custom height of all rows in multiples of line height                    |
|    + Multiline cells per column setting                                       |
 \*****************************************************************************/

#define CLION
#ifdef CLION
#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedMacroInspection"
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
#endif

/*** Types ***/

typedef struct tagZTableColumn {
    LPWSTR headerText;
    SHORT headerTextLength;
    USHORT width;
    USHORT minWidth;
    USHORT maxWidth;
    USHORT defaultWidth;
    ULONG flags;
    HWND editWin;
} ZTableColumn;

typedef struct tagZTableItem {
    LPWSTR text;
    SHORT textLength;
    LPARAM lParam;
} ZTableItem;

typedef struct tagZTableRow {
    ZTableItem* items;
    HBRUSH hFill;
    COLORREF textColor;
    BOOL filtered; // TRUE if shown, FALSE if hidden
    SHORT index;
    SHORT indexFiltered;
} ZTableRow;

typedef struct tagNM_ZTABLE {
    NMHDR hdr;
    SHORT ir;
    SHORT ic;
    POINT location;
    LPVOID data;
} NM_ZTABLE;

typedef struct tagZTableData {
    HWND hwnd;
    HWND parentHwnd;
    /* Client area size */
    LONG cWidth;
    LONG cHeight;
    HRGN rowsClipRgn;
    /* Edit window */
    HWND editWin;
    HWND currentEditWin;
    BYTE inEditing;
    SHORT editRow;
    SHORT editCol;
    /* Array sizes */
    USHORT nCols;
    USHORT nRows;
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
    /* Selector column data */
    LPBYTE sc_selbitmap;
    WORD sc_selbitmapsize;
    LPBYTE sc_newbitmap;
    WORD sc_newbitmapsize;
    HBRUSH sc_fill;
    COLORREF sc_bmpfg;
    BOOL autoMakeNewRow;
} ZTableData;

typedef INT (CALLBACK SORTPROC)(HWND, ZTableRow*, ZTableRow*, SHORT, LPARAM);

typedef struct tagZTableSortInfo {
    SORTPROC* sortproc;
    SHORT column;
    BOOL reversed;
    LPARAM lParam;
} ZTableSortInfo;

/*** Function definitions ***/
extern BOOL SetupClass();
BOOL ZTableAddNewRow(ZTableData* self);
BOOL ZTableBeginEditing(ZTableData* self, SHORT ir, SHORT ic);
BOOL ZTableCancelEditing(ZTableData* self);
BOOL ZTableDeleteRow(ZTableData* self, SHORT index);
BOOL ZTableEndEditing(ZTableData* self);
BOOL ZTableGetCellAt(ZTableData* self, LONG x, LONG y, SHORT* ir, SHORT* ic);
BOOL ZTableGetCellRect(ZTableData* self, RECT* rect, SHORT ir, SHORT ic);
BOOL ZTableGetItemParam(ZTableData* self, SHORT ir, SHORT ic, LPARAM* pLParam);
BOOL ZTableGetItemPtr(ZTableData* self, SHORT ir, SHORT ic, ZTableItem** pItem);
BOOL ZTableGetNextEditable(ZTableData* self, USHORT ir, USHORT ic, USHORT *nir, USHORT *nic);
BOOL ZTableGetNextVisibleRow(ZTableData* self, USHORT ir, USHORT *nir);
BOOL ZTableGetPrevEditable(ZTableData* self, USHORT ir, USHORT ic, USHORT *nir, USHORT *nic);
BOOL ZTableGetPrevVisibleRow(ZTableData* self, USHORT ir, USHORT *nir);
BOOL ZTableInsertRow(ZTableData* self, SHORT index, ZTableRow* row);
BOOL ZTableSeeColumn(ZTableData* self, SHORT ic);
BOOL ZTableSeeRow(ZTableData* self, SHORT ir);
BOOL ZTableSelect(ZTableData* self, SHORT ir);
BOOL ZTableSetAutoMakeNewRow(ZTableData *self, BOOL make);
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

/* Class name */
#define ZTABLE_CLASSNAME _T("ZTable")

/* Window ID of default editor */
#define ZTABLE_EDIT_ID 0x10

/* Window messages */
#define ZTM_FIRST (WM_USER+0x300)

#define ZTM_SETEMPTYTEXT (ZTM_FIRST+0x01)
#define ZTM_SORT (ZTM_FIRST+0x02)
#define ZTM_SETROWHEIGHT (ZTM_FIRST+0x03)
#define ZTM_SETAUTONEW (ZTM_FIRST+0x04)

#define ZTM_SETCOLUMNS (ZTM_FIRST+0x11)
#define ZTM_SETCOLUMNWIDTH (ZTM_FIRST+0x12)

#define ZTM_SELECT (ZTM_FIRST+0x21)
#define ZTM_INSERTROW (ZTM_FIRST+0x22)
#define ZTM_GETNROWS (ZTM_FIRST+0x23)
#define ZTM_DELETEROW (ZTM_FIRST+0x24)
#define ZTM_GETSELECTEDROW (ZTM_FIRST+0x25)
#define ZTM_SETROWCOLORS (ZTM_FIRST+0x26)
#define ZTM_SETROWFILTERED (ZTM_FIRST+0x27)
#define ZTM_ADDNEWROW (ZTM_FIRST+0x28)

#define ZTM_ENDEDITING (ZTM_FIRST+0x31)
#define ZTM_BEGINEDITING (ZTM_FIRST+0x32)
#define ZTM_CANCELEDITING (ZTM_FIRST+0x33)
#define ZTM_GETINEDITING (ZTM_FIRST+0x34)

#define ZTM_GETCELLAT (ZTM_FIRST+0x41)
#define ZTM_GETCELLRECT (ZTM_FIRST+0x42)
#define ZTM_SETITEMTEXT (ZTM_FIRST+0x43)
#define ZTM_GETITEMPTR (ZTM_FIRST+0x44)
#define ZTM_GETITEMPARAM (ZTM_FIRST+0x45)
#define ZTM_SETITEMPARAM (ZTM_FIRST+0x46)

/* Window notifications */
#define ZTN_FIRST (0U-1000U)
#define ZTN_LAST (0U-1025U)
#define ZTN_EDITEND (ZTN_FIRST-1)
#define ZTN_EDITSTART (ZTN_FIRST-2)
#define ZTN_HEADERCLICKED (ZTN_FIRST-3)
#define ZTN_DBLCLICKITEM (ZTN_FIRST-4)
#define ZTN_RCLICK (ZTN_FIRST-5)
#define ZTN_AUTONEWROW (ZTN_FIRST-6)
#define ZTN_EDITORCOMMAND (ZTN_FIRST-7)

/* Column flags */
#define ZTC_ALIGN_LEFT      0x00000
#define ZTC_ALIGN_CENTER    0x00001
#define ZTC_ALIGN_RIGHT     0x00002
#define ZTC_EDITABLE        0x00004
#define ZTC_DEFAULTSORT     0x00008
#define ZTC_REVERSED        0x00010
#define ZTC_NOTIFYDBLCLICKS 0x00020
#define ZTC_NORESIZE        0x00040
#define ZTC_DEFSIZEONRCLICK 0x00080
#define ZTC_CUSTOMBG        0x00100
#define ZTC_MULTILINE       0x00200
#define ZTC_CUSTOMFG        0x00400
#define _ZTC_SELECTOR       0x00800
#define ZTC_SELECTOR        (_ZTC_SELECTOR | ZTC_NORESIZE | ZTC_NOHEADERCLICK)
#define ZTC_SINGLECLICKEDIT 0x01000
#define ZTC_DELAYCLICKEDIT  0x02000
#define ZTC_DOUBLECLICKEDIT 0x04000
#define ZTC_CUSTOMEDITOR    0x08000
#define ZTC_CUSTOMEDITORROW 0x10000
#define ZTC_NOHEADERCLICK   0x20000

/*** Error enum - not implemented ***/
/*enum ZTableError {
    NO_ERROR_ = 0,
    SYSTEM_OR_OTHER_ERROR,
    COLUMN_NOT_EDITABLE,
};*/

#ifdef CLION
#pragma clang diagnostic pop
#endif

#endif // ZTABLE_H_DEFINED
