#pragma once

#include <string>
#include <map>
#include <vector>

namespace picker {

struct HttpResponse {
    int statusCode = 0;
    std::string body;
    std::string errorMessage;
    bool ok() const { return statusCode > 0 && errorMessage.empty(); }
};

class HttpClient {
public:
    HttpClient();
    ~HttpClient();

    HttpResponse get(const std::string& url,
                     const std::map<std::string, std::string>& headers = {},
                     int timeoutMs = 30000);

    HttpResponse post(const std::string& url,
                      const std::string& body,
                      const std::map<std::string, std::string>& headers = {},
                      int timeoutMs = 60000);

private:
    void* session_ = nullptr;
    void* connect_ = nullptr;
    void* request_ = nullptr;
};

}
