## 项目需求

本项目是一个仿QQ聊天的即时通讯系统，主要业务有登录、注册、注销、添加好友、创建或加入群组、一对一聊天以及群组聊天等。

## 开发环境

* ubuntu
* 安装Json开发库
* 安装boost+muduo网络库开发环境，参考[C++ muduo网络库知识分享01 - Linux平台下muduo网络库源码编译安装]([C++ muduo网络库知识分享01 - Linux平台下muduo网络库源码编译安装-CSDN博客](https://blog.csdn.net/QIANGWEIYUAN/article/details/89023980))
* 安装redis环境
* 安装mysql数据库环境
* 安装nginx
* 安装CMake环境

## 关键技术点

* Json序列化和反序列化
* 基于muduo网络库的服务端程序开发
* Nginx的TCP负载均衡器配置
* 基于发布-订阅的Redis消息队列编程实战
* MySQL数据库编程
* CMake构建编译环境

## 基础知识

### Json数据序列化方法

Json是一种**完全独立于编程语言**的文本格式数据存储方式（两个主机使用的编程语言可能不同），常用的数据传输序列化格式有XML，Json，ProtoBuf（公司级别项目用ProtoBuf，压缩编码传输，是1/10Json，1/20XML）。

(1) json第三方库的使用

```C++
#include "json.hpp"
using json = nlohmann::json
```

(2) json的数据序列化方式（把数据处理成Json字符串）

普通数据序列化（可以添加数组/二维）

```c++
string func1() {
    json js;
    js["msg_type"] = 2;
    js["from"] = "zhangsan";
    js["to"] = "lisi";
    js["msg"] = "hello";
    string sendBuf = js.dump(); // 将 json 转换为字符串
    return sendBuf;
}
/*
* 测试结果：{"from":"zhangsan","msg":"hello","msg_type":2,"to":"lisi"}
*/
```

容器序列化（直接将容器放进json）

```c++
string func2(){
    json js;
    vector<int> vec{1,2,3};
    js["list"] = vec;
    unordered_map<int, string> map;
    map.insert({1, "黄山"});
    map.insert({2, "华山"});
    map.insert({3, "高山"});
    js["path"] = map;
    string sendBuf = js.dump(); // 将 json 转换为字符串
    return sendBuf;
}
/*
* 测试结果：{"list":[1,2,3],"path":[[3,"高山"],[1,"黄山"],[2,"华山"]]}
*/
```

(3) json数据反序列化（把json字符串反序列化出对象/容器）

```c++
string jsonstr = func2();
json js = json::parse(jsonstr); // 反序列化
unordered_map<int, string> map = js["path"];
cout << map[1] << endl;
```

### muduo网络库

muduo网络库给用户提供了两个主要的类，用于编写服务器程序的TcpServer和用于编写客户端程序TcpClient。muduo网络库的优点是能够把网络I/O（用户的连接和断开）和业务代码（用户的可读写事件）区分开。

**基于muduo网络库开发服务器程序的步骤：**

1. 组合TcpServer对象
2. 创建EventLoop事件循环对象的指针
3. 明确TcpServer构造函数需要什么参数，输出ChatServer的构造函数
4. 在当前服务器类的构造函数中，注册处理连接的回调函数和处理读写事件的回调函数
5. 设置合适的服务端线程数量，muduo库会自己分配I/O线程和worker线程

示例：

```c++
#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <iostream>
#include <functional>
#include <string>

using namespace std;
using namespace muduo;
using namespace muduo::net;
using namespace placeholders;

class ChatServer {
public:
    ChatServer(EventLoop* loop, // 事件循环
            const InetAddress& listenAddr, // IP+Port
            const string& nameArg) // 服务器的名字
        : _server(loop, listenAddr,nameArg), _loop(loop)
    {
        // 给服务器注册用户连接的创建和断开的回调
        _server.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1));
        // 给服务器注册用户读写事件回调
        _server.setMessageCallback(std::bind(&ChatServer::onMessage, this, _1, _2, _3));
        // 设置服务器端的线程数量 1 I/O + 3 worker
        _server.setThreadNum(4);
    }

    // 开启事件循环
    void start() {
        _server.start();
    }
private:
    // 专门处理用户的连接创建和断开 epoll listenfd accept
    void onConnection(const TcpConnectionPtr& conn) {
        if(conn->connected()) {
            cout << conn->peerAddress().toIpPort() << "->" << conn->localAddress().toIpPort() << "state::online" << endl;
        }
        else {
            cout << conn->peerAddress().toIpPort() << "->" << conn->localAddress().toIpPort() << "state::offline" << endl;
            conn->shutdown();
            // _loop->quit();
        }
    }

    // 专门处理用户的读写事件
    void onMessage(const TcpConnectionPtr& conn, Buffer* buffer, Timestamp time) { // 连接、缓冲区、时间戳
        string buf = buffer->retrieveAllAsString();
        cout << "recv data:" << buf << "time" << time.toString() << endl;
        conn->send(buf);
    }
    TcpServer _server;
    EventLoop* _loop;
};
int main() {

    EventLoop loop;
    InetAddress addr("127.0.0.1", 6000);
    ChatServer server(&loop, addr, "ChatServer");

    server.start(); // listenfd epoll_ctl=>epoll
    loop.loop(); // epoll_wait 以阻塞方式等待新用户连接、已连接用户的读写事件

    return 0;
}
```

编译：

```bash
g++ muduo_server.cpp -o server -lmuduo_net -lmuduo_base -lpthread
```

{% note success %}
TIP

-I头文件搜索路径 -L库文件搜索路径 -l库名 

/usr/include /usr/local/include 下的文件会被程序默认包含，不需要特意写出

{% endnote %}

运行测试：

```bash
terminal1:
./server
terminal2:
telnet 127.0.0.1 6000
```

![](./集群聊天服务器/image-20231229194222707.png)

> 按ctrl+]键结束，输入quit退出

### CMake

CMake是一种跨平台的编译工具，可以通过CMakeLists.txt文件定制整个编译流程，再生成make所需要的makefile文件，最后使用make命令编译源码生成可执行程序或共享库。

#### CMake的安装

ubuntu CMake的安装：

```bash
$ sudo apt install cmake
$ cmake -version
```

VS Code CMake的安装：

(1) VS Code 扩展->搜索CMake->点击CMake和CMake Tools安装（注意：远程主机中也要安装）

![](./集群聊天服务器/image-20231229202506032.png)

(2) 点击CMake Tools右侧齿轮->Extension Setting（扩展设置）->远程->Cmake:Build Environment->添加项

项：`cmake.cmakePath` 值：`/usr/bin/cmake`

![](./集群聊天服务器/image-20231229203455632.png)

#### CMake的使用

标准的文件组织：

```
bin: 可执行文件
lib: 库文件
include: 头文件
src: 源文件
build: 编译过程中产生的临时文件
example: 测试文件
thridparty: 依赖的第三方库文件
CMakeList.txt
autobulid.sh: 一键编译
```

创建bin和build目录，并新建CMakeLists.txt文件，结构如下：

![](./集群聊天服务器/image-20231229205244343.png)

编写CMakeLists.txt文件：

```cmake
cmake_minimum_required(VERSION 3.0) # 要求cmake最低的版本号
project(main) # 定义当前工程名字

# 配置编译选项
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -g)

# 设置可执行文件最终存储的路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

# 设置需要编译的源文件列表
set(SRC_LIST ./muduo_server.cpp)

# 表示生成可执行文件server，由SRC_LIST变量所定义的源文件编译而来
add_executable(server ${SRC_LIST})

# 表示server这个目标程序，需要链接 muduo_net muduo_base pthread 这三个库文件
target_link_libraries(server muduo_net muduo_base pthread)
```

执行命令`cmake PATH`生成Makefile：

```bash
cd build
cmake ..
```

使用`make`命令进行编译：

```bash
make
```

在bin目录下成功生成了可执行文件。

#### 在VS Code下配置CMake

(1) 在chat的同级目录下新建CMakeLists.txt文件，将前面编写的CMakeLists.txt文件中的前两行**剪切**到新文件中，同时添加指定工程的子目录。

![](./集群聊天服务器/image-20231229211801320.png)

```cmake
cmake_minimum_required(VERSION 3.0) # 要求cmake最低的版本号
project(main) # 定义当前工程名字

# 指定工程的子目录
add_subdirectory(chat)
```

(2) 点击CMake->生成所有项目

![](./集群聊天服务器/image-20231229211843212.png)

(3) 执行完成后，在当前目录下会生成bin和build目录。

![](./集群聊天服务器/image-20231229211952719.png)

## 数据库表设计

**User表**

| 字段名称 |         字段类型          |   字段说明   |                 约束                  |
| :------: | :-----------------------: | :----------: | :-----------------------------------: |
|    id    |            INT            |    用户id    | PRIMARY KEY、AUTO_INCREMENT、NOT NULL |
|   name   |        VARCHAR(50)        |    用户名    |           NOT NULL, UNIQUE            |
| password |        VARCHAR(50)        |     密码     |               NOT NULL                |
|  state   | ENUM('online', 'offline') | 当前登录状态 |           DEFAULT 'offline'           |

```sql
CREATE TABLE `user`(id INT(11) PRIMARY KEY NOT NULL AUTO_INCREMENT,
    `name` VARCHAR(50) DEFAULT NULL UNIQUE,
    `password` VARCHAR(50) DEFAULT NULL,
    `state` ENUM('online','offline') DEFAULT 'offline')ENGINE=INNODB DEFAULT CHARSET=utf8;
```

**Friend 表**

| 字段名称 | 字段类型 | 字段说明 |        约束        |
| :------: | :------: | :------: | :----------------: |
|  userid  |   INT    |  用户id  | NOT NULL、联合主键 |
| friendid |   INT    |  好友id  | NOT NULL、联合主键 |

```sql
CREATE TABLE friend (
    userid INT(11) NOT NULL,
    friendid INT(11) NOT NULL,
    PRIMARY KEY (userid, friendid)
)ENGINE=INNODB DEFAULT CHARSET=utf8;
```

**AllGroup表**

| 字段名称  |  字段类型   |  字段说明  |            约束             |
| :-------: | :---------: | :--------: | :-------------------------: |
|    id     |     INT     |    组id    | PRIMARY KEY、AUTO_INCREMENT |
| groupname | VARCHAR(50) |   组名称   |      NOT NULL, UNIQUE       |
| groupdesc | VARCHAR(50) | 组功能描述 |         DEFAULT ' '         |

```sql
CREATE TABLE allgroup (
    id INT(11) PRIMARY KEY NOT NULL AUTO_INCREMENT,
    groupname VARCHAR(50) NOT NULL UNIQUE,
    groupdesc VARCHAR(50) DEFAULT ' '
)ENGINE=INNODB DEFAULT CHARSET=utf8;
```

**GroupUser表**

| 字段名称  |         字段类型          | 字段说明 |        约束        |
| :-------: | :-----------------------: | :------: | :----------------: |
|  groupid  |            INT            |   组id   | NOT NULL、联合主键 |
|  userid   |            INT            |  组员id  | NOT NULL、联合主键 |
| grouprole | ENUM('creator', 'normal') | 组内角色 | DEFAULT ' normal'  |

```sql
CREATE TABLE groupuser (
    groupid INT(11) NOT NULL,
    userid INT(11) NOT NULL,
    grouprole ENUM('creator', 'normal') DEFAULT 'normal',
    PRIMARY KEY (groupid, userid)
)ENGINE=INNODB DEFAULT CHARSET=utf8;
```

**OfflineMessage表**

| 字段名称 |   字段类型   |          字段说明          |   约束   |
| :------: | :----------: | :------------------------: | :------: |
|  userid  |     INT      |           用户id           | NOT NULL |
| message  | VARCHAR(500) | 离线消息（存储Json字符串） | NOT NULL |

```sql
CREATE TABLE offlinemessage (
    userid INT NOT NULL,
    message VARCHAR(500) NOT NULL
)ENGINE=INNODB DEFAULT CHARSET=utf8;
```

## 业务流程

基本业务有登录、注册和注销，在用户登录后，界面显示好友列表、群组列表和离线消息等个人信息，可以选择添加好友、创建或加入群组、一对一聊天以及群组聊天等服务，下面是业务流程的详细设计。

![](./集群聊天服务器/image-20240217155755520.png)

## 数据通信方式

客户端和服务端的通信使用JSON序列化和反序列化作为通信协议，对于不同的数据设计了不同的格式，具体设计如下：

```cpp
// 1.登录
json["msgid"] = LOGIN_MSG;
json["id"]			// 用户id
json["password"]	// 密码

// 2.登录响应
json["msgid"] = LOGIN_MSG_ACK;
json["id"]			// 用户id
json["name"]		// 用户名
json["offlinemsg"]	// 离线消息
json["friends"]		// 好友信息{id, name, state}
json["groups"]		// 群组信息{id, groupname, groupdesc，users{id, name，state, role}}
json["errno"]		// 错误号，0：登录成功，1：账号或密码错误，2：该用户已经登录，不允许重复登录
json["errmsg"]		// 错误信息

// 3.注册
json["msgid"] = REG_MSG;
json["name"]		//用户名
json["password"]	//用户密码

// 4.注册响应
json["msgid"] = REG_MSG_ACK;
json["id"]			// 返回用户注册的id
json["errno"]		// 错误号，0：成功，1：失败

// 5.添加好友
json["msgid"] = ADD_FRIEND_MSG;
json["id"]			// 当前用户id
json["friendid"]	// 好友id

// 6.一对一聊天
json["msgid"] = ONE_CHAT_MSG;
json["id"]			// 用户id
json["name"]		// 用户姓名
json["toid"]	    // 接受者id
json["msg"]			// 消息内容
json["time"]		// 发送时间

// 7.创建群
json["msgid"] = CREATE_GROUP_MSG;
json["id"]			// 群创建者id
json["groupname"]	// 群名
json["groupdesc"]	// 群描述

// 8.加入群
json["msgid"] = ADD_GROUP_MSG;
json["id"]			//用户id
json["groupid"]		//群id

9.群聊
json["msgid"] = GROUP_CHAT_MSG;
json["id"]			// id
json["name"]		// name
json["groupid"]		// groupid
json["msg"]			// 消息内容
json["time"]		// 发送时间

10.注销
json["msgid"] = LOGINOUT_MSG;
json["id"]			// id
```

## 网络和业务模块

### 网络模块

网络模块使用的是muduo提供的接口，muduo网络库的优点是能够把网络I/O（用户的连接和断开）和业务代码（用户的可读写事件）区分开，这样就可以专注于编写业务代码，而不需要考虑网络I/O的细节。

muduo网络库设计的核心思想是one loop per thread，有一个main reactor监听accept连接，然后把连接分发到某个sub reactor上（轮询的方式选择），sub reactor负责连接事件的处理。如果有过多耗费CPU I/O的计算任务，可以提交到线程池中处理。

muduo中提供了两个非常重要的回调函数：连接回调和读写回调

```cpp
// 注册连接回调
_server.setConnectionCallback(bind(&ChatServer::onConnection, this, _1));
// 注册读写回调
_server.setMessageCallback(bind(&ChatServer::onMessage, this, _1, _2, _3));
```

在_server上注册连接回调和读写回调函数，当网络IO事件或读写事件到达时，就会调用相应的回调函数进行处理。

### 业务模块

业务模块的设计采用线程安全的懒汉式单例模式，即一个类不管创建多少对象，永远只能得到该类型的一个对象实例。

**主要业务有：**

```cpp
// 处理登录业务
void login(const TcpConnectionPtr& conn, json &js, Timestamp time);
// 处理注册业务
void reg(const TcpConnectionPtr& conn, json &js, Timestamp time);
// 一对一聊天业务
void oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time);
// 添加好友业务
void addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time);
// 创建群组业务
void createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);
// 加入群组业务
void addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);
// 群组聊天业务
void groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time);
// 处理注销业务
void loginout(const TcpConnectionPtr &conn, json &js, Timestamp time);
```

**业务逻辑的处理主要可以分为两类：**

1. 登录/注册/添加好友/创建群组/注销：服务端将接收到的Json字符串反序列化，得到所需关键字对应的key值，按具体的业务逻辑进行处理后，再修改数据库中对应的用户信息。
2. 聊天业务：服务端检查私聊或群聊的对象是否在线，如果在线，就转发请求，如果不在线，就将内容存储到对应用户的离线消息中。

### 网络模块与业务模块解耦

![](./集群聊天服务器/image-20240102200736807.png)

如果在网络模块的读写回调中直接调用业务模块的方法，例如

```cpp
if (消息ID1) 
else 注册
```

那么当出现新的业务需求（如添加登录功能）时，网络模块也需要修改代码

```
if (消息ID1) 
else 注册
if (消息ID2)
else 登录
```

**网络模块与业务模块解耦的本质操作是使用unordered_map将消息id映射到业务处理方法。**网络模块`chatserver`可以通过`map`直接获取对应的业务处理方法，这样业务的增加或删除只在业务模块`chatservice`中进行，解耦了网络操作和业务操作。

## 数据模块

数据库编程相关API的使用已经在[手写数据库连接池](http://zhcan.online/%E6%89%8B%E5%86%99%E6%95%B0%E6%8D%AE%E5%BA%93%E8%BF%9E%E6%8E%A5%E6%B1%A0/)中介绍过，这里不再赘述。

### ORM框架

**ORM（对象-关系映射）可以将数据库表的结构和数据映射到对象模型中，封装了数据库操作，使上层开发可以直接使用面向对象的方式来操作数据库，不需要直接编写SQL语句。**

**ORM把数据库映射成对象：**

```
数据库的表（table） --> 类（class）
记录（record，行数据）--> 对象（object）
字段（field）--> 对象的属性（attribute）
```

**ORM的优点：**

* 数据模型都在一个地方定义，更容易更新维护和重用代码，提高开发效率。
* 基于ORM的业务代码比较简单，代码量少，语义性好，容易理解。
* 代码结构更清晰，并且不必编写性能不佳的 SQL。

**ORM的缺点：**

* 对于复杂的查询，ORM可能会无法表达，或者是性能不如原生的 SQL。

### 数据模块设计

User类、Group类负责暂存从数据库查询到的内容，或者是服务器解析的Json字符串的信息；Model类调用db.h中的函数，负责对数据库进行增删改查；db.h封装了MySQL C的API。

![](./集群聊天服务器/数据模块设计.jpg)

## 客户端程序实现

客户端主要有一个发送线程和一个接收线程，发送线程解析用户的命令，调用相应的回调函数，将封装好的Json字符串发送给服务端；接收线程负责接收服务的转发的数据，对接收到的数据进行反序列化，显示聊天的消息内容。客户端的详细设计如下：

![](./集群聊天服务器/image-20240220160811552.png)

## 服务器集群

单个服务器所支持的并发访问量有限，因此，为了提高服务器所支持的并发访问量，我们需要引入Nginx负载均衡器。

Nginx负载均衡器能够把客户端的请求按照负载均衡算法分发到具体业务服务器上，并且能够和Chatserver保持心跳机制，监测服务器的故障，避免将请求分发到故障的服务器上。同时，Nginx能够发现新添加的ChatServer服务器，可以自由地扩展服务器地数量。

![](./集群聊天服务器/未命名文件.jpg)

**配置Nginx负载均衡模块：**

**1. 编译Nginx**

(1) 安装pcre、openssl、zlib库

```bash
# sudo apt-get update
# sudo apt-get install libpcre3 libpcre3-dev libssl-dev zlib1g-dev
```

(2) 在nginx目录下执行编译命令

```bash
// --with-stream表示激活tcp模块，该命令需要在root用户下执行
# ./configure --with-stream
# make && make install
```

编译完成后，默认安装在/usr/local/nginx目录下

**2. Nginx配置TCP负载均衡**

(1) 进入conf目录

```bash
# cd /usr/local/nginx
# cd conf
```

(2) 配置nginx.conf文件

```bash
# vim nginx.conf
```

配置如下：

![](./集群聊天服务器/image-20240220164111777.png)

stream模块用于配置TCP和UDP的负载均衡器。在upstream中定义了两个后端服务器，每个后端服务器的权重为1，表示它们各自处理请求的比例是相同的。在server中配置了监听的端口号为8000，proxy pass为MyServer，表示将8000端口监听到的客户端请求转发给MyServer中的服务器。

(3) 配置完成后，重启nginx

```
nginx -s reload # 重新加载配置文件启动
nginx -s stop # 停止nginx服务
nginx -s reload 平滑重启
```

## 基于发布-订阅的redis消息队列

在ChatServer集群部署了多台服务器之后，登录在不同服务器上的用户需要进行跨服务器的通信。如果让每个ChatServer服务器之间直接建立TCP连接进行通信，这样的设计会占用系统大量的socket资源，各服务器之间的带宽压力会很大，并且各服务器之间的设计耦合度高，不利于扩展。

因此，可以引入中间件消息队列的方式，解耦各个服务器，提高服务器的响应能力，节省服务器的带宽资源。

**redis环境搭建：**

**1. 安装redis服务**

```bash
$ sudo apt-get install redis-server
```

**2. 安装hiredis（redis对应的C++客户端编程）**

```bash
// 从GitHub上下载hiredis客户端，进行源码编译
$ git clone https://github.com/redis/hiredis
$ cd hiredis
$ make
$ sudo make install
$ sudo ldconfig /usr/local/lib
```

**基于发布-订阅的redis消息队列的设计：**

当客户端登录成功后，**服务端将该用户的id号subscribe到redis消息队列上**，表示该服务器对这个id发生的事件感兴趣，服务器就能在接收到其他服务器往这个通道发来的消息；当用户下线时，需要从redis取消订阅。

在集群聊天服务器中，在执行私聊和群聊的业务时，服务端会检查私聊或群聊的对象是否登录在本地服务器上，如果没有，再检查数据库中该对象的信息是否在线。如果不在线，就储存到该用户的离线消息中；如果在线，**就将消息向对方id所对应的通道publish消息，此时redis会通知对方用户登录的服务器订阅的通道接收到了消息**，这样消息就成功进行了转发。

![](./集群聊天服务器/image-20240220201214068.png)

## 增加数据库连接池

数据库连接池的设计已经在[手写数据库连接池](http://zhcan.online/%E6%89%8B%E5%86%99%E6%95%B0%E6%8D%AE%E5%BA%93%E8%BF%9E%E6%8E%A5%E6%B1%A0/)中介绍过，这里不再赘述。

## 遇到的问题

### loginout后再login程序假死

![](./集群聊天服务器/image-20240220205127157.png)

**问题定位：**

**1. `ps -u`查看进程号**

**2. `gdb attach 有问题的进程`**

**3. `info threads`查看此时的线程数量**

可以看到，客户端程序的发送线程和接收线程都阻塞在recv操作上。

![](./集群聊天服务器/image-20240220210728106.png)

**4. `bt`查看线程的调用堆栈**

![](./集群聊天服务器/image-20240220211130681.png)

因此，通过分析main.cpp:107和main.cpp:289的代码，我们可以看出：由于注销时并没有结束接收线程，当再次登录时，本该由主线程接收的登录响应消息，被接收线程给接收了，于是主线程阻塞在recv了，而子线程在处理完接收的消息后，也阻塞在recv了。

**问题解决：**

主线程不再接收消息，将接收消息的任务全都交给子线程（接收线程）。

* 在client客户端完成socket连接后，就启动子线程
* 子线程接收到登录响应消息后，通过sem信号量通知主线程继续往下执行
* 使用基于CAS实现的atomic变量记录是否登录成功

> 注册的逻辑也相同

> 项目可以使用./autobuild.sh运行

## 参考文章

* [基于muduo网络库的集群聊天系统（C++实现）](https://blog.csdn.net/shenmingxueIT/article/details/113199719)
* [ORM 实例教程](https://www.ruanyifeng.com/blog/2019/02/orm-tutorial.html)
* 施磊. 集群聊天服务器
