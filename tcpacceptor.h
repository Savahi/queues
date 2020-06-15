#pragma once
#include <string>
#include <iostream>

#define _WIN32_WINNT 0x501
#include <WinSock2.h>
#include <WS2tcpip.h>

class TCPAcceptor
{
public:
  int       m_listen_socket;
  bool      m_listening;
  std::string    m_ip;
  std::string    m_port;

  struct    addrinfo* m_addr; // holds socket ip etc
    
public:
    TCPAcceptor::TCPAcceptor(std::string ip, std::string port) : 
        m_ip(ip), m_port(port), m_listen_socket(INVALID_SOCKET), m_listening(false),m_addr(NULL) {
    } 

    TCPAcceptor::~TCPAcceptor() {
        if( m_listening ) {
            freeaddrinfo(m_addr);
            closesocket(m_listen_socket);
            WSACleanup();
        }
    }

    int start() {
        if( m_listening == true ) {
            return 0;
        }

        WSADATA wsaData; //  use Ws2_32.dll
        size_t result;

        result = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (result != 0) {
            return -1;
        }

        // To be initialized with constants and values...
        struct addrinfo hints;
        ZeroMemory(&hints, sizeof(hints));

        hints.ai_family = AF_INET; 
        hints.ai_socktype = SOCK_STREAM; 
        hints.ai_protocol = IPPROTO_TCP; 
        hints.ai_flags = AI_PASSIVE; 

        result = ::getaddrinfo(m_ip.c_str(), m_port.c_str(), &hints, &m_addr); // Port 8000 is used
        if (result != 0) { 		// If failed...
            WSACleanup(); 
            return -1;
        }

        // Creating a socket
        m_listen_socket = ::socket(m_addr->ai_family, m_addr->ai_socktype, m_addr->ai_protocol);
        if (m_listen_socket == INVALID_SOCKET) { 		// If failed to create a socket...
            std::cout << "Error at socket:\n" + std::to_string( WSAGetLastError() );
            freeaddrinfo(m_addr);
            WSACleanup();
            return -1;
        }

        // Binding the socket to the ip-address
        result = ::bind(m_listen_socket, m_addr->ai_addr, (int)m_addr->ai_addrlen);
        if (result == SOCKET_ERROR) { 		// If failed to bind...
            std::cout << "bind failed with error:\n" + std::to_string( WSAGetLastError() );
            freeaddrinfo(m_addr);
            closesocket(m_listen_socket);
            WSACleanup();
            return -1;
        }

        // Init listening...
        if (listen(m_listen_socket, SOMAXCONN) == SOCKET_ERROR) {
            std::cout << "listen failed with error:\n" + std::to_string( WSAGetLastError() );
            freeaddrinfo(m_addr);
            closesocket(m_listen_socket);
            WSACleanup();
            return -1;
        }

        m_listening = true;
        return 0;
    }

    int accept() {
        if (m_listening == false) {
            return -1;
        }

        int client_socket = ::accept(m_listen_socket, NULL, NULL);
        if (client_socket == INVALID_SOCKET) {
            closesocket(m_listen_socket);
            WSACleanup();
            return -1;
        }
        return client_socket;
    }

    int run() {
        int client_socket;
        while(true) {
            client_socket = accept(); 
            if (client_socket == -1 ) {
                std::cout << "Could not accept a connection" << std::endl;
                continue;
            }
            break;
        }
        return client_socket;
    }
};
