#! /usr/bin/python3
import re
import os
import random
import socket

class mFTP():
    def __init__(self):
       self.open_status = False
       self.login_status = False
       self.conns = -1

       self.unsupported_command = 'This is an unsupported command.'
       self.connection_broken_str = 'Connection broken.'
       self.login_required = 'Login Required.'
       self.open_required = 'Connection Required'

    def socket_read_data(self, s):
        buffer = []
        while True:
            d = s.recv(1024)
            if d:
                buffer.append(d)
            else:
                break
        data = b''.join(buffer)
        return data

    def socket_read_command(self):
        buffer = []
        while True:
            d = self.conns.recv(1024)
            if len(d)==0:
                break
            buffer.append(d)
            if d.decode(encoding='utf-8')[-1]=='\n':
                break
        data = b''.join(buffer)
        return str(data, encoding='utf-8')

    def socket_send_command(self, data):
        try:
            self.conns.sendall(data)
        except Exception as e:
            print(e)
            return -1
        return 0

    def socket_send_data(self, soc, data):
        try:
            soc.sendall(data)
        except Exception as e:
            print(e)
            return -1
        return 0

    def unpack_response(self, res):
        if len(res)==0:
            return -1
        res = res.rstrip('\r\n')
        lines = res.split('\r\n')
        code = int(lines[-1][0:3])
        return code

    def connection_broken(self):
        self.open_status = False
        self.login_status = False
        self.conns = -1
        return [self.connection_broken_str, -1]

    def binary_handler(self):
        if not self.login_status:
            return [self.login_required, 1]
        command = b'TYPE I\r\n'
        if self.socket_send_command(command)==-1:
            return self.connection_broken()
        response = self.socket_read_command()
        return [response, 0]

    def bye_handler(self):
        if not self.login_status:
            return [self.login_required, 1]
        command = b'QUIT\r\n'
        if self.socket_send_command(command)==-1:
            return self.connection_broken()
        response = self.socket_read_command()
        code = self.unpack_response(response)
        if code == 221:
            self.open_status = False
            self.login_status = False
            self.conns.close()
            self.conns = -1
            return [response, 0]
        else:
            return [response, 1]

    def cd_handler(self, order_content):
        if not self.login_status:
            return [self.login_required, 1]
        command = b' '.join([b'CWD',order_content.encode(encoding='utf=8')])
        command = command + b'\r\n'
        if self.socket_send_command(command)==-1:
            return self.connection_broken()
        response = self.socket_read_command()
        code = self.unpack_response(response)
        if code == 250:
            return [response, 0]
        else:
            return [response, 1]

    def close_handler(self):
        if not self.login_status:
            return [self.login_required, 1]
        command = b'QUIT\r\n'
        if self.socket_send_command(command)==-1:
            return self.connection_broken()
        response = self.socket_read_command()
        code = self.unpack_response(response)
        if code == 221:
            self.open_status = False
            self.login_status = False
            self.conns.close()
            self.conns = -1
            return [response, 0]
        else:
            return [response, 1]

    def passive_connect(self):
        command = b'PASV\r\n'
        if self.socket_send_command(command)==-1:
            return self.connection_broken()
        response = self.socket_read_command()
        code = self.unpack_response(response)
        if code != 227:
            return [response, -2]
        res_content = response.split(' ', 1)[1]
        res_content = re.findall(r"[0-9]{1,3},[0-9]{1,3},[0-9]{1,3},[0-9]{1,3},[0-9]{1,3},[0-9]{1,3}", res_content)[0]
        addr_list = res_content.split(',')
        addr_str = '.'.join(addr_list[0:4])
        port = int(addr_list[4])*256+int(addr_list[5])
        datas = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        try:
            datas.connect((addr_str, port))
        except Exception as e:
            print(e)
            return [response, -1]
        return [response, datas]

    def get_handler(self, remotename, localname):
        if not self.login_status:
            return [self.login_required, 1]
        counter = 0
        while os.path.exists(localname):
            counter += 1
            localname_split = localname.rsplit('.', 1)
            localname_split[0] += str(counter)
            localname = '.'.join(localname_split)
        response, datas = self.passive_connect()
        if datas == -1:
            return [response, -1]
        if datas == -2:
            return [response, 1]
        command = b' '.join([b'RETR', remotename.encode(encoding='utf-8')])
        command = command + b'\r\n'
        if self.socket_send_command(command)==-1:
            datas.close()
            self.connection_broken()
            return [response + '\nConnection Broken.', -1]
        second_response = self.socket_read_command()
        response += second_response
        code = self.unpack_response(second_response)
        if code != 150:
            datas.close()
            return [response, 1]
        data_buffer = self.socket_read_data(datas)
        datas.close()
        third_response = self.socket_read_command()
        response += third_response
        code = self.unpack_response(third_response)
        if code != 226:
            return [response, 1]
        with open(localname, 'wb') as f:
            f.write(data_buffer)
        return [response, 0]

    def ls_handler(self):
        if not self.login_status:
            return [self.login_required, 1]
        response, datas = self.passive_connect()
        if datas == -1:
            return [response, -1]
        if datas == -2:
            return [response, 1]
        command = b'LIST\r\n'
        if self.socket_send_command(command)==-1:
            datas.close()
            self.connection_broken()
            return [response + '\nConnection Broken.', -1]
        second_response = self.socket_read_command()
        response += second_response
        code = self.unpack_response(second_response)
        if code != 150:
            datas.close()
            return [response, 1]
        data_buffer = self.socket_read_data(datas)
        datas.close()
        third_response = self.socket_read_command()
        response += third_response
        code = self.unpack_response(third_response)
        if code != 226:
            return [response, 1]
        else:
            list_file = str(data_buffer, encoding='utf-8').rstrip('\r\n').split('\r\n')
            return [response, 0, list_file]

    def mkdir_handler(self, order_content):
        if not self.login_status:
            return [self.login_required, 1]
        command = b' '.join([b'MKD', order_content.encode(encoding='utf-8')])
        command = command + b'\r\n'
        if self.socket_send_command(command)==-1:
            return self.connection_broken()
        response = self.socket_read_command()
        code = self.unpack_response(response)
        if code == 250 or code == 257:
            return [response, 0]
        else:
            return [response, 1]

    def open_handler(self, server_name, port):
        try:
            self.conns = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.conns.connect((server_name, port))
        except Exception as e:
            print(e)
            return ['Cannot connect to server', 1]
        response = self.socket_read_command()
        self.open_status = True
        return [response, 0]

    def put_handler(self, order_content, remotename):
        if not self.login_status:
            return [self.login_required, 1]
        try:
            f = open(order_content, 'rb')
        except Exception as e:
            print(e)
            return ['Cannot open the file: '+order_content, -1]
        try:
            data_buffer = f.read()
        except Exception as e:
            print(e)
            f.close()
            return ['Error while reading the file', -1]
        response, datas = self.passive_connect()
        if datas == -1:
            return [response, -1]
        if datas == -2:
            return [response, 1]
        command = b' '.join([b'STOR', remotename.encode(encoding='utf-8')])
        command = command + b'\r\n'
        if self.socket_send_command(command)==-1:
            datas.close()
            self.connection_broken()
            return [response+'\nConnection Broken.', -1]
        second_response = self.socket_read_command()
        response += second_response
        code = self.unpack_response(second_response)
        if code != 150:
            datas.close()
            f.close()
            return [response, 1]
        if self.socket_send_data(datas, data_buffer)==-1:
            f.close()
            datas.close()
            return [response+'\nData Connection Broken.', -1]
        f.close()
        datas.close()
        third_response = self.socket_read_command()
        response += third_response
        code = self.unpack_response(third_response)
        if code == 226:
            return [response, 0]
        else:
            return [response, 1]

    def pwd_handler(self):
        if not self.login_status:
            return [self.login_required, 1]
        command = b'PWD\r\n'
        if self.socket_send_command(command)==-1:
            return self.connection_broken()
        response = self.socket_read_command()
        code = self.unpack_response(response)
        if code == 257:
            path = response.split('"')[1]
            return [response, 0, path]
        else:
            return [response, 1]

    def rename_handler(self, old_filename, new_filename):
        if not self.login_status:
            return [self.login_required, 1]
        command = b' '.join([b'RNFR', old_filename.encode(encoding='utf-8')])
        command = command + b'\r\n'
        if self.socket_send_command(command)==-1:
            return self.connection_broken()
        response = self.socket_read_command()
        code = self.unpack_response(response)
        if code != 350:
            return [response, 1]
        command = b' '.join([b'RNTO', new_filename.encode(encoding='utf-8')])
        command = command + b'\r\n'
        if self.socket_send_command(command)==-1:
            self.connection_broken()
            return [response+'\nConnection Broken.', -1]
        second_response = self.socket_read_command()
        response += second_response
        code = self.unpack_response(second_response)
        if code == 250:
            return [response, 0]
        else:
            return [response, 1]

    def rmdir_handler(self, order_content):
        if not self.login_status:
            return [self.login_required, 1]
        command = b' '.join([b'RMD', order_content.encode(encoding='utf-8')])
        command = command + b'\r\n'
        if self.socket_send_command(command)==-1:
            return self.connection_broken()
        response = self.socket_read_command()
        code = self.unpack_response(response)
        if code == 250:
            return [response, 0]
        else:
            return [response, 1]

    def system_handler(self):
        if not self.open_status:
            return [self.open_required, 1]
        command = b'SYST\r\n'
        if self.socket_send_command(command)==-1:
            return self.connection_broken()
        response = self.socket_read_command()
        code = self.unpack_response(response)
        if code == 215:
            return [response, 0]
        else:
            return [response, 1]

    def user_handler(self, username, password):
        if not self.open_status:
            return [self.open_required, 1]
        command = b' '.join([b'USER', username.encode(encoding='utf-8')])
        command = command + b'\r\n'
        if self.socket_send_command(command)==-1:
            return self.connection_broken()
        response = self.socket_read_command()
        code = self.unpack_response(response)
        if code == 230:
            self.login_status = True
            return [response, 0]
        if code==530:
            return [response, 1]
        command = b' '.join([b'PASS', password.encode(encoding='utf-8')])
        command = command + b'\r\n'
        if self.socket_send_command(command)==-1:
            self.connection_broken()
            return [response+'\nConnection Broken.', -1]
        second_response = self.socket_read_command()
        response += second_response
        code = self.unpack_response(response)
        if code!= 230:
            return [response, 1]
        self.login_status = True
        return [response, 0]

    def rest_handler(self, n):
        if not self.open_status:
            return [self.open_required, 1]
        command = b' '.join([b'REST', str(n).encode(encoding='utf-8')])
        command = command + b'\r\n'
        if self.socket_send_command(command) == -1:
            return self.connection_broken()
        response = self.socket_read_command()
        code = self.unpack_response(response)
        if code != 350:
            return [response, 1]
        else:
            return [response, 0]

    def restart_download(self, remotename, localname):
        if not self.login_status:
            return [self.login_required, 1]
        response, datas = self.passive_connect()
        if datas == -1:
            return [response, -1]
        if datas == -2:
            return [response, 1]
        
        mode = 'ab'
        tmp_size = os.path.getsize(localname)
        rest_resp, rest_status = self.rest_handler(tmp_size)
        response += rest_resp
        if rest_status != 0:
            mode = 'wb'
        
        command = b' '.join([b'RETR', remotename.encode(encoding='utf-8')])
        command = command + b'\r\n'
        if self.socket_send_command(command)==-1:
            datas.close()
            self.connection_broken()
            return [response + '\nConnection Broken.', -1]
        second_response = self.socket_read_command()
        response += second_response
        code = self.unpack_response(second_response)
        if code != 150:
            datas.close()
            return [response, 1]
        data_buffer = self.socket_read_data(datas)
        datas.close()
        third_response = self.socket_read_command()
        response += third_response
        code = self.unpack_response(third_response)
        if code != 226:
            return [response, 1]
        with open(localname, mode) as f:
            f.write(data_buffer)
        return [response, 0]

def test():
    ftp = mFTP()
    print(ftp.open_handler('127.0.0.1', 8000))
    print(ftp.system_handler())
    print(ftp.user_handler('anonymous', 'password'))
    print(ftp.binary_handler())
    print(ftp.ls_handler())
    print(ftp.mkdir_handler('testfolder'))
    print(ftp.cd_handler('testfolder'))
    print(ftp.pwd_handler())
    print(ftp.put_handler('/Users/hongfz/Documents/Codes/test.py', 'test.py'))
    print(ftp.cd_handler('..'))
    print(ftp.pwd_handler())
    print(ftp.get_handler('testfolder/test.py', '/Users/hongfz/Documents/Codes/download_test.py'))
    print(ftp.rename_handler('testfolder/test.py', 'test.py'))
    print(ftp.rmdir_handler('testfolder'))
    print(ftp.restart_download('test.py', '/Users/hongfz/Documents/Codes/new_test.py'))
    print(ftp.bye_handler())

test()
