#include "ls.h"
#include "auth.h"
#include <iostream>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <sstream>
#include <iomanip>

using json = nlohmann::json;

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
    output->append((char*)contents, size * nmemb);
    return size * nmemb;
}
std::string url_encode(const std::string& value) {
    std::ostringstream escaped;
    for (unsigned char c : value) {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~' || c == '/')
            escaped << c;
        else
            escaped << '%' << std::uppercase << std::hex << std::setw(2) << int(c);
    }
    return escaped.str();
}

bool list_drive(const std::string& access_token, const std::string& path, std::string& output) {
    CURL* curl = curl_easy_init();
    output.clear();

    if (!curl) return false;

    std::string encoded_path = url_encode(path);
    std::string url = "https://graph.microsoft.com/v1.0/me/drive/root";
    if (!path.empty()) url += ":/" + encoded_path + ":";
    url += "/children";

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, ("Authorization: Bearer " + access_token).c_str());

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &output);

    CURLcode res = curl_easy_perform(curl);
    long response_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
    curl_easy_cleanup(curl);

    return (res == CURLE_OK && response_code == 200);
}

void list_drive_items(const std::string& remote_path) {
    std::string access_token = get_access_token(); 
    std::string response;

    if (!list_drive(access_token, remote_path, response)) {
        std::cout << "⚠️ Token may be expired. Trying refresh...\n";
        access_token = refresh_access_token();
        if (access_token.empty()) {
            std::cerr << "❌ Token refresh failed. Run './onedrivecli auth'\n";
            return;
        }

        if (!list_drive(access_token, remote_path, response)) {
            std::cerr << "❌ Listing failed even after token refresh.\n";
            return;
        }
    }

    try {
        json result = json::parse(response);
        if (!result.contains("value")) {
            std::cerr << "❌ Invalid API response:\n" << result.dump(2) << "\n";
            return;
        }

        std::cout << "\n📁 Contents of " << (remote_path.empty() ? "root" : remote_path) << ":\n";
        for (const auto& item : result["value"]) {
            std::string name = item["name"];
            bool is_folder = item.contains("folder") && !item["folder"].is_null();
            std::cout << (is_folder ? "📁 " : "📄 ") << name << "\n";
        }
    } catch (...) {
        std::cerr << "❌ Failed to parse API response:\n" << response << "\n";
    }
}