#pragma once
#include <coroutine>
#include <exception>
#include <cmath>
#include <stdexcept>
#include <utility>
namespace pipeframe {
struct WaitForSeconds { float seconds{}; };
// Move-only simulation task. Yield WaitForSeconds to suspend until a later fixed tick.
class Coroutine {
public:
    struct promise_type {
        float wait{};
        std::exception_ptr failure;
        Coroutine get_return_object(){return Coroutine{std::coroutine_handle<promise_type>::from_promise(*this)};}
        std::suspend_always initial_suspend() noexcept{return {};}
        std::suspend_always final_suspend() noexcept{return {};}
        std::suspend_always yield_value(WaitForSeconds value){
            if(!std::isfinite(value.seconds)||value.seconds<0)throw std::invalid_argument("Coroutine wait must be finite and nonnegative");
            wait=value.seconds;return {};
        }
        void return_void() noexcept{}
        void unhandled_exception() noexcept{failure=std::current_exception();}
    };
    Coroutine()=default;
    Coroutine(Coroutine &&other) noexcept:handle(std::exchange(other.handle,{})){}
    Coroutine &operator=(Coroutine &&other) noexcept{
        if(this!=&other){if(handle)handle.destroy();handle=std::exchange(other.handle,{});}return *this;
    }
    Coroutine(const Coroutine &)=delete;
    Coroutine &operator=(const Coroutine &)=delete;
    ~Coroutine(){if(handle)handle.destroy();}
    bool Tick(float delta){
        if(!handle||handle.done())return false;
        auto &promise=handle.promise();
        if(promise.wait>0){promise.wait-=delta;if(promise.wait>0)return true;}
        handle.resume();
        if(promise.failure)std::rethrow_exception(promise.failure);
        return !handle.done();
    }
private:
    explicit Coroutine(std::coroutine_handle<promise_type> value):handle(value){}
    std::coroutine_handle<promise_type> handle{};
};
}
