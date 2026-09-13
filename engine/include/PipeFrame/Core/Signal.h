#ifndef PIPEFRAME_CORE_SIGNAL_H
#define PIPEFRAME_CORE_SIGNAL_H
#include <cstdint>
#include <functional>
#include <unordered_map>
#include <utility>
namespace pipeframe {
template <typename... Args>
class Signal {
public:
    using Token=std::uint64_t;
    Token Connect(std::function<void(Args...)> callback) { const Token token=nextToken++; callbacks.emplace(token,std::move(callback)); return token; }
    bool Disconnect(Token token) { return callbacks.erase(token)>0; }
    void Emit(Args... args) const { const auto copy=callbacks; for (const auto &[token,callback]:copy) if (callbacks.contains(token)) callback(args...); }
    [[nodiscard]] std::size_t ListenerCount() const { return callbacks.size(); }
private:
    Token nextToken{1}; std::unordered_map<Token,std::function<void(Args...)>> callbacks;
};
} // namespace pipeframe
#endif
