#include "http.h"
#include "logger.h"

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define _WINSOCKAPI_
#include <windows.h>
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")
#endif

namespace picker {

#if defined(_WIN32)

HttpClient::HttpClient() {
    session_ = WinHttpOpen(L"RandomPicker/1.0",
                           WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                           WINHTTP_NO_PROXY_NAME,
                           WINHTTP_NO_PROXY_BYPASS, 0);
}

HttpClient::~HttpClient() {
    if (request_) WinHttpCloseHandle(request_);
    if (connect_) WinHttpCloseHandle(connect_);
    if (session_) WinHttpCloseHandle(session_);
}

namespace {
std::wstring toWide(const std::string& s) {
    if (s.empty()) return {};
    int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    std::wstring out(static_cast<std::size_t>(len - 1), L'\0');
    if (len > 1) MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &out[0], len);
    return out;
}
}

HttpResponse HttpClient::get(const std::string& url,
                              const std::map<std::string, std::string>& headers,
                              int timeoutMs) {
    HttpResponse resp;
    if (!session_) { resp.errorMessage = "WinHTTP 会话初始化失败"; return resp; }

    std::wstring wurl = toWide(url);
    URL_COMPONENTS uc{};
    uc.dwStructSize = sizeof(uc);
    wchar_t hostName[256]{};
    wchar_t urlPath[2048]{};
    uc.lpszHostName = hostName;
    uc.dwHostNameLength = 256;
    uc.lpszUrlPath = urlPath;
    uc.dwUrlPathLength = 2048;
    if (!WinHttpCrackUrl(wurl.c_str(), static_cast<DWORD>(wurl.size()), 0, &uc)) {
        resp.errorMessage = "URL 解析失败"; return resp;
    }

    connect_ = WinHttpConnect(session_, hostName, uc.nPort, 0);
    if (!connect_) { resp.errorMessage = "连接失败"; return resp; }

    DWORD flags = (uc.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
    request_ = WinHttpOpenRequest(connect_, L"GET", urlPath,
                                  nullptr, WINHTTP_NO_REFERER,
                                  WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!request_) { resp.errorMessage = "创建请求失败"; return resp; }

    WinHttpSetTimeouts(request_, timeoutMs, timeoutMs, timeoutMs, timeoutMs);

    for (const auto& [k, v] : headers) {
        std::wstring h = toWide(k + ": " + v);
        WinHttpAddRequestHeaders(request_, h.c_str(),
                                  static_cast<DWORD>(h.size()),
                                  WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE);
    }

    if (!WinHttpSendRequest(request_, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                             WINHTTP_NO_REQUEST_DATA, 0, 0, 0)) {
        resp.errorMessage = "发送请求失败"; return resp;
    }
    if (!WinHttpReceiveResponse(request_, nullptr)) {
        resp.errorMessage = "接收响应失败"; return resp;
    }

    DWORD status = 0; DWORD len = sizeof(status);
    WinHttpQueryHeaders(request_,
                        WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                        WINHTTP_HEADER_NAME_BY_INDEX, &status, &len, WINHTTP_NO_HEADER_INDEX);
    resp.statusCode = static_cast<int>(status);

    std::string body;
    for (;;) {
        DWORD avail = 0;
        if (!WinHttpQueryDataAvailable(request_, &avail)) break;
        if (avail == 0) break;
        std::string chunk(avail, '\0');
        DWORD read = 0;
        if (!WinHttpReadData(request_, &chunk[0], avail, &read)) break;
        chunk.resize(read);
        body += chunk;
    }
    resp.body = std::move(body);
    return resp;
}

HttpResponse HttpClient::post(const std::string& url,
                               const std::string& body,
                               const std::map<std::string, std::string>& headers,
                               int timeoutMs) {
    HttpResponse resp;
    if (!session_) { resp.errorMessage = "WinHTTP 会话初始化失败"; return resp; }

    std::wstring wurl = toWide(url);
    URL_COMPONENTS uc{};
    uc.dwStructSize = sizeof(uc);
    wchar_t hostName[256]{};
    wchar_t urlPath[2048]{};
    uc.lpszHostName = hostName;
    uc.dwHostNameLength = 256;
    uc.lpszUrlPath = urlPath;
    uc.dwUrlPathLength = 2048;
    if (!WinHttpCrackUrl(wurl.c_str(), static_cast<DWORD>(wurl.size()), 0, &uc)) {
        resp.errorMessage = "URL 解析失败"; return resp;
    }

    connect_ = WinHttpConnect(session_, hostName, uc.nPort, 0);
    if (!connect_) { resp.errorMessage = "连接失败"; return resp; }

    DWORD flags = (uc.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
    request_ = WinHttpOpenRequest(connect_, L"POST", urlPath,
                                  nullptr, WINHTTP_NO_REFERER,
                                  WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!request_) { resp.errorMessage = "创建请求失败"; return resp; }

    WinHttpSetTimeouts(request_, timeoutMs, timeoutMs, timeoutMs, timeoutMs);

    for (const auto& [k, v] : headers) {
        std::wstring h = toWide(k + ": " + v);
        WinHttpAddRequestHeaders(request_, h.c_str(),
                                  static_cast<DWORD>(h.size()),
                                  WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE);
    }

    if (!WinHttpSendRequest(request_, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                             const_cast<char*>(body.data()),
                             static_cast<DWORD>(body.size()),
                             static_cast<DWORD>(body.size()), 0)) {
        resp.errorMessage = "发送请求失败"; return resp;
    }
    if (!WinHttpReceiveResponse(request_, nullptr)) {
        resp.errorMessage = "接收响应失败"; return resp;
    }

    DWORD status = 0; DWORD len = sizeof(status);
    WinHttpQueryHeaders(request_,
                        WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                        WINHTTP_HEADER_NAME_BY_INDEX, &status, &len, WINHTTP_NO_HEADER_INDEX);
    resp.statusCode = static_cast<int>(status);

    std::string out;
    for (;;) {
        DWORD avail = 0;
        if (!WinHttpQueryDataAvailable(request_, &avail)) break;
        if (avail == 0) break;
        std::string chunk(avail, '\0');
        DWORD read = 0;
        if (!WinHttpReadData(request_, &chunk[0], avail, &read)) break;
        chunk.resize(read);
        out += chunk;
    }
    resp.body = std::move(out);
    return resp;
}

#else

HttpClient::HttpClient() = default;
HttpClient::~HttpClient() = default;

HttpResponse HttpClient::get(const std::string&, const std::map<std::string, std::string>&, int) {
    HttpResponse r; r.errorMessage = "当前平台不支持 HTTP"; return r;
}
HttpResponse HttpClient::post(const std::string&, const std::string&,
                               const std::map<std::string, std::string>&, int) {
    HttpResponse r; r.errorMessage = "当前平台不支持 HTTP"; return r;
}

#endif

}
