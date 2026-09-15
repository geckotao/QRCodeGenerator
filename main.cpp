#define UNICODE
#define _UNICODE
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <string>
#include <vector>
#include <algorithm>
#ifndef IDI_ICON1
#define IDI_ICON1 101 // 常见的默认图标 ID。如果图标仍不显示，请检查 app.rc 中的实际 ID 并修改此处
#endif
// 引入 qrencode 和 stb_image_write
#include "qrencode.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

// 控件 ID
#define IDC_TEXT       101
#define IDC_CHARCOUNT  102
#define IDC_BTN_GEN    103
#define IDC_BTN_CLEAR  104
#define IDC_BTN_SAVE   105
#define IDC_QR_DISPLAY 106
#define IDC_STATUS     107

// 应用程序状态结构
struct AppState {
    HWND hTextEditor = nullptr;
    HWND hCharCount = nullptr;
    HWND hBtnGen = nullptr;
    HWND hBtnClear = nullptr;
    HWND hBtnSave = nullptr;
    HWND hQrDisplay = nullptr;
    HWND hStatusBar = nullptr;
    HBITMAP hQrBitmap = nullptr;
    std::vector<unsigned char> imageData;
    int currentImgSize = 0;
};

// 保存旧的窗口过程指针（用于子类化）
WNDPROC g_lpfnOldQrProc = nullptr;

// ================= 辅助函数 =================

// UTF-16 转 UTF-8
std::string WStringToUTF8(const std::wstring& wstr) {
    if (wstr.empty()) return "";
    int size = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string str(size, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &str[0], size, nullptr, nullptr);
    if (!str.empty()) str.pop_back(); // 移除末尾的 '\0'
    return str;
}

// 更新状态栏
void UpdateStatus(HWND hWnd, const std::wstring& text) {
    AppState* state = reinterpret_cast<AppState*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    if (state && state->hStatusBar) {
        SendMessageW(state->hStatusBar, SB_SETTEXTW, 0, (LPARAM)text.c_str());
    }
}

// 更新字节数统计
void UpdateCharCount(HWND hWnd, AppState* state) {
    int len = GetWindowTextLengthW(state->hTextEditor);
    std::wstring wText(len + 1, L'\0');
    if (len > 0) GetWindowTextW(state->hTextEditor, &wText[0], len + 1);
    wText.pop_back();
    
    std::string utf8Text = WStringToUTF8(wText);
    long byteCount = utf8Text.length();
    
    wchar_t buf[128];
    wsprintfW(buf, L"字节数: %ld / 2953\n(最大约支持 984 个汉字)", byteCount);
    SetWindowTextW(state->hCharCount, buf);
}

// ================= 核心业务逻辑 =================

