#pragma once

#include <vector>
#include <memory>
#include <utility>
#include <tuple>
#include <type_traits>
#include <optional>
#include <stdexcept>
#include <functional>
#include <variant>

class MyTask {
public:
    virtual ~MyTask() = default;
    virtual void execute() = 0;
    virtual void* GetPtr() = 0;
};

class TTaskScheduler;

template <typename T>
class TFuture {
private:
    std::shared_ptr<MyTask> task_;
    bool was_moved_ = false;

public:
    explicit TFuture(std::shared_ptr<MyTask> task) : task_(task) {}

    T get() {
        if (!task_) {
            throw std::runtime_error("Empty task in future");
        }
        task_->execute();

    
        using RawT = std::remove_reference_t<T>;

        auto* ptr = static_cast<RawT*>(task_->GetPtr());
        if (!ptr) {
            throw std::runtime_error("Task result pointer is null");
        }

        if constexpr (std::is_reference_v<T>) {
            return *ptr;
        } else {
            return std::move(*ptr); 
        }
    }
};
                
template <typename T>
decltype(auto) ResolveArg(T& value) {
    return value;
}

template <typename T>
decltype(auto) ResolveArg(TFuture<T>& future) {
    return future.get();
}

template <typename Func, typename... Args>
class TTMyTask : public MyTask {
private:
    Func func_;
    std::tuple<Args...> args_;
    bool executed_ = false;

    using ResultType = std::invoke_result_t<Func, decltype(ResolveArg(std::declval<Args&>()))...>;

    using StoredType = std::conditional_t<std::is_void_v<ResultType>, std::monostate, ResultType>;

    std::optional<StoredType> result_;

    template <std::size_t... I>
    void executeImpl(std::index_sequence<I...>) {
        if constexpr (std::is_void_v<ResultType>) {
            std::invoke(func_, ResolveArg(std::get<I>(args_))...);
            result_ = std::monostate{};
        } else {
            result_ = std::invoke(func_, ResolveArg(std::get<I>(args_))...);
        }
    }

public:
    TTMyTask(Func func, Args... args) : 
        func_(std::move(func)),
        args_(std::move(args)...) {}

    void execute() override {
        if (!executed_) {
            executeImpl(std::index_sequence_for<Args...>{});
            executed_ = true;
        }
    }

    void* GetPtr() override {
        execute();
        if constexpr (std::is_void_v<ResultType>) {return nullptr;}
        else {return &(*result_);}
    }
};

class TTask {
private:
    std::shared_ptr<MyTask> task_;
    TTaskScheduler* sheduler_;

public:
    explicit TTask(std::shared_ptr<MyTask> my_t, TTaskScheduler* sheduler) : 
            task_(my_t),
            sheduler_(sheduler) {} 

    template <typename T>
    T getResultSync() {
        using RawT = std::remove_reference_t<T>;
        return *static_cast<RawT*>(task_->GetPtr());
    }

    template <typename T>
    TFuture<T> getFutureResult() {
        return TFuture<T>(task_);
    }

    template <typename PrevResult, typename Func>
    TTask apply(Func&& func);
};

class TTaskScheduler {
private:
    std::vector<std::shared_ptr<MyTask>> tasks_;

public:

    template <typename Func, typename... Args>
    TTask add(Func&& func, Args&& ...args) {
        auto task = std::make_shared <TTMyTask<std::decay_t<Func>, std::decay_t<Args>...>>
            (std::forward<Func>(func),
            std::forward<Args>(args)...);
        
            tasks_.push_back(task);
            return TTask(task, this);
    }       

    void executeAll() {
        for (const auto& i : tasks_) {
            i->execute();
        }
    }
};

template <typename PrevResult, typename Func>
TTask TTask::apply(Func&& func) {
    auto future = getFutureResult<PrevResult>();
    return sheduler_->add(std::forward<Func>(func), std::move(future));
}