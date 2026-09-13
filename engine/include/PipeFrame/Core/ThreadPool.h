#ifndef PIPEFRAME_CORE_THREAD_POOL_H
#define PIPEFRAME_CORE_THREAD_POOL_H
#include <algorithm>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>
namespace pipeframe {
class ThreadPool {
public:
    explicit ThreadPool(std::size_t count=std::thread::hardware_concurrency()) {
        count=std::max<std::size_t>(1,count);
        for(std::size_t i=0;i<count;++i) workers.emplace_back([this]{ Run(); });
    }
    ~ThreadPool() { { std::lock_guard lock(mutex); stopping=true; } ready.notify_all(); for(auto &worker:workers) worker.join(); }
    ThreadPool(const ThreadPool&)=delete; ThreadPool& operator=(const ThreadPool&)=delete;
    template <typename Function> auto Submit(Function &&function) {
        using Result=std::invoke_result_t<Function>;
        auto task=std::make_shared<std::packaged_task<Result()>>(std::forward<Function>(function));
        auto future=task->get_future(); { std::lock_guard lock(mutex); tasks.emplace([task]{ (*task)(); }); } ready.notify_one(); return future;
    }
private:
    void Run() { for(;;) { std::function<void()> task; { std::unique_lock lock(mutex); ready.wait(lock,[this]{return stopping||!tasks.empty();}); if(stopping&&tasks.empty()) return; task=std::move(tasks.front()); tasks.pop(); } task(); } }
    std::vector<std::thread> workers; std::queue<std::function<void()>> tasks; std::mutex mutex; std::condition_variable ready; bool stopping{};
};

} // namespace pipeframe
#endif
