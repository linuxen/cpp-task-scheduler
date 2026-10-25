#include "task_scheduler.h"
#include "getLocation.h"
#include "getWether.h"
#include "getPlaces.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <string>
#include <vector>

using ::testing::_;
using ::testing::InSequence;
using ::testing::Return;
using ::testing::StrictMock;

class RecommendationApiMock {
public:
    MOCK_METHOD(Location, GetLocation, (const std::string& ip, const std::string& token));
    MOCK_METHOD(Weather, GetWeather, (const Location& location, const std::string& token));
    MOCK_METHOD(std::vector<Place>, GetPlaces, (
        const Location& location,
        const Weather& weather,
        int radiusMeters,
        int limit
    ));
};

TEST(RecommendationAppTest, BuildsRecommendationPipelineWithTaskScheduler) {
    StrictMock<RecommendationApiMock> api;

    Location location;
    location.ip = "8.8.8.8";
    location.country = "US";
    location.city = "Mountain View";
    location.latitude = 37.386;
    location.longitude = -122.0838;

    Weather weather;
    weather.temperature = 20.0;
    weather.feelsLike = 19.0;
    weather.condition = "clear";
    weather.windSpeed = 2.5;
    weather.humidity = 50;

    Place place;
    place.name = "Test Cafe";
    place.type = "кафе";
    place.latitude = 37.387;
    place.longitude = -122.084;
    place.distanceMeters = 120.0;

    std::vector<Place> places;
    places.push_back(place);

    {
        InSequence sequence;

        EXPECT_CALL(api, GetLocation("8.8.8.8", "2ip_token"))
            .Times(1)
            .WillOnce(Return(location));

        EXPECT_CALL(api, GetWeather(_, "weather_token"))
            .Times(1)
            .WillOnce(Return(weather));

        EXPECT_CALL(api, GetPlaces(_, _, 3000, 5))
            .Times(1)
            .WillOnce(Return(places));
    }

    TTaskScheduler scheduler;

    auto locationTask = scheduler.add(
        [&api](const std::string& ip, const std::string& token) {
            return api.GetLocation(ip, token);
        },
        std::string("8.8.8.8"),
        std::string("2ip_token")
    );

    auto weatherTask = scheduler.add(
        [&api](const Location& location, const std::string& token) {
            return api.GetWeather(location, token);
        },
        locationTask.getFutureResult<const Location&>(),
        std::string("weather_token")
    );

    auto placesTask = scheduler.add(
        [&api](const Location& location, const Weather& weather, int radius, int limit) {
            return api.GetPlaces(location, weather, radius, limit);
        },
        locationTask.getFutureResult<const Location&>(),
        weatherTask.getFutureResult<const Weather&>(),
        3000,
        5
    );

    auto resultTask = scheduler.add(
        [](std::vector<Place> result) {
            return result.size();
        },
        placesTask.getFutureResult<std::vector<Place>>()
    );

    scheduler.executeAll();

    EXPECT_EQ(resultTask.getResultSync<std::size_t>(), 1);
}

TEST(RecommendationAppTest, DoesNotUseRealNetworkInUnitTest) {
    StrictMock<RecommendationApiMock> api;

    Location location;
    location.ip = "1.1.1.1";
    location.country = "AU";
    location.city = "Sydney";
    location.latitude = -33.8688;
    location.longitude = 151.2093;

    Weather weather;
    weather.condition = "rain";

    std::vector<Place> places;

    EXPECT_CALL(api, GetLocation(_, _))
        .Times(1)
        .WillOnce(Return(location));

    EXPECT_CALL(api, GetWeather(_, _))
        .Times(1)
        .WillOnce(Return(weather));

    EXPECT_CALL(api, GetPlaces(_, _, 3000, 5))
        .Times(1)
        .WillOnce(Return(places));

    TTaskScheduler scheduler;

    auto locationTask = scheduler.add(
        [&api]() {
            return api.GetLocation("1.1.1.1", "2ip_token");
        }
    );

    auto weatherTask = scheduler.add(
        [&api](const Location& location) {
            return api.GetWeather(location, "weather_token");
        },
        locationTask.getFutureResult<const Location&>()
    );

    auto placesTask = scheduler.add(
        [&api](const Location& location, const Weather& weather) {
            return api.GetPlaces(location, weather, 3000, 5);
        },
        locationTask.getFutureResult<const Location&>(),
        weatherTask.getFutureResult<const Weather&>()
    );

    scheduler.executeAll();

    EXPECT_TRUE(placesTask.getResultSync<std::vector<Place>>().empty());
}