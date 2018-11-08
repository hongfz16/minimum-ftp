# minimum-ftp
A simple ftp server and client which only implement minimum function.

## Current implemented features
### Commands implemented
- USER
- PASS
- RETR
- STOR (only support small file)
- QUIT
- SYST
- TYPE
- PORT (only support small file)
- PASV
- MKD
- CWD
- PWD
- LIST
- RMD
- RNFR
- RNTO

### Other features
- Support multi-client connection using pthread
- Non-blocking socket

## Next plan
- Transmit large files
- Transmit files without blocking the server
- Resume transmit after connection broken

## Problems and solutions
- The correct way to bing a certain port is `addr.sin_port = htons(port);` instead of `addr.sin_port = port;`
- In order to identify a completed transmit of socket, we have to check the end of each buffer we received . If it is ended with `\r\n`, then we should know that we've received the complete command as is required by the protocol.
- When transmit data using data transmit socket, in order to let the other side know that the transmit is over, we have to passivly close the connection. So that the other side will receive `FIN` and the return value of the `read()` function is 0.
- When using non-blocking socket, every function that should have blocked will now be non-blocking. For example, when you call `read()`, if there is nothing to read, the this function will immediatly return `-1`, and set the `errno` to `EWOULDBLOCK`. And that is how you use the non-blocking socket.
