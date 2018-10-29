# How to use the client
## Environment
- Python 3.x

## Commands
目前支持的命令有如下几个，命令的使用方法基本与标准实现类似
```
    binary, 指定为二进制模式传输文件
    bye, 关闭连接并退出
    cd [path], 进入目录
    close, 关闭连接
    get [filename], 获取远端文件
    ls, 列出远端文件夹中文件及文件夹
    mkdir [folder], 创建文件夹
    open [host] [port], 连接服务器
    put [filename], 上传一个文件
    pwd, 打印远端文件夹的当前路径
    rename [old_filename] [new_filename], 重命名
    rmdir [dirname], 删除文件夹
    system, 打印FTP服务器运行系统
    user [username] [password(optional)], 登录
    ?, 查看当前支持的命令
    quit, 退出客户端
```

## Usage
- 首先使用`python3 client.py`运行客户端程序
- 键入`open [host] [port]`来连接一个FTP服务器
- 使用`user [username] [password(optional)]`来登录
- 使用`close`命令关闭连接
- 使用`bye`命令关闭连接并退出客户端