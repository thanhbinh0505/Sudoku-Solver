// sudoku.cpp
#include <windows.h>
#include <windowsx.h>   // GET_X_LPARAM / GET_Y_LPARAM
#include <vector>
#include <string>
#include <climits>
#include "resource.h"

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

#define ID_SOLVE  1
#define ID_CLEAR  2

// Layout
const int GRID_SIZE = 9;
const int CELL_SIZE = 56;      // adjust to make numbers larger/smaller
const int MARGIN = 20;         // equal margin on all sides
const int BUTTON_HEIGHT = 36;
const int BUTTON_WIDTH = 100;
const int GAP = 16;

// State
static int board[GRID_SIZE][GRID_SIZE] = { 0 };
static bool isGiven[GRID_SIZE][GRID_SIZE] = { false };      // given before solve (or after snapshot)
static bool isSolvedCell[GRID_SIZE][GRID_SIZE] = { false }; // filled by solver
static int selectedRow = -1, selectedCol = -1;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

// ---------------- DLX (Dancing Links) implementation ----------------
struct DLXNode {
    int row, col;
    DLXNode *L, *R, *U, *D;
    DLXNode(): row(-1), col(-1), L(this), R(this), U(this), D(this) {}
};

struct DLXSolver {
    int nCols;
    std::vector<int> colSize;
    std::vector<DLXNode*> colHead;
    DLXNode head;
    std::vector<DLXNode*> solution;
    std::vector<int> outRows;

    DLXSolver() { init(0); }
    void init(int cols) {
        // clear previous (no memory freeing for simplicity)
        nCols = cols;
        colSize.assign(cols, 0);
        colHead.assign(cols, nullptr);
        head.L = head.R = &head;
        head.U = head.D = &head;
        // create column headers
        for (int i = 0; i < cols; ++i) {
            DLXNode* c = new DLXNode();
            c->col = i;
            c->U = c->D = c;
            // insert to header list
            c->R = &head;
            c->L = head.L;
            if (head.L) head.L->R = c;
            head.L = c;
            if (head.R == &head) head.R = c; // first insertion
            colHead[i] = c;
        }
    }

    void addRow(int rowID, const std::vector<int>& cols) {
        DLXNode* first = nullptr;
        DLXNode* prev = nullptr;
        for (int c : cols) {
            DLXNode* node = new DLXNode();
            node->row = rowID;
            node->col = c;
            // vertical insert into column c (at tail)
            node->D = colHead[c];
            node->U = colHead[c]->U;
            colHead[c]->U->D = node;
            colHead[c]->U = node;
            colSize[c]++;
            // horizontal link
            if (!first) {
                first = node;
                node->L = node->R = node;
            } else {
                node->L = prev;
                node->R = first;
                prev->R = node;
                first->L = node;
            }
            prev = node;
        }
    }

    void cover(DLXNode* c) {
        c->R->L = c->L;
        c->L->R = c->R;
        for (DLXNode* r = c->D; r != c; r = r->D) {
            for (DLXNode* j = r->R; j != r; j = j->R) {
                j->D->U = j->U;
                j->U->D = j->D;
                colSize[j->col]--;
            }
        }
    }

    void uncover(DLXNode* c) {
        for (DLXNode* r = c->U; r != c; r = r->U) {
            for (DLXNode* j = r->L; j != r; j = j->L) {
                colSize[j->col]++;
                j->D->U = j;
                j->U->D = j;
            }
        }
        c->R->L = c;
        c->L->R = c;
    }

    bool search() {
        if (head.R == &head) {
            outRows.clear();
            for (auto* n : solution) outRows.push_back(n->row);
            return true;
        }
        // choose column with minimal size
        DLXNode* c = nullptr;
        int best = INT_MAX;
        for (DLXNode* j = head.R; j != &head; j = j->R) {
            if (colSize[j->col] < best) { best = colSize[j->col]; c = j; }
        }
        if (!c || best == 0) return false;
        cover(c);
        for (DLXNode* r = c->D; r != c; r = r->D) {
            solution.push_back(r);
            for (DLXNode* j = r->R; j != r; j = j->R) cover(colHead[j->col]);
            if (search()) return true;
            for (DLXNode* j = r->L; j != r; j = j->L) uncover(colHead[j->col]);
            solution.pop_back();
        }
        uncover(c);
        return false;
    }
};

