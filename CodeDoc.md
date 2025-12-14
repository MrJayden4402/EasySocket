# EasySocket 2.0 代码文档

## 1. 库概述

该库是基于winsock2实现的网络库，提供了一些简化接口，可以进行简单的网络传输。

声明包含在 EasySocket.h 中。\
实现包含在 EasySocket.cpp 中。

实现中包含一些以__Easy_开头的函数，这些函数是库的内部函数，除非你知道你在做什么，否则你不应该直接调用它们。

使用这个库需要链接一系列lib，具体为加入以下参数：

- -lws2_32

该库内部有线程锁，所以应当是线程安全的。

## 2. 库函数/类

### 2.1 EasySocketStart EasySocketClose 函数

EasySocketStart用于初始化库，必须在调用任何其他函数之前调用。\
头部如下：
```cpp
bool EasySocketStart(void);
```
会返回一个bool值，表示是否初始化成功。

EasySocketClose 用于关闭连接并关闭库，头部如下：
```cpp
void EasySocketClose(void);
```


#### 2.2 服务器端

##### 2.2.1 EasyRunServer 函数

用于启动服务器，头部如下：

```cpp
bool EasyRunServer(int port);
```

参数为端口号，返回值为bool值，表示是否启动成功。

注意：该函数会阻塞当前线程，直到服务器关闭。

##### 2.2.2 EasyStopServer 函数

用于关闭服务器，头部如下：

```cpp
void EasyStopServer(void);
```


#### 2.3 客户端

##### 2.3.1 EasyConnectToServer 函数

用于连接服务器，头部如下：

```cpp
bool EasyConnectToServer(string IPv4, unsigned short port);
```

参数为IPv4地址和端口号，返回值为bool值，表示是否连接成功。


##### 2.3.2 EasySendMessage 函数

给服务器发送一个字节流，头部如下：

```cpp
bool EasySendMessage(vector<char> &mess);
bool EasySendMessage(string &mess);
```

参数为要发送的消息，返回值为bool值，表示是否发送成功。

暂时无法指定用户，服务器会将收到的消息转发给所有用户。

##### 2.3.3 EasyGetMessage 函数

从服务器接收一个字节流，头部如下：

```cpp
bool EasyGetMessage(vector<char> &mess);
```

参数为接收消息的容器，返回值为bool值，表示是否接收成功。

#### 2.4 错误处理 GetEasySocketErrorInfo 函数

返回值为false时，表示发生了错误，此时可以调用GetEasySocketErrorInfo函数获取错误信息，头部如下：

```cpp
string GetEasySocketErrorInfo(void);
```

返回值为错误信息。

## 3. 历史

1.0 - 2023.9.21\
实现基本功能，但是没有错误处理，且会有线程竟态问题。


By MrJayden.