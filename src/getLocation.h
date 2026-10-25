#pragma once

#include <string>
#include <optional>

struct Location {
    std::optional<std::string> ip;
    std::optional<std::string> country;
    std::optional<std::string> city;
    std::optional<double> latitude;
    std::optional<double> longitude;
};

std::string SetUrl(const std::string& ip, const std::string& token);

Location getLocationByIp(const std::string& ip, const std::string& token);