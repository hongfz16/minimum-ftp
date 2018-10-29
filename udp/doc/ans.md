# Answer to the questions
## Question1
如果是去中心化的聊天应用，则需要两个Client相互连接，每个Client拥有一个读Socket与一个写Socket，开启一个子线程，用于收取读Socket中收到的消息并打印在屏幕上，主线程则循环阻塞等待用户输入，输入完成则发送。
如果是有一个中心服务器，那么两个Client均与Server相连，Server负责转发即可，其余的部分同上。

## Question2
我们不能使用UDP传输文件，因为UDP协议不是一个可靠的协议，当报文发送之后，发送方无法知道是否完整发送过去，而对于文件传输这种需要高可靠性的应用，是不能使用UDP协议的。