#pragma once

#include "getLocation.h"
#include "getWether.h"

#include <optional>
#include <string>
#include <vector>

struct Place {
    std::optional<std::string> name;
    std::optional<std::string> type;
    std::optional<double> latitude;
    std::optional<double> longitude;
    std::optional<double> distanceMeters;
};

std::string SetPlacesUrl();

std::vector<Place> getPlacesByLocation(const Location& location, const Weather& weather, int radiusMeters = 1500, int limit = 5);