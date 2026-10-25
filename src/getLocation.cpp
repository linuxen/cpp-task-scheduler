#include "getLocation.h"

#include <cpr/cpr.h>
#include <nlohmann/json.hpp>
#include <stdexcept>

using json = nlohmann::json;

std::string SetUrl(const std::string& ip, const std::string& token) {
    std::string url = "https://api.2ip.io/";

    url += ip;
    url += "?token=";
    url += token;
    return url;
}

Location getLocationByIp(const std::string& ip, const std::string& token) {
    Location loc;
    std::string url = SetUrl(ip, token);

    cpr::Response r = cpr::Get(cpr::Url{url});

    if (r.status_code != 200) {
        throw std::runtime_error("2ip request failed");
    }

    json data = json::parse(r.text);

    if (data.contains("ip") && data.at("ip").is_string()) {
        loc.ip = data.at("ip").get<std::string>();
    }

    if (data.contains("country") && data.at("country").is_string()) {
        loc.country = data.at("country").get<std::string>();
    }

    if (data.contains("city") && data.at("city").is_string()) {
        loc.city = data.at("city").get<std::string>();
    }

    if (data.contains("lat")) {
        if (data.at("lat").is_number()) {
            loc.latitude = data.at("lat").get<double>();
        } else if (data.at("lat").is_string()) {
            loc.latitude = std::stod(data.at("lat").get<std::string>());
        }
    }

    if (data.contains("lon")) {
        if (data.at("lon").is_number()) {
            loc.longitude = data.at("lon").get<double>();
        } else if (data.at("lon").is_string()) {
            loc.longitude = std::stod(data.at("lon").get<std::string>());
        }
    }

    return loc;
}