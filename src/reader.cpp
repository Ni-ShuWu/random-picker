#include "reader.h"
#include "logger.h"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <cctype>
#include <vector>
#include <cstring>

namespace picker {

std::string TableData::nameOf(const Person& p) const {
    if (nameColumnIndex != static_cast<std::size_t>(-1) &&
        nameColumnIndex < headers.size()) {
        auto it = p.fields.find(headers[nameColumnIndex]);
        if (it != p.fields.end() && !it->second.empty()) return it->second;
    }
    for (const auto& [k, v] : p.fields) {
        if (!v.empty()) return v;
    }
    return {};
}

static std::vector<std::string> parseCsvLine(const std::string& line, char delim) {
    std::vector<std::string> fields;
    std::string cur;
    bool inQuotes = false;
    for (std::size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (inQuotes) {
            if (c == '"') {
                if (i + 1 < line.size() && line[i + 1] == '"') {
                    cur.push_back('"');
                    ++i;
                } else {
                    inQuotes = false;
                }
            } else {
                cur.push_back(c);
            }
        } else {
            if (c == '"') {
                inQuotes = true;
            } else if (c == delim) {
                fields.push_back(cur);
                cur.clear();
            } else {
                cur.push_back(c);
            }
        }
    }
    fields.push_back(cur);
    return fields;
}

static std::string readFileUtf8(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) throw ReaderError("无法打开文件: " + path);
    std::ostringstream oss;
    oss << f.rdbuf();
    std::string raw = oss.str();
    if (raw.size() >= 3 &&
        static_cast<unsigned char>(raw[0]) == 0xEF &&
        static_cast<unsigned char>(raw[1]) == 0xBB &&
        static_cast<unsigned char>(raw[2]) == 0xBF) {
        return raw.substr(3);
    }
    return raw;
}

TableData CsvReader::read(const std::string& path) {
    TableData data;
    data.sourceFile = path;
    std::string content = readFileUtf8(path);

    std::vector<std::string> lines;
    std::string cur;
    for (char c : content) {
        if (c == '\r') continue;
        if (c == '\n') {
            lines.push_back(cur);
            cur.clear();
        } else {
            cur.push_back(c);
        }
    }
    if (!cur.empty()) lines.push_back(cur);

    std::size_t startLine = 0;
    if (hasHeader_ && !lines.empty()) {
        data.headers = parseCsvLine(lines[0], delimiter_);
        for (auto& h : data.headers) h = trim(h);
        startLine = 1;

        std::size_t nameIdx = static_cast<std::size_t>(-1);
        for (std::size_t i = 0; i < data.headers.size(); ++i) {
            const std::string h = toLower(data.headers[i]);
            if (h == "name" || h == "姓名" || h == "xingming" ||
                h == "名字" || h == "student" || h == "学生") {
                nameIdx = i;
                break;
            }
        }
        if (nameIdx == static_cast<std::size_t>(-1) && !data.headers.empty()) {
            nameIdx = 0;
        }
        data.nameColumnIndex = nameIdx;
    } else {
        data.nameColumnIndex = 0;
    }

    for (std::size_t li = startLine; li < lines.size(); ++li) {
        const std::string& line = lines[li];
        if (line.empty()) continue;
        auto fields = parseCsvLine(line, delimiter_);
        Person p;
        p.sourceRow = li + 1;
        if (data.headers.empty()) {
            data.headers.resize(fields.size());
            for (std::size_t i = 0; i < fields.size(); ++i) {
                data.headers[i] = "列" + std::to_string(i + 1);
                p.fields[data.headers[i]] = trim(fields[i]);
            }
        } else {
            for (std::size_t i = 0; i < data.headers.size(); ++i) {
                std::string val = i < fields.size() ? trim(fields[i]) : std::string();
                p.fields[data.headers[i]] = val;
            }
        }
        if (!p.fields.empty()) data.rows.push_back(std::move(p));
    }

    Log::info("CSV 读取完成，共 {} 行 (含表头 {} 个字段)", data.rows.size(), data.headers.size());
    return data;
}

}

#ifdef PICKER_WITH_XLSX

#include <zip.h>
#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace picker {

static std::string xmlDecode(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (std::size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '&') {
            if (s.compare(i, 5, "&amp;") == 0) { out.push_back('&'); i += 4; }
            else if (s.compare(i, 4, "&lt;") == 0) { out.push_back('<'); i += 3; }
            else if (s.compare(i, 4, "&gt;") == 0) { out.push_back('>'); i += 3; }
            else if (s.compare(i, 6, "&quot;") == 0) { out.push_back('"'); i += 5; }
            else if (s.compare(i, 6, "&apos;") == 0) { out.push_back('\''); i += 5; }
            else out.push_back(s[i]);
        } else {
            out.push_back(s[i]);
        }
    }
    return out;
}

