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

int main(int argc, char* argv[])
{
    argparse::ArgumentParser program("server_app", "1.0", argparse::default_arguments::none);

    std::string adress = "0.0.0.0";
    int port = 7890;
    int maxRooms = 1;
    int maxPlayers = 2;

    program.add_argument("-h", "--help")
        .action([&] (const std::string&)
            {
                std::cout << program.help().str();
                exit(0);
            })
        .default_value(false)
        .implicit_value(true)
        .help("Zde je váš vlastní text nápovìdy");

    program.add_argument("-i", "--ip")
        .default_value(adress)
        .help("IP adresa (default: 0.0.0.0)")
        .action([adress] (const std::string& value)
            {
                return is_valid_host(value) ? value : adress;
            });

    program.add_argument("-p", "--port")
        .default_value(7890)
        .help("Port (default: 7890)")
        .action([port] (const std::string& value) -> int
            {
                try
                {
                    int p = std::stoi(value);
                    if (p >= 0 && p <= 65535) return p;
                }
                catch (...)
                {
                }
                return port;
            });

    program.add_argument("-r", "--max-rooms")
        .default_value(maxRooms)
        .help("Maximální poèet roomek")
        .action([maxRooms] (const std::string& value)
            {
                int v = Utils::Str2Uint(value);

                return v >= 1 && v <= 100 ? v : maxRooms;
            });

    program.add_argument("-c", "--max-clients")
        .default_value(maxPlayers)
        .help("Maximální poèet hráèù")
        .action([maxPlayers] (const std::string& value)
            {
                int v = Utils::Str2Uint(value);

                return v >= 2 && v <= 300 ? v : maxPlayers;
            });

    Logger::init("log.log");

    try
    {
        program.parse_args(argc, argv);
    }
    catch (const std::runtime_error& err)
    {
        Logger::LogError<Program>(ERROR_CODES::UNKNOW_PARAMETER);
        return 1;
    }

    adress = program.get<std::string>("--ip");
    port = program.get<int>("--port");
    maxRooms = program.get<int>("--max-rooms");
    maxPlayers = program.get<int>("--max-clients");

    Logger::LogMessage<Program>("Pouzita IP:\t" + adress);
    Logger::LogMessage<Program>("Pouziy port:\t" + std::to_string(port));
    Logger::LogMessage<Program>("Max roomek:\t" + std::to_string(maxRooms));
    Logger::LogMessage<Program>("Max hracu:\t" + std::to_string(maxPlayers));


    if (argc >= 3)
    {
        adress = argv[1];
        port = Utils::Str2Uint(argv[2]);
    }

    GameLobby g(adress, port, maxRooms, maxPlayers);
    g.CommandWorker.join();
    Logger::terminate();

    return 0;
}
