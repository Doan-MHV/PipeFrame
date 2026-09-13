#pragma once
#include <PipeFrame/Environment/VisualAssets2D.h>
#include <PipeFrame/Project/AssetDatabase.h>
#include <fstream>
#include <PipeFrame/Resources/ImageData.h>
#include <sstream>
#include <variant>

namespace pipeframe {
class VisualAssetEditor {
public:
    bool IsOpen()const{return !id.empty();}
    bool IsDirty()const{return IsOpen()&&(!saved||*saved!=cursor);}
    const std::string &AssetId()const{return id;}
    const std::string &LastError()const{return error;}
    assets::AssetType Type()const{return std::holds_alternative<Material2D>(value)?assets::AssetType::Material:assets::AssetType::Tileset;}
    SceneComponentTypeDescriptor Schema()const{return std::visit([](const auto &v){return std::decay_t<decltype(v)>::Schema().Describe();},value);}
    PropertyMap Values()const{return std::visit([](const auto &v){return std::decay_t<decltype(v)>::Schema().Serialize(v).properties;},value);}
    bool Close(){if(IsDirty())return Fail("Save the visual asset before closing");id.clear();database=nullptr;history.clear();cursor=0;saved=0;return true;}
    bool Open(assets::AssetDatabase &db,const std::string &assetId){
        if(IsDirty())return Fail("Save the visual asset before opening another");
        const auto *record=db.Find(assetId);
        if(!record||(record->type!=assets::AssetType::Material&&record->type!=assets::AssetType::Tileset))return Fail("Select a material or tileset");
        const auto path=db.GetProjectDirectory()/record->sourcePath;
        std::ifstream input(path);std::ostringstream bytes;bytes<<input.rdbuf();if(!input)return Fail("Cannot read visual asset");
        auto next=value;if(!Decode(bytes.str(),record->type,next))return false;
        value=std::move(next);database=&db;id=assetId;source=record->sourcePath;root=db.GetProjectDirectory();baseline=bytes.str();
        history={baseline};cursor=0;saved=0;error.clear();return true;
    }
    bool Create(assets::AssetDatabase &db,assets::AssetType type){
        if(db.HasActiveImport())return Fail("Wait for the active import before creating an asset");
        if(IsDirty())return Fail("Save the visual asset first");
        if(type!=assets::AssetType::Material&&type!=assets::AssetType::Tileset)return Fail("Unsupported visual asset type");
        const bool material=type==assets::AssetType::Material;
        const auto folder=db.GetProjectDirectory()/"Assets"/(material?"Materials":"Tilesets");
        std::error_code ec;std::filesystem::create_directories(folder,ec);if(ec)return Fail(ec.message());
        const std::string name=material?"Material":"Tileset",extension=material?".pfmat":".pftileset";
        auto path=folder/(name+extension);for(int n=2;std::filesystem::exists(path);++n)path=folder/(name+std::to_string(n)+extension);
        {std::ofstream output(path);if(material)SaveVisualAsset(Material2D{},output);else SaveVisualAsset(Tileset2D{},output);if(!output)return Fail("Cannot create asset");}
        auto created=db.ImportNow({path},&error);return created&&Open(db,*created);
    }
    bool Set(const std::string &key,const PropertyValue &property){
        if(!IsOpen())return Fail("Open an asset first");
        auto candidate=value;
        const auto schema=Schema();const auto found=std::ranges::find(schema.properties,key,&PropertyDescriptor::key);
        if(found==schema.properties.end())return Fail("Unknown asset field");
        if(const auto *ref=std::get_if<AssetReference>(&property);ref&&!ref->assetId.empty()){
            const auto *record=database->Find(ref->assetId);
            if(!record||record->state!=assets::AssetState::Ready||found->editorHint!="asset:"+std::string(assets::ToString(record->type)))return Fail("Choose a ready asset of the required type");
        }
        if(!std::visit([&](auto &v){return std::decay_t<decltype(v)>::Schema().Apply(v,{{key,property}},error);},candidate))return false;
        value=std::move(candidate);auto contents=Encode();if(contents==history[cursor])return true;
        if(saved&&*saved>cursor)saved.reset();history.resize(cursor+1);history.push_back(std::move(contents));++cursor;error.clear();return true;
    }
    bool CanUndo()const{return cursor>0;}
    bool CanRedo()const{return cursor+1<history.size();}
    bool Undo(){if(!CanUndo())return false;--cursor;return Decode(history[cursor],Type(),value);}
    bool Redo(){if(!CanRedo())return false;++cursor;return Decode(history[cursor],Type(),value);}
    bool ValidateReferences(){
        const auto textureReady=[&](const AssetReference &ref, ImageData &image){
            const auto *record=database->Find(ref.assetId);
            if(!record||record->type!=assets::AssetType::Texture||record->state!=assets::AssetState::Ready)return Fail("Choose a ready texture");
            return LoadImageData(root/record->cachePath.parent_path()/"image.png",image,error);
        };
        if(const auto *material=std::get_if<Material2D>(&value)){
            if(material->texture.assetId.empty())return true;
            ImageData image;return textureReady(material->texture,image);
        }
        const auto &atlas=std::get<Tileset2D>(value);
        if(atlas.material.assetId.empty())return Fail("Choose a textured material for the tileset");
        const auto *record=database->Find(atlas.material.assetId);
        if(!record||record->type!=assets::AssetType::Material||record->state!=assets::AssetState::Ready)return Fail("Choose a ready material");
        std::ifstream input(root/record->sourcePath);auto material=LoadVisualAsset<Material2D>(input,error);
        if(!material)return false;
        ImageData image;if(!textureReady(material->texture,image))return false;
        return atlas.Fits(image.Size())||Fail("Atlas exceeds texture dimensions; check tile size, rows, columns, margin and spacing");
    }
    bool Save(){
        if(!IsOpen()||!database||database->GetProjectDirectory()!=root)return Fail("Open an asset in this project first");
        if(database->HasActiveImport())return Fail("Wait for the active import before saving");
        const auto *record=database->Find(id);if(!record||record->sourcePath!=source)return Fail("Asset moved; reopen before saving");
        std::ifstream input(root/source);std::ostringstream contents;contents<<input.rdbuf();
        if(!input||contents.str()!=baseline)return Fail("Asset changed outside editor; reopen before saving");
        if(!ValidateReferences())return false;
        if(!IsDirty())return true;
        auto bytes=Encode();const auto temporary=(root/source).string()+".pipeframe-saving";
        if(std::filesystem::exists(temporary))return Fail("Save temporary file already exists");
        std::error_code ec;
        {std::ofstream output(temporary);output<<bytes;output.close();if(!output){std::filesystem::remove(temporary,ec);return Fail("Cannot save visual asset");}}
        std::filesystem::rename(temporary,root/source,ec);if(ec){std::filesystem::remove(temporary,ec);return Fail("Cannot replace asset source");}
        baseline=bytes;saved.reset();if(!database->Reimport(id,&error))return false;saved=cursor;error.clear();return true;
    }
private:
    using Value=std::variant<Material2D,Tileset2D>;
    bool Decode(const std::string &bytes,assets::AssetType type,Value &destination){
        std::istringstream input(bytes);
        if(type==assets::AssetType::Material){auto v=LoadVisualAsset<Material2D>(input,error);if(!v)return false;destination=*v;}
        else {auto v=LoadVisualAsset<Tileset2D>(input,error);if(!v)return false;destination=*v;}return true;
    }
    std::string Encode()const{std::ostringstream output;std::visit([&](const auto &v){SaveVisualAsset(v,output);},value);return output.str();}
    bool Fail(std::string message){error=std::move(message);return false;}
    Value value;assets::AssetDatabase *database{};std::filesystem::path root,source;std::string id,baseline,error;
    std::vector<std::string> history;std::size_t cursor{};std::optional<std::size_t> saved{0};
};
}
