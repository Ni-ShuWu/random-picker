#include "gui.h"
#include "reader.h"
#include "picker.h"
#include "ai.h"
#include "logger.h"
#include "cli.h"

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define _WINSOCKAPI_
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shlobj.h>
#include <shellapi.h>
#include <string>
#include <sstream>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <fstream>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

namespace picker {

namespace {

constexpr const wchar_t kClassName[] = L"RandomPickerMainWindow";
constexpr int kIdFileEdit       = 1001;
constexpr int kIdBrowseBtn      = 1002;
constexpr int kIdCountEdit      = 1003;
constexpr int kIdPickBtn        = 1004;
constexpr int kIdSaveBtn        = 1005;
constexpr int kIdClearBtn       = 1006;
constexpr int kIdResultList     = 1007;
constexpr int kIdAiCheck        = 1008;
constexpr int kIdApiKeyEdit     = 1009;
constexpr int kIdAiPromptEdit   = 1010;
constexpr int kIdStatusLabel    = 1011;
constexpr int kIdFileLabel      = 1012;
constexpr int kIdCountLabel     = 1013;
constexpr int kIdAiKeyLabel     = 1014;
constexpr int kIdAiPromptLabel  = 1015;
constexpr int kIdProgress       = 1016;
constexpr int kIdSeedLabel      = 1017;
constexpr int kIdCopyBtn        = 1018;

struct GuiState {
    HINSTANCE hInstance = nullptr;
    HWND hwnd = nullptr;
    HWND hFileEdit = nullptr;
    HWND hBrowseBtn = nullptr;
    HWND hCountEdit = nullptr;
    HWND hPickBtn = nullptr;
    HWND hSaveBtn = nullptr;
    HWND hClearBtn = nullptr;
    HWND hResultList = nullptr;
    HWND hAiCheck = nullptr;
    HWND hApiKeyEdit = nullptr;
    HWND hAiPromptEdit = nullptr;
    HWND hStatus = nullptr;
    HWND hProgress = nullptr;
    HFONT hFont = nullptr;
    HFONT hFontBold = nullptr;
    HWND hFileLabel = nullptr;
    HWND hCountLabel = nullptr;
    HWND hAiKeyLabel = nullptr;
    HWND hAiPromptLabel = nullptr;
    HWND hSeedLabel = nullptr;
    HWND hCopyBtn = nullptr;

    std::vector<std::string> resultCache;
    RunReport lastReport;
    TableData lastTable;
    bool haveResult = false;
    std::mutex mtx;
    std::atomic<bool> busy{false};
};

GuiState g_state;

std::wstring toWide(const std::string& s) {
    if (s.empty()) return {};
    int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    std::wstring out(static_cast<std::size_t>(len > 0 ? len - 1 : 0), L'\0');
    if (len > 1) MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &out[0], len);
    return out;
}

std::string fromWide(const std::wstring& w) {
    if (w.empty()) return {};
    int len = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string out(static_cast<std::size_t>(len > 0 ? len - 1 : 0), '\0');
    if (len > 1) WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, &out[0], len, nullptr, nullptr);
    return out;
}

void setStatus(const std::string& s) {
    if (!g_state.hStatus) return;
    SetWindowTextW(g_state.hStatus, toWide(s).c_str());
}

void setBusy(bool b) {
    g_state.busy = b;
    EnableWindow(g_state.hPickBtn, !b);
    EnableWindow(g_state.hBrowseBtn, !b);
    ShowWindow(g_state.hProgress, b ? SW_SHOW : SW_HIDE);
    if (b) {
        SendMessage(g_state.hProgress, PBM_SETMARQUEE, TRUE, 30);
    } else {
        SendMessage(g_state.hProgress, PBM_SETMARQUEE, FALSE, 0);
    }
}

