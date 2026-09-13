#include "ProjectBrowser.h"
namespace pipeframe::editor {
using namespace pipeframe::ui;
ProjectBrowser::ProjectBrowser()  {}
void ProjectBrowser::SetOnNewProject(ActionCallback cb){onNewProject=std::move(cb);InvalidateView();}
void ProjectBrowser::SetOnOpenProject(ActionCallback cb){onOpenProject=std::move(cb);InvalidateView();}
void ProjectBrowser::SetOnOpenRecent(OpenRecentCallback cb){onOpenRecent=std::move(cb);InvalidateView();}
void ProjectBrowser::SetRecentProjects(const std::vector<std::filesystem::path> &paths){recentProjectPaths=paths;InvalidateView();}
void ProjectBrowser::SetMessage(const std::string &text){message=text;InvalidateView();}
View ProjectBrowser::BuildView(){
    std::vector<View> children{views::Text("title","PIPEFRAME | PROJECTS").FitHeight(),
        views::Text("message",message).FitHeight(),
        views::Row("actions",{views::Button("new","NEW PROJECT",onNewProject),views::Button("open","OPEN PROJECT",onOpenProject)}),
        views::Text("recent","RECENT PROJECTS")};
    for(const auto &path:recentProjectPaths) children.push_back(views::Button(path.string(),path.filename().string()+" | "+path.parent_path().string(),
        [this,path]{if(onOpenRecent)onOpenRecent(path);}));
    return views::Scroll("projects",views::Column("content",std::move(children)).Padding(24)).FillHeight();
}
}
