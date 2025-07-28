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

void generate_download_url(const std::string& path) {
    std::string access_token = get_access_token();
    std::string output;

    std::string url = "https://graph.microsoft.com/v1.0/me/drive/root:/" + url_encode(path);

    CURL* curl = curl_easy_init();
    if (!curl) return;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, ("Authorization: Bearer " + access_token).c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &output);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    try {
        json result = json::parse(output);
        if (result.contains("@microsoft.graph.downloadUrl")) {
            std::cout << "⬇️ Direct Download: " << result["@microsoft.graph.downloadUrl"] << "\n";
        } else {
            std::cerr << "❌ Download URL not found:\n" << output << "\n";
        }
    } catch (...) {
        std::cerr << "❌ Failed to parse download URL:\n" << output << "\n";
    }
}