void OnGenerate(HWND hWnd, AppState* state) {
    int len = GetWindowTextLengthW(state->hTextEditor);
    if (len == 0) {
        MessageBoxW(hWnd, L"请输入文本！", L"提示", MB_OK | MB_ICONWARNING);
        return;
    }

    std::wstring wText(len + 1, L'\0');
    GetWindowTextW(state->hTextEditor, &wText[0], len + 1);
    wText.pop_back();

    std::string utf8Text = WStringToUTF8(wText);
    long byteCount = utf8Text.length();

    if (byteCount > 2953) {
        MessageBoxW(hWnd, L"文本超出二维码容量限制！(最大 2953 字节)", L"错误", MB_OK | MB_ICONERROR);
        return;
    }

    EnableWindow(state->hBtnGen, FALSE);
    SetWindowTextW(state->hBtnGen, L"生成中...");
    UpdateStatus(hWnd, L"正在生成...");

    QRcode* qrcode = QRcode_encodeString(utf8Text.c_str(), 0, QR_ECLEVEL_L, QR_MODE_8, 1);
    if (!qrcode) {
        MessageBoxW(hWnd, L"无法生成二维码", L"错误", MB_OK | MB_ICONERROR);
        EnableWindow(state->hBtnGen, TRUE);
        SetWindowTextW(state->hBtnGen, L"生成");
        return;
    }

    int width = qrcode->width;
    int scale = 8;
    int border = 2;
    state->currentImgSize = (width + border * 2) * scale;
    // 32-bit BGRA, 初始为白色 (0xFFFFFFFF)
    state->imageData.assign(state->currentImgSize * state->currentImgSize * 4, 0xFF); 

    for (int y = 0; y < width; ++y) {
        for (int x = 0; x < width; ++x) {
            if (qrcode->data[y * width + x] & 1) {
                int startX = (x + border) * scale;
                int startY = (y + border) * scale;
                for (int sy = 0; sy < scale; ++sy) {
                    for (int sx = 0; sx < scale; ++sx) {
                        int idx = ((startY + sy) * state->currentImgSize + (startX + sx)) * 4;
                        state->imageData[idx]     = 0x00; // B
                        state->imageData[idx + 1] = 0x00; // G
                        state->imageData[idx + 2] = 0x00; // R
                        state->imageData[idx + 3] = 0xFF; // A
                    }
                }
            }
        }
    }
    QRcode_free(qrcode);

    if (state->hQrBitmap) DeleteObject(state->hQrBitmap);
    BITMAPINFO bmi = {0};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = state->currentImgSize;
    bmi.bmiHeader.biHeight = -state->currentImgSize; 
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* pBits = nullptr;
    state->hQrBitmap = CreateDIBSection(NULL, &bmi, DIB_RGB_COLORS, &pBits, NULL, 0);
    if (state->hQrBitmap && pBits) {
        memcpy(pBits, state->imageData.data(), state->imageData.size());
    }

    EnableWindow(state->hBtnGen, TRUE);
    SetWindowTextW(state->hBtnGen, L"生成");
    EnableWindow(state->hBtnSave, TRUE);
    
    // 触发右侧区域重绘
    InvalidateRect(state->hQrDisplay, NULL, TRUE);
    
    std::wstring status = L"生成成功 | 尺寸: " + std::to_wstring(state->currentImgSize) + L"x" + std::to_wstring(state->currentImgSize) + L" | 字节: " + std::to_wstring(byteCount);
    UpdateStatus(hWnd, status);
}

void OnSave(HWND hWnd, AppState* state) {
    if (state->imageData.empty()) return;

    OPENFILENAMEW ofn = {0};
    wchar_t szFile[MAX_PATH] = L"qrcode.png";
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hWnd;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = L"PNG 文件\0*.png\0JPEG 文件\0*.jpg\0所有文件\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;

    if (GetSaveFileNameW(&ofn)) {
        std::string utf8Path = WStringToUTF8(std::wstring(szFile));
        int result = 0;
        
        if (utf8Path.length() >= 4 && utf8Path.substr(utf8Path.length() - 4) == ".jpg") {
            result = stbi_write_jpg(utf8Path.c_str(), state->currentImgSize, state->currentImgSize, 4, state->imageData.data(), 90);
        } else {
            result = stbi_write_png(utf8Path.c_str(), state->currentImgSize, state->currentImgSize, 4, state->imageData.data(), 0);
        }

        if (result) {
            UpdateStatus(hWnd, L"保存成功: " + std::wstring(szFile));
        } else {
            MessageBoxW(hWnd, L"保存失败！", L"错误", MB_OK | MB_ICONERROR);
        }
    }
}

void OnClear(HWND hWnd, AppState* state) {
    SetWindowTextW(state->hTextEditor, L"");
    if (state->hQrBitmap) {
        DeleteObject(state->hQrBitmap);
        state->hQrBitmap = nullptr;
        state->imageData.clear();
        EnableWindow(state->hBtnSave, FALSE);
        // 触发重绘，清除画面
        InvalidateRect(state->hQrDisplay, NULL, TRUE);
    }
    UpdateStatus(hWnd, L"已清除");
    UpdateCharCount(hWnd, state);
}

// ================= 窗口子类化过程 (处理二维码自适应绘制) =================