// mapping helpers
static inline int BoxIndex(int r, int c) { return (r/3)*3 + (c/3); }

// Solve with DLX; returns true and fills result if solvable
bool SolveSudokuDLX(const int input[9][9], int result[9][9]) {
    DLXSolver dlx;
    dlx.init(324); // 81*4 constraints
    struct MapRow { int r,c,v; };
    std::vector<MapRow> mapRows;
    int rowID = 0;
    for (int r=0;r<9;r++){
        for (int c=0;c<9;c++){
            for (int v=1; v<=9; v++){
                if (input[r][c] != 0 && input[r][c] != v) continue;
                int cell = r*9 + c; // 0..80
                int rowC = 81 + r*9 + (v-1); // 81..161
                int colC = 162 + c*9 + (v-1); // 162..242
                int boxC = 243 + BoxIndex(r,c)*9 + (v-1); // 243..323
                dlx.addRow(rowID, {cell, rowC, colC, boxC});
                mapRows.push_back({r,c,v});
                rowID++;
            }
        }
    }
    if (!dlx.search()) return false;
    // build result
    int tmp[9][9] = {0};
    for (int id : dlx.outRows) {
        MapRow m = mapRows[id];
        tmp[m.r][m.c] = m.v;
    }
    for (int r=0;r<9;r++) for (int c=0;c<9;c++) result[r][c] = tmp[r][c];
    return true;
}
// ---------------- end DLX ----------------


