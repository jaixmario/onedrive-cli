#include "dl.h"
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
    escaped.fill('0');
    escaped << std::hex;
    for (char c : value) {
        if (isalnum((unsigned char)c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
        } else {
            escaped << '%' << std::uppercase << std::setw(2) << int((unsigned char)c);
            escaped << std::nouppercase;
        }
    }
    return escaped.str();
}

bool fetch_download_url(const std::string& access_token, const std::string& path, std::string& output, std::string& error) {
    CURL* curl = curl_easy_init();
    output.clear();
    error.clear();
    if (!curl) return false;

    std::string encoded_path = url_encode(path);
    std::string url = "https://graph.microsoft.com/v1.0/me/drive/root:/" + encoded_path;

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

    if (res != CURLE_OK) {
        error = curl_easy_strerror(res);
        return false;
    }
    if (response_code == 404) {
        error = "File not found: " + path;
        return false;
    }

    return (response_code == 200);
}

void generate_download_url(const std::string& remote_path) {
    std::string access_token = get_access_token();
    std::string response, error;

    bool ok = fetch_download_url(access_token, remote_path, response, error);
    if (!ok) {
        std::cout << "⚠️ Token might be expired. Trying refresh...\n";
        access_token = refresh_access_token();
        if (access_token.empty()) {
            std::cerr << "❌ Failed to refresh token. Run './onedrivecli auth'.\n";
            return;
        }

        if (!fetch_download_url(access_token, remote_path, response, error)) {
            std::cerr << "❌ Failed to get download URL: " << error << "\n";
            return;
        }
    }

    try {
        json j = json::parse(response);
        if (j.contains("@microsoft.graph.downloadUrl")) {
            std::string link = j["@microsoft.graph.downloadUrl"];
            std::cout << "📥 Download URL:\n" << link << "\n";
        } else {
            std::cerr << "❌ This might be a folder or restricted file. No downloadUrl available.\n";
        }
    } catch (...) {
        std::cerr << "❌ Failed to parse API response:\n" << response << "\n";
    }
}