LRESULT CALLBACK QrDisplaySubclassProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    AppState* state = reinterpret_cast<AppState*>(GetWindowLongPtr(GetParent(hWnd), GWLP_USERDATA));
    
    if (msg == WM_PAINT && state && state->hQrBitmap) {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        RECT rc;
        GetClientRect(hWnd, &rc);
        
        HDC hdcMem = CreateCompatibleDC(hdc);
        HBITMAP hOldBmp = (HBITMAP)SelectObject(hdcMem, state->hQrBitmap);
        
        BITMAP bm;
        GetObject(state->hQrBitmap, sizeof(BITMAP), &bm);
        int srcW = bm.bmWidth;
        int srcH = bm.bmHeight;
        int dstW = rc.right - rc.left;
        int dstH = rc.bottom - rc.top;
        
        // 计算等比例缩放
        float scale = (float)dstW / srcW;
        if ((float)dstH / srcH < scale) scale = (float)dstH / srcH;
        
        int drawW = (int)(srcW * scale);
        int drawH = (int)(srcH * scale);
        int drawX = (dstW - drawW) / 2; // 居中
        int drawY = (dstH - drawH) / 2; // 居中
        
        // 黑白图像使用 BLACKONWHITE 保持边缘锐利
        SetStretchBltMode(hdc, BLACKONWHITE);
        StretchBlt(hdc, drawX, drawY, drawW, drawH, hdcMem, 0, 0, srcW, srcH, SRCCOPY);
        
        SelectObject(hdcMem, hOldBmp);
        DeleteDC(hdcMem);
        EndPaint(hWnd, &ps);
        return 0;
    } 
    else if (msg == WM_ERASEBKGND) {
        // 无论是没图片还是清空图片，都绘制纯白背景
        HDC hdc = (HDC)wParam;
        RECT rc;
        GetClientRect(hWnd, &rc);
        HBRUSH hBrush = CreateSolidBrush(RGB(255, 255, 255));
        FillRect(hdc, &rc, hBrush);
        DeleteObject(hBrush);
        return 1; // 表示已处理背景擦除
    }
    
    return CallWindowProc(g_lpfnOldQrProc, hWnd, msg, wParam, lParam);
}

