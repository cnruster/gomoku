#include <windows.h>
#include <windowsx.h>
#include <assert.h>
#include "resource.h"


// This is the 4th step of my teaching program
// Author: Yiping Cheng, Beijing Jiaotong University
// ypcheng@bjtu.edu.cn    April 2017
// This code is protected by the MIT License



LRESULT WINAPI MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

typedef enum
{
    PLAYING, GAMEWON, GAMETIE
} GAME_STATE;
typedef enum
{
    COM_MAN, MAN_COM, MAN_MAN
} PLAYER_CFG;

// global variables
HINSTANCE HInstance;
HWND Hwnd;
HMENU Hmenu;
HDC Hdc;
HBITMAP HbmpBoard, HbmpStones, HbmpGridBoard;
HDC HdcmemBoard, HdcmemStones, HdcmemGridBoard;
RECT ClientRect;
BITMAP BmBoard, BmStones;
HBRUSH HbrGrid, HbrHilite, HbrCohilite;
LONG CBoard, SxBoard, SyBoard;
LONG CBox, RSxBox0, RSyBox0, SxBox0, SyBox0;

PLAYER_CFG PlCfg;
GAME_STATE GmStat;


#define NUM_BOXES      15

UINT CMov;
BYTE HistI[NUM_BOXES*NUM_BOXES], HistJ[NUM_BOXES*NUM_BOXES];
BYTE RenjuI[5], RenjuJ[5];
BYTE STATE[NUM_BOXES][NUM_BOXES];

const char szGomoku[] = "Gomoku";
const char szComMan[] = "Gomoku (computer-human)";
const char szManCom[] = "Gomoku (human-computer)";
const char szManMan[] = "Gomoku (human-human)";

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nShowCmd)
{
    WNDCLASS wc;
    MSG msg;

    HInstance = hInstance;

    wc.style = CS_HREDRAW|CS_VREDRAW;
    wc.lpfnWndProc = MainWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = HInstance;
    wc.hIcon = LoadIcon(HInstance, (LPCTSTR) ID_GOMOKU);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(RGB(0, 128, 0));
    wc.lpszMenuName = (LPCTSTR) ID_GOMOKU;
    wc.lpszClassName = szGomoku;

    RegisterClass(&wc);

    Hwnd = CreateWindow(szGomoku,    // registered class name
        szManMan,                    // window title
        WS_OVERLAPPEDWINDOW,    // window style
        CW_USEDEFAULT,          // horizontal position of window
        CW_USEDEFAULT,          // vertical position of window
        800,                    // window width
        700,          // window height
        NULL,       // handle to parent or owner window
        NULL,       // menu handle or child identifier
        HInstance,  // handle to application instance
        NULL);      // window-creation data

    if (!Hwnd)
        return 0;

    ShowWindow(Hwnd, nShowCmd);
    UpdateWindow(Hwnd);

    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    DeleteObject(wc.hbrBackground);

    return msg.wParam;
}


void NewGame();
void OnSize();
void OnPaint(HDC hdc);
void OnCommand(WPARAM wParam);
void OnLButtonDown(LPARAM lParam);

