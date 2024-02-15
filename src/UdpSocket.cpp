/*
 * FeitCSI is the tool for extracting CSI information from supported intel NICs.
 * Copyright (C) 2024 Miroslav Hutar.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "UdpSocket.h"

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include "Logger.h"
#include "Arguments.h"
#include "MainController.h"

#define PORT "8008"
#define BUF_SIZE 1024

void UdpSocket::init()
{
    MainController *mainController = MainController::getInstance();
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    ssize_t nread;
    int s;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_DGRAM; /* Datagram socket */
    hints.ai_flags = AI_PASSIVE;    /* For wildcard IP address */
    hints.ai_protocol = 0;          /* Any protocol */
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    s = getaddrinfo(NULL, PORT, &hints, &result);
    if (s != 0)
    {
        Logger::log(error) << "getaddrinfo: " << gai_strerror(s) << "\n";
        exit(1);
    }

    /* getaddrinfo() returns a list of address structures.
       Try each address until we successfully bind(2).
       If socket(2) (or bind(2)) fails, we (close the socket
       and) try the next address. */

    for (rp = result; rp != NULL; rp = rp->ai_next)
    {
        sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sfd == -1)
            continue;

        if (bind(sfd, rp->ai_addr, rp->ai_addrlen) == 0)
            break; /* Success */

        close(sfd);
    }

    if (rp == NULL)
    { /* No address succeeded */
        Logger::log(error) << "Error bind socket \n";
        exit(1);
    }

    freeaddrinfo(result); /* No longer needed */

    /* Read datagrams and echo them back to sender */

    while (1)
    {
        char buf[BUF_SIZE] = {0};
        peer_addr_len = sizeof(struct sockaddr_storage);
        nread = recvfrom(sfd, buf, BUF_SIZE, 0, (struct sockaddr *)&peer_addr, &peer_addr_len);
        if (nread == -1)
            continue; /* Ignore failed request */

        char host[NI_MAXHOST], service[NI_MAXSERV];

        s = getnameinfo(
            (struct sockaddr *)&peer_addr,
            peer_addr_len,
            host,
            NI_MAXHOST,
            service,
            NI_MAXSERV,
            NI_NUMERICSERV);
        if (s == 0) {
            if (strncmp(buf, "stop", 4) == 0) {
                if (this->running) {
                    mainController->restoreState();
                    this->running = false;
                }
            } else {
                char *args[128];
                std::istringstream iss(buf);

                std::string token;
                int index = 0;
                while (iss >> token)
                {
                    char *arg = new char[token.size() + 1];
                    copy(token.begin(), token.end(), arg);
                    arg[token.size()] = '\0';
                    args[index] = arg;
                    index++;
                }

                Arguments::parse(index, &args[0]);

                for (int i = 0; i < index - 1; i++)
                    delete[] args[i];

                mainController->runNoGui(true);
                this->running = true;
            }
        }
        else {
            Logger::log(error) << "Error getnameinfo " << gai_strerror(s) << "\n";
        }
    }
}

void UdpSocket::send(char *buf, int size)
{
    if (
        sendto(
            sfd,
            buf,
            size,
            0,
            (struct sockaddr *)&peer_addr,
            peer_addr_len) != size)
    {
        Logger::log(error) << "Error sending response \n";
    }
}