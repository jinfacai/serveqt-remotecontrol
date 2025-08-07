#include "ServerSocket.h"
#include "Packet.h"
#include <vector>

CServerSocket::CServerSocket(const std::string& ip,int port)
    :m_ip(ip) , m_port(port), m_epollFd(-1), m_listenFd(-1), m_running(false), m_nextClientId(1)
{
}

CServerSocket::~CServerSocket() {
    stop();
}

bool CServerSocket::start() {
    if (!initialize()) {
        return false;
    }
    m_running = true;
    log("Server started successfully, listening on port: " + std::to_string(m_port));
    return true;
}

void CServerSocket::stop() {
    if (!m_running) {
        return;
    }
    m_running = false;
    // Close all client connections
    for (auto& pair : m_clients) {
        close(pair.second.socket);
    }
    m_clients.clear();
    m_clientIdMap.clear();
    // Close listener
    if (m_listenFd != -1) {
        close(m_listenFd);
        m_listenFd = -1;
    }
    // Close epoll
    if (m_epollFd != -1) {
        close(m_epollFd);
        m_epollFd = -1;
    }
    log("Server has stopped");
}

void CServerSocket::run() {
    if (!m_running) {
        log("Server not started");
        return;
    }
    struct epoll_event events[MAX_EVENTS];
    while (m_running) {
        int nfds = epoll_wait(m_epollFd, events, MAX_EVENTS, 100); // 100ms timeout
        if (nfds == -1) {
            // If interrupted by a signal
            if (errno == EINTR) {
                continue;
            }
            log("epoll_wait error: " + std::string(strerror(errno)));
            break;
        }
        for (int i = 0; i < nfds; i++) {
            if (events[i].data.fd == m_listenFd) {
                // New connection
                handleNewConnection();
            }
            else {
                // Handle client data
                handleClientData(events[i].data.fd);
            }
        }
    }
}

void CServerSocket::handleClientData(int clientSocket) {
    char buffer[BUFFER_SIZE];
    ssize_t bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
    if (bytesRead <= 0) {
        if (bytesRead == 0) {
            log("Client disconnected actively");
        }
        else {
            log("Data reception error: " + std::string(strerror(errno)));
        }
        handleClientDisconnect(clientSocket); // Fixed spelling: Disconnet → Disconnect
        return;
    }
    // Add data to receive buffer
    //auto& client = m_clients[clientSocket];
    auto it = m_clients.find(clientSocket);
    if (it == m_clients.end()) {
        log("Client not found in map, socket: " + std::to_string(clientSocket));
        return;
    }
    auto& client = it->second;
    client.receiverBuffer.append(buffer, bytesRead);
    log("Received " + std::to_string(bytesRead) + " bytes from client " + std::to_string(client.id));

    while (client.receiverBuffer.size() >= 8) { // Minimum packet size
        size_t remainingSize = client.receiverBuffer.size();
        log("Processing buffer of size: " + std::to_string(remainingSize));
        CPacket packet(reinterpret_cast<const uint8_t*>(client.receiverBuffer.data()), remainingSize);
        if (remainingSize == 0) {
            log("Failed to parse data, continuing to try");
            client.receiverBuffer.erase(0, 1);
            continue;
        }
        log("Successfully parsed packet, cmd: " + std::to_string(packet.getCmd()) +
            ", data size: " + std::to_string(packet.getData().size()));
        // Remove processed packet data
        int processedSize = client.receiverBuffer.size() - remainingSize;
        if (processedSize > 0) {
            client.receiverBuffer.erase(0, processedSize);
            log("Removed " + std::to_string(processedSize) + " bytes from buffer");
        }
        // Process the packet
        handlePacket(clientSocket, packet);
    }
}

