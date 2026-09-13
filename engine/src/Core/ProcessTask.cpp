#include <PipeFrame/Core/ProcessTask.h>
#include <atomic>
#include <chrono>
#include <fstream>
#include <mutex>
#include <thread>
#include <stdexcept>
#ifndef _WIN32
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
extern char **environ;
#endif
namespace pipeframe {
struct ProcessTask::State {
    mutable std::mutex mutex;
    ProcessProgress progress;
    std::atomic<bool> cancel{};
    std::thread worker;
    std::filesystem::path log;
};
ProcessTask::ProcessTask():state(std::make_unique<State>()){}
ProcessTask::~ProcessTask(){Cancel();if(state->worker.joinable())state->worker.join();}
void ProcessTask::Cancel(){state->cancel=true;std::lock_guard lock(state->mutex);state->progress.cancelled=true;}
ProcessProgress ProcessTask::Poll() const {
    std::lock_guard lock(state->mutex);auto result=state->progress;
    std::ifstream input(state->log,std::ios::binary);
    if(input){input.seekg(0,std::ios::end);const auto length=input.tellg();input.seekg(length>65536?length-std::streamoff(65536):std::streampos(0));result.output.assign(std::istreambuf_iterator<char>(input),{});}
    return result;
}
bool ProcessTask::Start(std::vector<ProcessCommand> commands,const std::filesystem::path &directory,
                        const std::filesystem::path &log,std::string &error){
    if(Poll().running){error="A process is already running";return false;}
    if(commands.empty()){error="No build commands";return false;}
    for(const auto &command:commands)if(command.arguments.empty()||command.arguments.front().empty()){error="Empty executable";return false;}
#ifdef _WIN32
    error="Native process execution is not implemented on Windows";return false;
#else
    if(state->worker.joinable())state->worker.join();
    std::error_code ec;std::filesystem::create_directories(log.parent_path(),ec);
    if(ec){error=ec.message();return false;}
    {std::ofstream output(log);if(!output){error="Cannot create build log";return false;}}
    state->cancel=false;
    {std::lock_guard lock(state->mutex);state->log=log;state->progress={.running=true,.stage=commands.front().label};}
    state->worker=std::thread([this,commands=std::move(commands),directory,log]{
        int code=0;bool cancelled=false;
        for(const auto &command:commands){
            if(state->cancel){cancelled=true;break;}
            {std::lock_guard lock(state->mutex);state->progress.stage=command.label;}
            {std::ofstream output(log,std::ios::app);output<<"\n--- "<<command.label<<" ---\n";}
            posix_spawn_file_actions_t actions;posix_spawn_file_actions_init(&actions);
            posix_spawnattr_t attributes;posix_spawnattr_init(&attributes);
            int setup=posix_spawn_file_actions_addchdir_np(&actions,directory.c_str());
            if(!setup)setup=posix_spawn_file_actions_addopen(&actions,STDOUT_FILENO,log.c_str(),O_WRONLY|O_APPEND,0600);
            if(!setup)setup=posix_spawn_file_actions_adddup2(&actions,STDOUT_FILENO,STDERR_FILENO);
            if(!setup)setup=posix_spawnattr_setflags(&attributes,POSIX_SPAWN_SETPGROUP);
            if(!setup)setup=posix_spawnattr_setpgroup(&attributes,0);
            std::vector<char*> argv;for(const auto &arg:command.arguments)argv.push_back(const_cast<char*>(arg.c_str()));argv.push_back(nullptr);
            pid_t pid{};code=setup?setup:posix_spawnp(&pid,argv.front(),&actions,&attributes,argv.data(),environ);
            posix_spawn_file_actions_destroy(&actions);posix_spawnattr_destroy(&attributes);
            if(code)break;
            int status=0;bool terminated=false;auto termination=std::chrono::steady_clock::time_point{};
            for(;;){
                const auto waited=waitpid(pid,&status,WNOHANG);
                if(waited==pid){code=WIFEXITED(status)?WEXITSTATUS(status):128+WTERMSIG(status);break;}
                if(waited<0){if(errno==EINTR)continue;code=errno;break;}
                if(state->cancel){cancelled=true;if(!terminated){kill(-pid,SIGTERM);terminated=true;termination=std::chrono::steady_clock::now();}
                    else if(std::chrono::steady_clock::now()-termination>std::chrono::milliseconds(500))kill(-pid,SIGKILL);}
                std::this_thread::sleep_for(std::chrono::milliseconds(15));
            }
            if(terminated)kill(-pid,SIGKILL);
            if(code||cancelled)break;
        }
        std::lock_guard lock(state->mutex);state->progress.running=false;state->progress.cancelled=cancelled||state->cancel;state->progress.exitCode=code;
    });
    error.clear();return true;
#endif
}
}
