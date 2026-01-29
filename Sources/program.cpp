/*#include "program.h"
#include <iostream>

#include "Logger.h"
#include "Comunicator.h"
#include "MessageManager.h"
#include "Utils.h"
#include "RequestManager.h"
#include "Chessboard.h"

#include <memory>

class Game
{
    Game()
    {

    }

    void MainLoop()
    {

    }
};

int main()
{
    /*std::vector<std::string> v = {"","-5"," ","99999999999999999999999999999999999","83", "a", "ahojda", "5 0"};

    for (std::string s : v)
    {
        std::cout << Utils::IsUint(s) << std::endl;
    }

    Logger::init("xd2.log");

    std::unique_ptr<Comunicator> comunicator = std::make_unique<Comunicator>(7890);
    std::unique_ptr<MessageManager> messageManager = std::make_unique<MessageManager>();
    messageManager->ConnectToComunicator(*comunicator, 2000, 3);

    std::this_thread::sleep_for(std::chrono::seconds(600));

    Logger::terminate();
    */
/*
    CheckersBoard cb;

    while (!cb.isGameOver())
    {
        cb.print();
        int r1, c1, r2, c2;
        std::cout << "Zadej tah (r1 c1 r2 c2): ";
        if (!(std::cin >> r1 >> c1 >> r2 >> c2)) break;
        if (!cb.movePiece({r1, c1}, {r2, c2})) std::cout << "Neplatny tah!\n";
    }
    std::cout << "Konec hry!\n";

    return 0;
    
}

void ForcedExit()
{
    exit(1);
}*/
#include "program.h"
#include "Logger.h"
#include "Comunicator.h"
#include "MessageManager.h"
#include "RequestManager.h"
#include "Client.h"
#include "Opcode.h"
#include "GameLobby.h"
#include "argparse.hpp"

#include <iostream>
#include <csignal>
#include <thread>
#include <chrono>
#include <arpa/inet.h>
#include <memory>
#include <iostream>
#include <string>
#include <arpa/inet.h>
#include <netdb.h>
#include <cstring>
#include <ranges>
#include <numeric>

const std::string APP_NAME = "UPS_SERVR";

class Program
{ };

void ForcedExit()
{
    exit(1);
}
// Globální pøíznak pro ukonèení aplikace (napø. pøes Ctrl+C)

std::atomic<bool> g_running(true);
void signalHandler(int s)
{
    g_running = false;
}

void onIncomingRequest(IncomingRequest req)
{
    // Zde server zpracovává požadavky od Python klienta
    std::cout << "[Server] Prijat pozadavek z Python klienta pres RequestManager." << std::endl;

}

bool is_valid_host(const std::string& ip)
{
    if (ip.empty()) return false;
    struct addrinfo hints, * res;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(ip.c_str(), nullptr, &hints, &res) == 0)
    {
        freeaddrinfo(res);
        return true;
    }
    return false;
}

std::tuple<std::string, uint32_t, uint32_t, uint32_t> ProcessParams(const int& argc, char* argv[])
{
    auto args_view = std::views::counted(argv, argc);

    std::string delimiter = ", ";

    // std::accumulate spojí prvky od druhého (index 1) k prvnímu (index 0)
    std::string result = std::accumulate(
        std::next(args_view.begin()),
        args_view.end(),
        std::string(args_view[0]),
        [&] (std::string a, const char* b)
        {
            return std::move(a) + delimiter + b;
        }
    );

    Logger::LogMessage<Program>(result);

    argparse::ArgumentParser argParser(APP_NAME);

    std::string defAdress = "0.0.0.0";
    int defPort = 7890;
    int defMaxRooms = 10;
    int defMaxPlayers = 20;

    argParser.add_argument("-h", "--help")
        .action([&] (const std::string&)
            {
                std::cout << argParser.help().str();
                Logger::terminate();
                exit(0);
            })
        .default_value(false)
        .implicit_value(true)
        .help("Zde je vas vlastni text napovedy");

    argParser.add_argument("-i", "--ip")
        .default_value(defAdress)
        .help("IP adresa")
        .nargs(1)
        .action([defAdress] (const std::string& value)
            {
                return is_valid_host(value) ? value : defAdress;
            });

    argParser.add_argument("-p", "--port")
        .default_value(7890)
        .help("Port <0-65535>")
        .nargs(1)
        .action([defPort] (const std::string& value) -> int
            {
                try
                {
                    int p = std::stoi(value);
                    if (p >= 0 && p <= 65535) return p;
                }
                catch (...)
                {
                }
                return defPort;
            });

    argParser.add_argument("-r", "--max-rooms")
        .default_value(defMaxRooms)
        .help("Maximalni pocet roomek <1-100>")
        .nargs(1)
        .action([defMaxRooms] (const std::string& value)
            {
                int v = Utils::Str2Uint(value);

                return v >= 1 && v <= 100 ? v : defMaxRooms;
            });

    argParser.add_argument("-c", "--max-clients")
        .default_value(defMaxPlayers)
        .help("Maximalni pocet hracu <2-300>")
        .nargs(1)
        .action([defMaxPlayers] (const std::string& value)
            {
                int v = Utils::Str2Uint(value);

                return v >= 2 && v <= 300 ? v : defMaxPlayers;
            });

    try
    {
        argParser.parse_args(argc, argv);
    }
    catch (const std::runtime_error& err)
    {
        Logger::LogError<Program>(ERROR_CODES::UNKNOW_PARAMETER);
        return {defAdress, defPort, defMaxRooms, defMaxPlayers};
    }

    return {argParser.get<std::string>("--ip"), argParser.get<int>("--port"), argParser.get<int>("--max-rooms"), argParser.get<int>("--max-clients")};
}

int main(int argc, char* argv[])
{
    Logger::init("server.log");

    std::vector<char *> argvV;

    if (argc <= 1)
    {
        // Pokud VS neposlalo parametry, pøidáme si je sami
        argvV.push_back(argv[0]);
        argvV.push_back("-r");
        argvV.push_back("xd");
    }
    else
    {
        for (int i = 0; i < argc; ++i) argvV.push_back(argv[i]);
    }



    argc = static_cast<int>(argvV.size());
    argv = &argvV[0];
    
    auto [adress, port, maxRooms, maxPlayers] = ProcessParams(argc, argv);

    Logger::LogMessage<Program>("Pouzita IP:\t" + adress);
    Logger::LogMessage<Program>("Pouziy port:\t" + std::to_string(port));
    Logger::LogMessage<Program>("Max roomek:\t" + std::to_string(maxRooms));
    Logger::LogMessage<Program>("Max hracu:\t" + std::to_string(maxPlayers));

    GameLobby g(adress, port, maxRooms, maxPlayers);
    g.CommandWorker.join();
    Logger::terminate();

    return 0;
}
