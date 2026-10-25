#include "getPlaces.h"

#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>
#include <vector>

using json = nlohmann::json;

namespace {
    constexpr double kEarthRadiusMeters = 6371000.0;
    constexpr double kPi = 3.14159265358979323846;

    double ToRadians(double degrees) {
        return degrees * kPi / 180.0;
    }

    double CalculateDistanceMeters(double lat1, double lon1, double lat2, double lon2) {
        const double dLat = ToRadians(lat2 - lat1);
        const double dLon = ToRadians(lon2 - lon1);

        const double lat1Rad = ToRadians(lat1);
        const double lat2Rad = ToRadians(lat2);

        const double a = std::sin(dLat / 2.0) * std::sin(dLat / 2.0) + std::cos(lat1Rad) * std::cos(lat2Rad) * std::sin(dLon / 2.0) * std::sin(dLon / 2.0);

        return kEarthRadiusMeters * 2.0 * std::atan2(std::sqrt(a), std::sqrt(1.0 - a));
    }

    bool IsBadWeather(const Weather& weather) {
        if (!weather.condition) {
            return false;
        }

        const std::string& condition = *weather.condition;

        return condition.find("rain") != std::string::npos ||
               condition.find("showers") != std::string::npos ||
               condition.find("thunderstorm") != std::string::npos ||
               condition.find("snow") != std::string::npos ||
               condition.find("hail") != std::string::npos ||
               condition.find("wet-snow") != std::string::npos;
    }

    std::string GetTag(const json& tags, const std::string& key) {
        if (tags.contains(key) && tags.at(key).is_string()) {
            return tags.at(key).get<std::string>();
        }

        return "";
    }

    bool NeedSkipByWeather(const json& tags, const Weather& weather) {
        if (!IsBadWeather(weather)) {
            return false;
        }

        const std::string tourism = GetTag(tags, "tourism");
        const std::string leisure = GetTag(tags, "leisure");

        return leisure == "park" || leisure == "garden" || tourism == "attraction";
    }

    std::string GetPlaceName(const json& tags) {
        const std::string nameRu = GetTag(tags, "name:ru");
        if (!nameRu.empty()) {
            return nameRu;
        }

        const std::string name = GetTag(tags, "name");
        if (!name.empty()) {
            return name;
        }

        return "Без названия";
    }

    std::string GetPlaceType(const json& tags) {
        const std::string tourism = GetTag(tags, "tourism");
        const std::string amenity = GetTag(tags, "amenity");
        const std::string leisure = GetTag(tags, "leisure");

        if (tourism == "museum") {return "музей";}
        if (tourism == "gallery") {return "галерея";}
        if (tourism == "attraction") {return "достопримечательность";}
        if (amenity == "restaurant") {return "ресторан";}
        if (amenity == "cafe") {return "кафе";}
        if (amenity == "cinema") {return "кинотеатр";}
        if (amenity == "theatre") {return "театр";}
        if (leisure == "park") {return "парк";}
        if (leisure == "garden") {return "сад";}

        return "место";
    }

    std::string BuildOverpassQuery(double latitude, double longitude, int radiusMeters) {
        const std::string radius = std::to_string(radiusMeters);
        const std::string lat = std::to_string(latitude);
        const std::string lon = std::to_string(longitude);

        std::string query;

        query += "[out:json][timeout:25];";
        query += "(";
        query += "nwr[\"tourism\"~\"museum|gallery|attraction\"](around:" + radius + "," + lat + "," + lon + ");";
        query += "nwr[\"amenity\"~\"restaurant|cafe|cinema|theatre\"](around:" + radius + "," + lat + "," + lon + ");";
        query += "nwr[\"leisure\"~\"park|garden\"](around:" + radius + "," + lat + "," + lon + ");";
        query += ");";
        query += "out center;";

        return query;
    }

    bool GetCoordinates(const json& element, double& latitude, double& longitude) {
        if (element.contains("lat") && element.contains("lon")) {
            latitude = element.at("lat").get<double>();
            longitude = element.at("lon").get<double>();
            return true;
        }

        if (element.contains("center")) {
            latitude = element.at("center").at("lat").get<double>();
            longitude = element.at("center").at("lon").get<double>();
            return true;
        }

        return false;
    }
}

std::string SetPlacesUrl() {
    return "https://overpass-api.de/api/interpreter";
}

std::vector<Place> getPlacesByLocation(const Location& location, const Weather& weather, int radiusMeters, int limit) {
    std::vector<Place> places;

    if (!location.latitude || !location.longitude) {
        throw std::runtime_error("location has no coordinates");
    }

    if (radiusMeters <= 0 || limit <= 0) {
        throw std::runtime_error("the radius or limit is incorrect");
    }

    cpr::Response response = cpr::Post(
        cpr::Url{SetPlacesUrl()},
        cpr::Body{BuildOverpassQuery(*location.latitude, *location.longitude, radiusMeters)},
        cpr::Header{{"Content-Type", "text/plain"}}
    );

    if (response.status_code != 200) {
        throw std::runtime_error("map: bad response");
    }

    json data = json::parse(response.text);

    if (!data.contains("elements") || !data.at("elements").is_array()) {
        throw std::runtime_error("bad places response");
    }

    for (const json& element : data.at("elements")) {
        if (!element.contains("tags")) {
            continue;
        }

        const json& tags = element.at("tags");

        if (NeedSkipByWeather(tags, weather)) {
            continue;
        }

        double placeLatitude = 0.0;
        double placeLongitude = 0.0;

        if (!GetCoordinates(element, placeLatitude, placeLongitude)) {
            continue;
        }

        Place place;

        place.name = GetPlaceName(tags);
        place.type = GetPlaceType(tags);
        place.latitude = placeLatitude;
        place.longitude = placeLongitude;
        place.distanceMeters = CalculateDistanceMeters(*location.latitude, *location.longitude, placeLatitude, placeLongitude);
        places.push_back(place);
    }

    std::sort(places.begin(), places.end(), [](const Place& lhs, const Place& rhs) {return *lhs.distanceMeters < *rhs.distanceMeters;});

    if (places.size() > static_cast<std::size_t>(limit)) { places.resize(limit);}

    return places;
}