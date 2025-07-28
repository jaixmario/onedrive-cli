#include "auth.h"
#include "utils.h"
#include <iostream>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <iomanip>

using json = nlohmann::json;

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
    output->append((char*)contents, size * nmemb);
    return size * nmemb;
}

bool get_storage_data(const std::string& access_token, std::string& output) {
    CURL* curl = curl_easy_init();
    if (!curl) return false;

    std::string url = "https://graph.microsoft.com/v1.0/me/drive";
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, ("Authorization: Bearer " + access_token).c_str());

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &output);

    CURLcode res = curl_easy_perform(curl);
    long code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    curl_easy_cleanup(curl);
    return (res == CURLE_OK && code == 200);
}

void show_storage_info() {
    std::string access_token = get_access_token();
    std::string response;

    if (!get_storage_data(access_token, response)) {
        std::cout << "⚠️ Token may be expired. Trying refresh...\n";
        access_token = refresh_access_token();
        if (access_token.empty()) {
            std::cerr << "❌ Could not refresh token. Run ./onedrivecli auth\n";
            return;
        }

        response.clear();  

        if (!get_storage_data(access_token, response)) {
            std::cerr << "❌ Failed to fetch storage info.\n";
            return;
        }
    }

    try {
        json result = json::parse(response);

        double used = result["quota"]["used"];
        double total = result["quota"]["total"];
        double trash = result["quota"]["deleted"];
        double remaining = result["quota"]["remaining"];
        double used_gb = used / (1024.0 * 1024 * 1024);
        double total_gb = total / (1024.0 * 1024 * 1024);
        double free_gb = remaining / (1024.0 * 1024 * 1024);
        double trash_gb = trash / (1024.0 * 1024 * 1024);

        int percent = static_cast<int>((used / total) * 100);
        int bar_length = 30;
        int filled = percent * bar_length / 100;

        std::cout << "📦 Storage Usage:\n";
        std::cout << "Used:  " << std::fixed << std::setprecision(2) << used_gb << " GB of " << total_gb << " GB\n";
        std::cout << "Free:  " << std::fixed << std::setprecision(2) << free_gb << " GB of " << total_gb << " GB\n";
        std::cout << "Trash: " << std::fixed << std::setprecision(2) << trash_gb << " GB of " << used_gb << " GB\n";

        std::cout << "[";
        for (int i = 0; i < bar_length; ++i)
            std::cout << (i < filled ? "#" : "-");
        std::cout << "] " << percent << "%\n";

    } catch (...) {
        std::cerr << "❌ Failed to parse response:\n" << response << "\n";
    }
}