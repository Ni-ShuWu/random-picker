#pragma once

#include "common.h"
#include <memory>
#include <string>
#include <stdexcept>

namespace picker {

class ReaderError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class ITableReader {
public:
    virtual ~ITableReader() = default;
    virtual TableData read(const std::string& path) = 0;
};

class CsvReader : public ITableReader {
public:
    explicit CsvReader(char delimiter = ',', bool hasHeader = true)
        : delimiter_(delimiter), hasHeader_(hasHeader) {}
    TableData read(const std::string& path) override;
private:
    char delimiter_;
    bool hasHeader_;
};

class XlsxReader : public ITableReader {
public:
    XlsxReader() = default;
    TableData read(const std::string& path) override;
};

class ReaderFactory {
public:
    static std::unique_ptr<ITableReader> create(const std::string& path);
};

}
