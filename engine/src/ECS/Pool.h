

#ifndef PIPEFRAME_POOL_H
#define PIPEFRAME_POOL_H

#include <vector>
#include <unordered_map>

class IPool
{
public:
    virtual ~IPool() = default;
    virtual void RemoveEntityFromPool(int entityId) = 0;
};

template <typename T>
class Pool : public IPool
{
    std::vector<T> data;
    int size;

    std::unordered_map<int, int> entityIdToPoolIndex;
    std::unordered_map<int, int> poolIndexToEntityId;

public:
    Pool(int capacity = 100)
    {
        size = 0;
        data.resize(capacity);
    }

    virtual ~Pool() = default;

    bool isEmpty() const
    {
        return size == 0;
    }

    int GetSize() const
    {
        return size;
    }

    void Clear()
    {
        data.clear();
        entityIdToPoolIndex.clear();
        poolIndexToEntityId.clear();
        size = 0;
    }

    void Set(int entityId, T object)
    {
        if (entityIdToPoolIndex.find(entityId) != entityIdToPoolIndex.end())
        {
            int index = entityIdToPoolIndex[entityId];
            data[index] = object;
        }
        else
        {
            int index = size;
            entityIdToPoolIndex.emplace(entityId, index);
            poolIndexToEntityId.emplace(index, entityId);

            if (index >= static_cast<int>(data.capacity()))
            {
                data.resize(size * 2);
            }

            data[index] = object;
            size++;
        }
    }

    void Remove(int entityId)
    {
        int indexOfRemovedEntity = entityIdToPoolIndex[entityId];
        int indexOfLastEntity = size - 1;
        data[indexOfRemovedEntity] = data[indexOfLastEntity];

        int entityIdOfLastElement = poolIndexToEntityId[indexOfLastEntity];
        entityIdToPoolIndex[entityIdOfLastElement] = indexOfRemovedEntity;
        poolIndexToEntityId[indexOfRemovedEntity] = entityIdOfLastElement;

        entityIdToPoolIndex.erase(entityId);
        poolIndexToEntityId.erase(indexOfLastEntity);

        size--;
    }

    void RemoveEntityFromPool(int entityId) override
    {
        if (entityIdToPoolIndex.find(entityId) != entityIdToPoolIndex.end())
        {
            Remove(entityId);
        }
    }

    T& Get(int entityId)
    {
        int index = entityIdToPoolIndex[entityId];
        return static_cast<T&>(data[index]);
    }

    T& operator[](unsigned int index)
    {
        return data[index];
    }
};


#endif //PIPEFRAME_POOL_H
