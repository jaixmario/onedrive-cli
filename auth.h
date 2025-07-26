#pragma once
#include <string>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

extern const std::string CLIENT_ID;
extern const std::string TENANT_ID;

void authenticate();
json load_tokens();
void save_tokens(const json& token_data);
std::string get_access_token();
std::string refresh_access_token();