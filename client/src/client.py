#! /usr/bin/python3
import socket
import re
import random

orders = [
    'binary', #0
    'bye', #1
    'cd', #2
    'close', #3
    'get', #4
    'ls', #5
    'mkdir', #6
    'open', #7
    'put', #8
    'pwd', #9
    'rename', #10
    'rmdir', #11
    'system', #12
    'user', #13
    '?', #14
    'quit' #15
]

def print_res(response):
    response = response.rstrip('\r\n')
    print(response)

def socket_read_data(s):
    buffer = []
    while True:
        d = s.recv(1024)
        if d:
            buffer.append(d)
        else:
            break
    data = b''.join(buffer)
    return data


def socket_read_command(s):
    buffer = []
    while True:
        d = s.recv(1024)
        buffer.append(d)
        if d.decode(encoding='utf-8')[-1]=='\n':
            break
    data = b''.join(buffer)
    return data

def socket_read_command_str(s):
    return str(socket_read_command(s), encoding='utf-8')

def socket_send(s, data):
    try:
        s.sendall(data)
    except Exception as e:
        print(e)
        return -1
    return 0

def unpack_response(res):
    # res = str(res, encoding='utf-8')
    code = int(res[0:3])
    content = res[4:]
    return [code, content]

def unsupported_command():
    print('This is an unsupported command.')

def binary_handler(conns, order_content):
    if len(order_content)>0:
        unsupported_command()
        return 1
    command = b'TYPE I\r\n'
    if socket_send(conns, command)==-1:
        return -1
    response = socket_read_command_str(conns)
    print_res(response)
    return 0

def bye_handler(conns, order_content):
    if len(order_content)>0:
        unsupported_command()
        return 1
    command = b'QUIT\r\n'
    if socket_send(conns, command)==-1:
        return -1
    response = socket_read_command_str(conns)
    code, res_content = unpack_response(response)
    if code == 221:
        conns.close()
    print_res(response)
    return 0

def cd_handler(conns, order_content):
    if len(order_content)==0:
        unsupported_command()
        return 1
    command = b' '.join([b'CWD',order_content.encode(encoding='utf=8')])
    command = command + b'\r\n'
    if socket_send(conns, command)==-1:
        return -1
    response = socket_read_command_str(conns)
    print_res(response)
    return 0

def close_handler(conns, order_content):
    if len(order_content)>0:
        unsupported_command()
        return 1
    command = b'QUIT\r\n'
    if socket_send(conns, command)==-1:
        return -1
    response = socket_read_command_str(conns)
    code, res_content = unpack_response(response)
    if code == 221:
        conns.close()
    print_res(response)
    return 0

def passive_connect(conns):
    command = b'PASV\r\n'
    if socket_send(conns, command)==-1:
        return -1
    response = socket_read_command_str(conns)
    print_res(response)
    code, res_content = unpack_response(response)
    if code != 227:
        return -1
    res_content = re.findall(r"[0-9]{1,3},[0-9]{1,3},[0-9]{1,3},[0-9]{1,3},[0-9]{1,3},[0-9]{1,3}", res_content)[0]
    # print(res_content)
    addr_list = res_content.split(',')
    addr_str = '.'.join(addr_list[0:4])
    port = int(addr_list[4])*256+int(addr_list[5])
    datas = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        datas.connect((addr_str, port))
    except Exception as e:
        print(e)
        return -1
    return datas

def get_handler(conns, order_content):
    if len(order_content)==0:
        unsupported_command()
        return 1
    datas = passive_connect(conns)
    if datas == -1:
        return 1
    command = b' '.join([b'RETR', order_content.encode(encoding='utf-8')])
    command = command + b'\r\n'
    if socket_send(conns, command)==-1:
        datas.close()
        return -1
    response = socket_read_command_str(conns)
    print_res(response)
    response = response.rstrip('\r\n')
    responses = response.split('\r\n')
    code, res_content = unpack_response(responses[-1])
    if code != 150:
        datas.close()
        return 1
    data_buffer = socket_read_data(datas)
    datas.close()
    response = socket_read_command_str(conns)
    print_res(response)
    code, _ = unpack_response(response)
    if code != 226:
        datas.close()
        return 1
    filename = order_content.split('/')[-1]
    with open(filename, 'wb') as f:
        f.write(data_buffer)
    return 0


def get_host_ip():
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.connect(('8.8.8.8', 80))
        ip = s.getsockname()[0]
    finally:
        s.close() 
    return ip

