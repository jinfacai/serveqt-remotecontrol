#pragma once
#include <map>
#include <string>
#include <vector>
#include <memory>

struct ClientInfo {
    int socket;                   
    int id;                        
    std::string ip;               
    int port;                      
    std::string username;          
    bool isConnected;              
    std::string receiverBuffer;    

    ClientInfo() : socket(-1), id(-1), ip(""), port(0), isConnected(false) {}

    ClientInfo(int clientSocket, int clientId, const std::string& clientIp, int clientPort)
        : socket(clientSocket), id(clientId), ip(clientIp), port(clientPort), isConnected(true) {
    }

    ClientInfo(const ClientInfo& other)
        : socket(other.socket), id(other.id), ip(other.ip), port(other.port),
        username(other.username), isConnected(other.isConnected),
        receiverBuffer(other.receiverBuffer) {
    }

    ClientInfo& operator=(const ClientInfo& other) {
        if (this != &other) {
            socket = other.socket;
            id = other.id;
            ip = other.ip;
            port = other.port;
            username = other.username;
            isConnected = other.isConnected;
            receiverBuffer = other.receiverBuffer;
        }
        return *this;
    }
};

class ClientManager {
public:
    ClientManager();
    ~ClientManager() = default;

    // �ͻ��˹���
    void addClient(int clientSocket, int clientId, const std::string& ip, int port);
    void removeClient(int clientId);
    void removeClientBySocket(int clientSocket);
    void updateClientUsername(int clientId, const std::string& username);
    void updateClientConnectionStatus(int clientId, bool connected);

    // �ͻ��˲�ѯ
    bool hasClient(int clientId) const;
    bool hasClientBySocket(int clientSocket) const;

    // ��ȡ�ͻ�����Ϣ
    const ClientInfo* getClient(int clientId) const;
    ClientInfo* getClient(int clientId);

    // ������ѯ�ӿ�
    const std::map<int, ClientInfo>& getAllClients() const { return m_clients; }
    std::vector<std::string> getUserList() const;
    std::vector<int> getConnectedClientIds() const;

    // ͳ����Ϣ
    size_t getClientCount() const { return m_clients.size(); }

    // ���绺��������
    void appendToBuffer(int clientId, const std::string& data);
    std::string getBuffer(int clientId) const;
    void clearBuffer(int clientId);
    void removeFromBuffer(int clientId, size_t bytes);

    // Socketӳ�����
    int getClientIdBySocket(int clientSocket) const;
    int getSocketByClientId(int clientId) const;

private:
    std::map<int, ClientInfo> m_clients;           // clientId -> ClientInfo
    std::map<int, int> m_socketToClientId;         // socket -> clientId ӳ��

    // ��������
    void updateUserList();
};