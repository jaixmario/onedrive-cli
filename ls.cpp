#include <iostream>
#include <fstream>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
    size_t totalSize = size * nmemb;
    output->append((char*)contents, totalSize);
    return totalSize;
}

std::string load_access_token() {
    std::ifstream file("token.json");
    if (!file.is_open()) {
        std::cerr << "❌ token.json not found. Please run './onedrivecli auth' first.\n";
        exit(1);
    }
    json token_data;
    file >> token_data;
    return token_data["access_token"];
}

void list_drive_items() {
    std::string access_token = load_access_token();

    CURL* curl = curl_easy_init();
    std::string response;

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, "https://graph.microsoft.com/v1.0/me/drive/root/children");

        std::string auth_header = "Authorization: Bearer " + access_token;
        curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, auth_header.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        CURLcode res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        if (res != CURLE_OK) {
            std::cerr << "❌ Request failed: " << curl_easy_strerror(res) << "\n";
            return;
        }
    }

    json data = json::parse(response);
    if (data.contains("value")) {
        std::cout << "\n📂 OneDrive Root:\n";
        for (auto& item : data["value"]) {
            std::string name = item["name"];
            bool is_folder = item.contains("folder");
            std::cout << (is_folder ? "📁 " : "📄 ") << name << "\n";
        }
    } else {
        std::cerr << "⚠️ Unexpected response format:\n" << response << "\n";
    }
}