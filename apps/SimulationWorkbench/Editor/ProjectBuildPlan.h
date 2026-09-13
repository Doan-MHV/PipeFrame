#pragma once
#include <PipeFrame/Core/ProcessTask.h>
#include <fstream>
#include <iomanip>
namespace pipeframe::editor {
struct ProjectBuildPlan {
    std::filesystem::path project,library;
    std::vector<ProcessCommand> commands;
    static ProjectBuildPlan Create(const std::filesystem::path &root,const std::filesystem::path &sdk,
                                  const std::filesystem::path &hostBuild,const std::string &configuration,
                                  const std::string &cmake,const std::filesystem::path &engineLibrary) {
        std::ifstream input(root/"Config/Build.pipeframe");std::string magic,target,mode;int version{};
        if(!(input>>magic>>version>>std::quoted(target)>>mode)||magic!="PIPEFRAME_BUILD"||version!=1||target.empty()||
           (mode!="workspace"&&mode!="standalone"))throw std::runtime_error("Missing or invalid Config/Build.pipeframe");
        ProjectBuildPlan plan;plan.project=root;
        const auto build=mode=="workspace"?hostBuild:root/".pipeframe/build"/configuration;
        const auto source=mode=="workspace"?sdk:root;
        ProcessCommand configure{"Configure",{cmake,"-S",source.string(),"-B",build.string(),"-DCMAKE_BUILD_TYPE="+configuration}};
        if(mode=="standalone"){
            configure.arguments.push_back("-DPIPEFRAME_SDK_ROOT="+sdk.string());
            configure.arguments.push_back("-DPIPEFRAME_ENGINE_LIBRARY="+engineLibrary.string());
            configure.arguments.push_back("-DBUILD_TESTING=OFF");
        }
        plan.commands.push_back(std::move(configure));
        plan.commands.push_back({"Build",{cmake,"--build",build.string(),"--config",configuration,"--target",target,"--parallel","4"}});
#ifdef __APPLE__
        const auto suffix=".dylib";
#elif defined(_WIN32)
        const auto suffix=".dll";
#else
        const auto suffix=".so";
#endif
        plan.library=std::filesystem::path("Build")/configuration/(target+suffix);return plan;
    }
};
}
