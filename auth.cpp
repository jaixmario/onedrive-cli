#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
#include <curl/curl.h>
#include <nlohmann/json.hpp> 

using json = nlohmann::json;

const std::string CLIENT_ID = "59790544-ca0c-4b77-b338-26ff9d1b676f";
const std::string TENANT_ID = "0fd666e8-0b3d-41ea-a5ef-1c509130bd94";
const std::string DEVICE_CODE_URL = "https://login.microsoftonline.com/" + TENANT_ID + "/oauth2/v2.0/devicecode";
const std::string TOKEN_URL = "https://login.microsoftonline.com/" + TENANT_ID + "/oauth2/v2.0/token";

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
    size_t totalSize = size * nmemb;
    output->append((char*)contents, totalSize);
    return totalSize;
}

json post_request(const std::string& url, const std::string& data) {
    CURL* curl = curl_easy_init();
    std::string response;
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }
    return json::parse(response);
}

void save_tokens(const json& token_data) {
    std::ofstream file("token.json");
    file << token_data.dump(4);
    file.close();
}

void authenticate() {
    std::string payload = "client_id=" + CLIENT_ID + "&scope=offline_access%20Files.ReadWrite.All";
    json device_resp = post_request(DEVICE_CODE_URL, payload);

    std::string user_code = device_resp["user_code"];
    std::string device_code = device_resp["device_code"];
    std::string verification_uri = device_resp["verification_uri"];
    int interval = device_resp["interval"];

    std::cout << "🔐 Open the following URL in your browser:\n" << verification_uri << "\n";
    std::cout << "👉 Enter this code: " << user_code << "\n\n";

    // Polling for token
    std::string poll_payload = "grant_type=urn:ietf:params:oauth:grant-type:device_code"
                               "&client_id=" + CLIENT_ID +
                               "&device_code=" + device_code;

    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(interval));
        json token_resp = post_request(TOKEN_URL, poll_payload);

        if (token_resp.contains("access_token")) {
            std::cout << "✅ Authenticated successfully!\n";
            save_tokens(token_resp);
            break;
        }

        if (token_resp.contains("error")) {
            std::string err = token_resp["error"];
            if (err == "authorization_pending") continue;
            else {
                std::cerr << "❌ Error: " << err << "\n";
                break;
            }
        }
    }
}