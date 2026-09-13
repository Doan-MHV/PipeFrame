#include "ProjectScaffolder.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <ranges>
#include <sstream>
#include <system_error>
#include <unordered_set>

namespace pipeframe::editor {
namespace {
bool Fail(std::string *error,const std::string &message){if(error)*error=message;return false;}
bool SafeName(const std::string &name){
    return !name.empty()&&std::isalpha(static_cast<unsigned char>(name.front()))&&
           std::ranges::all_of(name,[](unsigned char c){return std::isalnum(c)||c=='_';});
}
bool Write(const std::filesystem::path &path,const std::string &contents,std::string *error){
    std::error_code ec;std::filesystem::create_directories(path.parent_path(),ec);
    if(ec)return Fail(error,"Could not create "+path.parent_path().string()+": "+ec.message());
    if(std::filesystem::exists(path))return true;
    std::ofstream output(path);if(!output)return Fail(error,"Could not write "+path.string());
    output<<contents;return output.good()||Fail(error,"Failed while writing "+path.string());
}
bool AppendUnique(const std::filesystem::path &path,const std::string &line,std::string *error){
    std::ifstream input(path);const std::string existing((std::istreambuf_iterator<char>(input)),{});
    if(existing.find(line)!=std::string::npos)return true;
    std::ofstream output(path,std::ios::app);if(!output)return Fail(error,"Could not update "+path.string());
    output<<line<<'\n';return output.good()||Fail(error,"Failed while updating "+path.string());
}
std::string Identifier(std::string value){
    for(char &c:value)if(!std::isalnum(static_cast<unsigned char>(c)))c='_';
    if(value.empty()||!std::isalpha(static_cast<unsigned char>(value.front())))value="Project_"+value;
    return value;
}
}

bool ProjectScaffolder::CreateStandardStructure(const std::filesystem::path &root,
                                                const std::string &projectName,
                                                std::string *error){
    const std::string id=Identifier(projectName);
    for(const char *folder:{"Config","Config/Components","Assets/Audio","Assets/Materials","Assets/Models","Assets/Parts",
        "Assets/Prefabs","Assets/Shaders","Assets/Textures","Scenes","Source/Entities","Source/Behaviours","Source/Components","Source/Systems",
        "Source/Runtime","Source/Editor","Tests",".pipeframe/cache"}){
        std::error_code ec;std::filesystem::create_directories(root/folder,ec);
        if(ec)return Fail(error,"Could not create "+(root/folder).string()+": "+ec.message());
    }
    if(!Write(root/"Config/ProjectSettings.pipeframe","PIPEFRAME_PROJECT_SETTINGS 1\nunits meters\nfixedStep 0.016666667\n",error)||
       !Write(root/"Config/Input.pipeframe","PIPEFRAME_INPUT 1\n",error)||
       !Write(root/"Config/Physics.pipeframe","PIPEFRAME_PHYSICS 1\ngravity 0 0\n",error)||
       !Write(root/"Config/Modules.pfconfig","PIPEFRAME_MODULES 1\nComponents\nSystems\nRuntime\nEditor\n",error)||
       !Write(root/"CMakeLists.txt","cmake_minimum_required(VERSION 3.25)\nproject("+id+" LANGUAGES CXX)\nif(NOT TARGET PipeFrame::Engine)\n  if(DEFINED PIPEFRAME_ENGINE_LIBRARY AND DEFINED PIPEFRAME_SDK_ROOT)\n    add_library(PipeFrame::Engine SHARED IMPORTED)\n    set_target_properties(PipeFrame::Engine PROPERTIES IMPORTED_LOCATION \"${PIPEFRAME_ENGINE_LIBRARY}\" INTERFACE_INCLUDE_DIRECTORIES \"${PIPEFRAME_SDK_ROOT}/engine/include\")\n  elseif(DEFINED PIPEFRAME_SDK_ROOT)\n    add_subdirectory(\"${PIPEFRAME_SDK_ROOT}/engine\" \"${CMAKE_BINARY_DIR}/PipeFrameEngine\")\n  else()\n    find_package(PipeFrame CONFIG REQUIRED)\n  endif()\nendif()\nfile(GLOB_RECURSE PROJECT_SOURCES CONFIGURE_DEPENDS Source/*.cpp Source/*.h Source/*.hpp)\nadd_library("+id+"Runtime SHARED ${PROJECT_SOURCES})\ntarget_link_libraries("+id+"Runtime PRIVATE PipeFrame::Engine)\ntarget_include_directories("+id+"Runtime PRIVATE Source)\ntarget_compile_definitions("+id+"Runtime PRIVATE PIPEFRAME_BUILDING_PROJECT_RUNTIME)\ntarget_compile_features("+id+"Runtime PRIVATE cxx_std_20)\nset_target_properties("+id+"Runtime PROPERTIES PREFIX \"\" LIBRARY_OUTPUT_DIRECTORY \"${CMAKE_CURRENT_SOURCE_DIR}/Build/$<CONFIG>\" RUNTIME_OUTPUT_DIRECTORY \"${CMAKE_CURRENT_SOURCE_DIR}/Build/$<CONFIG>\")\n",error)||
       !Write(root/"Source/Components/README.md","# Components\n\nGenerate a Component through the editor. Keep data and static Schema() together.\nUse Editable, ReadOnly and Validate; registration consumes the schema.\nPrefer descriptive names ending in Component.\n",error)||
       !Write(root/"Source/Systems/README.md","# Systems\n\nDerive batch rules from FixedUpdateSystem and own/register them in your world.\nUse RuntimeWorld::AddSystem or PhysicsWorld steps to declare execution order.\nGenerating a file does not automatically schedule a system.\n",error)||
       !Write(root/"Source/Editor/README.md","# Editor and UI\n\nBuild panels with ViewPanel, View and StatefulView. Use SchemaBrush for map tools.\nNative windows, widgets and SFML belong to the engine backend.\n",error)||
       !Write(root/"Source/Runtime"/(id+"Runtime.h"),"#pragma once\n#include <PipeFrame/Project/SceneProjectRuntime.h>\n#include \"GeneratedRegistration.h\"\nnamespace "+id+" { class Runtime final : public pipeframe::SceneProjectRuntime { public: Runtime():SceneProjectRuntime(\""+projectName+"\"){pipeframe_generated::Register(*this);} }; }\n",error)||
       !Write(root/"Source/Runtime"/(id+"Runtime.cpp"),"#include \""+id+"Runtime.h\"\n",error)||
       !Write(root/"Source/Plugin.cpp","#include \"Runtime/"+id+"Runtime.h\"\nextern \"C\" PIPEFRAME_PROJECT_RUNTIME_EXPORT pipeframe::ProjectRuntime *PipeFrameCreateProjectRuntime(){return new "+id+"::Runtime;}\nextern \"C\" PIPEFRAME_PROJECT_RUNTIME_EXPORT void PipeFrameDestroyProjectRuntime(pipeframe::ProjectRuntime *runtime){delete runtime;}\n",error)||
       !Write(root/"Tests/README.md","# Project tests\n\nAdd executable tests for your project rules and register them in CMake with add_test.\nUse checks that remain active in Release. SDK examples: docs/tutorials/blank-project.\n",error))return false;
    for(const char *folder:{"Audio","Materials","Models","Parts","Prefabs","Shaders","Textures"})
        if(!Write(root/"Assets"/folder/".pipeframekeep","",error))return false;
    if(!Write(root/"Config/Build.pipeframe","PIPEFRAME_BUILD 1\n\""+id+"Runtime\" standalone\n",error))return false;
    if(!RefreshGeneratedRegistration(root,error))return false;
    return CreateObjectType(root,"Object",
        {"pipeframe.transform2d","pipeframe.identity"},error);
}

bool ProjectScaffolder::AddModule(const std::filesystem::path &root,GeneratedModuleKind kind,
                                  const std::string &name,std::string *error){
    if(!SafeName(name))return Fail(error,"Names must start with a letter and contain only letters, digits, or underscore.");
    std::filesystem::path path;std::string contents;
    switch(kind){
    case GeneratedModuleKind::Brush:
        path=root/"Source/Editor"/(name+".h");
        contents="#pragma once\n#include <PipeFrame/Editor/BrushTool.h>\nclass "+name+" final : public pipeframe::SchemaBrush<"+name+"> {\npublic:\n    double amount{1};\n    static auto Schema(){return pipeframe::ComponentSchema<"+name+">(\"project."+name+"\",\""+name+"\")\n        .Editable({.key=\"amount\",.displayName=\"Amount\",.kind=pipeframe::PropertyKind::Number,.defaultValue=1.0,.minimum=0,.maximum=1},&"+name+"::amount); }\n    std::string_view DataTarget()const override{return \"project."+name+"\";}\n    double PaintData(pipeframe::GridCoordinate,double)const override{return amount;}\n};\n";break;
    case GeneratedModuleKind::Entity:
        path=root/"Source/Entities"/(name+".h");
        contents="#pragma once\n#include <PipeFrame/Entities/EntityArchetype.h>\n#include <PipeFrame/Components/Transform2DComponent.h>\nclass "+name+" final : public pipeframe::EntityArchetype {\nprotected:\n    void Build(pipeframe::ecs::World &world, pipeframe::ecs::Entity entity) const override {\n        // Compose this object with world.Add<YourComponent>(entity).\n        world.Add<pipeframe::Transform2DComponent>(entity);\n    }\n};\n";break;
    case GeneratedModuleKind::Behaviour:
        path=root/"Source/Behaviours"/(name+".h");
        contents="#pragma once\n#include <PipeFrame/ECS/Scene.h>\nclass "+name+" final : public pipeframe::Behaviour {\npublic:\n    void Start() override {}\n    void Update(float deltaTime) override { (void)deltaTime; }\n    void FixedUpdate(float fixedDeltaTime) override { (void)fixedDeltaTime; }\n};\n";break;
    case GeneratedModuleKind::Component:
        path=root/"Source/Components"/(name+".h");
        contents="#pragma once\n#include <PipeFrame/Project/ComponentSchema.h>\nstruct "+name+" {\n    // Unlisted members stay runtime-only. Expose members with Editable or ReadOnly.\n    static auto Schema() {\n        return pipeframe::ComponentSchema<"+name+">(\"project."+name+"\", \""+name+"\");\n        // Add field minimum/maximum constraints, or Validate(message, predicate).\n    }\n};\n";break;
    case GeneratedModuleKind::System:
        path=root/"Source/Systems"/(name+".h");
        contents="#pragma once\n#include <PipeFrame/Simulation/System.h>\nclass "+name+" final : public pipeframe::FixedUpdateSystem<void> { public: std::string_view GetSystemId()const override{return \"project."+name+"\";} void Update(float fixedDeltaTime)override{(void)fixedDeltaTime;} };\n";break;
    case GeneratedModuleKind::Runtime:
        path=root/"Source/Runtime"/(name+".h");
        contents="#pragma once\n#include <PipeFrame/Project/ProjectRuntime.h>\nclass "+name+" final : public pipeframe::RuntimeModule { public: std::string_view GetModuleId()const override{return \"project."+name+"\";} bool Load(const pipeframe::ProjectRuntimeContext&,std::string &error)override{error.clear();return true;} void Unload()override{} };\n";break;
    case GeneratedModuleKind::EditorExtension:
        path=root/"Source/Editor"/(name+".h");
        contents="#pragma once\n#include <PipeFrame/Editor/EditorTool.h>\nclass "+name+" final : public pipeframe::EditorTool { public: std::string_view GetToolId()const override{return \"project."+name+"\";} void SetEnabled(bool value)override{enabled=value;} bool IsEnabled()const override{return enabled;} private: bool enabled{}; };\n";break;
    }
    if(!Write(path,contents,error))return false;
    if(kind==GeneratedModuleKind::Component &&
       !Write(root/"Config/Components"/(name+".pfcomponent"),
              "PIPEFRAME_COMPONENT 1\nproject."+name+"\n"+name+"\n",error))return false;
    const char *kindName=kind==GeneratedModuleKind::Brush?"Brush":kind==GeneratedModuleKind::Entity?"Entity":
        kind==GeneratedModuleKind::Behaviour?"Behaviour":kind==GeneratedModuleKind::Component?"Component":
        kind==GeneratedModuleKind::System?"System":kind==GeneratedModuleKind::Runtime?"Runtime":"EditorExtension";
    if(!AppendUnique(root/"Config/Modules.pfconfig",std::string(kindName)+" "+name,error))return false;
    return RefreshGeneratedRegistration(root,error);
}

bool ProjectScaffolder::RefreshGeneratedRegistration(const std::filesystem::path &root,std::string *error) {
    const auto path=root/"Source/Runtime/GeneratedRegistration.h";
    const std::string marker="// Generated by PipeFrame. Edit component/behaviour headers instead.";
    if(std::filesystem::exists(path)) {
        std::ifstream previous(path);std::string first;std::getline(previous,first);
        if(first!=marker)return Fail(error,"Refusing to overwrite user-owned GeneratedRegistration.h");
    }
    std::ifstream modules(root/"Config/Modules.pfconfig");
    if(!modules)return Fail(error,"Cannot read module registrations");
    std::ostringstream includes,registrations;std::string line;
    std::unordered_set<std::string> names;
    while(std::getline(modules,line)) {
        std::istringstream row(line);std::string kind,name;row>>kind>>name;
        if(kind!="Component" && kind!="Behaviour" && kind!="Entity" && kind!="Brush")continue;
        if(!SafeName(name)||!names.insert(name).second)return Fail(error,"Invalid or duplicate generated module: "+name);
        const std::string folder=kind=="Brush"?"Editor":kind=="Component"?"Components":kind=="Entity"?"Entities":"Behaviours";
        includes<<"#include \""<<folder<<"/"<<name<<".h\"\n";
        if(kind=="Brush")registrations<<"    runtime.template RegisterBrush<"<<name<<">();\n";
        else if(kind=="Component")registrations<<"    runtime.template RegisterComponent<"<<name<<">();\n";
        else if(kind=="Entity")registrations<<"    runtime.template RegisterEntity<"<<name<<">({\"project."<<name<<"\", \""<<name<<"\", {}, {pipeframe::Transform2DComponentTypeId}});\n";
        else registrations<<"    runtime.template RegisterBehaviour<"<<name<<">(\"project."<<name<<"\", \""<<name<<"\");\n";
    }
    std::ofstream output(path,std::ios::trunc);
    if(!output)return Fail(error,"Cannot write generated registration");
    output<<marker<<"\n#pragma once\n"<<includes.str()<<"namespace pipeframe_generated {\ntemplate<class Runtime> void Register(Runtime &runtime) {\n"
          <<registrations.str()<<"    (void)runtime;\n}\n}\n";
    return output.good()||Fail(error,"Failed to write generated registration");
}

bool ProjectScaffolder::CreateObjectType(const std::filesystem::path &root,const std::string &name,
                                         const std::vector<std::string> &componentIds,std::string *error){
    if(!SafeName(name))return Fail(error,"Object type names must use a valid identifier.");
    std::ostringstream out;out<<"PIPEFRAME_OBJECT_TYPE 1\n"<<name<<'\n';
    for(const auto &id:componentIds){if(id.empty())return Fail(error,"Component IDs cannot be empty.");out<<id<<'\n';}
    return Write(root/"Assets/Prefabs"/(name+".pftype"),out.str(),error);
}

std::vector<ProjectValidationIssue> ProjectScaffolder::Validate(const std::filesystem::path &root){
    std::vector<ProjectValidationIssue> issues;
    for(const char *path:{"project.pipeframe","CMakeLists.txt","Config/ProjectSettings.pipeframe","Config/Input.pipeframe",
        "Config/Physics.pipeframe","Config/Modules.pfconfig","Scenes","Source/Entities","Source/Behaviours","Source/Components","Source/Systems","Source/Runtime",
        "Source/Editor","Source/Plugin.cpp","Tests"})if(!std::filesystem::exists(root/path))issues.push_back({path,"Required project entry is missing."});
    const auto source=root/"Source";if(std::filesystem::exists(source))for(const auto &entry:std::filesystem::recursive_directory_iterator(source)){
        if(!entry.is_regular_file()||(entry.path().extension()!=".h"&&entry.path().extension()!=".cpp"))continue;
        std::ifstream input(entry.path());const std::string contents((std::istreambuf_iterator<char>(input)),{});
        if(contents.find("#include <SFML")!=std::string::npos||contents.find("#include \"SFML")!=std::string::npos||
           contents.find("sf::")!=std::string::npos||contents.find("PipeFrame/Backend/")!=std::string::npos)
            issues.push_back({std::filesystem::relative(entry.path(),root),"Project source crosses the PipeFrame backend boundary."});
        if(contents.find("SimulationWorkbench")!=std::string::npos)
            issues.push_back({std::filesystem::relative(entry.path(),root),"Project source depends on editor internals."});
    }
    const auto modulesPath=root/"Config/Modules.pfconfig";
    if(std::filesystem::is_regular_file(modulesPath)){
        std::ifstream modules(modulesPath);std::string line;
        while(std::getline(modules,line)){
            std::istringstream row(line);std::string kind,name;row>>kind>>name;
            if(name.empty())continue;
            std::filesystem::path expected;
            if(kind=="Entity")expected=root/"Source/Entities"/(name+".h");
            else if(kind=="Behaviour")expected=root/"Source/Behaviours"/(name+".h");
            else if(kind=="Component")expected=root/"Source/Components"/(name+".h");
            else if(kind=="System")expected=root/"Source/Systems"/(name+".h");
            else if(kind=="Runtime")expected=root/"Source/Runtime"/(name+".h");
            else if(kind=="EditorExtension"||kind=="Brush")expected=root/"Source/Editor"/(name+".h");
            else continue;
            if(!std::filesystem::is_regular_file(expected))
                issues.push_back({std::filesystem::relative(expected,root),"Registered module source is missing."});
        }
    }
    const auto prefabs=root/"Assets/Prefabs";
    if(std::filesystem::is_directory(prefabs))for(const auto &entry:std::filesystem::directory_iterator(prefabs)){
        if(!entry.is_regular_file()||entry.path().extension()!=".pftype")continue;
        std::ifstream input(entry.path());std::string header,name,line;
        std::getline(input,header);std::getline(input,name);
        if(header!="PIPEFRAME_OBJECT_TYPE 1"||name.empty()){
            issues.push_back({std::filesystem::relative(entry.path(),root),"Object type header or display name is invalid."});continue;
        }
        std::unordered_set<std::string> components;
        while(std::getline(input,line))if(line.empty()||!components.insert(line).second)
            issues.push_back({std::filesystem::relative(entry.path(),root),"Object type contains an empty or duplicate component ID."});
        if(components.empty())issues.push_back({std::filesystem::relative(entry.path(),root),"Object type must contain at least one component."});
    }
    return issues;
}
} // namespace pipeframe::editor
