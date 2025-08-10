#pragma once
#include "Packet.h"
#include "ClientManager.h"

// ǰ������
class CCommand;
#include <sys/socket.h>
#include <iostream>
#include <map>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <memory>
#include <vector>

#define MAX_EVENTS 1024 
#define MAX_CLIENTS 100
#define BUFFER_SIZE 2048
#define DEFAULT_PORT 8080

// ������Socket�� - ��������ͨ�źͿͻ������ӹ���
// ʹ��epoll���и�Ч���¼�����I/O����
class CServerSocket
{
public:
    CServerSocket(const std::string& ip = "127.0.0.1", int port = DEFAULT_PORT);
    ~CServerSocket();

    // �������������ڹ���
    bool start();      // ����������
    void stop();       // ֹͣ������
    void run();        // ���з�������ѭ��

    // ״̬��ѯ
    bool isRunning() const {
        return m_running;
    }

    // ��ȡ���ӵĿͻ�������
    int getClientCount() const;

    // ����ͨ�Žӿ�
    bool sendPacketToClient(int clientId, const CPacket& packet);

    // ��ȡCommandʵ�������ã���������ServerSocketָ��
    CCommand* getCommand();

private:
    bool m_running;                                    // ����������״̬
    std::unique_ptr<CCommand> m_command;               // �������
    int m_listenFd;                                    // ����socket�ļ�������
    int m_epollFd;                                     // epollʵ���ļ�������
    int m_port;                                        // �������˿�
    int m_nextClientId;                                // ��һ���ͻ���ID
    std::string m_ip;                                  // ������IP��ַ

    // ��������ʼ��
    bool initialize();

    // ��־��¼
    void log(const std::string message) const;

    // Socket����
    void setNonBlocking(int fd);

    // �ͻ������ӹ���
    void handleNewConnection();                        // ����������
    void addClientToEpoll(int clientSocket);          // ��ӿͻ��˵�epoll
    void removeClientFromEpoll(int clientSocket);     // ��epoll�Ƴ��ͻ���

    // �ͻ������ݴ���
    void handleClientData(int clientSocket);          // ����ͻ�������
    void handleClientDisconnect(int clientSocket);    // ����ͻ��˶Ͽ�
    void handlePacket(int clientSocket, const CPacket& packet); // �������ݰ�
};

