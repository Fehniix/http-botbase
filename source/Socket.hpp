#ifndef SOCKET_HPP
#define SOCKET_HPP

#include <string>
#include <iostream>
#include <switch.h>
#include <sstream>
#include <sys/socket.h>
#include <arpa/inet.h> // This contains inet_addr
#include <unistd.h> // This contains close

#define HPP_INVALID_SOCKET (SOCKET)(~0)
#define SOCKET_ERROR (-1)
#define LOGS_ENABLED 0
#define __log(msg) PrivateLogger::_log(msg)

typedef int SOCKET;

class PrivateLogger {
    public:
        inline static void _log(const char *msg) {
            if (!LOGS_ENABLED)
                return;
            std::cout << msg << std::endl;
        }

        inline static void _log(const std::string &msg) {
            if (!LOGS_ENABLED)
                return;
            std::cout << msg << std::endl;
        }
};



class Socket {
public:
    enum class SocketType {
        TYPE_STREAM = SOCK_STREAM,
        TYPE_DGRAM = SOCK_DGRAM
    };

protected:
    explicit Socket(SocketType socket_type);
    ~Socket();
    SOCKET m_socket;
    sockaddr_in m_addr;
    void set_port(u_short port);
    int set_address(const std::string& ip_address);
};

class UDPClient : public Socket
{
public:
    UDPClient(u_short port = 8000, const std::string& ip_address = "127.0.0.1");
    ssize_t send_message(const std::string& message);
};

class UDPServer : public Socket
{
public:
    UDPServer(u_short port = 8000, const std::string& ip_address = "0.0.0.0");
    int socket_bind();
    void listen();
};

class TCPClient : public Socket
{
public:
    TCPClient(u_short port = 8000, const std::string& ip_address = "127.0.0.1");
    int make_connection();
    int send_message(const std::string& message);
};

class TCPServer : public Socket
{
public:
    TCPServer(u_short port, const std::string& ip_address = "0.0.0.0");
    int socket_bind();
};
#endif