#ifndef PIPEFRAME_CORE_ASYNC_TASK_H
#define PIPEFRAME_CORE_ASYNC_TASK_H
#include <future>
#include <type_traits>
#include <utility>
namespace pipeframe {
template <typename T> using AsyncTask=std::future<T>;
template <typename Function>
auto RunAsync(Function &&function) -> AsyncTask<std::invoke_result_t<Function>> {
    return std::async(std::launch::async,std::forward<Function>(function));
}
} // namespace pipeframe
#endif
