#ifndef PIPEFRAME_LEVEL_LOAD_REQUESTS_H
#define PIPEFRAME_LEVEL_LOAD_REQUESTS_H

#include <filesystem>
#include <optional>

class LevelLoadRequests
{
public:
    void RequestLoad(const std::filesystem::path& levelPath)
    {
        if (!pendingLevelPath.has_value())
        {
            pendingLevelPath = levelPath;
        }
    }

    bool HasPendingLoad() const
    {
        return pendingLevelPath.has_value();
    }

    std::optional<std::filesystem::path> ConsumePendingLoad()
    {
        std::optional<std::filesystem::path> result = pendingLevelPath;
        pendingLevelPath.reset();
        return result;
    }

    void Clear()
    {
        pendingLevelPath.reset();
    }

private:
    std::optional<std::filesystem::path> pendingLevelPath;
};

#endif // PIPEFRAME_LEVEL_LOAD_REQUESTS_H
