#ifndef PIPEFRAME_PROJECT_ASSET_DATABASE_H
#define PIPEFRAME_PROJECT_ASSET_DATABASE_H

#include <cstdint>
#include <filesystem>
#include <functional>
#include <optional>
#include <memory>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

#include <PipeFrame/Foundation/MathTypes.h>

namespace pipeframe::assets {

using AssetId = std::string;
using AssetOperationId = std::uint64_t;

enum class AssetType : std::uint8_t {
    Unknown,
    Texture,
    Audio,
    Shader,
    Material,
    Model,
    Scene,
    Part,
    Prefab,
    Tilemap,
    Tileset,
};

enum class AssetState : std::uint8_t { Ready, Missing, Failed };
enum class AssetOperationState : std::uint8_t { Queued, Running, Succeeded, Failed, Cancelled };

struct PartAttachmentMetadata {
    std::string id;
    std::string type;
    Transform2D localTransform{};
    std::vector<std::string> accepts;
    bool multiple{false};
    bool operator==(const PartAttachmentMetadata &) const = default;
};

struct PartConfigurationLimit {
    std::string key;
    double minimum{};
    double maximum{};
    std::string unit;
    bool operator==(const PartConfigurationLimit &) const = default;
};

struct PartAssetMetadata {
    std::string category;
    Vector2f physicalSize{};
    std::string collisionShape{"box"};
    std::vector<PartAttachmentMetadata> attachments;
    std::vector<std::string> compatibilityTags;
    std::vector<PartConfigurationLimit> configurationLimits;
    bool operator==(const PartAssetMetadata &) const = default;
};

struct AssetRecord {
    AssetId id;
    std::filesystem::path sourcePath;
    std::filesystem::path cachePath;
    std::filesystem::path thumbnailPath;
    std::filesystem::path previewPath;
    AssetType type{AssetType::Unknown};
    AssetState state{AssetState::Ready};
    std::string importerId;
    std::uint32_t importerVersion{1};
    std::uint64_t revision{1};
    std::uint64_t contentHash{};
    std::vector<AssetId> dependencies;
    std::vector<std::string> tags;
    std::optional<PartAssetMetadata> part;
    std::string error;
    bool operator==(const AssetRecord &) const = default;
};

struct AssetImportRequest {
    std::filesystem::path sourcePath;
    std::filesystem::path destinationFolder;
    std::optional<AssetType> type;
    std::vector<AssetId> dependencies;
    std::vector<std::string> tags;
    std::optional<PartAssetMetadata> part;
};

struct AssetImportOutput {
    std::filesystem::path cacheFile;
    std::filesystem::path thumbnailFile;
    std::filesystem::path previewFile;
    std::vector<AssetId> dependencies;
};

struct AssetImportContext {
    AssetId assetId;
    std::filesystem::path sourceFile;
    std::filesystem::path cacheDirectory;
    std::function<bool()> isCancelled;
};

using AssetImporterFunction =
    std::function<std::optional<AssetImportOutput>(const AssetImportContext &, std::string &)>;

struct AssetImporterDescriptor {
    std::string id;
    std::uint32_t version{1};
    AssetType type{AssetType::Unknown};
    std::vector<std::string> extensions;
    AssetImporterFunction import;
};

struct AssetOperation {
    AssetOperationId id{};
    AssetOperationState state{AssetOperationState::Queued};
    std::string label;
    AssetId assetId;
    std::string error;
};

struct AssetSearchQuery {
    std::string text;
    std::filesystem::path folder;
    std::optional<AssetType> type;
    std::string tag;
    bool includeMissing{true};
};

class AssetDatabase final {
public:
    AssetDatabase();
    ~AssetDatabase();
    AssetDatabase(const AssetDatabase &)=delete;
    AssetDatabase &operator=(const AssetDatabase &)=delete;
    // All database calls belong to the owning (editor) thread. PumpOperations never
    // waits: it starts one CPU importer or publishes a completed staged result.
    // Importer callbacks must use only their context/captured thread-safe data, never
    // this database or GPU APIs. Destruction cancels and joins the active worker.
    // Synchronous ImportNow/ProcessOperation remain for command-line/build callers.
    bool PumpOperations();
    bool HasActiveImport() const;
    static constexpr std::uint32_t CurrentFormatVersion = 2;
    static constexpr std::uint32_t OldestSupportedFormatVersion = 1;

    bool Open(std::filesystem::path projectDirectory, std::string *error = nullptr);
    bool Save(std::string *error = nullptr) const;
    // Discover supported, unindexed files recursively under Assets, without copying them.
    // Existing records/IDs are preserved; import failures remain in operation diagnostics.
    bool DiscoverProjectAssets(std::string *error = nullptr);

    bool RegisterImporter(AssetImporterDescriptor importer, std::string *error = nullptr);
    void RegisterBuiltInImporters();

    AssetOperationId QueueImport(AssetImportRequest request);
    AssetOperationId QueueReimport(const AssetId &assetId);
    bool ProcessNextOperation();
    bool ProcessOperation(AssetOperationId operationId);
    bool CancelOperation(AssetOperationId operationId);

    std::optional<AssetId> ImportNow(AssetImportRequest request, std::string *error = nullptr);
    bool Reimport(const AssetId &assetId, std::string *error = nullptr);
    bool Move(const AssetId &assetId, const std::filesystem::path &newRelativePath,
              std::string *error = nullptr);
    bool RepairMissingSource(const AssetId &assetId, const std::filesystem::path &replacement,
                             std::string *error = nullptr);
    void RefreshMissingStates();

    const AssetRecord *Find(const AssetId &assetId) const;
    std::span<const AssetRecord> GetAssets() const;
    std::span<const AssetOperation> GetOperations() const;
    std::vector<const AssetRecord *> Search(const AssetSearchQuery &query = {}) const;
    std::vector<AssetId> GetDependents(const AssetId &assetId) const;
    std::vector<std::string> Validate() const;

    const std::filesystem::path &GetProjectDirectory() const;
    std::filesystem::path GetDatabasePath() const;

private:
    struct PendingOperation {
        AssetOperation publicState;
        std::optional<AssetImportRequest> import;
        bool reimport{false};
        bool cancellationRequested{false};
    };

    bool Load(std::string *error);
    bool Execute(PendingOperation &operation);
    bool ExecuteImport(PendingOperation &operation, const AssetImportRequest &request);
    bool ExecuteReimport(PendingOperation &operation, AssetRecord &record);
    const AssetImporterDescriptor *FindImporter(const std::filesystem::path &path,
                                                std::optional<AssetType> type = {}) const;
    AssetRecord *FindMutable(const AssetId &assetId);
    PendingOperation *FindPending(AssetOperationId operationId);
    void RefreshPublicOperations();
    AssetId AllocateId();

    struct AsyncJob;
    std::unique_ptr<AsyncJob> asyncJob;
    std::filesystem::path projectDirectory;
    std::vector<AssetRecord> assets;
    std::vector<AssetImporterDescriptor> importers;
    std::vector<PendingOperation> pendingOperations;
    std::vector<AssetOperation> operations;
    std::uint64_t nextAssetNumber{1};
    AssetOperationId nextOperationId{1};
};

const char *ToString(AssetType type);
const char *ToString(AssetState state);
const char *ToString(AssetOperationState state);

} // namespace pipeframe::assets

#endif
