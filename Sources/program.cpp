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
#include "opcode.h"

#include <iostream>
#include <csignal>
#include <thread>
#include <chrono>
#include <arpa/inet.h>

void ForcedExit()
{
    exit(1);
}
// Globální příznak pro ukončení aplikace (např. přes Ctrl+C)

std::atomic<bool> g_running(true);
void signalHandler(int s)
{
    g_running = false;
}

void onIncomingRequest(IncomingRequest req)
{
    // Zde server zpracovává požadavky od Python klienta
    std::cout << "[Server] Přijat požadavek z Python klienta přes RequestManager." << std::endl;
}

int main()
{
    signal(SIGINT, signalHandler);

    if (!Logger::init("server_log.txt")) return 1;

    try
    {
        Comunicator comunicator(7890); // Server port

        MessageManager msgManager;
        msgManager.ConnectToComunicator(comunicator, 2000, 3); // Timeout 2s, 3 pokusy

        RequestManager reqManager;
        reqManager.ConnectToComunicator(msgManager);
        reqManager.RegisterProcessingFunction(onIncomingRequest);

        std::cout << "Server běží na portu 7890..." << std::endl;
        while (g_running)
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        reqManager.Stop();
        msgManager.Stop();
        comunicator.Stop();
    }
    catch (const std::exception& e)
    {
        std::cerr << "Chyba: " << e.what() << std::endl;
    }

    Logger::terminate();
    return 0;
}