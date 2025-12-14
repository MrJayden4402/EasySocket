#include "EasySocket.h"

namespace EasySocket
{
    SOCKET easySocket;
    vector<SOCKET> clientSockets;
    mutex clientSocketsMutex;
    thread_local string errorInfo;
    thread_local mutex errorInfoMutex;
    deque<char> messBuffer;
    mutex messBufferMutex;
    int serverState = EASY_SERVER_OK;
    mutex serverStateMutex;
    condition_variable serverStateCV;
};

bool EasySocketStart(void)
{
    using namespace EasySocket;
    // 初始化 Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        lock_guard<mutex> lock(errorInfoMutex);
        errorInfo = "Failed to initialize winsock";
        return false;
    }

    easySocket = socket(AF_INET, SOCK_STREAM, 0);
    if (easySocket == INVALID_SOCKET)
    {
        lock_guard<mutex> lock(errorInfoMutex);
        errorInfo = "Failed to create client socket";
        WSACleanup();
        return false;
    }
    return true;
}

void __Easy_ReceiveThread(void)
{
    using namespace EasySocket;

    char buffer[4096];

    while (true)
    {
        memset(buffer, 0, sizeof(buffer));
        int recvBytes = recv(easySocket, buffer, 4096, 0);

        lock_guard<mutex> lock(messBufferMutex);
        if (recvBytes == SOCKET_ERROR)
        {
            serverState = EASY_SERVER_ERROR;
            break;
        }
        else if (recvBytes == 0)
        {
            serverState = EASY_SERVER_DISCONNECT;
            break;
        }

        for (int i = 0; i < recvBytes; i++)
            messBuffer.push_back(buffer[i]);
    }
}

bool EasyConnectToServer(string IPv4, unsigned short port)
{
    using namespace EasySocket;

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port); // Port

    if (!inet_pton(AF_INET, IPv4.c_str(), &serverAddr.sin_addr.s_addr)) // IP
    {
        lock_guard<mutex> lock(errorInfoMutex);
        errorInfo = "Invalid IP address";
        return false;
    }

    // 连接到服务器
    if (connect(easySocket, reinterpret_cast<sockaddr *>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR)
    {
        lock_guard<mutex> lock(errorInfoMutex);
        errorInfo = "Failed to connect to the server, Code: ";
        errorInfo += to_string(WSAGetLastError());
        closesocket(easySocket);
        WSACleanup();
        return false;
    }

    serverState = EASY_SERVER_OK;

    thread recvThread(__Easy_ReceiveThread);
    recvThread.detach();

    return true;
}
void EasySocketClose(void)
{
    using namespace EasySocket;

    closesocket(easySocket);
    WSACleanup();
}

template <typename DataType>
bool __Easy_SendMessage(DataType &mess)
{
    using namespace EasySocket;

    static mutex sendMutex;
    lock_guard<mutex> lock(sendMutex);

    // 先发送消息长度
    string len = to_string(mess.size());
    if (len.size() > 8)
    {
        lock_guard<mutex> lock(errorInfoMutex);
        errorInfo = "Message too long";
        return false;
    }
    while (len.size() < 8)
        len = "0" + len;
    if (send(easySocket, len.c_str(), len.size(), 0) == SOCKET_ERROR)
    {
        lock_guard<mutex> lock(errorInfoMutex);
        errorInfo = "Failed to send message, Code: ";
        errorInfo += to_string(WSAGetLastError());
        return false;
    }
    // 再发送消息内容
    if (send(easySocket, mess.data(), mess.size(), 0) == SOCKET_ERROR)
    {
        lock_guard<mutex> lock(errorInfoMutex);
        errorInfo = "Failed to send message, Code: ";
        errorInfo += to_string(WSAGetLastError());
        return false;
    }
    return true;
}

bool EasySendMessage(vector<char> &mess)
{
    return __Easy_SendMessage(mess);
}

bool EasySendMessage(string &mess)
{
    return __Easy_SendMessage(mess);
}

