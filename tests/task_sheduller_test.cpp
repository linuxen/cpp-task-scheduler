#include "lib/task_scheduler.h"

#include <gtest/gtest.h>
#include <vector>

TEST(TaskScheduler, TaskWithoutArgs) {
    TTaskScheduler scheduler;

    auto task = scheduler.add([] {
        return 42;
    });

    EXPECT_EQ(task.getResultSync<int>(), 42);
}

TEST(TaskScheduler, TaskWithOneArg) {
    TTaskScheduler scheduler;

    auto task = scheduler.add([](int x) {
        return x * 2;
    }, 21);

    EXPECT_EQ(task.getResultSync<int>(), 42);
}

TEST(TaskScheduler, ResultIsCached) {
    TTaskScheduler scheduler;
    int calls = 0;

    auto task = scheduler.add([&calls] {
        ++calls;
        return 10;
    });

    EXPECT_EQ(task.getResultSync<int>(), 10);
    EXPECT_EQ(task.getResultSync<int>(), 10);
    EXPECT_EQ(calls, 1);
}

TEST(TaskScheduler, FutureDependency) {
    TTaskScheduler scheduler;

    auto a = scheduler.add([] {
        return 10;
    });

    auto b = scheduler.add([](int x) {
        return x * 2;
    }, a.getFutureResult<int>());

    EXPECT_EQ(b.getResultSync<int>(), 20);
}

TEST(TaskScheduler, ConstRefFutureCanBeReadManyTimes) {
    TTaskScheduler scheduler;

    auto task = scheduler.add([] {
        return std::vector<int>{1, 2, 3};
    });

    auto future = task.getFutureResult<const std::vector<int>&>();

    EXPECT_EQ(future.get().size(), 3);
    EXPECT_EQ(future.get()[1], 2);
}

TEST(TaskScheduler, MoveFutureCanBeReadOnce) {
    TTaskScheduler scheduler;

    auto task = scheduler.add([] {
        return std::vector<int>{1, 2, 3};
    });

    auto future = task.getFutureResult<std::vector<int>>();

    auto value = future.get();

    EXPECT_EQ(value.size(), 3);
    EXPECT_ANY_THROW(future.get());
}

TEST(TaskScheduler, ApplyVoidTask) {
    TTaskScheduler scheduler;
    int result = 0;

    auto task = scheduler.add([] {
        return 21;
    });

    task.apply<int>([&result](int x) {
        result = x * 2;
    });

    scheduler.executeAll();

    EXPECT_EQ(result, 42);
}