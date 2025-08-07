#include <iostream>
#include <signal.h>
#include <unistd.h>
#include "ServerSocket.h"

// Global server pointer for signal handling
CServerSocket* g_server = nullptr;

// Signal handler function
void signalHandler(int signum) {
    std::cout << "\nReceived signal " << signum << ", shutting down the server..." << std::endl;

    if (g_server) {
        g_server->stop();
    }

    exit(0);
}

int main(int argc, char* argv[]) {
    std::string ip = "127.0.0.1";// Default IP
    int port = 8080;  // Default port

    // Parse command line arguments
    if (argc > 1) {
        ip = argv[1];
    }
    if (argc > 2) {
        port = std::atoi(argv[2]);
        if (port <= 0 || port > 65535) {
            std::cerr << "Error: Port number must be between 1 and 65535." << std::endl;
            return 1;
        }
    }else{
        std::cout<<"Enter port number (default 8080):";
        std::string portInput;
        std::getline(std::cin, portInput);
        if (!portInput.empty()) {
            port = std::atoi(portInput.c_str());
            if (port <= 0 || port > 65535) {
                std::cerr << "Error: Port number must be between 1 and 65535, using default port 8080" << std::endl;
                port = 8080;
            }
        }
    }
    // Print welcome information
    std::cout << "=== Linux Server - Qt Client Test ===" << std::endl;
    std::cout << "IP: " << ip << std::endl;
    std::cout << "Port: " << port << std::endl;
    std::cout << "Supported commands:" << std::endl;
    std::cout << "  100 - Text Message" << std::endl;
    std::cout << "  101 - File Begin" << std::endl;   // Fixed spelling: filebegain ¡ú filebegin
    std::cout << "  102 - File Data" << std::endl;
    std::cout << "  103 - File End" << std::endl;
    std::cout << "Press Ctrl+C to exit" << std::endl;  // More intuitive description
    std::cout << "=====================================" << std::endl;

    // Register signal handlers
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    // Create and start server
    CServerSocket server(ip,port);
    g_server = &server;

    if (!server.start()) {
        std::cerr << "Server failed to start!" << std::endl;
        return 1;
    }

    std::cout << "Server started successfully, waiting for Qt client connection..." << std::endl;

    // Run server main loop
    server.run();

    return 0;
}