const int EASY_SERVER_ERROR = -1;
const int EASY_SERVER_OK = 0;
const int EASY_SERVER_DISCONNECT = 1;

bool EasyGetMessage(vector<char> &mess)
{
    using namespace EasySocket;

    mess.clear();

    {
        lock_guard<mutex> lock(errorInfoMutex);
        if (serverState == EASY_SERVER_DISCONNECT)
        {
            errorInfo = "Server disconnected.";
            return false;
        }
        else if (serverState == EASY_SERVER_ERROR)
        {
            errorInfo = "Unknown error. Code: " + to_string(WSAGetLastError());
            return false;
        }
    }

    lock_guard<mutex> lock(messBufferMutex);

    // 先读8位，表示消息长度
    if (messBuffer.size() < 8)
        return true;
    int len = 0;
    for (int i = 0; i < 8; i++)
        len = len * 10 + (messBuffer[i] - '0');
    // 再读len位，表示消息内容
    if (messBuffer.size() < len + 8)
        return true;

    for (int i = 0; i < 8; i++)
        messBuffer.pop_front();
    for (int i = 0; i < len; i++)
    {
        mess.push_back(messBuffer.front());
        messBuffer.pop_front();
    }
    return true;
}

void __Easy_ServerThread(SOCKET clientSocket)
{
    using namespace EasySocket;

    char buffer[4096];
    int bytesRead;

    while (true)
    {
        memset(buffer, 0, sizeof(buffer));
        // 接收客户端消息
        bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesRead <= 0)
        {
            // cout << "Client disconnected";
            break;
        }
        // cout << "Get message from client: " << buffer << endl;
        // 将消息转发给其他客户端
        clientSocketsMutex.lock();
        for (const auto &socket : clientSockets)
            if (socket != clientSocket)
                send(socket, buffer, bytesRead, 0);
        clientSocketsMutex.unlock();
    }
}

bool EasyRunServer(unsigned short port)
{
    using namespace EasySocket;

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    if (bind(easySocket, reinterpret_cast<sockaddr *>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR)
    {
        lock_guard<mutex> lock(errorInfoMutex);
        errorInfo = "Failed to bind, Code: ";
        errorInfo += to_string(WSAGetLastError());
        closesocket(easySocket);
        WSACleanup();
        return false;
    }

    if (listen(easySocket, SOMAXCONN) == SOCKET_ERROR)
    {
        lock_guard<mutex> lock(errorInfoMutex);
        errorInfo = "Failed to listen, Code: ";
        errorInfo += to_string(WSAGetLastError());
        closesocket(easySocket);
        WSACleanup();
        return false;
    }

    auto AcceptThread = []()
    {
        // 接受新的客户端连接
        while (true)
        {
            SOCKET clientSocket = accept(easySocket, nullptr, nullptr);
            if (clientSocket == INVALID_SOCKET)
                break;

            // 将新的客户端连接添加到列表中
            clientSocketsMutex.lock();
            clientSockets.push_back(clientSocket);
            clientSocketsMutex.unlock();

            // 创建新线程来处理每个客户端的消息
            thread clientThread(__Easy_ServerThread, clientSocket);

            clientThread.detach();
        }
    };
    thread acceptThread(AcceptThread);
    acceptThread.detach();

    unique_lock<mutex> lock(serverStateMutex);

    serverStateCV.wait(lock, []()
                       { return serverState == EASY_SERVER_DISCONNECT; });

    for (auto &clientSocket : clientSockets)
    {
        shutdown(clientSocket, SD_SEND);
        closesocket(clientSocket);
    }

    closesocket(easySocket);

    return true;
}

void EasyStopServer(void)
{
    using namespace EasySocket;
    lock_guard<mutex> lock(serverStateMutex);
    serverState = EASY_SERVER_DISCONNECT;
    serverStateCV.notify_all();
}

string GetEasySocketErrorInfo(void)
{
    using namespace EasySocket;
    lock_guard<mutex> lock(errorInfoMutex);
    return errorInfo;
}