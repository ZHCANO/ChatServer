## 项目需求

本项目是一个仿QQ聊天的即时通讯系统，主要业务有登录、注册、注销、添加好友、创建或加入群组、一对一聊天以及群组聊天等。

## 开发环境

* Ubuntu 18.04.6 LTS
* 安装Json开发库
* 安装boost+muduo网络库开发环境，参考[C++ muduo网络库知识分享01 - Linux平台下muduo网络库源码编译安装]([C++ muduo网络库知识分享01 - Linux平台下muduo网络库源码编译安装-CSDN博客](https://blog.csdn.net/QIANGWEIYUAN/article/details/89023980))
* 安装redis环境
* 安装mysql数据库环境
* 安装nginx
* 安装CMake环境

## 项目启动

打开mysql.ini配置数据库信息

```ini
# 数据库的配置文件（注意：等号左右不要有空格）
ip=127.0.0.1
port=3306
username=root
passward=123456
dbname=pool
initSize=10
maxSize=1024
# 最大空闲时间，默认单位：秒
maxIdleTime=60
# 连接超时时间，默认单位：毫秒
connectionTimeout=100
```

在ChatServer下输入`./autobuild.sh`运行。