def port_connect(conns):
    port = random.randint(20000, 30000)
    datas = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    host = get_host_ip()
    while True:
        try:
            datas.bind((host, port))
            datas.listen(5)
        except:
            port = random.randint(20000, 30000)
            continue
        break
    print(host, port)
    res_content = re.findall(r"[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}", host)[0]
    addr_list = res_content.split('.')
    addr_list.append(str(int(port/256)))
    addr_list.append(str(port%256))
    command = 'PORT ='+','.join(addr_list)
    print(command)
    command = command.encode()
    if socket_send(conns, command)==-1:
        datas.close()
        return -1
    try:
        port_socket = datas.accept()
    except:
        return -1
    return port_socket


def ls_handler(conns, order_content):
    if len(order_content)>0:
        unsupported_command()
        return 1
    # datas = passive_connect(conns)
    datas = port_connect(conns)
    if datas == -1:
        return 1
    command = b'LIST\r\n'
    if socket_send(conns, command)==-1:
        datas.close()
        return -1
    response = socket_read_command_str(conns)
    print_res(response)
    response = response.rstrip('\r\n')
    responses = response.split('\r\n')
    code, res_content = unpack_response(responses[-1])
    if code != 150:
        datas.close()
        return 1
    data_buffer = socket_read_data(datas)
    datas.close()
    response = socket_read_command_str(conns)
    print_res(response)
    list_file = str(data_buffer, encoding='utf-8').rstrip('\r\n').split('\r\n')
    print('\n'.join(list_file))
    return 0

def mkdir_handler(conns, order_content):
    if len(order_content)==0:
        unsupported_command()
        return 1
    command = b' '.join([b'MKD', order_content.encode(encoding='utf-8')])
    command = command + b'\r\n'
    if socket_send(conns, command)==-1:
        return -1
    response =socket_read_command_str(conns)
    print_res(response)
    return 0

def open_handler(order_content):
    if len(order_content)==0:
        unsupported_command()
        return -1
    contents = order_content.split(' ')
    if not contents[-1].isdigit():
        unsupported_command()
        return -1
    server_name = contents[0]
    port = int(contents[-1])
    # print(server_name, port)
    try:
        conns = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        conns.connect((server_name, port))
    except Exception as e:
        print(e)
        return -1
    response = socket_read_command_str(conns)
    print_res(response)
    return conns

def put_handler(conns, order_content):
    if len(order_content)==0:
        unsupported_command()
        return -1
    try:
        f = open(order_content, 'rb')
    except Exception as e:
        print(e)
        return 1
    try:
        data_buffer = f.read()
    except Exception as e:
        print(e)
        f.close()
        return 1
    datas = passive_connect(conns)
    if datas == -1:
        return 1
    command = b' '.join([b'STOR', order_content.encode(encoding='utf-8')])
    command = command + b'\r\n'
    if socket_send(conns, command)==-1:
        datas.close()
        return -1
    response = socket_read_command_str(conns)
    print_res(response)
    response = response.rstrip('\r\n')
    responses = response.split('\r\n')
    code, res_content = unpack_response(responses[-1])
    if code != 150:
        datas.close()
        f.close()
        return 1
    if socket_send(datas, data_buffer)==-1:
        f.close()
        datas.close()
        return -1
    f.close()
    datas.close()
    response = socket_read_command_str(conns)
    print_res(response)
    return 0

def pwd_handler(conns, order_content):
    if len(order_content)>0:
        unsupported_command()
        return 1
    command = b'PWD\r\n'
    if socket_send(conns, command)==-1:
        return -1
    response = socket_read_command_str(conns)
    print_res(response)
    return 0

def rename_handler(conns, order_content):
    if len(order_content)==0:
        unsupported_command()
        return 1
    contents = order_content.split(' ');
    old_filename = contents[0]
    new_filename = contents[-1]
    command = b' '.join([b'RNFR', old_filename.encode(encoding='utf-8')])
    command = command + b'\r\n'
    if socket_send(conns, command)==-1:
        return -1
    response = socket_read_command_str(conns)
    print_res(response)
    code, res_content = unpack_response(response)
    if code != 350:
        return 1
    command = b' '.join([b'RNTO', new_filename.encode(encoding='utf-8')])
    command = command + b'\r\n'
    if socket_send(conns, command)==-1:
        return -1
    response = socket_read_command_str(conns)
    print_res(response)
    return 0

def rmdir_handler(conns, order_content):
    if len(order_content)==0:
        unsupported_command()
        return 1
    command = b' '.join([b'RMD', order_content.encode(encoding='utf-8')])
    command = command + b'\r\n'
    if socket_send(conns, command)==-1:
        return -1
    response = socket_read_command_str(conns)
    print_res(response)
    return 0

