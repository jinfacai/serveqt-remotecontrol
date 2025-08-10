#include "ServerSocket.h"
#include "Packet.h"
#include "Command.h"
#include <vector>

CServerSocket::CServerSocket(const std::string& ip, int port)
    :m_ip(ip), m_port(port), m_epollFd(-1), m_listenFd(-1), m_running(false), m_nextClientId(1)
{
    m_command = std::unique_ptr<CCommand>(new CCommand()); //����command
    // ����Command���ServerSocketָ��
    m_command->setServerSocket(this);
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

    // �ر����пͻ�������
    auto& clientManager = m_command->getClientManager();
    for (const auto& pair : clientManager.getAllClients()) {
        close(pair.second.socket);
    }

    // �رռ���socket
    if (m_listenFd != -1) {
        close(m_listenFd);
        m_listenFd = -1;
    }
    // �ر�epoll
    if (m_epollFd != -1) {
        close(m_epollFd);
        m_epollFd = -1;
    }
    log("Server has stopped");
}

int CServerSocket::getClientCount() const {
    return m_command->getClientManager().getClientCount();
}

CCommand* CServerSocket::getCommand() {
    return m_command.get();
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
        handleClientDisconnect(clientSocket);
        return;
    }

    // ��������ӵ�ClientManager�Ļ�����
    auto& clientManager = m_command->getClientManager();
    int clientId = clientManager.getClientIdBySocket(clientSocket);
    if (clientId != -1) {
        clientManager.appendToBuffer(clientId, std::string(buffer, bytesRead));
        log("Received " + std::to_string(bytesRead) + " bytes from client " + std::to_string(clientId));

        // ���������е����ݰ�
        std::string receiverBuffer = clientManager.getBuffer(clientId);
        while (receiverBuffer.size() >= 8) { // Minimum packet size
            size_t remainingSize = receiverBuffer.size();
            log("Processing buffer of size: " + std::to_string(remainingSize));

            CPacket packet(reinterpret_cast<const uint8_t*>(receiverBuffer.data()), remainingSize);
            if (remainingSize == 0) {
                log("Failed to parse data, continuing to try");
                receiverBuffer.erase(0, 1);
                continue;
            }
            log("Successfully parsed packet, cmd: " + std::to_string(packet.getCmd()) +
                ", data size: " + std::to_string(packet.getData().size()));

            // �Ƴ��Ѵ�������ݰ�����
            int processedSize = receiverBuffer.size() - remainingSize;
            if (processedSize > 0) {
                receiverBuffer.erase(0, processedSize);
                log("Removed " + std::to_string(processedSize) + " bytes from buffer");
            }

            // �������ݰ�
            handlePacket(clientSocket, packet);
        }

        // ���»�����
        clientManager.clearBuffer(clientId);
        clientManager.appendToBuffer(clientId, receiverBuffer);
    }
}

void CServerSocket::handlePacket(int clientSocket, const CPacket& packet) {
    auto& clientManager = m_command->getClientManager();
    int clientId = clientManager.getClientIdBySocket(clientSocket);
    if (clientId == -1) {
        log("Client not found for packet handling, socket: " + std::to_string(clientSocket));
        return;
    }

    log("Received packet from client " + std::to_string(clientId) + ": " +
        "Command=" + std::to_string(packet.getCmd()) +
        ", Data=" + packet.getData());

    // ���������ȫ���� CCommand �࣬����clientId
    std::list<CPacket> outPackets;
    CPacket packetCopy = packet;  // ���������Ա���const����
    int result = m_command->ExecuteCommand(packet.getCmd(), outPackets, packetCopy, clientId);

    if (result != 0) {
        log("Command execution failed for cmd: " + std::to_string(packet.getCmd()));
    }

    // ���� CCommand ���ɵ���Ӧ��
    for (const auto& outPacket : outPackets) {
        // �㲥�����пͻ���
        for (const auto& clientPair : clientManager.getAllClients()) {
            if (clientPair.second.isConnected) {
                sendPacketToClient(clientPair.second.id, outPacket);
            }
        }
    }
}

