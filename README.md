# FTP课程实验报告

## 作者信息

- 姓名：洪方舟
- 班级：软件62
- 学号：2016013259

## 实验目标

- 完成一个符合RFC959标准的FTP服务器，并且支持能够提供最低限度服务的命令
- 完成一个符合RFC959标准的FTP客户端，支持的命令与服务器端相对应
- 在完成基本命令的基础上
  - 服务器端需要支持：多客户端连接；非阻塞的大文件传输；断点续传；
  - 客户端需要提供GUI界面
- 通过本次实验，理解网络协议的重要意义，掌握网络编程的基础技能

## 实验环境

### 服务器端

- Ubuntu 16.04
- GCC 7.3.0

### 客户端

- Ubuntu 16.04
- Python 3.7

## 实验结果概述

### 服务器端

- 目前实现的命令如下所示：`USER,PASS,RETR,STOR,QUIT,SYST,TYPE,PORT,PASV,MKD,CWD,PWD,LIST,RMD,LIST,RMD,RNFR,RNTO,REST,APPE`
- 通过多线程的方式做到多客户端同时访问
- 通过多线程的方式做到大文件非阻塞传输，并且可以做到多文件同时上传、下载
- 通过`REST,APPE`等命令实现断点续传

### 客户端

- 实现的命令与服务端一致
- 使用`pyqt`提供GUI
- 通过多线程的方式支持多文件同时上传、下载
- 支持断点续传（也即暂停下载/继续下载/断网后的恢复）

## 客户端运行指南

### 环境准备

- 安装`pyqt`：`pip3 install PyQt5==5.11.3`
- 运行：`python3 ftp-gui.py`或者执行`bin`文件夹中的可执行文件

### 使用说明

- 运行后进入主界面，从上到下依次为：登录相关输入框；所发指令以及接受服务器返回值的显示框；本地文件夹以及远端文件夹；下载、上传任务列表
- 首先请在登录相关输入框中依照提示输入正确的信息，点击右侧`Connect`后即可登录，登录后点击`Disconnect`即可退出登录
- 如需浏览远端文件夹，可以上下滚动查看远端文件列表，如需进入文件夹只需双击对应项
- 如需下载文件，请在左侧栏进入下载文件的保存文件夹，在右侧栏右击需要下载的文件，点击`Download`即可开始下载文件
- 如需上传文件，请在右侧栏进入上传文件的保存地址，在左侧栏右击需要上传的文件，点击`Upload`即可开始上传文件
- 所有的上传下载文件任务会显示在最下层的任务列表中，如需暂停/继续下载，请点击对应项右侧的按钮
- 如需进入`Binary`模式，请点击左上角菜单栏中`File->Mode`，选中`Binary`模式即可
- 如需查看服务器系统，请点击菜单栏中`About->System`，在弹出的窗口中可以查看系统信息

## 服务端运行指南

### 环境准备

- 在一台Linux/MacOS系统的电脑上安装`GUN make`以及`GUN gcc`
- 在对应文件夹中运行`make`即可完成编译

### 使用说明

- 可执行文件`server`一共接受三个参数
- `-port`指定了监听的端口号
- `-root`指定了FTP根目录地址
- `-host`指定了FTP服务器本机IP地址

## 实验中遇到问题及其解决方案

- 使用`pthread`时绑定端口的正确方式是`addr.sin_port=htons(port)`，如果不使用`htons()`函数转化则无法绑定正确的端口
- 传输指令的`socket`在读数据的时候需要判断数据结尾是否为`\r\n`，以此作为判断一次传输完成的依据；而传输数据的`socket`在读数据的时候如果在某一时刻读到的数据长度为0，则说明另一端的`socket`被关闭，因而判断为传输完成
- 多用户的实现方案如下：使用非阻塞的`socket`循环调用`accept()`，如果遇到连接，则新建一个线程，传入套接字文件描述符、根目录地址等必要的信息
- 大文件传输的实现方案如下：每当需要文件传输的时候就新建一个线程进行文件传输，主线程仍然能够继续接受其他的命令；在大文件传输的线程中，如果是上传数据，则应当每读取一段数据就往文件中写一段数据，如果是下载文件，应当每从文件中读取一段数据就发送一段数据（客户端同理）
- 断点续传的实现方案如下：
  - 断点上传文件：先使用`LIST`指令读取上次上传的文件目前的大小，将本地文件对应的文件指针后移到对应的续传位置，使用`APPE`命令直接将剩余数据追加到远端的文件后
  - 断点下载文件：先读取本地下载到一半的文件大小，使用`REST`命令告知远端跳过前若干字节，使用`RETR`命令下载剩余数据，并追加到对应的文件后

## 创新点
- 在实现大文件传输非阻塞的时候，没有限制数据传输套接字的数量，对于每一个用户维护了一个套接字列表，因此本服务器支持多文件同时上传，同时下载
- 由于没有数据传输套接字数量的限制，因此本服务器在传输文件的时候能够支持`LIST`命令，实现了真正的大文件传输非阻塞
- 客户端配合服务器端实现了上传、下载任务队列，能够做到多文件同时上传下载