void CServerSocket::handlePacket(int clientSocket, const CPacket& packet) {
   // auto& client = m_clients[clientSocket];
    auto it = m_clients.find(clientSocket);
    if (it == m_clients.end()) {
        log("Client not found in map for packet handling, socket: " + std::to_string(clientSocket));
        return;
    }
    auto& client = it->second;
    log("Received packet from client " + std::to_string(client.id) + ": " +
        "Command=" + std::to_string(packet.getCmd()) +
        ", Data=" + packet.getData());

    // Handle based on command
    switch (packet.getCmd()) {
    case 100: // Text message
    {
        std::string textData = packet.getData();
        log("Client " + std::to_string(client.id) + " sent text: " + textData);

        // Broadcast text message to all clients
        std::string broadcastText = "Client " + std::to_string(client.id) + " says: " + textData;
        CPacket broadcastPacket = createResponsePacket(100, broadcastText);
        this->broadcastPacket(broadcastPacket);
    }
    break;

    case 101: // File begin
    {
        std::string fileName = packet.getData();
        log("Client " + std::to_string(client.id) + " started sending file: " + fileName);

        // Send confirmation message
        std::string ackMsg = "Server received file start notification: " + fileName;
        CPacket ackPacket = createResponsePacket(100, ackMsg);
        sendPacketToClient(client.id, ackPacket);
    }
    break;

    case 102: // File data
    {
        size_t dataSize = packet.getData().size();
        log("Client " + std::to_string(client.id) + " sent file data chunk: " +
            std::to_string(dataSize) + " bytes");

        // Send confirmation message
        std::string ackMsg = "Server received file data chunk: " + std::to_string(dataSize) + " bytes";
        CPacket ackPacket = createResponsePacket(100, ackMsg);
        sendPacketToClient(client.id, ackPacket);
    }
    break;

    case 103: // File end
    {
        log("Client " + std::to_string(client.id) + " finished sending file");

        // Send confirmation message
        std::string ackMsg = "Server received file end notification";
        CPacket ackPacket = createResponsePacket(100, ackMsg);
        sendPacketToClient(client.id, ackPacket);
    }
    break;

    default:
    {
        std::string unknownCmd = "Unknown command: " + std::to_string(packet.getCmd());
        log("Received unknown command: " + std::to_string(packet.getCmd()));
        CPacket response = createResponsePacket(100, unknownCmd);
        sendPacketToClient(client.id, response);
    }
    break;
    }
}

CPacket CServerSocket::createResponsePacket(uint16_t cmd, const std::string& data) {
    return CPacket(cmd, reinterpret_cast<const uint8_t*>(data.data()), data.size());
}

void CServerSocket::handleClientDisconnect(int clientSocket) { // Fixed spelling: Disconnet → Disconnect
    log("Handling client disconnect for socket: " + std::to_string(clientSocket));
    auto it = m_clients.find(clientSocket);
    // If client not found
    if (it != m_clients.end()) {
        //log("Client disconnected: ID=" + std::to_string(it->second.id) +
        //    ", IP=" + it->second.ip);
        int clientId = it->second.id;
        log("Client disconnected: ID=" + std::to_string(clientId) +
            ", IP=" + it->second.ip + ", Socket=" + std::to_string(clientSocket));
        m_clientIdMap.erase(clientId);
        //log("Removing client from map: socket=" + std::to_string(clientSocket));
        m_clients.erase(it);
        log("Successfully removed client from maps. New size: " + std::to_string(m_clients.size()));
    }
    else {
        log("WARNING: Client socket " + std::to_string(clientSocket) + " not found in m_clients");
    }
    removeClientFromEpoll(clientSocket);
   // close(clientSocket);
    //关闭套接字
    if (close(clientSocket) == -1) {
        log("Failed to close socket " + std::to_string(clientSocket) + ": " + std::string(strerror(errno)));
    }
    else {
        log("Successfully closed socket " + std::to_string(clientSocket));
    }

}


void CServerSocket::removeClientFromEpoll(int clientSocket) {
    if (epoll_ctl(m_epollFd, EPOLL_CTL_DEL, clientSocket, nullptr) == -1) {
        log("Failed to remove client from epoll: " + std::string(strerror(errno)));
    }
}

void CServerSocket::handleNewConnection() {
    struct sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);
    // Accept client connection
    int clientSocket = accept(m_listenFd, (struct sockaddr*)&clientAddr, &clientLen);
    if (clientSocket == -1) {
        log("Failed to accept connection: " + std::string(strerror(errno)));
        return;
    }
    // Check client limit
    if (m_clients.size() >= MAX_CLIENTS) {
        log("Client limit reached, connection rejected");
        close(clientSocket);
        return;
    }
    // Set non-blocking mode
    setNonBlocking(clientSocket);
    // Add to epoll
    addClientToEpoll(clientSocket);
    // Create client info
    ClientInfo client;
    client.socket = clientSocket;
    client.id = m_nextClientId++;
    client.ip = inet_ntoa(clientAddr.sin_addr);
    client.port = ntohs(clientAddr.sin_port);
    client.isConnected = true;
    client.receiverBuffer.clear();
    //m_clients[clientSocket] = client;
    //检查客户端数量限制
    if (m_clients.size() >= MAX_CLIENTS) {
        log("Client limit reached, connection rejected");
        close(clientSocket);
        return;
    }
    //检查套接字是否已经存在
    if (m_clients.find(clientSocket) != m_clients.end()) {
        log("Error: Socket" + std::to_string(clientSocket) + "already exists in m_clients!");
        close(clientSocket);
        return;
    }
    //插入客户端
    log("Attempting to insert client with socket: " + std::to_string(clientSocket));
    log("Current map size before insert: " + std::to_string(m_clients.size()));

    auto result = m_clients.insert(std::make_pair(clientSocket, client));
    if (!result.second) {
        log("Failed to insert client into map");
        close(clientSocket);
        return;
    }
    log("Successfully inserted client. New map size: " + std::to_string(m_clients.size()));

    m_clientIdMap[client.id] = clientSocket;
    log("New client connected: ID=" + std::to_string(client.id) +
       ", IP=" + client.ip + ", Port=" + std::to_string(client.port));
 
}