static std::string zipReadEntry(zip_t* zf, const std::string& name) {
    zip_stat_t st{};
    zip_stat_init(&st);
    if (zip_stat_index(zf, zip_name_locate(zf, name.c_str(), 0), 0, &st) != 0) return {};
    zip_file_t* zfile = zip_fopen_index(zf, st.index, 0);
    if (!zfile) return {};
    std::string out;
    out.resize(st.size);
    zip_int64_t total = 0;
    while (total < static_cast<zip_int64_t>(st.size)) {
        zip_int64_t n = zip_fread(zfile, &out[total], st.size - total);
        if (n <= 0) break;
        total += n;
    }
    zip_fclose(zfile);
    out.resize(total);
    return out;
}

static std::vector<std::string> xlsxSharedStrings(zip_t* zf) {
    std::vector<std::string> result;
    std::string xml = zipReadEntry(zf, "xl/sharedStrings.xml");
    if (xml.empty()) return result;
    std::size_t pos = 0;
    while ((pos = xml.find("<si>", pos)) != std::string::npos) {
        std::size_t end = xml.find("</si>", pos);
        if (end == std::string::npos) break;
        std::string item = xml.substr(pos + 4, end - pos - 4);
        std::string text;
        std::size_t tpos = 0;
        while ((tpos = item.find("<t", tpos)) != std::string::npos) {
            std::size_t gt = item.find('>', tpos);
            if (gt == std::string::npos) break;
            std::size_t te = item.find("</t>", gt);
            if (te == std::string::npos) break;
            text += xmlDecode(item.substr(gt + 1, te - gt - 1));
            tpos = te + 4;
        }
        result.push_back(text);
        pos = end + 5;
    }
    return result;
}