def system_handler(conns, order_content):
    if len(order_content)>0:
        unsupported_command()
        return 1
    command = b'SYST\r\n'
    if socket_send(conns, command)==-1:
        return -1
    response = socket_read_command_str(conns)
    print_res(response)
    return 0

def user_handler(conns, order_content):
    if len(order_content)==0:
        unsupported_command()
        return 1
    contents = order_content.split(' ')
    username = contents[0]
    command = b' '.join([b'USER', username.encode(encoding='utf-8')])
    command = command + b'\r\n'
    if socket_send(conns, command)==-1:
        return -1
    response = socket_read_command_str(conns)
    print_res(response)
    response = response.rstrip('\r\n')
    responses = response.split('\r\n')
    code, _ = unpack_response(responses[-1])
    if code == 230:
        return 0
    if code==530:
        return 1
    if len(contents)==1:
        password = input('Please input your password:')
    else:
        password = contents[-1]
    command = b' '.join([b'PASS', password.encode(encoding='utf-8')])
    command = command + b'\r\n'
    if socket_send(conns, command)==-1:
        return -1
    response = socket_read_command_str(conns)
    print_res(response)
    response = response.rstrip('\r\n')
    responses = response.split('\r\n')
    code, _ = unpack_response(responses[-1])
    if code!= 230:
        return 1
    return 0

def help_handler():
    help_str = 'Currently supported commands are:\n' + '\n'.join(orders)
    print(help_str)

def unpatch_order(order):
    order = order.strip(' ')
    for i, s in enumerate(orders):
        if order.find(s) == 0:
            return i
    return -1

def get_order_content(order):
    order = order.strip(' ')
    contents = order.split(' ', 1)
    if len(contents)==1:
        return ''
    return contents[-1]

def main():
    open_status = 0
    login_status = 0
    conns = -1
    while True:
        order = input('ftp>')
        code = unpatch_order(order)
        content = get_order_content(order)
        # print(code)
        # print(content)
        if code == -1:
            print('Invalid command. Please use ? to check out available commands')
            continue
        if code == 14:
            help_handler()
            continue
        if code == 15:
            if conns!=-1:
                conns.close()
            break
        if not open_status:
            if code == 7:
                conns = open_handler(content)
                if conns == -1:
                    print('Connection refused.')
                else:
                    open_status = 1
            else:
                print('You have not open a connection.')
            continue
        if open_status and not login_status:
            if code == 13:
                result = user_handler(conns, content)
                if result == -1:
                    open_status = 0
                    conns.close()
                    conns = -1
                elif result == 0:
                    login_status = 1
            elif code == 1:
                result = bye_handler(conns, content)
                if result == -1:
                    open_status = 0
                    conns.close()
                    conns = -1
                elif result == 0:
                    conns.close()
                    conns = -1
                    open_status = 0
                    break
            elif code == 3:
                result = close_handler(conns, content)
                if result == -1:
                    open_status = 0
                    conns.close()
                    conns = -1
                elif result==0:
                    conns.close()
                    conns = -1
                    open_status = 0
            else:
                print('Please login first.')
            continue
        if open_status and login_status:
            result = None
            if code == 0:
                result = binary_handler(conns, content)
            elif code == 2:
                result = cd_handler(conns, content)
            elif code == 4:
                result = get_handler(conns, content)
            elif code == 5:
                result = ls_handler(conns, content)
            elif code == 6:
                result = mkdir_handler(conns, content)
            elif code == 8:
                result = put_handler(conns, content)
            elif code == 9:
                result = pwd_handler(conns, content)
            elif code == 10:
                result = rename_handler(conns, content)
            elif code == 11:
                result = rmdir_handler(conns, content)
            elif code == 12:
                result = system_handler(conns, content)
            if result!=None:
                if result==-1:
                    open_status=0
                    login_status=0
                    conns.close()
                    conns = -1
            else:
                if code == 1:
                    result = bye_handler(conns, content)
                    if result == -1:
                        open_status=0
                        login_status=0
                        conns.close()
                        conns = -1
                    elif result == 0:
                        conns.close()
                        break
                if code == 3:
                    result = close_handler(conns, content)
                    if result == -1:
                        open_status=0
                        login_status=0
                        conns.close()
                        conns = -1
                    elif result == 0:
                        open_status=0
                        login_status=0
                        conns.close()
                        conns=-1





if __name__ == '__main__':
    main()