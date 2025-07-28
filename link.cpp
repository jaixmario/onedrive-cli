#include "auth.h"
#include "utils.h"

#include <iostream>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
    output->append((char*)contents, size * nmemb);
    return size * nmemb;
}

bool try_generate_link(const std::string& token, const std::string& path, std::string& output) {
    std::string url = "https://graph.microsoft.com/v1.0/me/drive/root:/" + url_encode(path) + ":/createLink";

    CURL* curl = curl_easy_init();
    if (!curl) return false;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, ("Authorization: Bearer " + token).c_str());
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, R"({"type":"view","scope":"anonymous"})");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &output);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    return (res == CURLE_OK);
}

void generate_shareable_link(const std::string& path) {
    std::string token = get_access_token();
    std::string response;

    if (!try_generate_link(token, path, response)) {
        std::cerr << "❌ Request failed.\n";
        return;
    }

    json result;
    try {
        result = json::parse(response);
    } catch (...) {
        std::cerr << "❌ Failed to parse link response:\n" << response << "\n";
        return;
    }

    if (result.contains("error")) {
        std::string code = result["error"]["code"];
        if (code == "InvalidAuthenticationToken") {
            std::cout << "⚠️ Token expired. Refreshing...\n";
            token = refresh_access_token();
            if (token.empty()) {
                std::cerr << "❌ Token refresh failed.\n";
                return;
            }

            response.clear();
            if (!try_generate_link(token, path, response)) {
                std::cerr << "❌ Request failed after token refresh.\n";
                return;
            }

            try {
                result = json::parse(response);
            } catch (...) {
                std::cerr << "❌ Failed to parse response after refresh:\n" << response << "\n";
                return;
            }

            if (result.contains("error")) {
                std::string code2 = result["error"]["code"];
                if (code2 == "itemNotFound") {
                    std::cerr << "❌ File not found: " << path << "\n";
                    return;
                } else {
                    std::cerr << "❌ API Error after refresh: " << code2 << "\n";
                    return;
                }
            }
        } else if (code == "itemNotFound") {
            std::cerr << "❌ File not found: " << path << "\n";
            return;
        } else {
            std::cerr << "❌ API Error: " << code << "\n";
            return;
        }
    }

    std::string link = result["link"]["webUrl"];
    std::cout << "🔗 Shareable Link: " << link << "\n";
}