// Draw board: paint highlight FIRST (fill cell), then draw lines (so borders remain visible), then numbers.
void DrawBoard(HWND /*hwnd*/, HDC hdc) {
    int startX = MARGIN;
    int startY = MARGIN;
    int gridPx = GRID_SIZE * CELL_SIZE;

    // Fill entire board background white
    RECT boardRect = { startX, startY, startX + gridPx, startY + gridPx };
    HBRUSH hWhite = CreateSolidBrush(RGB(255,255,255));
    FillRect(hdc, &boardRect, hWhite);
    DeleteObject(hWhite);

    // If selected, fill that cell fully (we will redraw border later)
    if (selectedRow >= 0 && selectedCol >= 0) {
        HBRUSH hSel = CreateSolidBrush(RGB(220,220,230)); // light gray
        RECT sel = {
            startX + selectedCol * CELL_SIZE,
            startY + selectedRow * CELL_SIZE,
            startX + (selectedCol+1) * CELL_SIZE,
            startY + (selectedRow+1) * CELL_SIZE
        };
        FillRect(hdc, &sel, hSel);
        DeleteObject(hSel);
    }

    // Draw grid lines (thin and thick)
    for (int i=0;i<=GRID_SIZE;i++) {
        // vertical
        HPEN penV = CreatePen(PS_SOLID, (i%3==0)?3:1, RGB(0,0,0));
        HPEN oldV = (HPEN)SelectObject(hdc, penV);
        int x = startX + i*CELL_SIZE;
        MoveToEx(hdc, x, startY, NULL);
        LineTo(hdc, x, startY + gridPx);
        SelectObject(hdc, oldV);
        DeleteObject(penV);
        // horizontal
        HPEN penH = CreatePen(PS_SOLID, (i%3==0)?3:1, RGB(0,0,0));
        HPEN oldH = (HPEN)SelectObject(hdc, penH);
        int y = startY + i*CELL_SIZE;
        MoveToEx(hdc, startX, y, NULL);
        LineTo(hdc, startX + gridPx, y);
        SelectObject(hdc, oldH);
        DeleteObject(penH);
    }

    // Prepare font scaled to cell
    int fontHeight = CELL_SIZE * 3 / 5; // ~60% of cell height
    HFONT hFont = CreateFontA(-fontHeight, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                              DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                              DEFAULT_PITCH | FF_SWISS, "Segoe UI");
    HFONT oldFont = (HFONT)SelectObject(hdc, hFont);
    SetBkMode(hdc, TRANSPARENT);

    // Draw numbers
    for (int r=0;r<GRID_SIZE;r++){
        for (int c=0;c<GRID_SIZE;c++){
            int v = board[r][c];
            if (v == 0) continue;
            RECT cell = {
                startX + c*CELL_SIZE,
                startY + r*CELL_SIZE,
                startX + (c+1)*CELL_SIZE,
                startY + (r+1)*CELL_SIZE
            };
            if (isGiven[r][c]) SetTextColor(hdc, RGB(0,0,0));
            else if (isSolvedCell[r][c]) SetTextColor(hdc, RGB(120,120,120));
            else SetTextColor(hdc, RGB(20,90,200));
            std::string s = std::to_string(v);
            DrawTextA(hdc, s.c_str(), -1, &cell, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        }
    }

    SelectObject(hdc, oldFont);
    DeleteObject(hFont);
}

// Convert click point to cell; returns true if inside board
bool PointToCell(HWND /*hwnd*/, int x, int y, int &outR, int &outC) {
    int startX = MARGIN;
    int startY = MARGIN;
    int gridPx = GRID_SIZE * CELL_SIZE;
    if (x < startX || y < startY) return false;
    if (x >= startX + gridPx || y >= startY + gridPx) return false;
    outC = (x - startX) / CELL_SIZE;
    outR = (y - startY) / CELL_SIZE;
    return true;
}

// Snapshot current board as givens and run solver (DLX). Mark solver-filled cells with isSolvedCell.
void DoSolve(HWND hwnd) {
    int input[9][9];
    for (int r=0;r<9;r++) for (int c=0;c<9;c++) input[r][c] = board[r][c];
    // mark givens (any non-zero at moment of Solve)
    for (int r=0;r<9;r++) for (int c=0;c<9;c++) {
        if (input[r][c] != 0) isGiven[r][c] = true;
        isSolvedCell[r][c] = false; // reset solved marks
    }
    int result[9][9] = {0};
    if (!SolveSudokuDLX(input, result)) {
        MessageBoxA(hwnd, "No solution found.", "Sudoku", MB_OK | MB_ICONWARNING);
        return;
    }
    // fill only previously-empty cells as solver-filled
    for (int r=0;r<9;r++) for (int c=0;c<9;c++){
        if (input[r][c] == 0) {
            board[r][c] = result[r][c];
            isSolvedCell[r][c] = true;
        } else {
            board[r][c] = result[r][c]; // synchronize (in case user input inconsistent)
        }
    }
    selectedRow = selectedCol = -1;
    InvalidateRect(hwnd, NULL, TRUE);
}

// Clear everything
void DoClear(HWND hwnd) {
    for (int r=0;r<9;r++) for (int c=0;c<9;c++){
        board[r][c] = 0;
        isGiven[r][c] = false;
        isSolvedCell[r][c] = false;
    }
    selectedRow = selectedCol = -1;
    InvalidateRect(hwnd, NULL, TRUE);
}

// Main
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow){
    const char CLASS_NAME[] = "SudokuWinAPI";
    WNDCLASSEXA wc = {0};
    wc.cbSize        = sizeof(WNDCLASSEXA);
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APP_ICON));
    wc.hIconSm       = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APP_ICON));
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);

    RegisterClassExA(&wc);

    int winW = 2*MARGIN + GRID_SIZE*CELL_SIZE + 17;
    int winH = 2*MARGIN + GRID_SIZE*CELL_SIZE + BUTTON_HEIGHT + GAP + MARGIN + 1;

    HWND hwnd = CreateWindowExA(
        0,
        CLASS_NAME,
        "Sudoku Solver    *    Dancing Links (Algorithm X - Donald Knuth)",
        WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, winW, winH,
        NULL, NULL, hInstance, NULL
    );
    if (!hwnd) return 0;

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // Create centered buttons (use client coords)
    RECT cli; GetClientRect(hwnd, &cli);
    int centerX = (cli.right - cli.left) / 2;
    CreateWindowA("BUTTON", "Solve", WS_CHILD|WS_VISIBLE|BS_DEFPUSHBUTTON,
                  centerX - BUTTON_WIDTH - 10, MARGIN + GRID_SIZE*CELL_SIZE + 10,
                  BUTTON_WIDTH, BUTTON_HEIGHT, hwnd, (HMENU)ID_SOLVE, hInstance, NULL);
    CreateWindowA("BUTTON", "Clear", WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,
                  centerX + 10, MARGIN + GRID_SIZE*CELL_SIZE + 10,
                  BUTTON_WIDTH, BUTTON_HEIGHT, hwnd, (HMENU)ID_CLEAR, hInstance, NULL);

    // Message loop
    MSG msg;
    while (GetMessageA(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    return 0;
}

// Window proc: mouse selection, keyboard, buttons, paint
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_LBUTTONDOWN: {
        int x = GET_X_LPARAM(lParam);
        int y = GET_Y_LPARAM(lParam);
        int r, c;
        if (PointToCell(hwnd, x, y, r, c)) {
            selectedRow = r; selectedCol = c;
            SetFocus(hwnd);
            InvalidateRect(hwnd, NULL, TRUE);
        }
        break;
    }
    case WM_KEYDOWN: {
        bool needInvalidate = false;
        if (selectedRow >= 0 && selectedCol >= 0) {
            // navigation
            switch (wParam) {
            case VK_LEFT:  if (selectedCol>0) { selectedCol--; needInvalidate=true; } break;
            case VK_RIGHT: if (selectedCol<GRID_SIZE-1) { selectedCol++; needInvalidate=true; } break;
            case VK_UP:    if (selectedRow>0) { selectedRow--; needInvalidate=true; } break;
            case VK_DOWN:  if (selectedRow<GRID_SIZE-1) { selectedRow++; needInvalidate=true; } break;
            case VK_HOME:  selectedCol = 0; needInvalidate=true; break;
            case VK_END:   selectedCol = GRID_SIZE-1; needInvalidate=true; break;
            case VK_PRIOR: selectedRow = 0; needInvalidate=true; break;   // PageUp
            case VK_NEXT:  selectedRow = GRID_SIZE-1; needInvalidate=true; break; // PageDown
            case VK_SPACE:
            case VK_DELETE:
            case VK_BACK:
                board[selectedRow][selectedCol] = 0;
                isGiven[selectedRow][selectedCol] = false;
                isSolvedCell[selectedRow][selectedCol] = false;
                needInvalidate = true;
                break;
            // NumPad digits
            case VK_NUMPAD1: case VK_NUMPAD2: case VK_NUMPAD3:
            case VK_NUMPAD4: case VK_NUMPAD5: case VK_NUMPAD6:
            case VK_NUMPAD7: case VK_NUMPAD8: case VK_NUMPAD9: {
                int val = (int)wParam - VK_NUMPAD0;
                board[selectedRow][selectedCol] = val;
                isGiven[selectedRow][selectedCol] = false;
                isSolvedCell[selectedRow][selectedCol] = false;
                needInvalidate = true;
                break;
            }
            default: break;
            }
        }
        if (needInvalidate) InvalidateRect(hwnd, NULL, TRUE);
        break;
    }
    case WM_CHAR: {
        if (selectedRow >= 0 && selectedCol >= 0) {
            if (wParam >= '1' && wParam <= '9') {
                board[selectedRow][selectedCol] = (int)(wParam - '0');
                isGiven[selectedRow][selectedCol] = false;
                isSolvedCell[selectedRow][selectedCol] = false;
                InvalidateRect(hwnd, NULL, TRUE);
            } else if (wParam == '0') {
                board[selectedRow][selectedCol] = 0;
                isGiven[selectedRow][selectedCol] = false;
                isSolvedCell[selectedRow][selectedCol] = false;
                InvalidateRect(hwnd, NULL, TRUE);
            }
        }
        break;
    }
    case WM_COMMAND: {
        int id = LOWORD(wParam);
        if (id == ID_CLEAR) DoClear(hwnd);
        else if (id == ID_SOLVE) DoSolve(hwnd);
        break;
    }
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        DrawBoard(hwnd, hdc);
        EndPaint(hwnd, &ps);
        break;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProcA(hwnd, msg, wParam, lParam);
    }
    return 0;
}
