#include "task_scheduler.h"
#include "getLocation.h"
#include "getPlaces.h"
#include "getWether.h"

#include <cstdlib>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
    std::string GetEnvOrThrow(const char* name) {
        const char* value = std::getenv(name);

        if (value == nullptr || std::string(value).empty()) {
            throw std::runtime_error(std::string("environment variable is not set: ") + name);
        }

        return std::string(value);
    }

    void PrintLocation(const Location& location) {
        std::cout << "Местоположение:\n";

        if (location.ip) {
            std::cout << "IP: " << *location.ip << "\n";
        }

        if (location.country) {
            std::cout << "Страна: " << *location.country << "\n";
        }

        if (location.city) {
            std::cout << "Город: " << *location.city << "\n";
        }

        if (location.latitude && location.longitude) {
            std::cout << "Координаты: " << *location.latitude << ", " << *location.longitude << "\n";
        }

        std::cout << "\n";
    }

    void PrintWeather(const Weather& weather) {
        std::cout << "Погода:\n";

        if (weather.temperature) {
            std::cout << "Температура: " << *weather.temperature << " C\n";
        }

        if (weather.feelsLike) {
            std::cout << "Ощущается как: " << *weather.feelsLike << " C\n";
        }

        if (weather.condition) {
            std::cout << "Состояние: " << *weather.condition << "\n";
        }

        if (weather.windSpeed) {
            std::cout << "Ветер: " << *weather.windSpeed << " м/с\n";
        }

        if (weather.humidity) {
            std::cout << "Влажность: " << *weather.humidity << "%\n";
        }

        std::cout << "\n";
    }

    void PrintPlaces(const std::vector<Place>& places) {
        std::cout << "Рекомендованные места:\n";

        if (places.empty()) {
            std::cout << "Подходящие места рядом не найдены.\n";
            return;
        }

        for (std::size_t i = 0; i < places.size(); ++i) {
            const Place& place = places[i];

            std::cout << i + 1 << ". ";

            if (place.name) {std::cout << *place.name;}
            else {std::cout << "Без названия";}

            if (place.type) {std::cout << " — " << *place.type;}

            if (place.distanceMeters) {
                std::cout << ", расстояние: " << static_cast<int>(*place.distanceMeters) << " м";
            }

            std::cout << "\n";
        }
    }
}

int main(int argc, char** argv) {
    std::string ip;

    if (argc >= 2) { ip = argv[1];}

    try {
        const std::string twoIpToken = GetEnvOrThrow("TWO_IP_TOKEN");
        const std::string weatherToken = GetEnvOrThrow("YANDEX_WEATHER_TOKEN");

        TTaskScheduler scheduler;

        auto locationTask = scheduler.add(getLocationByIp, ip, twoIpToken);

        auto weatherTask = scheduler.add(
            getWeatherByLocation,
            locationTask.getFutureResult<const Location&>(),
            weatherToken
        );

        auto placesTask = scheduler.add(getPlacesByLocation,
            locationTask.getFutureResult<const Location&>(),
            weatherTask.getFutureResult<const Weather&>(),
            3000,
            5
        );

        auto printTask = scheduler.add([](const Location& location, const Weather& weather,std::vector<Place> places) {
                PrintLocation(location);
                PrintWeather(weather);
                PrintPlaces(places);
            },
            locationTask.getFutureResult<const Location&>(),
            weatherTask.getFutureResult<const Weather&>(),
            placesTask.getFutureResult<std::vector<Place>>()
        );

        scheduler.executeAll();

        (void)printTask;
    } 
    
    catch (const std::exception& error) {
        std::cerr << "Ошибка: " << error.what() << '\n';
    }
}