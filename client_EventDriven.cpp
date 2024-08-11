#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>

#pragma comment(lib, "Ws2_32.lib")

int main() {
    WSADATA wsa_data;
    SOCKET connect_socket = INVALID_SOCKET;
    struct addrinfo* result = nullptr, * ptr = nullptr, hints;
    WSAEVENT recv_event;
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

    // Resolve the server address and port
    if (getaddrinfo("127.0.0.1", "8080", &hints, &result) != 0) {  // Changed port to 8080
        std::cerr << "getaddrinfo failed" << std::endl;
        WSACleanup();
        return 1;
    }

    // Attempt to connect to the first address returned by getaddrinfo
    for (ptr = result; ptr != nullptr; ptr = ptr->ai_next) {
        connect_socket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (connect_socket == INVALID_SOCKET) {
            std::cerr << "socket failed with error: " << WSAGetLastError() << std::endl;
            WSACleanup();
            return 1;
        }

        if (connect(connect_socket, ptr->ai_addr, (int)ptr->ai_addrlen) == SOCKET_ERROR) {
            closesocket(connect_socket);
            connect_socket = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(result);

    if (connect_socket == INVALID_SOCKET) {
        std::cerr << "Unable to connect to server!" << std::endl;
        WSACleanup();
        return 1;
    }

    // Create an event object for the socket
    recv_event = WSACreateEvent();
    if (recv_event == WSA_INVALID_EVENT) {
        std::cerr << "WSACreateEvent failed with error: " << WSAGetLastError() << std::endl;
        closesocket(connect_socket);
        WSACleanup();
        return 1;
    }

    // Associate the event object with the receive event
    if (WSAEventSelect(connect_socket, recv_event, FD_READ) == SOCKET_ERROR) {
        std::cerr << "WSAEventSelect failed with error: " << WSAGetLastError() << std::endl;
        closesocket(connect_socket);
        WSACloseEvent(recv_event);
        WSACleanup();
        return 1;
    }

    // Wait for the event to be signaled with a timeout (5 seconds)
    DWORD wait_result = WSAWaitForMultipleEvents(1, &recv_event, FALSE, 5000, FALSE);
    if (wait_result == WSA_WAIT_TIMEOUT) {
        std::cerr << "Timeout: No data received from server within the time limit." << std::endl;
        closesocket(connect_socket);
        WSACloseEvent(recv_event);
        WSACleanup();
        return 1;
    }
    else if (wait_result == WSA_WAIT_FAILED) {
        std::cerr << "WSAWaitForMultipleEvents failed with error: " << WSAGetLastError() << std::endl;
        closesocket(connect_socket);
        WSACloseEvent(recv_event);
        WSACleanup();
        return 1;
    }

    // Retrieve the network events
    if (WSAEnumNetworkEvents(connect_socket, recv_event, &network_events) == SOCKET_ERROR) {
        std::cerr << "WSAEnumNetworkEvents failed with error: " << WSAGetLastError() << std::endl;
        closesocket(connect_socket);
        WSACloseEvent(recv_event);
        WSACleanup();
        return 1;
    }

    if (network_events.lNetworkEvents & FD_READ) {
        if (network_events.iErrorCode[FD_READ_BIT] != 0) {
            std::cerr << "FD_READ failed with error: " << network_events.iErrorCode[FD_READ_BIT] << std::endl;
        }
        else {
            char recv_buf[512];
            int recv_result = recv(connect_socket, recv_buf, sizeof(recv_buf), 0);
            if (recv_result > 0) {
                std::cout << "Received message: " << std::string(recv_buf, recv_result) << std::endl;
            }
            else {
                std::cerr << "recv failed with error: " << WSAGetLastError() << std::endl;
            }
        }
    }

    // Clean up
    closesocket(connect_socket);
    WSACloseEvent(recv_event);
    WSACleanup();
    return 0;
}