void CServerSocket::addClientToEpoll(int clientSocket) {
    struct epoll_event event;
    event.events = EPOLLIN | EPOLLET; // Edge-triggered mode
    event.data.fd = clientSocket;
    if (epoll_ctl(m_epollFd, EPOLL_CTL_ADD, clientSocket, &event) == -1) {
        log("Failed to add client to epoll: " + std::string(strerror(errno)));
    }
}

bool CServerSocket::initialize() {
    log("Starting server initialization...");
    // Create socket
    m_listenFd = socket(AF_INET, SOCK_STREAM, 0);
    if (m_listenFd == -1) {
        log("Failed to create socket: " + std::string(strerror(errno)));
        return false;
    }
    log("Socket created successfully, file descriptor: " + std::to_string(m_listenFd));
    // Set socket options
    int opt = 1;
    if (setsockopt(m_listenFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        log("Failed to set socket options: " + std::string(strerror(errno)));
        close(m_listenFd);
        return false;
    }
    // Set to non-blocking mode
    setNonBlocking(m_listenFd);

    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(m_port);
    // Bind address
    if (bind(m_listenFd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        log("Failed to bind address: " + std::string(strerror(errno)));
        close(m_listenFd);
        return false;
    }
    // Start listening
    if (listen(m_listenFd, SOMAXCONN) == -1) {
        log("Failed to listen: " + std::string(strerror(errno)));
        close(m_listenFd);
        return false;
    }
    // Create epoll
    m_epollFd = epoll_create1(0);
    if (m_epollFd == -1) {
        log("Failed to create epoll: " + std::string(strerror(errno)));
        close(m_listenFd);
        return false;
    }
    log("Epoll created successfully, file descriptor: " + std::to_string(m_epollFd));

    // Add listening socket to epoll
    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = m_listenFd;
    if (epoll_ctl(m_epollFd, EPOLL_CTL_ADD, m_listenFd, &event) == -1) {
        log("Failed to add listening socket to epoll: " + std::string(strerror(errno)));
        close(m_epollFd);
        close(m_listenFd);
        return false;
    }
    log("Listening socket added to epoll");
    log("Server initialization completed");
    return true;
}

void CServerSocket::setNonBlocking(int fd) {
    // Get current flags
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        log("Failed to get file descriptor flags: " + std::string(strerror(errno)));
        return;
    }
    // Add non-blocking flag
    flags |= O_NONBLOCK;
    if (fcntl(fd, F_SETFL, flags) == -1) {
        log("Failed to set non-blocking mode: " + std::string(strerror(errno)));
        return;
    }
}

void CServerSocket::log(const std::string message) const {
    std::cout << "[Server] " << message << std::endl;
}


void CServerSocket::broadcastPacket(const CPacket& packet) {
    for (const auto& clientPair : m_clients) {
        const auto& client = clientPair.second;
        if (client.isConnected) {
            sendPacketToClient(client.id, packet);
        }
    }
}

bool CServerSocket::sendPacketToClient(int clientId, const CPacket& packet) {
    auto it = m_clientIdMap.find(clientId);
    if (it == m_clientIdMap.end()) {
        log("Client ID does not exist: " + std::to_string(clientId));
        return false;
    }
    int clientSocket = it->second;
    const char* data = packet.Data();
    int size = packet.Size();

    ssize_t bytesSent = send(clientSocket, data, size, 0);
    if (bytesSent == -1) {
        log("Failed to send data: " + std::string(strerror(errno)));
        return false;
    }
    if (bytesSent != size) {
        log("Incomplete data sent: " + std::to_string(bytesSent) + "/" + std::to_string(size));
        return false;
    }
    log("Sent packet to client " + std::to_string(clientId) + ": " +
        "Command=" + std::to_string(packet.getCmd()) +
        ", Size=" + std::to_string(size) + " bytes");

    return true;
}



