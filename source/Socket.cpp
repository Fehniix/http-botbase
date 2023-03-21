#include "Socket.hpp"

// These could also be enums
const int socket_bind_err = 3;
const int socket_accept_err = 4;
const int connection_err = 5;
const int message_send_err = 6;
const int receive_err = 7;

Socket::Socket(const SocketType socket_type) : m_socket(), m_addr()
{
    // Create the socket handle
    m_socket = socket(AF_INET, static_cast<int>(socket_type), 0);
    if (m_socket == HPP_INVALID_SOCKET)
    {
        fatalThrow(0x999);
    }

    m_addr.sin_family = AF_INET;
}

void Socket::set_port(u_short port)
{
    m_addr.sin_port = htons(port);
}

int Socket::set_address(const std::string& ip_address)
{
    return inet_pton(AF_INET, ip_address.c_str(), &m_addr.sin_addr);
}

Socket::~Socket() = default;

UDPClient::UDPClient(u_short port, const std::string& ip_address) : Socket(SocketType::TYPE_DGRAM)
{
    set_address(ip_address);
    set_port(port);
    __log("UDP Client created.");
};

ssize_t UDPClient::send_message(const std::string& message)
{
    size_t message_length = message.length();
    return sendto(m_socket, message.c_str(), message_length, 0, reinterpret_cast<sockaddr*>(&m_addr), sizeof(m_addr));
};

UDPServer::UDPServer(u_short port, const std::string& ip_address) : Socket(SocketType::TYPE_DGRAM)
{
    set_port(port);
    set_address(ip_address);
    __log("UDP Server created.");
}

int UDPServer::socket_bind()
{
    if (bind(m_socket, reinterpret_cast<sockaddr*>(&m_addr), sizeof(m_addr)) == SOCKET_ERROR)
    {
        __log("UDP Bind error.");
        return socket_bind_err;
    }
    __log("UDP Socket Bound.");
    return 0;
}

void UDPServer::listen()
{
    sockaddr_in client;
    char client_ip[INET_ADDRSTRLEN];
    socklen_t slen = sizeof(client);
    char message_buffer[512];
    __log("Waiting for data...");

    while(true)
    {
        // This is a blocking call
        ssize_t recv_len = recvfrom(m_socket, message_buffer, sizeof(message_buffer), 0, reinterpret_cast<sockaddr*>(&client), &slen);
        if (recv_len == SOCKET_ERROR)
        {
            __log("Receive Data error.");
        }
        
        std::stringstream ss;
        ss << "Received packet from " << inet_ntop(AF_INET, &client.sin_addr, client_ip, INET_ADDRSTRLEN) << ':' << ntohs(client.sin_port) << std::endl;
        __log(ss.str());
        __log(message_buffer);
    }
}

TCPClient::TCPClient(u_short port, const std::string& ip_address) : Socket(SocketType::TYPE_STREAM)
{
    set_address(ip_address);
    set_port(port);
    __log("TCP client created.");
}

int TCPClient::make_connection()
{
    __log("Connecting");
    int err = connect(m_socket, reinterpret_cast<sockaddr*>(&m_addr), sizeof(m_addr));
    if (err < 0) {
        __log("Connection error");
        return err;
    }
    __log("connected");

    return 0;
}

int TCPClient::send_message(const std::string& message, bool waitForResponse = false)
{
    char server_reply[2000];
    size_t length = message.length();

    if (send(m_socket, message.c_str(), length, 0) < 0) {
        __log("Send failed");
        return message_send_err;
    } else
        __log("Data sent");

    if (!waitForResponse)
        return 0;

    if (recv(m_socket, server_reply, 2000, 0) == SOCKET_ERROR) {
        __log("Receive Failed");
        return receive_err;
    } else
        __log(server_reply);

    return 0;
}

TCPServer::TCPServer(u_short port, const std::string& ip_address) : Socket(SocketType::TYPE_STREAM)
{
    set_port(port);
    set_address(ip_address);
    __log("TCP Server created.");
};

int TCPServer::socket_bind()
{
    if (bind(m_socket, reinterpret_cast<sockaddr*>(&m_addr), sizeof(m_addr)) == SOCKET_ERROR)
    {
        __log("TCP Socket Bind error.");
        return socket_bind_err;
    }

    __log("TCP Socket Bound.");
    listen(m_socket, 3);
    __log("TCP Socket waiting for incoming connections...");

    socklen_t client_size = sizeof(sockaddr_in);
    sockaddr_in client;
    SOCKET new_socket;
    char message_buffer[512];

    new_socket = accept(m_socket, reinterpret_cast<sockaddr*>(&client), &client_size);
    if (new_socket == HPP_INVALID_SOCKET)
    {
        __log("TCP Socket accept error");
        return socket_accept_err;
    }
    else
    {
        // std::cout << "Connection accepted from IP address " << inet_ntoa(client.sin_addr) << " on port " << ntohs(client.sin_port) << std::endl;
        ssize_t recv_len = recv(new_socket, message_buffer, sizeof(message_buffer), 0);
        __log((std::string("Incoming message is:\n") + message_buffer).c_str());
        __log("Message length was: " + recv_len);
        std::string message = "Your message has been received client\n";
        size_t message_length = message.length();
        send(new_socket, message.c_str(), message_length, 0);
    }
    close(m_socket);
    return 0;
}