LRESULT WINAPI MainWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        OnPaint(BeginPaint(hWnd, &ps));
        EndPaint(hWnd, &ps);
        break;
    }

    case WM_SIZE:
        ClientRect.right = (LONG) LOWORD(lParam);
        ClientRect.bottom = (LONG) HIWORD(lParam);
        OnSize();
        
        break;

    case WM_COMMAND:
        OnCommand(wParam);

    case WM_LBUTTONDOWN:
        OnLButtonDown(lParam);
        break;

    case WM_CREATE:
    {
        Hdc = GetDC(hWnd);
        HdcmemGridBoard = CreateCompatibleDC(NULL);

        HbmpBoard = LoadBitmap(HInstance, (LPCTSTR) IDB_BITMAP1);
        HdcmemBoard = CreateCompatibleDC(NULL);
        SelectObject(HdcmemBoard, HbmpBoard);
        GetObject(HbmpBoard, sizeof(BITMAP), &BmBoard);

        HbrGrid = GetStockObject(BLACK_BRUSH);
        HbrHilite = CreateSolidBrush(RGB(200, 36, 146));
        HbrCohilite = CreateSolidBrush(RGB(55, 219, 109));

        GetClientRect(hWnd, &ClientRect);
        OnSize();

        HbmpStones = LoadBitmap(HInstance, (LPCTSTR) IDB_BITMAP2);
        HdcmemStones = CreateCompatibleDC(NULL);
        SelectObject(HdcmemStones, HbmpStones);
        GetObject(HbmpStones, sizeof(BITMAP), &BmStones);

        Hmenu = GetMenu(hWnd);
        PlCfg = MAN_MAN;
        NewGame();

        break;
    }

    case WM_DESTROY:
        DeleteDC(HdcmemStones);
        DeleteDC(HdcmemBoard);
        DeleteDC(HdcmemGridBoard);
        ReleaseDC(hWnd, Hdc);

        DeleteObject(HbmpStones);
        DeleteObject(HbmpBoard);
        DeleteObject(HbrHilite);
        DeleteObject(HbrCohilite);

        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }

    return 0;
}


void NewGame()
{
    int i, j;

    for (i = 0; i<NUM_BOXES; i++)
        for (j = 0; j<NUM_BOXES; j++) {
            STATE[i][j] = 2;
        }

    GmStat = PLAYING;
    CMov = 0;
}


int FillRectLTWH(HDC hdc, LONG l, LONG t, LONG w, LONG h, HBRUSH hbr)
{
    RECT rect = {l, t, l+w, t+h};
    return FillRect(hdc, &rect, hbr);
}


void ShowHilite(HDC hdc, int i, int j, HBRUSH hbr)
{
    LONG tb = CBox/3;
    FillRectLTWH(hdc, SxBox0+i*CBox+tb, SyBox0+j*CBox+tb, tb, tb, hbr);
}


void ShowPiece(HDC hdc, int i, int j, UINT state)
{
    if (state<2) {
        TransparentBlt(hdc, SxBox0+i*CBox, SyBox0+j*CBox, CBox, CBox,
            HdcmemStones, state*BmStones.bmHeight, 0, BmStones.bmHeight, BmStones.bmHeight, RGB(255, 0, 0));
    }
}


#define MIN(x,y)	((x<y)?(x):(y))


void OnSize()
{
    LONG cmain, s;
    HBITMAP oldHbmpGridBoard;
    RECT rect;
    LONG linethick, start, tythick;
    int i, j;

    cmain = MIN(ClientRect.right, ClientRect.bottom);

    CBoard = (15*cmain)/16;
    SxBoard = (ClientRect.right-CBoard)/2;
    SyBoard = (ClientRect.bottom-CBoard)/2;


    CBox = CBoard/(NUM_BOXES+1);
    s = (CBox+CBoard%(NUM_BOXES+1))/2;

    RSxBox0 = s;
    RSyBox0 = s;
    SxBox0 = SxBoard+RSxBox0;
    SyBox0 = SyBoard+RSyBox0;


    HbmpGridBoard = CreateCompatibleBitmap(Hdc, CBoard, CBoard);
    oldHbmpGridBoard = SelectObject(HdcmemGridBoard, HbmpGridBoard);
    DeleteObject(oldHbmpGridBoard);

    if (CBoard > BmBoard.bmWidth) {
        StretchBlt(HdcmemGridBoard, 0, 0, CBoard, CBoard,
            HdcmemBoard, 0, 0, BmBoard.bmWidth, BmBoard.bmHeight, SRCCOPY);
    } else {
        BitBlt(HdcmemGridBoard, 0, 0, CBoard, CBoard,
            HdcmemBoard, 0, 0, SRCCOPY);
    }

    linethick = (CBox+23)/24;
    start = (CBox-linethick)/2;

    rect.top = RSyBox0+start;
    rect.bottom = rect.top + (NUM_BOXES-1)*CBox;
    rect.left = RSxBox0+start;
    rect.right = rect.left+linethick;

    for (i = NUM_BOXES; i>0; i--) {
        FillRect(HdcmemGridBoard, &rect, HbrGrid);
        rect.left += CBox;
        rect.right += CBox;
    }

    rect.left = RSxBox0+start;
    rect.right = rect.left + (NUM_BOXES-1)*CBox +linethick;
    rect.top = RSyBox0+start;
    rect.bottom = rect.top+linethick;

    for (i = NUM_BOXES; i>0; i--) {
        FillRect(HdcmemGridBoard, &rect, HbrGrid);
        rect.top += CBox;
        rect.bottom += CBox;
    }

    tythick = (CBox+5)/6;
    start = (CBox-tythick)/2;

#if NUM_BOXES>=9
    for (i = 3; i<NUM_BOXES; i += (NUM_BOXES-7)/2)
        for (j = 3; j<NUM_BOXES; j += (NUM_BOXES-7)/2) {
            FillRectLTWH(HdcmemGridBoard, RSxBox0+start+i*CBox, RSyBox0+start+j*CBox,
                tythick, tythick, HbrGrid);
        }
#endif
}


