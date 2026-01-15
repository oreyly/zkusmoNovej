#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h>
#include "program.h"
#include "Logger.h"

constexpr int BUFFSIZE = 255;
constexpr int DEFAULT_PORT = 7890;

void die(const std::string& message)
{
    std::perror(message.c_str());
    std::exit(EXIT_FAILURE);
}

int main2()
{
    std::cout << "XDDD" << std::endl;
    Logger::init("xd.log");
    //Logger::Log()
    return 0;
    int sock;
    sockaddr_in echoserver {};
    sockaddr_in echoclient {};
    char buffer[BUFFSIZE];
    unsigned int clientlen;
    ssize_t received = 0;

    // VytvoĹ™enĂ­ UDP soketu
    if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
    {
        die("Failed to create socket");
    }

    // NastavenĂ­ struktury serveru
    std::memset(&echoserver, 0, sizeof(echoserver));
    echoserver.sin_family = AF_INET;
    echoserver.sin_addr.s_addr = htonl(INADDR_ANY);
    echoserver.sin_port = htons(DEFAULT_PORT);

    std::cout << "Port: " << DEFAULT_PORT << " (Network byte order: " << echoserver.sin_port << ")" << std::endl;

    // Bind soketu
    if (bind(sock, reinterpret_cast<sockaddr*>(&echoserver), sizeof(echoserver)) < 0)
    {
        die("Failed to bind server socket");
    }

    // HlavnĂ­ smyÄŤka serveru
    while (true)
    {
        clientlen = sizeof(echoclient);

        // PĹ™Ă­jem dat
        received = recvfrom(sock, buffer, BUFFSIZE, 0,
            reinterpret_cast<sockaddr*>(&echoclient), &clientlen);

        if (received < 0)
        {
            die("Failed to receive message");
        }

        std::cerr << "Client connected: " << inet_ntoa(echoclient.sin_addr) << std::endl;

        // OdeslĂˇnĂ­ dat zpÄ›t (Echo)
        ssize_t sent = sendto(sock, buffer, received, 0,
            reinterpret_cast<sockaddr*>(&echoclient), sizeof(echoclient));

        if (sent != received)
        {
            die("Mismatch in number of echo'd bytes");
        }
    }

    close(sock);
    return 0;
}