// ================= 主窗口过程 =================

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    AppState* state = reinterpret_cast<AppState*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

    switch (msg) {
    case WM_CREATE: {
        state = new AppState();
        SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)state);

        state->hTextEditor = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", 
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL,
            0, 0, 0, 0, hWnd, (HMENU)IDC_TEXT, NULL, NULL);
        SendMessageW(state->hTextEditor, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);

        state->hCharCount = CreateWindowExW(0, L"STATIC", L"字节数: 0 / 2953\n(最大约支持 984 个汉字)", 
            WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hWnd, (HMENU)IDC_CHARCOUNT, NULL, NULL);
        SendMessageW(state->hCharCount, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);

        state->hBtnGen = CreateWindowExW(0, L"BUTTON", L"生成", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0, 0, 0, 0, hWnd, (HMENU)IDC_BTN_GEN, NULL, NULL);
        state->hBtnClear = CreateWindowExW(0, L"BUTTON", L"清除", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0, 0, 0, 0, hWnd, (HMENU)IDC_BTN_CLEAR, NULL, NULL);
        state->hBtnSave = CreateWindowExW(0, L"BUTTON", L"保存", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0, 0, 0, 0, hWnd, (HMENU)IDC_BTN_SAVE, NULL, NULL);
        EnableWindow(state->hBtnSave, FALSE);

        // 创建二维码显示区，并进行子类化
        state->hQrDisplay = CreateWindowExW(WS_EX_CLIENTEDGE, L"STATIC", L"", 
            WS_CHILD | WS_VISIBLE | SS_NOPREFIX, 0, 0, 0, 0, hWnd, (HMENU)IDC_QR_DISPLAY, NULL, NULL);
        g_lpfnOldQrProc = (WNDPROC)SetWindowLongPtr(state->hQrDisplay, GWLP_WNDPROC, (LONG_PTR)QrDisplaySubclassProc);

        state->hStatusBar = CreateWindowExW(0, STATUSCLASSNAME, L"项目地址：https://github.com/geckotao/QRCodeGenerator",
            WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP, 0, 0, 0, 0, hWnd, (HMENU)IDC_STATUS, NULL, NULL);

        RECT rc;
        GetClientRect(hWnd, &rc);
        SendMessageW(hWnd, WM_SIZE, SIZE_RESTORED, MAKELPARAM(rc.right, rc.bottom));
        break;
    }

    case WM_SIZE: {
        if (!state) break;
        int winW = LOWORD(lParam);
        int winH = HIWORD(lParam);
        int margin = 8;
        int statusH = 24;
        int contentH = winH - statusH - margin * 2;
        int leftW = 230;
        int rightW = winW - leftW - margin * 3;

        MoveWindow(state->hStatusBar, 0, winH - statusH, winW, statusH, TRUE);

        int textH = contentH - 110;
        int charY = margin + textH + 16;
        int btnY = charY + 56;

        MoveWindow(state->hTextEditor, margin, margin, leftW - 16, textH, TRUE);
        MoveWindow(state->hCharCount, margin, charY, leftW - 16, 48, TRUE);
        MoveWindow(state->hBtnGen, margin, btnY, 70, 28, TRUE);
        MoveWindow(state->hBtnClear, margin + 84, btnY, 56, 28, TRUE);
        MoveWindow(state->hBtnSave, margin + 146, btnY, leftW - 154, 28, TRUE);

        MoveWindow(state->hQrDisplay, margin + leftW + margin, margin, rightW, contentH, TRUE);
        
        // 窗口大小改变时，强制刷新二维码区域以重新计算缩放
        InvalidateRect(state->hQrDisplay, NULL, TRUE);
        break;
    }

    case WM_COMMAND: {
        int wmId = LOWORD(wParam);
        int wmEvent = HIWORD(wParam);

        if (wmId == IDC_TEXT && wmEvent == EN_CHANGE) {
            UpdateCharCount(hWnd, state);
        }
        else if (wmId == IDC_BTN_GEN) OnGenerate(hWnd, state);
        else if (wmId == IDC_BTN_CLEAR) OnClear(hWnd, state);
        else if (wmId == IDC_BTN_SAVE) OnSave(hWnd, state);
        break;
    }

    case WM_DESTROY: {
        if (state) {
            if (state->hQrBitmap) DeleteObject(state->hQrBitmap);
            // 恢复旧的窗口过程
            if (g_lpfnOldQrProc && state->hQrDisplay) {
                SetWindowLongPtr(state->hQrDisplay, GWLP_WNDPROC, (LONG_PTR)g_lpfnOldQrProc);
            }
            delete state;
        }
        PostQuitMessage(0);
        break;
    }

    default:
        return DefWindowProcW(hWnd, msg, wParam, lParam);
    }
    return 0;
}

// ================= 入口函数 =================

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow) {
    INITCOMMONCONTROLSEX icex = { sizeof(INITCOMMONCONTROLSEX), ICC_BAR_CLASSES };
    InitCommonControlsEx(&icex);

    const wchar_t CLASS_NAME[] = L"QRCodeGeneratorClass";
    WNDCLASSEXW wc = {0};           
    wc.cbSize = sizeof(WNDCLASSEXW); 
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    
    wc.hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_ICON1));
    wc.hIconSm = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_ICON1));

    if (!wc.hIcon) {
        wc.hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(1));
        wc.hIconSm = LoadIconW(hInstance, MAKEINTRESOURCEW(1));
    }
    
    RegisterClassExW(&wc);     

    HWND hWnd = CreateWindowExW(0, CLASS_NAME, L"二维码生成器 by geckotao",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 600, 520,
        NULL, NULL, hInstance, NULL);

    if (hWnd == NULL) return 0;

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}