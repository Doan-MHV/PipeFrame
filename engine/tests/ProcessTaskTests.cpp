#include <PipeFrame/Core/ProcessTask.h>
#include <chrono>
#include <fstream>
#include <iostream>
#include <thread>
#include <cstdlib>
#ifndef _WIN32
#include <signal.h>
#endif
using namespace pipeframe;
void Require(bool value,const char *message){if(!value){std::cerr<<message<<'\n';std::exit(1);}}
ProcessProgress Wait(ProcessTask &task){
    const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(10);
    for(;;){auto result=task.Poll();if(!result.running)return result;Require(std::chrono::steady_clock::now()<deadline,"Process timed out");std::this_thread::sleep_for(std::chrono::milliseconds(10));}
}
int main(int argc,char **argv){
    if(argc>1){
        if(std::string(argv[1])=="--hold"){
#ifndef _WIN32
            signal(SIGTERM,SIG_IGN);
#endif
            std::cout<<"ready\n"<<std::flush;for(;;)std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
        std::cout<<(argc>2?argv[2]:"output")<<'\n';return std::stoi(argv[1]);
    }
    const auto root=std::filesystem::temp_directory_path()/("pipeframe process test "+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(root);const auto self=std::filesystem::absolute(argv[0]).string();ProcessTask task;std::string error;
    Require(task.Start({{"first",{self,"0","literal ; $(not-a-command)"}},{"second",{self,"0","second stage"}}},root,root/"log.txt",error),error.c_str());
    auto result=Wait(task);Require(result.exitCode==0&&result.output.find("literal ; $(not-a-command)")!=std::string::npos&&result.output.find("second stage")!=std::string::npos,"Arguments or sequential output lost");
    Require(task.Start({{"failure",{self,"7"}},{"must skip",{self,"0","NEVER RUN"}}},root,root/"log.txt",error),error.c_str());
    result=Wait(task);Require(result.exitCode==7&&result.output.find("NEVER RUN")==std::string::npos,"Failure must stop subsequent stages");
    Require(task.Start({{"hold",{self,"--hold"}}},root,root/"log.txt",error),error.c_str());
    Require(!task.Start({{"duplicate",{self,"0"}}},root,root/"other.txt",error),"Concurrent start must fail");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));task.Cancel();result=Wait(task);Require(result.cancelled,"Cancellation must be reported");
    Require(task.Start({{"missing",{(root/"missing-program").string()}}},root,root/"log.txt",error),error.c_str());
    result=Wait(task);Require(result.exitCode!=0,"Missing executable must fail");
    std::filesystem::remove_all(root);std::cout<<"Process output, argv preservation, stage failure, cancellation and retry passed\n";
}