void appendResultRow(int idx, const std::string& name, const std::string& extra) {
    if (!g_state.hResultList) return;
    LVITEMW item{};
    item.mask = LVIF_TEXT;
    item.iItem = idx;
    item.iSubItem = 0;
    std::wstring wIdx = std::to_wstring(idx + 1);
    item.pszText = const_cast<wchar_t*>(wIdx.c_str());
    ListView_InsertItem(g_state.hResultList, &item);

    std::wstring wName = toWide(name);
    ListView_SetItemText(g_state.hResultList, idx, 1, const_cast<wchar_t*>(wName.c_str()));

    std::wstring wExtra = toWide(extra);
    ListView_SetItemText(g_state.hResultList, idx, 2, const_cast<wchar_t*>(wExtra.c_str()));
}

void clearResults() {
    if (g_state.hResultList) ListView_DeleteAllItems(g_state.hResultList);
}

void populateResults(const std::vector<PickResult>& picks,
                     const TableData& table,
                     bool aiEnabled) {
    clearResults();
    g_state.resultCache.clear();
    for (std::size_t i = 0; i < picks.size(); ++i) {
        const auto& p = picks[i].person;
        std::string name = table.nameOf(p);
        std::ostringstream extra;
        for (std::size_t c = 0; c < table.headers.size(); ++c) {
            if (c == table.nameColumnIndex) continue;
            auto it = p.fields.find(table.headers[c]);
            if (it == p.fields.end() || it->second.empty()) continue;
            extra << table.headers[c] << ": " << it->second << "; ";
        }
        if (aiEnabled && !picks[i].aiReason.empty()) {
            extra << " [AI] " << picks[i].aiReason;
        }
        std::string extraStr = extra.str();
        if (!extraStr.empty() && extraStr.back() == ' ') extraStr.pop_back();
        if (!extraStr.empty() && extraStr.back() == ';') extraStr.pop_back();
        g_state.resultCache.push_back(name + " | " + extraStr);
        appendResultRow(static_cast<int>(i), name, extraStr);
    }
}

std::string getWindowTextStd(HWND h) {
    if (!h) return {};
    int len = GetWindowTextLengthW(h);
    if (len <= 0) return {};
    std::wstring buf(static_cast<std::size_t>(len + 1), L'\0');
    int got = GetWindowTextW(h, &buf[0], len + 1);
    buf.resize(static_cast<std::size_t>(got > 0 ? got : 0));
    return fromWide(buf);
}

bool browseForFile(HWND owner, std::string& outPath) {
    wchar_t buf[MAX_PATH]{};
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = owner;
    ofn.lpstrFile = buf;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = L"支持的表格 (*.csv;*.xlsx;*.tsv;*.txt)\0*.csv;*.xlsx;*.tsv;*.txt\0CSV (*.csv)\0*.csv\0Excel (*.xlsx)\0*.xlsx\0所有文件 (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_EXPLORER;
    if (GetOpenFileNameW(&ofn)) {
        outPath = fromWide(buf);
        return true;
    }
    return false;
}

bool saveToFile(HWND owner, const std::string& suggested, std::string& outPath) {
    wchar_t buf[MAX_PATH]{};
    std::wstring ws = toWide(suggested);
    wcsncpy_s(buf, ws.c_str(), MAX_PATH - 1);
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = owner;
    ofn.lpstrFile = buf;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = L"文本 (*.txt)\0*.txt\0CSV (*.csv)\0*.csv\0所有文件 (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_EXPLORER;
    if (GetSaveFileNameW(&ofn)) {
        outPath = fromWide(buf);
        return true;
    }
    return false;
}