void ShowRenju(HDC hdc, int i, int j)
{
    int k, ii, jj;

    for (k = 0; k<5; k++) {
        ii = RenjuI[k];
        jj = RenjuJ[k];

        if (ii!=i || jj!=j)
            ShowHilite(hdc, ii, jj, HbrCohilite);
    }
}


void OnPaint(HDC hdc)
{
    int i, j;

    BitBlt(hdc, SxBoard, SyBoard, CBoard, CBoard,
        HdcmemGridBoard, 0, 0, SRCCOPY);

    for (i = 0; i<NUM_BOXES; i++)
        for (j = 0; j<NUM_BOXES; j++) {
            ShowPiece(hdc, i, j, STATE[i][j]);
        }

    if (CMov) {
        i = HistI[CMov-1];
        j = HistJ[CMov-1];
        ShowHilite(hdc, i, j, HbrHilite);
        if (GmStat == GAMEWON)
            ShowRenju(hdc, i, j);
    }
}


void OnCommand(WPARAM wParam)
{
    switch (LOWORD(wParam)) {
    case IDM_COM_MAN:
        if (PlCfg != COM_MAN) {
            PlCfg = COM_MAN;
            CheckMenuItem(Hmenu, IDM_COM_MAN, MF_CHECKED);
            CheckMenuItem(Hmenu, IDM_MAN_COM, MF_UNCHECKED);
            CheckMenuItem(Hmenu, IDM_MAN_MAN, MF_UNCHECKED);
            SetWindowText(Hwnd, szComMan);
        }
        break;

    case IDM_MAN_COM:
        if (PlCfg != MAN_COM) {
            PlCfg = MAN_COM;
            CheckMenuItem(Hmenu, IDM_COM_MAN, MF_UNCHECKED);
            CheckMenuItem(Hmenu, IDM_MAN_COM, MF_CHECKED);
            CheckMenuItem(Hmenu, IDM_MAN_MAN, MF_UNCHECKED);
            SetWindowText(Hwnd, szManCom);
        }
        break;

    case IDM_MAN_MAN:
        if (PlCfg != MAN_MAN) {
            PlCfg = MAN_MAN;
            CheckMenuItem(Hmenu, IDM_COM_MAN, MF_UNCHECKED);
            CheckMenuItem(Hmenu, IDM_MAN_COM, MF_UNCHECKED);
            CheckMenuItem(Hmenu, IDM_MAN_MAN, MF_CHECKED);
            SetWindowText(Hwnd, szManMan);
        }
        break;

    case IDM_NEWGAME:
        NewGame();
        OnPaint(Hdc);
        break;

    case IDM_EXIT:
        SendMessage(Hwnd, WM_CLOSE, 0, 0);
    }
}


