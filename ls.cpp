#include "ls.h"
#include "auth.h"
#include <iostream>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
    output->append((char*)contents, size * nmemb);
    return size * nmemb;
}

bool list_drive(const std::string& access_token, std::string& output) {
    CURL* curl = curl_easy_init();
    if (!curl) return false;

    curl_easy_setopt(curl, CURLOPT_URL, "https://graph.microsoft.com/v1.0/me/drive/root/children");
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, ("Authorization: Bearer " + access_token).c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &output);

    CURLcode res = curl_easy_perform(curl);
    long response_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
    curl_easy_cleanup(curl);

    return (res == CURLE_OK && response_code == 200);
}

void list_drive_items() {
    std::string access_token = get_access_token(); 
    std::string response;

    if (!list_drive(access_token, response)) {
        std::cout << "⚠️ Access token might be expired. Attempting to refresh...\n";
        access_token = refresh_access_token();
        if (access_token.empty()) {
            std::cerr << "❌ Failed to refresh token. Please run './onedrivecli auth' again.\n";
            return;
        }

        if (!list_drive(access_token, response)) {
            std::cerr << "❌ Failed to list drive items after token refresh.\n";
            return;
        }
    }

    try {
        json result = json::parse(response);
        std::cout << "\n📁 OneDrive Contents:\n";
        for (const auto& item : result["value"]) {
            std::string name = item["name"];
            bool is_folder = item.contains("folder") && !item["folder"].is_null();
            std::cout << (is_folder ? "📁 " : "📄 ") << name << "\n";
        }
    } catch (...) {
        std::cerr << "❌ Failed to parse API response:\n" << response << "\n";
    }
}