LRESULT CALLBACK wndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
        case WM_CREATE: {
            g_state.hwnd = hwnd;
            g_state.hFont = reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
            LOGFONTW lf{};
            GetObjectW(g_state.hFont, sizeof(lf), &lf);
            lf.lfWeight = FW_BOLD;
            g_state.hFontBold = CreateFontIndirectW(&lf);

            int x = 16, y = 16, labelW = 80, ctrlW = 460;

            CreateWindowExW(0, L"STATIC", L"表格文件:",
                WS_CHILD | WS_VISIBLE,
                x, y + 3, labelW, 22, hwnd, reinterpret_cast<HMENU>(kIdFileLabel), g_state.hInstance, nullptr);
            g_state.hFileEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                x + labelW, y, ctrlW - 100, 24, hwnd, reinterpret_cast<HMENU>(kIdFileEdit), g_state.hInstance, nullptr);
            g_state.hBrowseBtn = CreateWindowExW(0, L"BUTTON", L"浏览…",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                x + labelW + ctrlW - 95, y - 1, 90, 26, hwnd, reinterpret_cast<HMENU>(kIdBrowseBtn), g_state.hInstance, nullptr);
            y += 36;

            CreateWindowExW(0, L"STATIC", L"抽取人数:",
                WS_CHILD | WS_VISIBLE,
                x, y + 3, labelW, 22, hwnd, reinterpret_cast<HMENU>(kIdCountLabel), g_state.hInstance, nullptr);
            g_state.hCountEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"1",
                WS_CHILD | WS_VISIBLE | ES_NUMBER | ES_AUTOHSCROLL,
                x + labelW, y, 120, 24, hwnd, reinterpret_cast<HMENU>(kIdCountEdit), g_state.hInstance, nullptr);
            CreateWindowExW(0, L"STATIC", L"  随机种子 (自动):",
                WS_CHILD | WS_VISIBLE,
                x + labelW + 130, y + 3, 200, 22, hwnd, reinterpret_cast<HMENU>(kIdSeedLabel), g_state.hInstance, nullptr);
            y += 36;

            g_state.hAiCheck = CreateWindowExW(0, L"BUTTON", L"启用 AI 推荐 (需要 API Key)",
                WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                x, y, 360, 22, hwnd, reinterpret_cast<HMENU>(kIdAiCheck), g_state.hInstance, nullptr);
            y += 28;

            CreateWindowExW(0, L"STATIC", L"API Key:",
                WS_CHILD | WS_VISIBLE,
                x, y + 3, labelW, 22, hwnd, reinterpret_cast<HMENU>(kIdAiKeyLabel), g_state.hInstance, nullptr);
            g_state.hApiKeyEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                WS_CHILD | WS_VISIBLE | ES_PASSWORD | ES_AUTOHSCROLL,
                x + labelW, y, ctrlW, 24, hwnd, reinterpret_cast<HMENU>(kIdApiKeyEdit), g_state.hInstance, nullptr);
            y += 32;

            CreateWindowExW(0, L"STATIC", L"附加提示:",
                WS_CHILD | WS_VISIBLE,
                x, y + 3, labelW, 22, hwnd, reinterpret_cast<HMENU>(kIdAiPromptLabel), g_state.hInstance, nullptr);
            g_state.hAiPromptEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                x + labelW, y, ctrlW, 24, hwnd, reinterpret_cast<HMENU>(kIdAiPromptEdit), g_state.hInstance, nullptr);
            y += 36;

            g_state.hPickBtn = CreateWindowExW(0, L"BUTTON", L"🎲 开始抽签",
                WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
                x, y, 140, 36, hwnd, reinterpret_cast<HMENU>(kIdPickBtn), g_state.hInstance, nullptr);
            SendMessage(g_state.hPickBtn, WM_SETFONT, reinterpret_cast<WPARAM>(g_state.hFontBold), TRUE);

            g_state.hSaveBtn = CreateWindowExW(0, L"BUTTON", L"保存结果…",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                x + 150, y, 110, 36, hwnd, reinterpret_cast<HMENU>(kIdSaveBtn), g_state.hInstance, nullptr);
            g_state.hCopyBtn = CreateWindowExW(0, L"BUTTON", L"复制到剪贴板",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                x + 270, y, 130, 36, hwnd, reinterpret_cast<HMENU>(kIdCopyBtn), g_state.hInstance, nullptr);
            g_state.hClearBtn = CreateWindowExW(0, L"BUTTON", L"清空",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                x + 410, y, 80, 36, hwnd, reinterpret_cast<HMENU>(kIdClearBtn), g_state.hInstance, nullptr);
            y += 46;

            g_state.hResultList = CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"",
                WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
                x, y, ctrlW + labelW - 70, 260,
                hwnd, reinterpret_cast<HMENU>(kIdResultList), g_state.hInstance, nullptr);
            ListView_SetExtendedListViewStyle(g_state.hResultList,
                LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

            LVCOLUMNW col{};
            col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
            col.pszText = const_cast<wchar_t*>(L"#");
            col.cx = 40;
            col.iSubItem = 0;
            ListView_InsertColumn(g_state.hResultList, 0, &col);

            col.pszText = const_cast<wchar_t*>(L"姓名");
            col.cx = 160;
            col.iSubItem = 1;
            ListView_InsertColumn(g_state.hResultList, 1, &col);

            col.pszText = const_cast<wchar_t*>(L"附加信息 / AI 推荐");
            col.cx = 360;
            col.iSubItem = 2;
            ListView_InsertColumn(g_state.hResultList, 2, &col);
            y += 270;

            g_state.hProgress = CreateWindowExW(0, PROGRESS_CLASSW, L"",
                WS_CHILD | PBS_MARQUEE,
                x, y, ctrlW + labelW - 70, 8,
                hwnd, reinterpret_cast<HMENU>(kIdProgress), g_state.hInstance, nullptr);
            ShowWindow(g_state.hProgress, SW_HIDE);
            y += 14;

            g_state.hStatus = CreateWindowExW(0, L"STATIC", L"就绪",
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                x, y, ctrlW + labelW - 70, 22,
                hwnd, reinterpret_cast<HMENU>(kIdStatusLabel), g_state.hInstance, nullptr);

            for (HWND h : { g_state.hFileEdit, g_state.hCountEdit, g_state.hApiKeyEdit,
                            g_state.hAiPromptEdit, g_state.hFileLabel, g_state.hCountLabel,
                            g_state.hAiKeyLabel, g_state.hAiPromptLabel,
                            g_state.hSeedLabel, g_state.hStatus }) {
                if (h) SendMessage(h, WM_SETFONT, reinterpret_cast<WPARAM>(g_state.hFont), TRUE);
            }
            return 0;
        }
        case WM_COMMAND: {
            int id = LOWORD(wp);
            int code = HIWORD(wp);
            if (id == kIdBrowseBtn && code == BN_CLICKED) {
                std::string p;
                if (browseForFile(hwnd, p)) {
                    SetWindowTextW(g_state.hFileEdit, toWide(p).c_str());
                }
            } else if (id == kIdPickBtn && code == BN_CLICKED) {
                if (g_state.busy) return 0;
                std::string filePath = getWindowTextStd(g_state.hFileEdit);
                std::string countStr = getWindowTextStd(g_state.hCountEdit);
                if (filePath.empty()) {
                    MessageBoxW(hwnd, L"请先选择表格文件！", L"提示", MB_ICONWARNING);
                    return 0;
                }
                int cnt = atoi(countStr.c_str());
                if (cnt <= 0) {
                    MessageBoxW(hwnd, L"请输入大于 0 的抽取人数。", L"提示", MB_ICONWARNING);
                    return 0;
                }
                bool aiEnabled = (SendMessage(g_state.hAiCheck, BM_GETCHECK, 0, 0) == BST_CHECKED);
                std::string apiKey = getWindowTextStd(g_state.hApiKeyEdit);
                std::string aiPrompt = getWindowTextStd(g_state.hAiPromptEdit);

                if (aiEnabled && apiKey.empty()) {
                    if (MessageBoxW(hwnd, L"已启用 AI 但未填写 API Key，是否继续 (仅本地抽签)?",
                                    L"提示", MB_ICONQUESTION | MB_YESNO) != IDYES) {
                        return 0;
                    }
                    aiEnabled = false;
                }

                setBusy(true);
                setStatus("正在读取表格…");

                std::thread worker([filePath, cnt, aiEnabled, apiKey, aiPrompt]() {
                    try {
                        auto reader = ReaderFactory::create(filePath);
                        TableData table = reader->read(filePath);
                        if (table.empty()) {
                            SendMessage(g_state.hwnd, WM_USER + 100, 0,
                                reinterpret_cast<LPARAM>(new std::string("表格中没有任何数据行。")));
                            setBusy(false);
                            return;
                        }
                        RandomPicker picker(0);
                        std::vector<Person> picked = picker.sample(table.rows, cnt);

                        RunReport rep;
                        rep.sourceFile = filePath;
                        rep.totalCount = table.size();
                        rep.requestedCount = cnt;
                        rep.runTime = std::chrono::system_clock::now();
                        rep.aiEnabled = aiEnabled;
                        for (auto& p : picked) {
                            PickResult r;
                            r.person = std::move(p);
                            r.timestamp = rep.runTime;
                            rep.picks.push_back(std::move(r));
                        }

                        if (aiEnabled) {
                            AiConfig cfg;
                            cfg.enabled = true;
                            cfg.apiKey = apiKey;
                            cfg.extraPrompt = aiPrompt;
                            AiClient ai(cfg);
                            std::string err;
                            if (ai.analyze(rep, rep.aiSummary, err)) {
                                Log::info("AI 分析完成");
                            } else {
                                Log::warn("AI 分析失败: {}", err);
                                rep.aiEnabled = false;
                            }
                        }

                        std::lock_guard<std::mutex> lk(g_state.mtx);
                        g_state.lastReport = std::move(rep);
                        g_state.lastTable = std::move(table);
                        g_state.haveResult = true;
                        SendMessage(g_state.hwnd, WM_USER + 101, 0, 0);
                    } catch (const std::exception& ex) {
                        std::string err = std::string("抽签失败: ") + ex.what();
                        SendMessage(g_state.hwnd, WM_USER + 100, 0,
                            reinterpret_cast<LPARAM>(new std::string(err)));
                    }
                });
                worker.detach();
            } else if (id == kIdSaveBtn && code == BN_CLICKED) {
                std::lock_guard<std::mutex> lk(g_state.mtx);
                if (!g_state.haveResult) {
                    MessageBoxW(hwnd, L"暂无可保存的结果，请先抽签。", L"提示", MB_ICONINFORMATION);
                    return 0;
                }
                std::string suggested = "随机抽签结果.txt";
                std::string outPath;
                if (saveToFile(hwnd, suggested, outPath)) {
                    std::string text = Reporter::toText(g_state.lastReport, g_state.lastTable);
                    std::string lower = toLower(outPath);
                    bool ok = false;
                    if (lower.size() >= 4 && lower.substr(lower.size() - 4) == ".csv") {
                        ok = Reporter::saveCsv(outPath, g_state.lastReport, g_state.lastTable);
                    } else {
                        ok = Reporter::saveText(outPath, text);
                    }
                    if (ok) {
                        std::string msg = "结果已保存到: " + outPath;
                        MessageBoxW(hwnd, toWide(msg).c_str(), L"成功", MB_ICONINFORMATION);
                        setStatus(msg);
                    } else {
                        MessageBoxW(hwnd, L"保存失败。", L"错误", MB_ICONERROR);
                    }
                }
            } else if (id == kIdCopyBtn && code == BN_CLICKED) {
                std::lock_guard<std::mutex> lk(g_state.mtx);
                if (!g_state.haveResult || g_state.resultCache.empty()) {
                    MessageBoxW(hwnd, L"暂无可复制的内容。", L"提示", MB_ICONINFORMATION);
                    return 0;
                }
                std::ostringstream oss;
                for (std::size_t i = 0; i < g_state.resultCache.size(); ++i) {
                    oss << (i + 1) << ". " << g_state.resultCache[i] << "\r\n";
                }
                std::string text = oss.str();
                if (OpenClipboard(hwnd)) {
                    EmptyClipboard();
                    HGLOBAL hg = GlobalAlloc(GMEM_MOVEABLE, text.size() + 1);
                    if (hg) {
                        memcpy(GlobalLock(hg), text.c_str(), text.size() + 1);
                        GlobalUnlock(hg);
                        SetClipboardData(CF_TEXT, hg);
                    }
                    CloseClipboard();
                    setStatus("已复制到剪贴板");
                }
            } else if (id == kIdClearBtn && code == BN_CLICKED) {
                clearResults();
                std::lock_guard<std::mutex> lk(g_state.mtx);
                g_state.haveResult = false;
                g_state.resultCache.clear();
                setStatus("已清空");
            }
            return 0;
        }
        case WM_USER + 100: {
            std::unique_ptr<std::string> msg(reinterpret_cast<std::string*>(lp));
            if (msg) {
                MessageBoxW(hwnd, toWide(*msg).c_str(), L"错误", MB_ICONERROR);
                setStatus(*msg);
            }
            setBusy(false);
            return 0;
        }
        case WM_USER + 101: {
            {
                std::lock_guard<std::mutex> lk(g_state.mtx);
                populateResults(g_state.lastReport.picks, g_state.lastTable,
                                g_state.lastReport.aiEnabled);
            }
            std::ostringstream oss;
            oss << "完成: 从 " << g_state.lastReport.totalCount << " 人中抽出 "
                << g_state.lastReport.picks.size() << " 位";
            setStatus(oss.str());
            setBusy(false);
            return 0;
        }
        case WM_SIZE: {
            int w = LOWORD(lp), h = HIWORD(lp);
            if (g_state.hResultList) {
                RECT rc{};
                GetClientRect(hwnd, &rc);
                SetWindowPos(g_state.hResultList, nullptr, 16, 200,
                             rc.right - 32, rc.bottom - 240,
                             SWP_NOZORDER);
            }
            (void)w; (void)h;
            return 0;
        }
        case WM_DESTROY:
            if (g_state.hFontBold) DeleteObject(g_state.hFontBold);
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

