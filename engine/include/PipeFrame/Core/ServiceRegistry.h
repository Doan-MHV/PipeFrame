#pragma once
#include <string>
#include <string_view>
#include <typeindex>
#include <unordered_map>
namespace pipeframe {
class ServiceRegistry final {
public:
    template<class Service> void Provide(Service &service, std::string id = {}) {
        services.insert_or_assign(typeid(Service), &service);
        if (!id.empty()) named.insert_or_assign(std::move(id), &service);
    }
    template<class Service> Service *Find() const {
        const auto found = services.find(typeid(Service));
        return found == services.end() ? nullptr : static_cast<Service *>(found->second);
    }
    bool Contains(std::string_view id) const { return named.contains(std::string(id)); }
    void Clear() { services.clear(); named.clear(); }
private:
    std::unordered_map<std::type_index, void *> services;
    std::unordered_map<std::string, void *> named;
};

}
