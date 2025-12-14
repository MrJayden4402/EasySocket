#include <vector>
#include <thread>
#include <mutex>
#include <deque>
#include <condition_variable>

#include <winsock2.h>
#include <ws2tcpip.h>

using namespace std;
namespace EasySocket
{
    extern SOCKET easySocket;

    extern vector<SOCKET> clientSockets;
    extern mutex clientSocketsMutex;

    extern thread_local string errorInfo;
    extern thread_local mutex errorInfoMutex;
    extern deque<char> messBuffer;
    extern mutex messBufferMutex;
    extern int serverState;
    extern mutex serverStateMutex;
};

extern const int EASY_SERVER_ERROR;
extern const int EASY_SERVER_OK;
extern const int EASY_SERVER_DISCONNECT;

bool EasySocketStart(void);
bool EasyConnectToServer(string IPv4, unsigned short port);
void EasySocketClose(void);
bool EasySendMessage(vector<char> &mess);
bool EasySendMessage(string &mess);
bool EasyGetMessage(vector<char> &mess);
bool EasyRunServer(unsigned short port);
void EasyStopServer(void);
string GetEasySocketErrorInfo(void);