bool registerClass(HINSTANCE hInst) {
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = wndProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursorW(nullptr, reinterpret_cast<LPCWSTR>(IDC_ARROW));
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.lpszClassName = kClassName;
    wc.hIcon = LoadIconW(nullptr, reinterpret_cast<LPCWSTR>(IDI_APPLICATION));
    return RegisterClassExW(&wc) != 0;
}

}

int runGui(const GuiOptions& opts) {
#if defined(_WIN32)
    FreeConsole();
#endif
    INITCOMMONCONTROLSEX icc{ sizeof(icc), ICC_LISTVIEW_CLASSES | ICC_PROGRESS_CLASS };
    InitCommonControlsEx(&icc);

    HINSTANCE hInst = GetModuleHandleW(nullptr);
    g_state.hInstance = hInst;

    if (!registerClass(hInst)) {
        MessageBoxW(nullptr, L"窗口类注册失败", L"错误", MB_ICONERROR);
        return 1;
    }

    HWND hwnd = CreateWindowExW(0, kClassName, L"🎲 随机抽签工具  v1.0",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 720, 620,
        nullptr, nullptr, hInst, nullptr);
    if (!hwnd) {
        MessageBoxW(nullptr, L"窗口创建失败", L"错误", MB_ICONERROR);
        return 1;
    }

    if (!opts.initialFile.empty()) {
        SetWindowTextW(g_state.hFileEdit, toWide(opts.initialFile).c_str());
    }
    if (opts.ai.enabled && !opts.ai.apiKey.empty()) {
        SendMessage(g_state.hAiCheck, BM_SETCHECK, BST_CHECKED, 0);
        SetWindowTextW(g_state.hApiKeyEdit, toWide(opts.ai.apiKey).c_str());
        if (!opts.ai.extraPrompt.empty()) {
            SetWindowTextW(g_state.hAiPromptEdit, toWide(opts.ai.extraPrompt).c_str());
        }
    }

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return static_cast<int>(msg.wParam);
}

}

#else

namespace picker {

int runGui(const GuiOptions&) {
    fprintf(stderr, "GUI 仅在 Windows 平台可用。请使用 --no-gui 参数运行命令行模式。\n");
    return 1;
}

}

#endif
