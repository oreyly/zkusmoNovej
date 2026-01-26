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

#include <iostream>
#include <csignal>
#include <thread>
#include <chrono>
#include <arpa/inet.h>
#include <memory>


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

int main(int argc, char* argv[])
{
    try
    {
        Logger::init("log.log");
        std::string adress = "0.0.0.0";
        uint32_t port = 7890;

        if (argc >= 3)
        {
            adress = argv[1];
            port = Utils::Str2Uint(argv[2]);
        }

        GameLobby g(adress, port);
        std::this_thread::sleep_for(std::chrono::hours(1));
        //g.WaitForEnd("pepa");
        Logger::terminate();
    }
    catch (const std::exception&)
    {

    }
    return 0;
}