int CheckWin(int i, int j)
{
    UINT state = STATE[i][j];
    int ia, ib, ja, jb, a, b, ii, jj, renju;

    renju = 0;
    ia = (i<4) ? i : 4;
    ib = (i>NUM_BOXES-5) ? NUM_BOXES-1-i : 4;
    for (ii = i-ia; ii<=i+ib; ii++) {
        if (STATE[ii][j]==state) {
            if (++renju==5) {
                RenjuI[0] = ii-4;
                RenjuI[1] = ii-3;
                RenjuI[2] = ii-2;
                RenjuI[3] = ii-1;
                RenjuI[4] = ii;

                RenjuJ[0] = j;
                RenjuJ[1] = j;
                RenjuJ[2] = j;
                RenjuJ[3] = j;
                RenjuJ[4] = j;

                return 1;
            }
        } else
            renju = 0;
    }

    renju = 0;
    ja = (j<4) ? j : 4;
    jb = (j>NUM_BOXES-5) ? NUM_BOXES-1-j : 4;
    for (jj = j-ja; jj<=j+jb; jj++) {
        if (STATE[i][jj]==state) {
            if (++renju==5) {
                RenjuI[0] = i;
                RenjuI[1] = i;
                RenjuI[2] = i;
                RenjuI[3] = i;
                RenjuI[4] = i;

                RenjuJ[0] = jj-4;
                RenjuJ[1] = jj-3;
                RenjuJ[2] = jj-2;
                RenjuJ[3] = jj-1;
                RenjuJ[4] = jj;

                return 1;
            }
        } else
            renju = 0;
    }


    renju = 0;
    a = MIN(ia, ja);
    b = MIN(ib, jb);

    for (ii = i-a, jj = j-a; ii<=i+b; ii++, jj++) {
        if (STATE[ii][jj]==state) {
            if (++renju==5) {
                RenjuI[0] = ii-4;
                RenjuI[1] = ii-3;
                RenjuI[2] = ii-2;
                RenjuI[3] = ii-1;
                RenjuI[4] = ii;

                RenjuJ[0] = jj-4;
                RenjuJ[1] = jj-3;
                RenjuJ[2] = jj-2;
                RenjuJ[3] = jj-1;
                RenjuJ[4] = jj;

                return 1;
            }
        } else
            renju = 0;
    }


    renju = 0;
    a = MIN(ia, jb);
    b = MIN(ib, ja);

    for (ii = i-a, jj = j+a; ii<=i+b; ii++, jj--) {
        if (STATE[ii][jj]==state) {
            if (++renju==5) {
                RenjuI[0] = ii-4;
                RenjuI[1] = ii-3;
                RenjuI[2] = ii-2;
                RenjuI[3] = ii-1;
                RenjuI[4] = ii;

                RenjuJ[0] = jj+4;
                RenjuJ[1] = jj+3;
                RenjuJ[2] = jj+2;
                RenjuJ[3] = jj+1;
                RenjuJ[4] = jj;

                return 1;
            }
        } else
            renju = 0;
    }

    return 0;
}


void OnLButtonDown(LPARAM lParam)
{
    static char MsgWon[2][16] = {"Black has won!", "White has won!"};
    static char MsgTie[] = "Game tied.";
    int togo, i, j, ii, jj;

    if (GmStat != PLAYING)
        return;

    if (CBox==0)
        return;

    i = (GET_X_LPARAM(lParam)-SxBox0)/CBox;
    if (i<0 || i>=NUM_BOXES)
        return;

    j = (GET_Y_LPARAM(lParam)-SyBox0)/CBox;
    if (j<0 || j>=NUM_BOXES)
        return;

    if (STATE[i][j] <= 1) {
        MessageBeep(MB_ICONEXCLAMATION);
        return;
    }

    if (CMov) {
        ii = HistI[CMov-1];
        jj = HistJ[CMov-1];
        ShowPiece(Hdc, ii, jj, STATE[ii][jj]);
    }

    togo = CMov%2;
    STATE[i][j] = togo;
    HistI[CMov] = i;
    HistJ[CMov] = j;
    CMov++;

    ShowPiece(Hdc, i, j, togo);
    ShowHilite(Hdc, i, j, HbrHilite);

    if (CheckWin(i, j)) {
        ShowRenju(Hdc, i, j);
        MessageBox(Hwnd, MsgWon[togo], szGomoku, 0);
        GmStat = GAMEWON;
    } else if (CMov==NUM_BOXES*NUM_BOXES) {
        MessageBox(Hwnd, MsgTie, szGomoku, 0);
        GmStat = GAMETIE;
    }
}

