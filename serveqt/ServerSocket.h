#pragma once
#include "Packet.h"
#include "ClientManager.h"

// 前向声明
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

// 服务器Socket类 - 负责网络通信和客户端连接管理
// 使用epoll进行高效的事件驱动I/O处理
class CServerSocket
{
public:
    CServerSocket(const std::string& ip = "127.0.0.1", int port = DEFAULT_PORT);
    ~CServerSocket();

    // 服务器生命周期管理
    bool start();      // 启动服务器
    void stop();       // 停止服务器
    void run();        // 运行服务器主循环

    // 状态查询
    bool isRunning() const {
        return m_running;
    }

    // 获取连接的客户端数量
    int getClientCount() const;

    // 网络通信接口
    bool sendPacketToClient(int clientId, const CPacket& packet);

    // 获取Command实例的引用，用于设置ServerSocket指针
    CCommand* getCommand();

private:
    bool m_running;                                    // 服务器运行状态
    std::unique_ptr<CCommand> m_command;               // 命令处理器
    int m_listenFd;                                    // 监听socket文件描述符
    int m_epollFd;                                     // epoll实例文件描述符
    int m_port;                                        // 服务器端口
    int m_nextClientId;                                // 下一个客户端ID
    std::string m_ip;                                  // 服务器IP地址

    // 服务器初始化
    bool initialize();

    // 日志记录
    void log(const std::string message) const;

    // Socket配置
    void setNonBlocking(int fd);

    // 客户端连接管理
    void handleNewConnection();                        // 处理新连接
    void addClientToEpoll(int clientSocket);          // 添加客户端到epoll
    void removeClientFromEpoll(int clientSocket);     // 从epoll移除客户端

    // 客户端数据处理
    void handleClientData(int clientSocket);          // 处理客户端数据
    void handleClientDisconnect(int clientSocket);    // 处理客户端断开
    void handlePacket(int clientSocket, const CPacket& packet); // 处理数据包
};

