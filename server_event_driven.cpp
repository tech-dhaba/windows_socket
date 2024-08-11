#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>

#pragma comment(lib, "Ws2_32.lib")

int main() {
    WSADATA wsa_data;
    SOCKET listen_socket = INVALID_SOCKET, client_socket = INVALID_SOCKET;
    struct addrinfo* result = nullptr, hints;
    WSAEVENT accept_event;
    WSANETWORKEVENTS network_events;

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        std::cerr << "WSAStartup failed" << std::endl;
        return 1;
    }

    // Set up the hints address info structure
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the server address and port
    if (getaddrinfo(nullptr, "8080", &hints, &result) != 0) {  // Changed port to 8080
        std::cerr << "getaddrinfo failed" << std::endl;
        WSACleanup();
        return 1;
    }

    // Create a socket for connecting to the server
    listen_socket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (listen_socket == INVALID_SOCKET) {
        std::cerr << "socket failed with error: " << WSAGetLastError() << std::endl;
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    // Bind the socket
    if (bind(listen_socket, result->ai_addr, (int)result->ai_addrlen) == SOCKET_ERROR) {
        std::cerr << "bind failed with error: " << WSAGetLastError() << std::endl;
        freeaddrinfo(result);
        closesocket(listen_socket);
        WSACleanup();
        return 1;
    }

    freeaddrinfo(result);

    // Listen for incoming connections
    if (listen(listen_socket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "listen failed with error: " << WSAGetLastError() << std::endl;
        closesocket(listen_socket);
        WSACleanup();
        return 1;
    }

    // Create an event object for the socket
    accept_event = WSACreateEvent();
    if (accept_event == WSA_INVALID_EVENT) {
        std::cerr << "WSACreateEvent failed with error: " << WSAGetLastError() << std::endl;
        closesocket(listen_socket);
        WSACleanup();
        return 1;
    }

    // Associate the event object with the accept event
    if (WSAEventSelect(listen_socket, accept_event, FD_ACCEPT) == SOCKET_ERROR) {
        std::cerr << "WSAEventSelect failed with error: " << WSAGetLastError() << std::endl;
        closesocket(listen_socket);
        WSACloseEvent(accept_event);
        WSACleanup();
        return 1;
    }

    std::cout << "Waiting for client connection..." << std::endl;

    // Wait for the event to be signaled
    if (WSAWaitForMultipleEvents(1, &accept_event, FALSE, WSA_INFINITE, FALSE) == WSA_WAIT_FAILED) {
        std::cerr << "WSAWaitForMultipleEvents failed with error: " << WSAGetLastError() << std::endl;
        closesocket(listen_socket);
        WSACloseEvent(accept_event);
        WSACleanup();
        return 1;
    }

    // Retrieve the network events
    if (WSAEnumNetworkEvents(listen_socket, accept_event, &network_events) == SOCKET_ERROR) {
        std::cerr << "WSAEnumNetworkEvents failed with error: " << WSAGetLastError() << std::endl;
        closesocket(listen_socket);
        WSACloseEvent(accept_event);
        WSACleanup();
        return 1;
    }

    if (network_events.lNetworkEvents & FD_ACCEPT) {
        client_socket = accept(listen_socket, nullptr, nullptr);
        if (client_socket == INVALID_SOCKET) {
            std::cerr << "accept failed with error: " << WSAGetLastError() << std::endl;
        }
        else {
            std::cout << "Client connected!" << std::endl;

            // Send a message to the client
            std::string send_buf = "Hello from server!";
            send(client_socket, send_buf.c_str(), (int)send_buf.size(), 0);
        }
    }

    // Clean up
    closesocket(client_socket);
    closesocket(listen_socket);
    WSACloseEvent(accept_event);
    WSACleanup();
    return 0;
}
