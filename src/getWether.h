#pragma once

#include "getLocation.h"

#include <optional>
#include <string>

struct Weather {
    std::optional<int> temperature;
    std::optional<int> feelsLike;
    std::optional<std::string> condition;
    std::optional<double> windSpeed;
    std::optional<int> humidity;
};

std::string SetWeatherUrl(double latitude, double longitude);

Weather getWeatherByLocation(const Location& location, const std::string& token);