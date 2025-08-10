#include "ClientManager.h"
#include <iostream>
#include <algorithm>

ClientManager::ClientManager() {
    // 构造函数，初始化客户端管理器
}

void ClientManager::addClient(int clientSocket, int clientId, const std::string& ip, int port) {
    m_clients[clientId] = ClientInfo(clientSocket, clientId, ip, port);
    m_socketToClientId[clientSocket] = clientId;
    std::cout << "[ClientManager] Client added: Socket=" << clientSocket
        << ", ID=" << clientId << ", IP=" << ip << ", Port=" << port << std::endl;
    updateUserList();
}

void ClientManager::removeClient(int clientId) {
    auto it = m_clients.find(clientId);
    if (it != m_clients.end()) {
        int socket = it->second.socket;
        m_socketToClientId.erase(socket);
        m_clients.erase(it);
        std::cout << "[ClientManager] Client removed: ID=" << clientId
            << ", Socket=" << socket << std::endl;
        updateUserList();
    }
}

void ClientManager::removeClientBySocket(int clientSocket) {
    auto socketIt = m_socketToClientId.find(clientSocket);
    if (socketIt != m_socketToClientId.end()) {
        int clientId = socketIt->second;
        removeClient(clientId);
    }
}

void ClientManager::updateClientUsername(int clientId, const std::string& username) {
    auto it = m_clients.find(clientId);
    if (it != m_clients.end()) {
        it->second.username = username;
        std::cout << "[ClientManager] Username updated for client "
            << clientId << ": " << username << std::endl;
    }
}

void ClientManager::updateClientConnectionStatus(int clientId, bool connected) {
    auto it = m_clients.find(clientId);
    if (it != m_clients.end()) {
        it->second.isConnected = connected;
        std::cout << "[ClientManager] Client " << clientId
            << " connection status: " << (connected ? "connected" : "disconnected") << std::endl;
    }
}

bool ClientManager::hasClient(int clientId) const {
    return m_clients.find(clientId) != m_clients.end();
}

bool ClientManager::hasClientBySocket(int clientSocket) const {
    return m_socketToClientId.find(clientSocket) != m_socketToClientId.end();
}

const ClientInfo* ClientManager::getClient(int clientId) const {
    auto it = m_clients.find(clientId);
    return (it != m_clients.end()) ? &(it->second) : nullptr;
}

ClientInfo* ClientManager::getClient(int clientId) {
    auto it = m_clients.find(clientId);
    return (it != m_clients.end()) ? &(it->second) : nullptr;
}

int ClientManager::getClientIdBySocket(int clientSocket) const {
    auto it = m_socketToClientId.find(clientSocket);
    return (it != m_socketToClientId.end()) ? it->second : -1;
}

int ClientManager::getSocketByClientId(int clientId) const {
    auto it = m_clients.find(clientId);
    return (it != m_clients.end()) ? it->second.socket : -1;
}

std::vector<std::string> ClientManager::getUserList() const {
    std::vector<std::string> userList;
    for (const auto& pair : m_clients) {
        if (!pair.second.username.empty()) {
            userList.push_back(pair.second.username);
        }
    }
    return userList;
}

std::vector<int> ClientManager::getConnectedClientIds() const {
    std::vector<int> connectedIds;
    for (const auto& pair : m_clients) {
        if (pair.second.isConnected) {
            connectedIds.push_back(pair.first);
        }
    }
    return connectedIds;
}

void ClientManager::appendToBuffer(int clientId, const std::string& data) {
    auto it = m_clients.find(clientId);
    if (it != m_clients.end()) {
        it->second.receiverBuffer += data;
    }
}

std::string ClientManager::getBuffer(int clientId) const {
    auto it = m_clients.find(clientId);
    return (it != m_clients.end()) ? it->second.receiverBuffer : "";
}

void ClientManager::clearBuffer(int clientId) {
    auto it = m_clients.find(clientId);
    if (it != m_clients.end()) {
        it->second.receiverBuffer.clear();
    }
}

void ClientManager::removeFromBuffer(int clientId, size_t bytes) {
    auto it = m_clients.find(clientId);
    if (it != m_clients.end() && it->second.receiverBuffer.size() >= bytes) {
        it->second.receiverBuffer.erase(0, bytes);
    }
}

void ClientManager::updateUserList() {
    // 更新用户列表的逻辑可以在这里实现
    // 目前只是占位符
}