TableData XlsxReader::read(const std::string& path) {
    TableData data;
    data.sourceFile = path;

    zip_error_t zerr;
    zip_error_init(&zerr);
    zip_source_t* src = nullptr;
#if defined(_WIN32)
    auto tryOpenWin32 = [&](UINT cp, const char* tag) -> bool {
        int wlen = MultiByteToWideChar(cp, 0, path.c_str(), -1, nullptr, 0);
        if (wlen <= 0) return false;
        std::wstring wpath(static_cast<std::size_t>(wlen - 1), L'\0');
        MultiByteToWideChar(cp, 0, path.c_str(), -1, &wpath[0], wlen);
        zip_source_t* s = zip_source_win32w_create(wpath.c_str(), 0, -1, &zerr);
        if (s) { src = s; return true; }
        Log::debug("CP{} (zip_err={}): {}", tag, zip_error_code_zip(&zerr),
                   zip_error_strerror(&zerr));
        return false;
    };
    if (!tryOpenWin32(CP_UTF8, "UTF-8")) {
        tryOpenWin32(CP_ACP, "ANSI");
    }
#else
    src = zip_source_file_create(path.c_str(), 0, -1, &zerr);
#endif
    if (!src) {
        int code = zip_error_code_zip(&zerr);
        zip_error_fini(&zerr);
        std::string msg = "无法创建 xlsx 文件源 (zip 错误码: " + std::to_string(code) + ")";
        throw ReaderError(msg);
    }
    zip_t* zf = zip_open_from_source(src, ZIP_RDONLY, &zerr);
    if (!zf) {
        int code = zip_error_code_zip(&zerr);
        zip_error_fini(&zerr);
        std::string msg = "无法打开 xlsx 文件 (zip 错误码: " + std::to_string(code) + ")";
        throw ReaderError(msg);
    }

    auto sharedStrings = xlsxSharedStrings(zf);

    std::string sheetXml = zipReadEntry(zf, "xl/worksheets/sheet1.xml");
    if (sheetXml.empty()) {
        zip_close(zf);
        throw ReaderError("xlsx 中找不到 sheet1.xml");
    }

    auto extractCells = [&](const std::string& rowXml,
                            std::unordered_map<std::string, std::string>& cells) {
        std::size_t cp = 0;
        while ((cp = rowXml.find("<c ", cp)) != std::string::npos) {
            std::size_t cend = rowXml.find("</c>", cp);
            if (cend == std::string::npos) cend = rowXml.find("/>", cp);
            if (cend == std::string::npos) break;
            std::string cell = rowXml.substr(cp, cend - cp);
            std::string ref;
            std::size_t rp = cell.find("r=\"");
            if (rp != std::string::npos) {
                std::size_t re = cell.find('"', rp + 3);
                ref = cell.substr(rp + 3, re - rp - 3);
            }
            std::string tVal = "n";
            std::size_t tp = cell.find(" t=\"");
            if (tp != std::string::npos) {
                std::size_t te = cell.find('"', tp + 4);
                tVal = cell.substr(tp + 4, te - tp - 4);
            }
            std::string value;
            std::size_t vp = cell.find("<v>");
            if (vp != std::string::npos) {
                std::size_t ve = cell.find("</v>", vp);
                if (ve != std::string::npos) value = cell.substr(vp + 3, ve - vp - 3);
            }
            std::size_t isp = cell.find("<is><t");
            if (isp != std::string::npos) {
                std::size_t ise = cell.find("</t></is>", isp);
                if (ise != std::string::npos) {
                    std::size_t a = cell.find('>', isp);
                    value = cell.substr(a + 1, ise - a - 1);
                }
            }

            if (!ref.empty()) {
                std::string colLetters;
                for (char c : ref) {
                    if (std::isalpha(static_cast<unsigned char>(c))) colLetters.push_back(c);
                    else break;
                }
                cells[colLetters] = (tVal == "s" && !value.empty() &&
                                     std::stoi(value) >= 0 &&
                                     std::stoi(value) < static_cast<int>(sharedStrings.size()))
                                    ? sharedStrings[std::stoi(value)]
                                    : xmlDecode(value);
            }
            cp = cend > cp ? cend : cp + 1;
        }
    };

    std::size_t rowPos = 0;
    int rowIndex = 0;
    bool firstRow = true;
    std::unordered_map<std::string, std::string> maxColMap;

    while ((rowPos = sheetXml.find("<row ", rowPos)) != std::string::npos) {
        std::size_t rowEnd = sheetXml.find("</row>", rowPos);
        if (rowEnd == std::string::npos) break;
        std::string rowXml = sheetXml.substr(rowPos, rowEnd - rowPos);

        std::unordered_map<std::string, std::string> cells;
        extractCells(rowXml, cells);

        std::vector<std::pair<std::string, std::string>> sorted(cells.begin(), cells.end());
        std::sort(sorted.begin(), sorted.end());

        if (firstRow) {
            int colIdx = 0;
            for (auto& [col, val] : sorted) {
                data.headers.push_back(trim(val.empty() ? ("列" + std::to_string(colIdx + 1)) : val));
                maxColMap[col] = std::to_string(colIdx);
                ++colIdx;
            }
            std::size_t nameIdx = static_cast<std::size_t>(-1);
            for (std::size_t i = 0; i < data.headers.size(); ++i) {
                std::string h = toLower(data.headers[i]);
                if (h == "name" || h == "姓名" || h == "xingming" ||
                    h == "名字" || h == "student" || h == "学生") {
                    nameIdx = i;
                    break;
                }
            }
            if (nameIdx == static_cast<std::size_t>(-1) && !data.headers.empty()) nameIdx = 0;
            data.nameColumnIndex = nameIdx;
            firstRow = false;
        } else {
            Person p;
            p.sourceRow = rowIndex + 1;
            int lastCol = -1;
            std::string lastColLetters;
            for (auto& [col, val] : sorted) {
                int cur = 0;
                for (char c : col) cur = cur * 26 + (std::toupper(static_cast<unsigned char>(c)) - 'A' + 1);
                cur -= 1;
                while (lastCol + 1 < cur) {
                    ++lastCol;
                    std::string header = lastCol < static_cast<int>(data.headers.size())
                                         ? data.headers[lastCol] : "";
                    p.fields[header] = "";
                }
                lastCol = cur;
                lastColLetters = col;
                std::string header = cur < static_cast<int>(data.headers.size())
                                     ? data.headers[cur] : ("列" + std::to_string(cur + 1));
                p.fields[header] = trim(val);
            }
            if (!p.fields.empty()) data.rows.push_back(std::move(p));
        }
        rowIndex++;
        rowPos = rowEnd + 6;
    }

    zip_close(zf);
    Log::info("XLSX 读取完成，共 {} 行 (含表头 {} 个字段)", data.rows.size(), data.headers.size());
    return data;
}

}

#else

namespace picker {

TableData XlsxReader::read(const std::string&) {
    throw ReaderError("XLSX 支持未编译。请使用 libzip 重新编译 (设置 PICKER_WITH_XLSX 宏)");
}

}

#endif

namespace picker {

std::unique_ptr<ITableReader> ReaderFactory::create(const std::string& path) {
    std::string p = toLower(path);
    if (p.size() >= 4 && p.substr(p.size() - 4) == ".csv") {
        return std::make_unique<CsvReader>(',', true);
    }
    if (p.size() >= 5 && p.substr(p.size() - 5) == ".xlsx") {
        return std::make_unique<XlsxReader>();
    }
    if (p.size() >= 4 && (p.substr(p.size() - 4) == ".txt" || p.substr(p.size() - 4) == ".tsv")) {
        char delim = (p.substr(p.size() - 4) == ".tsv") ? '\t' : ',';
        return std::make_unique<CsvReader>(delim, true);
    }
    throw ReaderError("不支持的文件格式: " + path + " (仅支持 .csv, .xlsx, .txt, .tsv)");
}

}
