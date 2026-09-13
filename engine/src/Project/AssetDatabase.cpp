#include <PipeFrame/Project/AssetDatabase.h>
#include <PipeFrame/Resources/ImageData.h>
#include <PipeFrame/Environment/TilemapSerializer.h>
#include <PipeFrame/Environment/VisualAssets2D.h>
#include <future>
#include <atomic>
#include <chrono>

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <ranges>
#include <sstream>
#include <system_error>
#include <unordered_set>

namespace pipeframe::assets {
namespace {
constexpr const char *Header = "PIPEFRAME_ASSET_DATABASE";

void SetError(std::string *target, std::string value) { if (target) *target = std::move(value); }

std::string Lower(std::string value) {
    std::ranges::transform(value, value.begin(), [](const unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

bool IsSafeRelative(const std::filesystem::path &path) {
    if (path.empty() || path.is_absolute()) return false;
    for (const auto &part : path.lexically_normal()) if (part == "..") return false;
    return true;
}

std::uint64_t HashFile(const std::filesystem::path &path, std::string &error) {
    std::ifstream input(path, std::ios::binary);
    if (!input) { error = "Could not read source asset: " + path.string(); return 0; }
    std::uint64_t hash = 1469598103934665603ull;
    char buffer[64 * 1024];
    while (input) {
        input.read(buffer, sizeof(buffer));
        for (std::streamsize index = 0; index < input.gcount(); ++index) {
            hash ^= static_cast<unsigned char>(buffer[index]);
            hash *= 1099511628211ull;
        }
    }
    return hash;
}

std::filesystem::path DefaultFolder(const AssetType type) {
    switch (type) {
    case AssetType::Texture: return "Textures";
    case AssetType::Audio: return "Audio";
    case AssetType::Shader: return "Shaders";
    case AssetType::Material: return "Materials";
    case AssetType::Model: return "Models";
    case AssetType::Scene: return "Scenes";
    case AssetType::Part: return "Parts";
    case AssetType::Prefab: return "Prefabs";
    case AssetType::Tilemap: return "Tilemaps";
    case AssetType::Tileset: return "Tilesets";
    case AssetType::Unknown: return "Other";
    }
    return "Other";
}

bool StartsWithPath(const std::filesystem::path &path, const std::filesystem::path &root) {
    const auto relative = path.lexically_normal().lexically_relative(root.lexically_normal());
    return IsSafeRelative(relative);
}

bool ValidateSource(const AssetType type,const std::filesystem::path &path,std::string &error){
    if(type==AssetType::Material){std::ifstream input(path);return LoadVisualAsset<Material2D>(input,error).has_value();}
    if(type==AssetType::Tileset){std::ifstream input(path);return LoadVisualAsset<Tileset2D>(input,error).has_value();}
    if(type==AssetType::Tilemap){std::ifstream input(path);return TilemapSerializer::Load(input,error).has_value();}
    std::ifstream input(path,std::ios::binary);std::string bytes((std::istreambuf_iterator<char>(input)),{});
    if(bytes.empty()){error="Asset source is empty.";return false;}
    const auto extension=Lower(path.extension().string());
    const auto starts=[&](std::string_view signature){return bytes.size()>=signature.size()&&
        std::equal(signature.begin(),signature.end(),bytes.begin());};
    bool valid=true;
    if(type==AssetType::Audio){
        if(extension==".wav")valid=bytes.size()>=12&&starts("RIFF")&&bytes.substr(8,4)=="WAVE";
        else if(extension==".ogg")valid=starts("OggS");
        else valid=starts("fLaC");
    }else if(type==AssetType::Model){
        if(extension==".glb")valid=starts("glTF");
        else if(extension==".gltf")valid=bytes.find('{')!=std::string::npos;
        else valid=bytes.find("v ")!=std::string::npos||bytes.find("o ")!=std::string::npos;
    }else if(type==AssetType::Shader){
        valid=bytes.find_first_not_of(" \t\r\n")!=std::string::npos;
    }
    if(!valid)error="Asset contents do not match the "+extension+" format.";
    return valid;
}

AssetImporterDescriptor BuiltInImporter(std::string id, const AssetType type,
                                        std::vector<std::string> extensions) {
    return {std::move(id), type == AssetType::Texture ? 4u : 3u, type, std::move(extensions),
            [type](const AssetImportContext &context, std::string &error)
                -> std::optional<AssetImportOutput> {
                if (context.isCancelled()) { error = "Import cancelled."; return std::nullopt; }
                ImageData texture;
                if (type == AssetType::Texture) {
                    if (!LoadImageData(context.sourceFile, texture, error)) return std::nullopt;
                } else if (!ValidateSource(type, context.sourceFile, error)) return std::nullopt;
                if (context.isCancelled()) { error = "Import cancelled."; return std::nullopt; }
                std::error_code filesystemError;
                std::filesystem::create_directories(context.cacheDirectory, filesystemError);
                if (filesystemError) { error = filesystemError.message(); return std::nullopt; }
                const auto output = context.cacheDirectory / "compiled.pfasset";
                std::ifstream input(context.sourceFile, std::ios::binary);
                std::ofstream cache(output, std::ios::binary | std::ios::trunc);
                if (!input || !cache) { error = "Could not open asset source or cache output."; return std::nullopt; }
                const auto sourceSize=std::filesystem::file_size(context.sourceFile,filesystemError);
                if(filesystemError){error=filesystemError.message();return std::nullopt;}
                cache<<"PIPEFRAME_COMPILED_ASSET 1\nTYPE "<<ToString(type)<<"\nFORMAT "
                     <<Lower(context.sourceFile.extension().string())<<"\nSIZE "<<sourceSize<<"\nDATA\n";
                char buffer[64 * 1024];
                while (input) {
                    if (context.isCancelled()) { error = "Import cancelled."; return std::nullopt; }
                    input.read(buffer, sizeof(buffer));
                    cache.write(buffer, input.gcount());
                }
                if (!cache.good()) { error = "Failed while writing imported cache output."; return std::nullopt; }
                if (type == AssetType::Texture) {
                    const auto preview = context.cacheDirectory / "preview.png";
                    const auto thumbnail = context.cacheDirectory / "thumbnail.png";
                    // Nearest-neighbour preserves tile/pixel-art colours and alpha. Never upscale.
                    const auto resize = [&](unsigned limit) {
                        const auto source = texture.Size();
                        const auto longest = std::max(source.x, source.y);
                        const auto divisor = std::max(longest, limit);
                        const Vector2u size{
                            std::max(1u, static_cast<unsigned>(std::uint64_t(source.x) * limit / divisor)),
                            std::max(1u, static_cast<unsigned>(std::uint64_t(source.y) * limit / divisor))};
                        ImageData result(size);
                        for (unsigned y = 0; y < size.y; ++y)
                            for (unsigned x = 0; x < size.x; ++x)
                                result.SetPixel({x, y}, texture.Pixel({
                                    static_cast<unsigned>(std::uint64_t(x) * source.x / size.x),
                                    static_cast<unsigned>(std::uint64_t(y) * source.y / size.y)}));
                        return result;
                    };
                    if (!SaveImageData(context.cacheDirectory/"image.png", texture, error) || !SaveImageData(preview, resize(512), error) ||
                        !SaveImageData(thumbnail, resize(128), error)) return std::nullopt;
                    if (context.isCancelled()) { error = "Import cancelled."; return std::nullopt; }
                    return AssetImportOutput{output, thumbnail, preview};
                }
                const auto preview=context.cacheDirectory/"preview.pfpreview";
                const auto thumbnail=context.cacheDirectory/"thumbnail.pfpreview";
                std::ofstream(preview,std::ios::trunc)<<"PIPEFRAME_PREVIEW 1\nTYPE "<<ToString(type)<<"\nSOURCE "
                                                       <<context.sourceFile.filename().generic_string()<<'\n';
                std::ofstream(thumbnail,std::ios::trunc)<<"PIPEFRAME_THUMBNAIL 1\nTYPE "<<ToString(type)<<'\n';
                if(!std::filesystem::is_regular_file(preview)||!std::filesystem::is_regular_file(thumbnail)){
                    error="Could not create deterministic preview artifacts.";return std::nullopt;
                }
                std::vector<AssetId> dependencies;
                std::ifstream authored(context.sourceFile);std::string parseError;
                if(type==AssetType::Material){auto v=LoadVisualAsset<Material2D>(authored,parseError);if(v&&!v->texture.assetId.empty())dependencies.push_back(v->texture.assetId);}
                if(type==AssetType::Tileset){auto v=LoadVisualAsset<Tileset2D>(authored,parseError);if(v&&!v->material.assetId.empty())dependencies.push_back(v->material.assetId);}
                if(type==AssetType::Tilemap){auto v=TilemapSerializer::Load(authored,parseError);if(v&&!v->Tileset().empty())dependencies.push_back(v->Tileset());}
                return AssetImportOutput{output,thumbnail,preview,std::move(dependencies)};
            }};
}
}

const char *ToString(const AssetType type) {
    switch (type) {
    case AssetType::Unknown: return "Unknown";
    case AssetType::Texture: return "Texture";
    case AssetType::Audio: return "Audio";
    case AssetType::Shader: return "Shader";
    case AssetType::Material: return "Material";
    case AssetType::Model: return "Model";
    case AssetType::Scene: return "Scene";
    case AssetType::Part: return "Part";
    case AssetType::Prefab: return "Prefab";
    case AssetType::Tilemap: return "Tilemap";
    case AssetType::Tileset: return "Tileset";
    }
    return "Unknown";
}

const char *ToString(const AssetState state) {
    switch (state) {
    case AssetState::Ready: return "Ready";
    case AssetState::Missing: return "Missing";
    case AssetState::Failed: return "Failed";
    }
    return "Failed";
}

const char *ToString(const AssetOperationState state) {
    switch (state) {
    case AssetOperationState::Queued: return "Queued";
    case AssetOperationState::Running: return "Running";
    case AssetOperationState::Succeeded: return "Succeeded";
    case AssetOperationState::Failed: return "Failed";
    case AssetOperationState::Cancelled: return "Cancelled";
    }
    return "Failed";
}

struct AssetDatabase::AsyncJob {
    struct Result {std::optional<AssetRecord> record;std::filesystem::path staging,copiedSource;std::string error;};
    AssetOperationId operation{};std::uint64_t expectedRevision{};
    std::shared_ptr<std::atomic_bool> cancelled=std::make_shared<std::atomic_bool>(false);
    std::future<Result> future;
    ~AsyncJob(){
        cancelled->store(true);
        if(future.valid())try{
            auto result=future.get();std::error_code ec;
            if(!result.staging.empty())std::filesystem::remove_all(result.staging,ec);
            if(!result.copiedSource.empty())std::filesystem::remove(result.copiedSource,ec);
        }catch(...){ /* Destruction must not throw while joining a failed worker. */ }
    }
};
AssetDatabase::AssetDatabase()=default;
AssetDatabase::~AssetDatabase()=default;
bool AssetDatabase::HasActiveImport()const{return bool(asyncJob);}

bool AssetDatabase::PumpOperations(){
    if(asyncJob){
        if(asyncJob->future.wait_for(std::chrono::seconds(0))!=std::future_status::ready)return false;
        auto result=asyncJob->future.get();const auto id=asyncJob->operation;
        const bool cancelled=asyncJob->cancelled->load();const auto expected=asyncJob->expectedRevision;
        asyncJob.reset();auto *operation=FindPending(id);if(!operation)return false;
        auto *previous=FindMutable(operation->publicState.assetId);
        if(operation->reimport&&(!previous||previous->revision!=expected))result.error="Asset changed during import";
        if(!cancelled&&result.error.empty()&&result.record){
            if(previous)*previous=std::move(*result.record);else assets.push_back(std::move(*result.record));
            operation->publicState.state=AssetOperationState::Succeeded;
            std::string saveError;if(!Save(&saveError)){operation->publicState.state=AssetOperationState::Failed;operation->publicState.error=saveError;}
        } else {
            operation->publicState.state=cancelled?AssetOperationState::Cancelled:AssetOperationState::Failed;
            operation->publicState.error=result.error;std::error_code ec;
            if(!result.staging.empty())std::filesystem::remove_all(result.staging,ec);
            if(!result.copiedSource.empty())std::filesystem::remove(result.copiedSource,ec);
        }
        RefreshPublicOperations();return true;
    }
    auto found=std::ranges::find_if(pendingOperations,[](const auto &op){return op.publicState.state==AssetOperationState::Queued;});
    if(found==pendingOperations.end())return false;
    auto &operation=*found;AssetRecord record;AssetImportRequest request;
    const AssetImporterDescriptor *importer=nullptr;
    if(operation.import){request=*operation.import;importer=FindImporter(request.sourcePath,request.type);record.id=AllocateId();record.tags=request.tags;record.part=request.part;record.dependencies=request.dependencies;}
    else if(const auto *old=Find(operation.publicState.assetId)){
        record=*old;request.sourcePath=projectDirectory/old->sourcePath;importer=FindImporter(old->sourcePath,old->type);++record.revision;
    }
    if(!importer){operation.publicState.state=AssetOperationState::Failed;operation.publicState.error="Importer or asset is unavailable";RefreshPublicOperations();return true;}
    record.type=importer->type;record.importerId=importer->id;record.importerVersion=importer->version;
    operation.publicState.assetId=record.id;operation.publicState.state=AssetOperationState::Running;
    asyncJob=std::make_unique<AsyncJob>();asyncJob->operation=operation.publicState.id;
    asyncJob->expectedRevision=operation.reimport?record.revision-1:0;
    const auto root=projectDirectory;const auto cancel=asyncJob->cancelled;const auto import=importer->import;
    const auto token=std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    asyncJob->future=std::async(std::launch::async,[root,cancel,import,request,record,token]()mutable{
        AsyncJob::Result result;
        try{
            auto source=std::filesystem::absolute(request.sourcePath).lexically_normal();
            if(!std::filesystem::is_regular_file(source))throw std::runtime_error("Source asset is missing");
            if(!StartsWithPath(source,root/"Assets")){
                const auto folder=request.destinationFolder.empty()?DefaultFolder(record.type):request.destinationFolder;
                if(!IsSafeRelative(folder))throw std::runtime_error("Unsafe asset destination");
                auto destination=root/"Assets"/folder/source.filename();
                if(std::filesystem::exists(destination))destination=destination.parent_path()/(destination.stem().string()+"-"+record.id+"-"+token+destination.extension().string());
                std::filesystem::create_directories(destination.parent_path());
                std::filesystem::copy_file(source,destination);result.copiedSource=destination;source=destination;
            }
            record.sourcePath=source.lexically_relative(root);
            result.staging=root/".pipeframe/cache"/record.id/("import-"+token);
            std::string error;const auto originalHash=HashFile(source,error);
            if(!error.empty())throw std::runtime_error(error);
            auto output=import({record.id,source,result.staging,[cancel]{return cancel->load();}},error);
            if(!output||cancel->load())throw std::runtime_error(error.empty()?"Import cancelled":error);
            record.contentHash=HashFile(source,error);
            if(!error.empty()||record.contentHash!=originalHash)throw std::runtime_error("Source changed during import; retry");
            record.cachePath=output->cacheFile.lexically_relative(root);record.previewPath=output->previewFile.lexically_relative(root);record.thumbnailPath=output->thumbnailFile.lexically_relative(root);
            if(record.type==AssetType::Material||record.type==AssetType::Tileset||record.type==AssetType::Tilemap)record.dependencies=output->dependencies;
            else record.dependencies.insert(record.dependencies.end(),output->dependencies.begin(),output->dependencies.end());
            std::ranges::sort(record.dependencies);record.dependencies.erase(std::unique(record.dependencies.begin(),record.dependencies.end()),record.dependencies.end());
            record.state=AssetState::Ready;record.error.clear();result.record=std::move(record);
        }catch(const std::exception &failure){result.error=failure.what();}
        return result;
    });
    RefreshPublicOperations();return true;
}

bool AssetDatabase::Open(std::filesystem::path root, std::string *error) {
    if(asyncJob){SetError(error,"An asset import is still active");return false;}
    if (root.empty()) { SetError(error, "Project directory cannot be empty."); return false; }
    std::error_code filesystemError;
    projectDirectory = std::filesystem::absolute(root, filesystemError).lexically_normal();
    if (filesystemError) { SetError(error, filesystemError.message()); return false; }
    std::filesystem::create_directories(projectDirectory / "Assets", filesystemError);
    std::filesystem::create_directories(projectDirectory / ".pipeframe" / "cache", filesystemError);
    if (filesystemError) { SetError(error, filesystemError.message()); return false; }
    assets.clear(); pendingOperations.clear(); operations.clear(); importers.clear();
    nextAssetNumber = nextOperationId = 1;
    RegisterBuiltInImporters();
    if (!Load(error)) return false;
    // Individual source import failures must not prevent opening the project.
    DiscoverProjectAssets();
    return true;
}

bool AssetDatabase::DiscoverProjectAssets(std::string *error) {
    if (projectDirectory.empty()) { SetError(error, "No project is open."); return false; }
    std::error_code ec;
    const auto root = projectDirectory / "Assets";
    std::vector<std::filesystem::path> candidates;
    std::filesystem::recursive_directory_iterator iterator(root, ec), end;
    while (!ec && iterator != end) {
        const auto &entry = *iterator;
        // Do not discover linked files/directories outside the project's asset tree.
        if (entry.is_symlink(ec)) iterator.disable_recursion_pending();
        else if (!ec && entry.is_regular_file(ec) && FindImporter(entry.path()))
            candidates.push_back(entry.path());
        if (!ec) iterator.increment(ec);
    }
    if (ec) { SetError(error, ec.message()); return false; }
    std::ranges::sort(candidates);
    bool success = true;
    for (const auto &source : candidates) {
        const auto relative = source.lexically_relative(projectDirectory);
        const auto existing = std::ranges::find(assets, relative, &AssetRecord::sourcePath);
        if (existing != assets.end()) {
            const auto *importer = FindImporter(source, existing->type);
            const auto missing = [&](const std::filesystem::path &path) {
                std::error_code error;
                return path.empty() || !std::filesystem::is_regular_file(projectDirectory / path, error);
            };
            // Cache contents are disposable. An indexed source must not bypass an
            // importer upgrade or leave consumers pointing at missing artifacts.
            const bool incomplete = missing(existing->cachePath) ||
                (!existing->previewPath.empty() && missing(existing->previewPath)) ||
                (!existing->thumbnailPath.empty() && missing(existing->thumbnailPath)) ||
                (existing->importerId == "pipeframe.texture" &&
                 missing(existing->cachePath.parent_path() / "image.png"));
            if (importer && (existing->importerVersion != importer->version || incomplete)) {
                const auto id = existing->id;
                std::string importError;
                if (!Reimport(id, &importError)) {
                    success = false; SetError(error, source.string() + ": " + importError);
                }
            }
            continue;
        }
        std::string importError;
        if (!ImportNow({source}, &importError)) {
            success = false; SetError(error, source.string() + ": " + importError);
        }
    }
    return success;
}

bool AssetDatabase::RegisterImporter(AssetImporterDescriptor importer, std::string *error) {
    if (importer.id.empty() || importer.version == 0 || importer.type == AssetType::Unknown ||
        importer.extensions.empty() || !importer.import) {
        SetError(error, "Importer ID, version, type, extensions, and function are required.");
        return false;
    }
    if (std::ranges::any_of(importers, [&](const auto &item) { return item.id == importer.id; })) {
        SetError(error, "Importer ID is already registered."); return false;
    }
    for (auto &extension : importer.extensions) {
        extension = Lower(extension);
        if (!extension.empty() && extension.front() != '.') extension.insert(extension.begin(), '.');
    }
    importers.push_back(std::move(importer));
    return true;
}

void AssetDatabase::RegisterBuiltInImporters() {
    RegisterImporter(BuiltInImporter("pipeframe.texture", AssetType::Texture,
                                  {".png", ".jpg", ".jpeg", ".bmp", ".tga"}));
    RegisterImporter(BuiltInImporter("pipeframe.audio", AssetType::Audio, {".wav", ".ogg", ".flac"}));
    RegisterImporter(BuiltInImporter("pipeframe.shader", AssetType::Shader,
                                  {".vert", ".frag", ".glsl"}));
    RegisterImporter(BuiltInImporter("pipeframe.material", AssetType::Material, {".mat", ".pfmat"}));
    RegisterImporter(BuiltInImporter("pipeframe.model", AssetType::Model, {".obj", ".gltf", ".glb"}));
    RegisterImporter(BuiltInImporter("pipeframe.scene", AssetType::Scene, {".pfscene"}));
    RegisterImporter(BuiltInImporter("pipeframe.part", AssetType::Part, {".pfpart"}));
    RegisterImporter(BuiltInImporter("pipeframe.prefab", AssetType::Prefab, {".pfprefab"}));
    RegisterImporter(BuiltInImporter("pipeframe.tileset", AssetType::Tileset, {".pftileset"}));
    RegisterImporter(BuiltInImporter("pipeframe.tilemap", AssetType::Tilemap, {".pftilemap"}));
}

AssetOperationId AssetDatabase::QueueImport(AssetImportRequest request) {
    PendingOperation operation;
    operation.publicState = {nextOperationId++, AssetOperationState::Queued,
                             "Import " + request.sourcePath.filename().string(), {}, {}};
    operation.import = std::move(request);
    pendingOperations.push_back(std::move(operation));
    RefreshPublicOperations();
    return pendingOperations.back().publicState.id;
}

AssetOperationId AssetDatabase::QueueReimport(const AssetId &assetId) {
    PendingOperation operation;
    operation.publicState = {nextOperationId++, AssetOperationState::Queued,
                             "Reimport " + assetId, assetId, {}};
    operation.reimport = true;
    pendingOperations.push_back(std::move(operation));
    RefreshPublicOperations();
    return pendingOperations.back().publicState.id;
}

bool AssetDatabase::ProcessNextOperation() {
    if(asyncJob)return false;
    const auto found = std::ranges::find_if(pendingOperations, [](const auto &operation) {
        return operation.publicState.state == AssetOperationState::Queued;
    });
    return found != pendingOperations.end() && Execute(*found);
}

bool AssetDatabase::ProcessOperation(const AssetOperationId id) {
    if(asyncJob)return false;
    PendingOperation *operation = FindPending(id);
    return operation && operation->publicState.state == AssetOperationState::Queued && Execute(*operation);
}

bool AssetDatabase::CancelOperation(const AssetOperationId id) {
    PendingOperation *operation = FindPending(id);
    if (!operation || (operation->publicState.state != AssetOperationState::Queued &&
                       operation->publicState.state != AssetOperationState::Running)) return false;
    operation->cancellationRequested = true;
    if(asyncJob&&asyncJob->operation==id)asyncJob->cancelled->store(true);
    if (operation->publicState.state == AssetOperationState::Queued)
        operation->publicState.state = AssetOperationState::Cancelled;
    RefreshPublicOperations();
    return true;
}

std::optional<AssetId> AssetDatabase::ImportNow(AssetImportRequest request, std::string *error) {
    if(asyncJob){SetError(error,"An asset import is active; retry when it completes");return {};}
    const AssetOperationId id = QueueImport(std::move(request));
    ProcessOperation(id);
    const PendingOperation *operation = FindPending(id);
    if (!operation || operation->publicState.state != AssetOperationState::Succeeded) {
        SetError(error, operation ? operation->publicState.error : "Import operation disappeared.");
        return std::nullopt;
    }
    return operation->publicState.assetId;
}

bool AssetDatabase::Reimport(const AssetId &assetId, std::string *error) {
    if(asyncJob){SetError(error,"An asset import is active; retry when it completes");return false;}
    const AssetOperationId id = QueueReimport(assetId);
    ProcessOperation(id);
    const PendingOperation *operation = FindPending(id);
    if (!operation || operation->publicState.state != AssetOperationState::Succeeded) {
        SetError(error, operation ? operation->publicState.error : "Reimport operation disappeared.");
        return false;
    }
    return true;
}

bool AssetDatabase::Execute(PendingOperation &operation) {
    if (operation.cancellationRequested) {
        operation.publicState.state = AssetOperationState::Cancelled;
        RefreshPublicOperations(); return false;
    }
    operation.publicState.state = AssetOperationState::Running;
    bool result = false;
    if (operation.import) result = ExecuteImport(operation, *operation.import);
    else if (operation.reimport) {
        AssetRecord *record = FindMutable(operation.publicState.assetId);
        if (record) result = ExecuteReimport(operation, *record);
        else operation.publicState.error = "Asset does not exist.";
    }
    if (operation.cancellationRequested) operation.publicState.state = AssetOperationState::Cancelled;
    else operation.publicState.state = result ? AssetOperationState::Succeeded : AssetOperationState::Failed;
    RefreshPublicOperations();
    if (result || operation.reimport) Save();
    return result;
}

bool AssetDatabase::ExecuteImport(PendingOperation &operation, const AssetImportRequest &request) {
    const AssetImporterDescriptor *importer = FindImporter(request.sourcePath, request.type);
    const AssetId id = AllocateId();
    operation.publicState.assetId = id;
    if (!importer) { operation.publicState.error = "No importer supports this asset type."; return false; }
    std::error_code filesystemError;
    auto source = std::filesystem::absolute(request.sourcePath, filesystemError).lexically_normal();
    if (filesystemError || !std::filesystem::is_regular_file(source, filesystemError)) {
        operation.publicState.error = "Source asset is missing: " + request.sourcePath.string();
        return false;
    }
    std::filesystem::path relativeSource;
    const auto assetsRoot = projectDirectory / "Assets";
    if (StartsWithPath(source, assetsRoot)) relativeSource = source.lexically_relative(projectDirectory);
    else {
        const auto folder = request.destinationFolder.empty() ? DefaultFolder(importer->type)
                                                               : request.destinationFolder;
        if (!IsSafeRelative(folder)) { operation.publicState.error = "Destination folder is unsafe."; return false; }
        relativeSource = std::filesystem::path("Assets") / folder / source.filename();
        auto destination = projectDirectory / relativeSource;
        std::filesystem::create_directories(destination.parent_path(), filesystemError);
        if (std::filesystem::exists(destination, filesystemError))
            destination = destination.parent_path() /
                          (destination.stem().string() + "-" + id + destination.extension().string());
        std::filesystem::copy_file(source, destination, std::filesystem::copy_options::overwrite_existing,
                                   filesystemError);
        if (filesystemError) { operation.publicState.error = filesystemError.message(); return false; }
        source = destination;
        relativeSource = source.lexically_relative(projectDirectory);
    }
    std::string importError;
    const auto cacheDirectory = projectDirectory / ".pipeframe" / "cache" / id;
    const auto output = importer->import({id, source, cacheDirectory,
                                          [&operation] { return operation.cancellationRequested; }},
                                         importError);
    if (!output) { operation.publicState.error = std::move(importError); return false; }
    AssetRecord record;
    record.id = id; record.sourcePath = relativeSource;
    record.cachePath = output->cacheFile.lexically_relative(projectDirectory);
    record.type = importer->type; record.importerId = importer->id;
    record.importerVersion = importer->version; record.dependencies = request.dependencies;
    record.dependencies.insert(record.dependencies.end(), output->dependencies.begin(), output->dependencies.end());
    record.tags = request.tags; record.part = request.part;
    record.contentHash = HashFile(source, importError);
    record.state = record.contentHash == 0 && !importError.empty() ? AssetState::Failed : AssetState::Ready;
    record.error = std::move(importError);
    record.thumbnailPath = output->thumbnailFile.empty() && importer->type == AssetType::Texture
                               ? record.cachePath : output->thumbnailFile.lexically_relative(projectDirectory);
    record.previewPath = output->previewFile.empty() ? record.cachePath
                                                     : output->previewFile.lexically_relative(projectDirectory);
    std::ranges::sort(record.dependencies); record.dependencies.erase(std::unique(record.dependencies.begin(),record.dependencies.end()),record.dependencies.end());
    std::ranges::sort(record.tags); record.tags.erase(std::unique(record.tags.begin(),record.tags.end()),record.tags.end());
    assets.push_back(std::move(record));
    return true;
}

bool AssetDatabase::ExecuteReimport(PendingOperation &operation, AssetRecord &record) {
    const auto registered = std::ranges::find(importers, record.importerId,
                                              &AssetImporterDescriptor::id);
    const AssetImporterDescriptor *importer = registered == importers.end() ? nullptr : &*registered;
    if (!importer) importer = FindImporter(record.sourcePath, record.type);
    const auto source = projectDirectory / record.sourcePath;
    if (!std::filesystem::is_regular_file(source)) {
        record.state = AssetState::Missing; record.error = "Source asset is missing.";
        operation.publicState.error = record.error; return false;
    }
    if (!importer) { operation.publicState.error = "The recorded importer is unavailable."; return false; }
    std::string importError;
    const auto output = importer->import({record.id, source, projectDirectory / ".pipeframe" / "cache" / record.id,
                                          [&operation] { return operation.cancellationRequested; }}, importError);
    if (!output) { record.state = AssetState::Failed; record.error = importError;
                   operation.publicState.error = importError; return false; }
    record.cachePath = output->cacheFile.lexically_relative(projectDirectory);
    record.thumbnailPath = output->thumbnailFile.empty() && record.type == AssetType::Texture
                               ? record.cachePath : output->thumbnailFile.lexically_relative(projectDirectory);
    record.previewPath = output->previewFile.empty() ? record.cachePath
                                                     : output->previewFile.lexically_relative(projectDirectory);
    record.contentHash = HashFile(source, importError); record.importerVersion = importer->version;
    if(record.type==AssetType::Material||record.type==AssetType::Tileset||record.type==AssetType::Tilemap)record.dependencies=output->dependencies;
    ++record.revision; record.state = AssetState::Ready; record.error.clear();
    return true;
}

bool AssetDatabase::Move(const AssetId &id, const std::filesystem::path &newRelativePath,
                         std::string *error) {
    if(asyncJob){SetError(error,"Wait for the active import");return false;}
    AssetRecord *record = FindMutable(id);
    if (!record) { SetError(error, "Asset does not exist."); return false; }
    if (!IsSafeRelative(newRelativePath) || newRelativePath.extension() != record->sourcePath.extension()) {
        SetError(error, "Asset destination must be a safe relative path with the same extension."); return false;
    }
    const auto destinationRelative = newRelativePath.begin()->string() == "Assets"
                                         ? newRelativePath : std::filesystem::path("Assets") / newRelativePath;
    std::error_code filesystemError;
    std::filesystem::create_directories((projectDirectory / destinationRelative).parent_path(), filesystemError);
    std::filesystem::rename(projectDirectory / record->sourcePath, projectDirectory / destinationRelative,
                            filesystemError);
    if (filesystemError) { SetError(error, filesystemError.message()); return false; }
    record->sourcePath = destinationRelative.lexically_normal();
    return Save(error);
}

bool AssetDatabase::RepairMissingSource(const AssetId &id, const std::filesystem::path &replacement,
                                        std::string *error) {
    if(asyncJob){SetError(error,"Wait for the active import");return false;}
    AssetRecord *record = FindMutable(id);
    if (!record) { SetError(error, "Asset does not exist."); return false; }
    std::error_code filesystemError;
    if (!std::filesystem::is_regular_file(replacement, filesystemError)) {
        SetError(error, "Replacement source is missing."); return false;
    }
    auto destination = projectDirectory / "Assets" / "Recovered" / replacement.filename();
    std::filesystem::create_directories(destination.parent_path(), filesystemError);
    std::filesystem::copy_file(replacement, destination, std::filesystem::copy_options::overwrite_existing,
                               filesystemError);
    if (filesystemError) { SetError(error, filesystemError.message()); return false; }
    record->sourcePath = destination.lexically_relative(projectDirectory);
    return Reimport(id, error);
}

void AssetDatabase::RefreshMissingStates() {
    for (auto &record : assets) {
        if (!std::filesystem::is_regular_file(projectDirectory / record.sourcePath)) {
            record.state = AssetState::Missing; record.error = "Source asset is missing.";
        } else if (record.state == AssetState::Missing) {
            record.state = AssetState::Ready; record.error.clear();
        }
    }
}

const AssetRecord *AssetDatabase::Find(const AssetId &id) const {
    const auto found = std::ranges::find(assets, id, &AssetRecord::id);
    if(found!=assets.end())return &*found;
    // Portable references in shipped source scenes resolve through imported assets.
    if(id.starts_with("source:")){
        const auto source=std::filesystem::path(id.substr(7)).lexically_normal();
        const auto match=std::ranges::find(assets,source,&AssetRecord::sourcePath);
        if(match!=assets.end())return &*match;
    }
    return nullptr;
}
AssetRecord *AssetDatabase::FindMutable(const AssetId &id) {
    const auto found = std::ranges::find(assets, id, &AssetRecord::id);
    return found == assets.end() ? nullptr : &*found;
}
std::span<const AssetRecord> AssetDatabase::GetAssets() const { return assets; }
std::span<const AssetOperation> AssetDatabase::GetOperations() const { return operations; }

std::vector<const AssetRecord *> AssetDatabase::Search(const AssetSearchQuery &query) const {
    std::vector<const AssetRecord *> result;
    const auto text = Lower(query.text);
    for (const auto &record : assets) {
        if (!query.includeMissing && record.state != AssetState::Ready) continue;
        if (query.type && record.type != *query.type) continue;
        if (!query.folder.empty() && record.sourcePath.parent_path().generic_string().find(query.folder.generic_string()) != 0) continue;
        if (!query.tag.empty() && std::ranges::find(record.tags, query.tag) == record.tags.end()) continue;
        const auto searchable = Lower(record.id + " " + record.sourcePath.generic_string() + " " +
                                      (record.part ? record.part->category : std::string{}));
        if (!text.empty() && searchable.find(text) == std::string::npos) continue;
        result.push_back(&record);
    }
    std::ranges::sort(result, {}, [](const AssetRecord *record) { return record->sourcePath.generic_string(); });
    return result;
}

std::vector<AssetId> AssetDatabase::GetDependents(const AssetId &id) const {
    std::vector<AssetId> result;
    for (const auto &record : assets)
        if (std::ranges::find(record.dependencies, id) != record.dependencies.end()) result.push_back(record.id);
    std::ranges::sort(result); return result;
}

std::vector<std::string> AssetDatabase::Validate() const {
    std::vector<std::string> errors;
    std::unordered_set<AssetId> ids;
    for (const auto &record : assets) {
        if (record.id.empty() || !ids.insert(record.id).second) errors.push_back("Duplicate or empty asset ID.");
        if (!IsSafeRelative(record.sourcePath) || !IsSafeRelative(record.cachePath))
            errors.push_back("Asset " + record.id + " has an unsafe path.");
        for (const auto &dependency : record.dependencies)
            if (!Find(dependency)) errors.push_back("Asset " + record.id + " has a missing dependency " + dependency + '.');
        if (record.part) {
            std::unordered_set<std::string> attachments;
            for (const auto &attachment : record.part->attachments)
                if (attachment.id.empty() || attachment.type.empty() || !attachments.insert(attachment.id).second)
                    errors.push_back("Part " + record.id + " has an invalid attachment.");
            for (const auto &limit : record.part->configurationLimits)
                if (limit.key.empty() || limit.minimum > limit.maximum)
                    errors.push_back("Part " + record.id + " has an invalid configuration limit.");
        }
    }
    return errors;
}

const std::filesystem::path &AssetDatabase::GetProjectDirectory() const { return projectDirectory; }
std::filesystem::path AssetDatabase::GetDatabasePath() const {
    return projectDirectory / ".pipeframe" / "assets.db";
}

AssetDatabase::PendingOperation *AssetDatabase::FindPending(const AssetOperationId id) {
    const auto found = std::ranges::find_if(pendingOperations, [=](const auto &item) { return item.publicState.id == id; });
    return found == pendingOperations.end() ? nullptr : &*found;
}
void AssetDatabase::RefreshPublicOperations() {
    operations.clear(); operations.reserve(pendingOperations.size());
    for (const auto &operation : pendingOperations) operations.push_back(operation.publicState);
}

const AssetImporterDescriptor *AssetDatabase::FindImporter(const std::filesystem::path &path,
                                                           const std::optional<AssetType> type) const {
    const auto extension = Lower(path.extension().string());
    const auto found = std::ranges::find_if(importers, [&](const auto &importer) {
        return (!type || importer.type == *type) &&
               std::ranges::find(importer.extensions, extension) != importer.extensions.end();
    });
    return found == importers.end() ? nullptr : &*found;
}

AssetId AssetDatabase::AllocateId() {
    std::ostringstream id; id << "asset-" << std::hex << std::setw(8) << std::setfill('0') << nextAssetNumber++;
    return id.str();
}

bool AssetDatabase::Save(std::string *error) const {
    if (projectDirectory.empty()) { SetError(error, "Asset database is not open."); return false; }
    const auto path = GetDatabasePath(); const auto temporary = path.string() + ".tmp";
    std::ofstream output(temporary, std::ios::trunc);
    if (!output) { SetError(error, "Could not write asset database."); return false; }
    output << Header << ' ' << CurrentFormatVersion << ' ' << nextAssetNumber << '\n';
    for (const auto &record : assets) {
        output << "ASSET " << std::quoted(record.id) << ' ' << static_cast<int>(record.type) << ' '
               << static_cast<int>(record.state) << ' ' << std::quoted(record.sourcePath.generic_string()) << ' '
               << std::quoted(record.cachePath.generic_string()) << ' ' << std::quoted(record.thumbnailPath.generic_string()) << ' '
               << std::quoted(record.previewPath.generic_string()) << ' ' << std::quoted(record.importerId) << ' '
               << record.importerVersion << ' ' << record.revision << ' ' << record.contentHash << ' '
               << std::quoted(record.error) << ' ' << record.dependencies.size() << ' ' << record.tags.size() << ' '
               << (record.part ? 1 : 0) << '\n';
        for (const auto &dependency : record.dependencies) output << "DEPENDENCY " << std::quoted(dependency) << '\n';
        for (const auto &tag : record.tags) output << "TAG " << std::quoted(tag) << '\n';
        if (record.part) {
            const auto &part = *record.part;
            output << "PART " << std::quoted(part.category) << ' ' << part.physicalSize.x << ' ' << part.physicalSize.y
                   << ' ' << std::quoted(part.collisionShape) << ' ' << part.attachments.size() << ' '
                   << part.compatibilityTags.size() << ' ' << part.configurationLimits.size() << '\n';
            for (const auto &attachment : part.attachments) {
                output << "ATTACHMENT " << std::quoted(attachment.id) << ' ' << std::quoted(attachment.type) << ' '
                       << attachment.localTransform.position.x << ' ' << attachment.localTransform.position.y << ' '
                       << attachment.localTransform.rotationRadians << ' ' << attachment.localTransform.scale.x << ' '
                       << attachment.localTransform.scale.y << ' ' << attachment.multiple << ' ' << attachment.accepts.size();
                for (const auto &accepted : attachment.accepts) output << ' ' << std::quoted(accepted);
                output << '\n';
            }
            for (const auto &tag : part.compatibilityTags) output << "COMPATIBILITY " << std::quoted(tag) << '\n';
            for (const auto &limit : part.configurationLimits)
                output << "LIMIT " << std::quoted(limit.key) << ' ' << limit.minimum << ' ' << limit.maximum << ' '
                       << std::quoted(limit.unit) << '\n';
        }
        output << "ENDASSET\n";
    }
    output.close();
    if (!output) { SetError(error, "Failed while writing asset database."); return false; }
    std::error_code filesystemError;
    std::filesystem::rename(temporary, path, filesystemError);
    if (filesystemError) {
        std::filesystem::remove(path, filesystemError); filesystemError.clear();
        std::filesystem::rename(temporary, path, filesystemError);
    }
    if (filesystemError) { SetError(error, filesystemError.message()); return false; }
    return true;
}

bool AssetDatabase::Load(std::string *error) {
    const auto path = GetDatabasePath();
    if (!std::filesystem::exists(path)) return true;
    std::ifstream input(path);
    std::string header; std::uint32_t version{};
    if (!(input >> header >> version >> nextAssetNumber) || header != Header ||
        version < OldestSupportedFormatVersion || version > CurrentFormatVersion) {
        SetError(error, "Unsupported or invalid asset database header."); return false;
    }
    std::string token;
    while (input >> token) {
        if (token != "ASSET") { SetError(error, "Invalid asset database record."); return false; }
        AssetRecord record; int type{}, state{}; std::string source, cache, thumbnail, preview;
        std::size_t dependencyCount{}, tagCount{}; int hasPart{};
        if (!(input >> std::quoted(record.id) >> type >> state >> std::quoted(source) >> std::quoted(cache))) {
            SetError(error, "Invalid asset record."); return false;
        }
        if (version >= 2) {
            if (!(input >> std::quoted(thumbnail) >> std::quoted(preview) >> std::quoted(record.importerId)
                      >> record.importerVersion >> record.revision >> record.contentHash >> std::quoted(record.error)
                      >> dependencyCount >> tagCount >> hasPart)) {
                SetError(error, "Invalid version-2 asset record."); return false;
            }
        } else {
            if (!(input >> std::quoted(record.importerId) >> record.importerVersion >> record.revision
                      >> record.contentHash >> std::quoted(record.error) >> dependencyCount >> tagCount)) {
                SetError(error, "Invalid legacy asset record."); return false;
            }
        }
        if(type<0||type>int(AssetType::Tileset)||state<0||state>int(AssetState::Failed)){
            SetError(error,"Invalid asset type or state.");return false;
        }
        record.type = static_cast<AssetType>(type); record.state = static_cast<AssetState>(state);
        record.sourcePath = source; record.cachePath = cache; record.thumbnailPath = thumbnail; record.previewPath = preview;
        for (std::size_t index=0; index<dependencyCount; ++index) {
            std::string value; if (!(input >> token >> std::quoted(value)) || token != "DEPENDENCY") return false;
            record.dependencies.push_back(std::move(value));
        }
        for (std::size_t index=0; index<tagCount; ++index) {
            std::string value; if (!(input >> token >> std::quoted(value)) || token != "TAG") return false;
            record.tags.push_back(std::move(value));
        }
        if (hasPart) {
            PartAssetMetadata part; std::size_t attachments{}, compatibility{}, limits{};
            if (!(input >> token >> std::quoted(part.category) >> part.physicalSize.x >> part.physicalSize.y
                      >> std::quoted(part.collisionShape) >> attachments >> compatibility >> limits) || token != "PART") return false;
            for (std::size_t index=0; index<attachments; ++index) {
                PartAttachmentMetadata attachment; std::size_t accepts{};
                if (!(input >> token >> std::quoted(attachment.id) >> std::quoted(attachment.type)
                          >> attachment.localTransform.position.x >> attachment.localTransform.position.y
                          >> attachment.localTransform.rotationRadians >> attachment.localTransform.scale.x
                          >> attachment.localTransform.scale.y >> attachment.multiple >> accepts) || token != "ATTACHMENT") return false;
                for (std::size_t accepted=0; accepted<accepts; ++accepted) { std::string value; input >> std::quoted(value); attachment.accepts.push_back(std::move(value)); }
                part.attachments.push_back(std::move(attachment));
            }
            for (std::size_t index=0; index<compatibility; ++index) { std::string value; if (!(input>>token>>std::quoted(value))||token!="COMPATIBILITY")return false; part.compatibilityTags.push_back(std::move(value)); }
            for (std::size_t index=0; index<limits; ++index) { PartConfigurationLimit limit; if (!(input>>token>>std::quoted(limit.key)>>limit.minimum>>limit.maximum>>std::quoted(limit.unit))||token!="LIMIT")return false; part.configurationLimits.push_back(std::move(limit)); }
            record.part = std::move(part);
        }
        if (!(input >> token) || token != "ENDASSET") { SetError(error, "Asset record terminator is missing."); return false; }
        assets.push_back(std::move(record));
    }
    RefreshMissingStates();
    return true;
}

} // namespace pipeframe::assets
