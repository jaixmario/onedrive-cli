#include "auth.h"
#include "utils.h"
#include <iostream>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <vector>
#include <csignal>

using json = nlohmann::json;

static bool running = true;

void signal_handler(int signal) {
    if (signal == SIGINT) {
        std::cout << "\n\U0001F44B Exiting Explorer...\n";
        running = false;
    }
}

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
    output->append((char*)contents, size * nmemb);
    return size * nmemb;
}

bool fetch_drive_items(const std::string& path, std::string& output, std::string& token) {
    std::string url = path.empty()
        ? "https://graph.microsoft.com/v1.0/me/drive/root/children"
        : "https://graph.microsoft.com/v1.0/me/drive/root:/" + url_encode(path) + ":/children";

    CURL* curl = curl_easy_init();
    if (!curl) return false;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, ("Authorization: Bearer " + token).c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &output);

    CURLcode res = curl_easy_perform(curl);
    long response_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK || response_code != 200) {
        if (response_code == 401 || response_code == 404) {
            std::cout << "\u26A0\uFE0F Token may be expired or invalid (HTTP " << response_code << "). Trying refresh...\n";
            token = refresh_access_token();
            if (token.empty()) return false;
            output.clear();
            return fetch_drive_items(path, output, token);
        }
        std::cerr << "\u274C Unexpected response code after refresh: " << response_code << "\nRaw response:\n" << output << "\n";
        return false;
    }

    return true;
}

void run_explorer() {
    std::signal(SIGINT, signal_handler);
    std::string access_token = get_access_token();
    std::string current_path = "";

    while (running) {
        std::cout << "\033[2J\033[H";  // Clear screen
        std::string response;
        if (!fetch_drive_items(current_path, response, access_token)) {
            std::cerr << "\u274C Failed to fetch folder.\n";
            break;
        }

        try {
            json data = json::parse(response);
            const auto& items = data["value"];

            std::vector<std::string> names;
            std::vector<bool> is_folder;

            std::cout << "\U0001F4C2 Explorer - Path: /" << (current_path.empty() ? "" : current_path) << "\n\n";

            int index = 1;
            if (!current_path.empty()) {
                std::cout << "  [0] .. (Back)\n";
            }

            for (const auto& item : items) {
                std::string name = item["name"];
                bool folder = item.contains("folder") && !item["folder"].is_null();
                std::cout << "  [" << index << "] " << (folder ? "\U0001F4C1 " : "\U0001F4C4 ") << name << "\n";
                names.push_back(name);
                is_folder.push_back(folder);
                ++index;
            }

            std::cout << "\nEnter number to open folder (Ctrl+C to exit): ";
            std::string input;
            std::getline(std::cin, input);
            if (input.empty()) continue;

            int selected = std::stoi(input);

            if (!current_path.empty() && selected == 0) {
                size_t pos = current_path.find_last_of('/');
                if (pos != std::string::npos) {
                    current_path = current_path.substr(0, pos);
                } else {
                    current_path.clear();
                }
                continue;
            }

            if (selected >= 1 && selected <= (int)names.size()) {
                if (is_folder[selected - 1]) {
                    current_path += (current_path.empty() ? "" : "/") + names[selected - 1];
                } else {
                    std::cout << "\u26A0\uFE0F Not a folder. Press Enter to continue...";
                    std::cin.get();
                }
            }

        } catch (...) {
            std::cerr << "\u274C Failed to parse API response:\n" << response << "\n";
        }
    }
}
