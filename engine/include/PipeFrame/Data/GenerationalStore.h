#ifndef PIPEFRAME_DATA_GENERATIONAL_STORE_H
#define PIPEFRAME_DATA_GENERATIONAL_STORE_H

#include <cstdint>
#include <limits>
#include <utility>
#include <vector>

namespace pipeframe {
template <typename Tag = void>
struct Handle {
    std::uint32_t index{std::numeric_limits<std::uint32_t>::max()};
    std::uint32_t generation{};
    [[nodiscard]] constexpr bool IsValid() const { return index != std::numeric_limits<std::uint32_t>::max(); }
    constexpr bool operator==(const Handle &) const = default;
};

// Stable handles address a sparse slot table; live values stay tightly packed.
template <typename T, typename Tag = T>
class GenerationalStore {
public:
    using HandleType = Handle<Tag>;

    template <typename... Args>
    HandleType Emplace(Args &&...args) {
        std::uint32_t slotIndex;
        if (freeHead == InvalidIndex) {
            slotIndex=static_cast<std::uint32_t>(slots.size());
            slots.emplace_back();
        } else {
            slotIndex=freeHead;
            freeHead=slots[slotIndex].nextFree;
        }
        auto &slot=slots[slotIndex];
        slot.occupied=true;
        slot.denseIndex=static_cast<std::uint32_t>(values.size());
        slot.nextFree=InvalidIndex;
        values.emplace_back(std::forward<Args>(args)...);
        denseToSlot.push_back(slotIndex);
        return {slotIndex,slot.generation};
    }

    bool Remove(HandleType handle) {
        Slot *slot=FindSlot(handle);
        if (!slot) return false;
        const std::uint32_t removedDense=slot->denseIndex;
        const std::uint32_t lastDense=static_cast<std::uint32_t>(values.size()-1);
        if (removedDense!=lastDense) {
            values[removedDense]=std::move(values[lastDense]);
            const std::uint32_t movedSlot=denseToSlot[lastDense];
            denseToSlot[removedDense]=movedSlot;
            slots[movedSlot].denseIndex=removedDense;
        }
        values.pop_back();
        denseToSlot.pop_back();
        slot->occupied=false;
        ++slot->generation;
        slot->nextFree=freeHead;
        freeHead=handle.index;
        return true;
    }

    T *Get(HandleType handle) { auto *slot=FindSlot(handle); return slot ? &values[slot->denseIndex] : nullptr; }
    const T *Get(HandleType handle) const { const auto *slot=FindSlot(handle); return slot ? &values[slot->denseIndex] : nullptr; }
    [[nodiscard]] bool Contains(HandleType handle) const { return FindSlot(handle)!=nullptr; }
    [[nodiscard]] std::size_t Size() const { return values.size(); }
    [[nodiscard]] std::vector<T> &Values() { return values; }
    [[nodiscard]] const std::vector<T> &Values() const { return values; }

private:
    static constexpr std::uint32_t InvalidIndex=std::numeric_limits<std::uint32_t>::max();
    struct Slot { std::uint32_t generation{1}; std::uint32_t denseIndex{}; std::uint32_t nextFree{InvalidIndex}; bool occupied{}; };
    Slot *FindSlot(HandleType h) { return h.index<slots.size() && slots[h.index].occupied && slots[h.index].generation==h.generation ? &slots[h.index] : nullptr; }
    const Slot *FindSlot(HandleType h) const { return h.index<slots.size() && slots[h.index].occupied && slots[h.index].generation==h.generation ? &slots[h.index] : nullptr; }
    std::vector<Slot> slots;
    std::vector<T> values;
    std::vector<std::uint32_t> denseToSlot;
    std::uint32_t freeHead{InvalidIndex};
};
} // namespace pipeframe
#endif
