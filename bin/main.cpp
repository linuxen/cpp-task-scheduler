#include "lib/task_scheduler.h"

#include <iostream>
#include <unordered_map>
#include <fstream>
#include <sstream>

int main() {
    TTaskScheduler scheduler;

    auto students = scheduler.add(
        [](const std::string& filePath) {
            std::ifstream file(filePath);
            std::string line;
            std::getline(file, line); // skip header

            std::unordered_map<int, int> gpaByIsu;
            while (std::getline(file, line)) {
                int isu, gpa;
                std::sscanf(line.c_str(), "%d,%d", &isu, &gpa);
                gpaByIsu[isu] = gpa;
            }
            return gpaByIsu;
        },
        "students.csv"
    );

    auto groups = scheduler.add(
        [](const std::string& filePath) {
            std::ifstream file(filePath);
            std::string line;
            std::getline(file, line); // skip header

            std::unordered_map<std::string, std::vector<int>> groupToIsus;
            while (std::getline(file, line)) {
                std::stringstream ss(line);
                std::string group;
                int isu;
                std::getline(ss, group, ',');
                ss >> isu;
                groupToIsus[group].push_back(isu);
            }
            return groupToIsus;
        },
        "groups.csv"
    );

    // Берём ссылку: таких future result можно получить много раз
    auto studentsFuture =
        students.getFutureResult<const std::unordered_map<int, int>&>();
    // Берём значение, ожидаем только move, никаких copy, соответственно get можно вызвать единожды
    auto groupsFuture =
        groups.getFutureResult<std::unordered_map<std::string, std::vector<int>>>();

    auto groupGpa = scheduler.add(
        [](
            const std::unordered_map<int, int>& gpaByIsu,
            std::unordered_map<std::string, std::vector<int>> groupToIsus // ждём, что нам мувнут
        ) {
            std::unordered_map<std::string, int> gpaByGroup;
            for (const auto& [group, isus] : groupToIsus) {
                int sum = 0, cnt = 0;
                for (const int isu : isus) {
                    auto it = gpaByIsu.find(isu);
                    if (it != gpaByIsu.end()) {
                        sum += it->second;
                        ++cnt;
                    }
                }
                if (cnt != 0) {
                    gpaByGroup[group] = sum / cnt;
                }
            }
            return gpaByGroup;
        },
        studentsFuture,
        groupsFuture
    );

    groupGpa.apply<std::unordered_map<std::string, int>>(
        [](std::unordered_map<std::string, int>&& gpaByGroup) {
            for (const auto& [group, gpa] : gpaByGroup) {
                std::cout << group << ": " << gpa << "\n";
            }
        }
    );


    scheduler.executeAll();

    // Так делать нельзя, потому что уже мувнули, ожидаем exception
    // groupsFuture.get()

    // Так можно
    const std::unordered_map<int, int>& gpaByIsu = studentsFuture.get();
}
