#pragma once
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>

class ThreadPool {
public:
    explicit ThreadPool(size_t threadCount = std::thread::hardware_concurrency()) {
        for (size_t i = 0; i < threadCount; i++) {
            workers.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock lock(queueMutex);
                        cv.wait(lock, [this] { return !tasks.empty() || stopping; });
                        if (stopping && tasks.empty()) return;
                        task = std::move(tasks.front());
                        tasks.pop();
                    }
                    task();
                }
            });
        }
    }

    ~ThreadPool() {
        { std::unique_lock lock(queueMutex); stopping = true; }
        cv.notify_all();
        for (auto& w : workers) w.join();
    }

    template<typename F>
    std::future<std::invoke_result_t<F>> submit(F&& f) {
        using R = std::invoke_result_t<F>;
        auto task = std::make_shared<std::packaged_task<R()>>(std::forward<F>(f));
        std::future<R> fut = task->get_future();
        { std::unique_lock lock(queueMutex); tasks.emplace([task] { (*task)(); }); }
        cv.notify_one();
        return fut;
    }

    size_t size() const { return workers.size(); }

private:
    std::vector<std::thread>          workers;
    std::queue<std::function<void()>> tasks;
    std::mutex                        queueMutex;
    std::condition_variable           cv;
    bool                              stopping = false;
};

// global instance; include this header wherever threading is needed
inline ThreadPool gThreadPool;