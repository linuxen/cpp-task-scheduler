#include "getWether.h"

#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

#include <stdexcept>
#include <string>

using json = nlohmann::json;

std::string SetWeatherUrl(double latitude, double longitude) {
    (void)latitude;
    (void)longitude;

    return "https://api.weather.yandex.ru/v2/forecast/";
}

Weather getWeatherByLocation(const Location& location, const std::string& token) {
    if (!location.latitude || !location.longitude) {
        throw std::runtime_error("location has no coordinates");
    }

    cpr::Response response = cpr::Get(
        cpr::Url{SetWeatherUrl(*location.latitude, *location.longitude)},
        cpr::Parameters{
            {"lat", std::to_string(*location.latitude)},
            {"lon", std::to_string(*location.longitude)},
            {"lang", "ru_RU"},
            {"limit", "1"},
            {"hours", "false"},
            {"extra", "false"}
        },
        cpr::Header{
            {"X-Yandex-Weather-Key", token}
        }
    );

    if (response.error.code != cpr::ErrorCode::OK) {
        throw std::runtime_error("Yandex weather request failed: " + response.error.message);
    }

    if (response.status_code != 200) {
        throw std::runtime_error(
            "Yandex weather request failed. Status: " +
            std::to_string(response.status_code) +
            ". Body: " +
            response.text
        );
    }

    json data = json::parse(response.text);

    if (!data.contains("fact") || !data.at("fact").is_object()) {
        throw std::runtime_error("bad weather response: " + response.text);
    }

    const json& fact = data.at("fact");

    Weather weather;

    if (fact.contains("temp") && fact.at("temp").is_number()) {
        weather.temperature = fact.at("temp").get<double>();
    }

    if (fact.contains("feels_like") && fact.at("feels_like").is_number()) {
        weather.feelsLike = fact.at("feels_like").get<double>();
    }

    if (fact.contains("condition") && fact.at("condition").is_string()) {
        weather.condition = fact.at("condition").get<std::string>();
    }

    if (fact.contains("wind_speed") && fact.at("wind_speed").is_number()) {
        weather.windSpeed = fact.at("wind_speed").get<double>();
    }

    if (fact.contains("humidity") && fact.at("humidity").is_number()) {
        weather.humidity = fact.at("humidity").get<int>();
    }

    return weather;
}