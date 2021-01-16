#include <awesome/displayserver.h>

#include <string>
#include <csignal>
#include <unistd.h>

#include <getopt.h>

#define DEFAULT_INIT_CMD "/Users/ian/projects/awesome-desktop/cmake-build-debug/awesome-desktop"

using namespace Awesome;
using namespace Geek;
using namespace std;

DisplayServer* g_displayServer = nullptr;

static const struct option g_options[] =
{
    { "config",    required_argument, nullptr, 'c' },
    { "init",    required_argument, nullptr, 'i' },
    { nullptr,      0,                 nullptr, 0 }
};

void startClient(const string& command, char* const* envp);

static void sigintHandler(int sig, siginfo_t* siginfo, void* context)
{
    if (g_displayServer != nullptr)
    {
        g_displayServer->quit();
    }
}

int main(int argc, char** argv, char* envp[])
{
    struct sigaction act{};
    memset (&act, 0, sizeof(act));
    act.sa_sigaction = sigintHandler;
    act.sa_flags = SA_SIGINFO;
    sigaction(SIGINT, &act, nullptr);

    g_displayServer = new DisplayServer();

    string configFile = "/usr/local/etc/awesome/awesome.yaml";
    vector<string> initCmds;

    while (true)
    {
        int c = getopt_long(
            argc,
            argv,
            "+ci",
            g_options,
            nullptr);

        if (c == -1)
        {
            break;
        }

        switch (c)
        {
            case 'c':
                configFile = string(optarg);
                break;

            case 'i':
                initCmds.emplace_back(optarg);
                break;

            default:
                exit(1);
        }
    }

    bool res;
    res = g_displayServer->loadConfig(configFile);
    if (!res)
    {
        delete g_displayServer;
        return 1;
    }

    res = g_displayServer->init();
    if (!res)
    {
        delete g_displayServer;
        return 1;
    }

    if (initCmds.empty())
    {
        if (g_displayServer->getConfig()["startup"])
        {
            auto startup = g_displayServer->getConfig()["startup"];
            if (startup["commands"])
            {
                for (auto command : startup["commands"])
                {
                    initCmds.push_back(command.as<string>());
                }
            }
        }

        // Still empty?
        if (initCmds.empty())
        {
            initCmds.emplace_back(DEFAULT_INIT_CMD);
        }
    }

    for (const string& initCmd : initCmds)
    {
        startClient(initCmd, envp);
    }

    g_displayServer->main();

    delete g_displayServer;
}

void startClient(const string& command, char* const* envp)
{
    int envc = 0;
    while (envp[envc++] != nullptr);
    envc--;

    const char* env[envc + 1];
    int i;
    int envpos = 0;
    for (i = 0; i < envc; i++)
    {
        bool skip = false;
        string e = envp[i];
        int pos = e.find('=');
        if (pos != string::npos)
        {
            string name = e.substr(0, pos);
            printf("%s: %s\n", e.c_str(), name.c_str());
            if (name == "FRONTIER_ENGINE")
            {
                skip = true;
            }
        }
        if (!skip)
        {
            env[envpos++] = envp[i];
        }
    }

    env[envpos++] = "FRONTIER_ENGINE=Awesome";
    env[envpos] = nullptr;

    int pid = fork();
    if (pid == 0)
    {
        string arg0 = command;
        int pos = command.rfind('/');
        if (pos != string::npos)
        {
            arg0 = command.substr(pos);
        }

        execle(
            command.c_str(),
            arg0.c_str(),
            nullptr,
            env);

        exit(0);
    }
}