void CServerSocket::handleClientDisconnect(int clientSocket) {
    log("Handling client disconnect for socket: " + std::to_string(clientSocket));

    auto& clientManager = m_command->getClientManager();
    int clientId = clientManager.getClientIdBySocket(clientSocket);
    if (clientId != -1) {
        log("Client disconnected: ID=" + std::to_string(clientId) + ", Socket=" + std::to_string(clientSocket));

        // ֪ͨCommand���Ƴ��ͻ���
        m_command->removeClient(clientId);

        log("Successfully removed client from ClientManager. New size: " + std::to_string(clientManager.getClientCount()));
    }
    else {
        log("WARNING: Client socket " + std::to_string(clientSocket) + " not found in ClientManager");
    }
    removeClientFromEpoll(clientSocket);

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

    // ���÷�����
    setNonBlocking(clientSocket);

    // ��ӵ�epoll
    addClientToEpoll(clientSocket);

    // ��ȡ�ͻ���IP�Ͷ˿�
    char clientIP[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, INET_ADDRSTRLEN);
    int clientPort = ntohs(clientAddr.sin_port);

    // ����ͻ���ID
    int clientId = m_nextClientId++;

    // ͨ��Command����ӿͻ��˵�ClientManager
    m_command->addClient(clientSocket, clientId, std::string(clientIP), clientPort);

    log("New client connected: Socket=" + std::to_string(clientSocket) +
        ", ID=" + std::to_string(clientId) +
        ", IP=" + std::string(clientIP) +
        ", Port=" + std::to_string(clientPort));
}

void CServerSocket::addClientToEpoll(int clientSocket) {
    struct epoll_event event;
    event.events = EPOLLIN | EPOLLET; // Edge-triggered
    event.data.fd = clientSocket;

    if (epoll_ctl(m_epollFd, EPOLL_CTL_ADD, clientSocket, &event) == -1) {
        log("Failed to add client to epoll: " + std::string(strerror(errno)));
        close(clientSocket);
        return;
    }
}

bool CServerSocket::initialize() {
    // ��������socket
    m_listenFd = socket(AF_INET, SOCK_STREAM, 0);
    if (m_listenFd == -1) {
        log("Failed to create socket: " + std::string(strerror(errno)));
        return false;
    }

    // ����socketѡ��
    int opt = 1;
    if (setsockopt(m_listenFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        log("Failed to set socket option: " + std::string(strerror(errno)));
        close(m_listenFd);
        return false;
    }

    // �󶨵�ַ
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(m_ip.c_str());
    serverAddr.sin_port = htons(m_port);

    if (bind(m_listenFd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        log("Failed to bind: " + std::string(strerror(errno)));
        close(m_listenFd);
        return false;
    }

    // ��������
    if (listen(m_listenFd, SOMAXCONN) == -1) {
        log("Failed to listen: " + std::string(strerror(errno)));
        close(m_listenFd);
        return false;
    }

    // ����epollʵ��
    m_epollFd = epoll_create1(0);
    if (m_epollFd == -1) {
        log("Failed to create epoll: " + std::string(strerror(errno)));
        close(m_listenFd);
        return false;
    }

    // ��Ӽ���socket��epoll
    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = m_listenFd;

    if (epoll_ctl(m_epollFd, EPOLL_CTL_ADD, m_listenFd, &event) == -1) {
        log("Failed to add listen socket to epoll: " + std::string(strerror(errno)));
        close(m_epollFd);
        close(m_listenFd);
        return false;
    }

    // ���÷�����
    setNonBlocking(m_listenFd);

    return true;
}

void CServerSocket::setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        log("Failed to get socket flags: " + std::string(strerror(errno)));
        return;
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        log("Failed to set socket non-blocking: " + std::string(strerror(errno)));
    }
}

void CServerSocket::log(const std::string message) const {
    std::cout << "[ServerSocket] " << message << std::endl;
}

bool CServerSocket::sendPacketToClient(int clientId, const CPacket& packet) {
    auto& clientManager = m_command->getClientManager();
    int clientSocket = clientManager.getSocketByClientId(clientId);
    if (clientSocket == -1) {
        log("Client not found for sending packet: " + std::to_string(clientId));
        return false;
    }

    // ���л����ݰ�
    const char* packetData = packet.Data();
    int packetSize = packet.Size();
    ssize_t bytesSent = send(clientSocket, packetData, packetSize, 0);

    if (bytesSent == -1) {
        log("Failed to send packet to client " + std::to_string(clientId) +
            ": " + std::string(strerror(errno)));
        return false;
    }

    if (bytesSent != static_cast<ssize_t>(packetSize)) {
        log("Partial send to client " + std::to_string(clientId) +
            ": " + std::to_string(bytesSent) + "/" + std::to_string(packetSize));
        return false;
    }

    log("Successfully sent packet to client " + std::to_string(clientId) +
        ", bytes: " + std::to_string(bytesSent) + "/" + std::to_string(packetSize));
    return true;
}



