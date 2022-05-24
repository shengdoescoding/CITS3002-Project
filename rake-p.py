import socket
import selectors
import sys
import os
import pathlib
from dataclasses import dataclass, field


@dataclass
class Action:
    required_files: list[str] = field(default_factory=list)
    command: str = ''
    remote: bool = False
    total_files: int = 0


@dataclass
class Actionset:
    acts: list[Action] = field(default_factory=list)
    total_actions: int = 0


@dataclass
class Rake_File:
    hosts: list[str] = field(default_factory=list)
    actsets: list[Actionset] = field(default_factory=list)
    port: int = -1
    total_hosts: int = 0
    total_actionsets: int = 0


__INT32_MAX__ = 0x7fffffff

CURRENT_ACT_INDEX = 0
CURRENT_ACTSET_INDEX = 0

# Protocol Required Standards:
SIZEOF_INT = 4  # bytes
BUFFSIZE = 1024

ISCOMMAND = 1
ISFILE = 2
FILERECIEVED = 3
ISLOADQUERY = 4
ISLOAD = 5
ALLCOMMANDSSENT = 6
ALLCOMMANDEXECUTED = 7
FAILEDCOMMANDEXECUTION = 8

# def service_connection(sel_key, event_mask):
# 	sock = sel_key.fileobj
# 	data = sel_key.data
# 	# & is bitwise AND
# 	if event_mask & selectors.EVENT_READ:
# 		recv_data = sock.recv(1024)
# 		if recv_data:
# 			data.outb += recv_data
# 		else:
# 			print(f"Closing connection to {data.addr}")
# 			sel.unregister(sock)
# 			sock.close()
# 	if event_mask & selectors.EVENT_WRITE:
# 		if data.outb:
# 			print(f"Echoing {data.outb!r} to {data.addr}")
# 			sent = sock.send(data.outb)
# 			data.outb = data.outb[sent:]	# Python slicing notation, s[m:n], returns a string starting at m up to but not including n, blank means start/end.


def send_command(socket, command):
    # First send ISCOMMAND
    socket.sendall(ISCOMMAND.to_bytes(4, 'big'))

    # Send command length
    command_len = len(command)
    socket.sendall(command_len.to_bytes(4, 'big'))

    # Send command
    socket.sendall(command.encode('utf-8'))


def send_file(socket, filename):
    path = os.path.join(os.getcwd(), filename)
    # First send ISFILE
    socket.sendall(ISFILE.to_bytes(4, 'big'))

    # Send size of file name
    print(f"Sending size of file name = {len(filename)}")
    socket.sendall(len(filename).to_bytes(4, 'big'))

    # Send file name
    print(f"Sending file name = {filename}")
    socket.sendall(filename.encode('utf-8'))

    # Send size of file
    file_size = pathlib.Path(filename).stat().st_size
    print(f"Sending file size = {file_size}, byte form:")
    socket.sendall(file_size.to_bytes(4, 'big'))

    # Send file
    with open(filename, "rb") as fd:
        while True:
            bytes_read = fd.read(file_size)
            if not bytes_read:
                print("File transfer complete")
                break
            socket.sendall(bytes_read)


def main():
    if len(sys.argv) <= 1 or len(sys.argv) > 2:
        print("Fatal: must provide rakefile")
        sys.exit(-1)

    rake_file = Rake_File()
    rake_file_address = sys.argv[1]

    with open(rake_file_address, 'r') as fd:
        lines = fd.readlines()
        for line in lines:
            stripped_line = line.strip()
            if stripped_line.startswith('#'):
                continue
            elif stripped_line.startswith('PORT  = ') and line[0] != '\t':
                port_num = stripped_line.split('PORT  = ', 1)[1]
                rake_file.port = int(port_num)
                continue
            elif stripped_line.startswith('HOST') and line[0] != '\t':
                hosts = stripped_line.split("HOSTS = ", 1)[1]
                hosts = hosts.split()
                for host in hosts:
                    rake_file.hosts.append(host)
                continue
            elif stripped_line.startswith('actionset') and line[0] != '\t':
                actset = Actionset()
                rake_file.total_actionsets += 1
                CURRENT_ACTSET_INDEX = rake_file.total_actionsets - 1
                print(f'current actset index = {CURRENT_ACTSET_INDEX}')
                # rake_file.actsets[CURRENT_ACTSET_INDEX] = actset
                rake_file.actsets.append(actset)
            elif line[0] == '\t' and line[1] != '\t':
                action = Action()
                if stripped_line.startswith('remote-'):
                    action.remote = True
                action.command = stripped_line.split('remote-', 1)[1]

                rake_file.actsets[CURRENT_ACTSET_INDEX].total_actions += 1
                CURRENT_ACT_INDEX = rake_file.actsets[CURRENT_ACTSET_INDEX].total_actions - 1
                # rake_file.actsets[CURRENT_ACTSET_INDEX].acts[CURRENT_ACT_INDEX] = action
                rake_file.actsets[CURRENT_ACTSET_INDEX].acts.append(action)
                continue
            elif line[0] == '\t' and line[1] == '\t':
                required_files = stripped_line.split("requires", 1)[1]
                required_files = required_files.split()
                for file in required_files:
                    rake_file.actsets[CURRENT_ACTSET_INDEX].acts[CURRENT_ACT_INDEX].required_files.append(
                        file)
                    rake_file.actsets[CURRENT_ACTSET_INDEX].acts[CURRENT_ACT_INDEX].total_files += 1

    current_actset = 0
    current_act = 0

    lowest_load = __INT32_MAX__
    lowest_load_socket = None

    actsets_finished = False

    all_act_sent = False

    command_sent = {}
    total_sock_recv_command = 0

    load_quried = {}
    total_quries = 0

    command_exec_error = False

    all_sock_obj = {}

    # Create selector
    sel = selectors.DefaultSelector()
    print(rake_file)
    for host in rake_file.hosts:
        server_addr = ()
        if ':' in host:
            host, port = host.split(':')
            server_addr = (host, int(port))
        else:
            server_addr = (host, rake_file.port)

        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        print(f"Connecting to server address: {server_addr}")
        sock.connect_ex(server_addr)
        events = selectors.EVENT_READ | selectors.EVENT_WRITE
        print(f"SOCK = {sock.fileno()}")
        sel.register(fileobj=sock, events=events, data=None)

        all_sock_obj[sock.fileno()] = sock
        load_quried[sock.fileno()] = False
        command_sent[sock.fileno()] = False

    # Event Loop
    try:
        while True:
            if actsets_finished or command_exec_error:
                if actsets_finished:
                    print("All actset successfully executed, shutting down all sockets")
                elif command_exec_error:
                    print(
                        "An error occured when executing remote commands, shutting down all sockets")
                for sock in all_sock_obj:
                    all_sock_obj[sock].close()
                break

            event = sel.select(timeout=None)
            for key, event_bitmask in event:
                if actsets_finished == False:
                    if event_bitmask & selectors.EVENT_WRITE and load_quried[key.fd] == False and all_act_sent == False:
                        print(f"Sending load query to {key.fd}")
                        key.fileobj.sendall(ISLOADQUERY.to_bytes(4, 'big'))
                        load_quried[key.fd] = True
                        total_quries += 1

                    if event_bitmask & selectors.EVENT_WRITE and all_act_sent == True and command_sent[key.fd] == True:
                        print(
                            f"Sending execute all command request to {key.fd}")
                        key.fileobj.sendall(ALLCOMMANDSSENT.to_bytes(4, 'big'))
                        command_sent[key.fd] = False

                if event_bitmask & selectors.EVENT_READ:
                    print("READ READY")
                    data_type = key.fileobj.recv(SIZEOF_INT)
                    data_type = int.from_bytes(
                        data_type, byteorder='little', signed=False)
                    if data_type == 0:
                        print(f"Socket {key.fd} closed unexpectedly")
                        sel.unregister(key.fileobj)
                        key.fileobj.close()
                    elif data_type == ISLOAD:
                        # INCOMING LOAD
                        # event = sel.select(timeout=None)
                        # for key, event_bitmask in event:
                        #     if event_bitmask & selectors.EVENT_READ and :

                        server_load = key.fileobj.recv(SIZEOF_INT)
                        server_load = int.from_bytes(
                            server_load, byteorder='little', signed=False)
                        total_quries -= 1
                        print(
                            f"Socket {key.fd} returned server load {server_load}")
                        if server_load <= lowest_load:
                            lowest_load = server_load
                            lowest_load_socket = all_sock_obj[key.fd]
                    elif data_type == ALLCOMMANDEXECUTED:
                        total_sock_recv_command -= 1

                        if total_sock_recv_command == 0:
                            current_actset += 1
                            current_act = 0
                            all_act_sent = False
                            if current_actset == rake_file.total_actionsets:
                                actsets_finished = True
                    elif data_type == FAILEDCOMMANDEXECUTION:
                        command_exec_error = True
                    elif data_type == ISFILE:
                        print("IN FILE RECIEVING")
                        # Recieve size of file name of incoming file
                        file_name_len = key.fileobj.recv(SIZEOF_INT)
                        file_name_len = int.from_bytes(file_name_len, 'little')
                        print(f"File name len = {file_name_len}")

                        # Recieve name of file
                        file_name = key.fileobj.recv(file_name_len)
                        file_name = file_name.decode('utf-8')
                        print(f"File name = {file_name}")

                        # Recieve size of file
                        file_size = key.fileobj.recv(SIZEOF_INT)
                        file_size = int.from_bytes(file_size, 'little')
                        print(f"File size = {file_size}")

                        # Create temp file in cwd
                        bytes_remaining = file_size
                        with open(file_name, 'wb') as fd:
                            while bytes_remaining > 0:
                                data_chunk = min(bytes_remaining, BUFFSIZE)
                                file_bytes = key.fileobj.recv(data_chunk)
                                fd.write(file_bytes)
                                bytes_remaining -= len(file_bytes)
                        # Inform client file has been recieved
                        print(f"Sending FILERECIEVED")
                        key.fileobj.sendall(FILERECIEVED.to_bytes(4, 'big'))

                if event_bitmask & selectors.EVENT_WRITE and lowest_load_socket == key.fileobj:
                    if all_act_sent == False and actsets_finished == False:
                        if rake_file.actsets[current_actset].acts[current_act].remote == True and total_quries == 0:
                            if rake_file.actsets[current_actset].acts[current_act].total_files == 0:
                                print(
                                    f"current command = {rake_file.actsets[current_actset].acts[current_act].command}")
                                print("Sending to socket %i\n",
                                      lowest_load_socket)
                                command = rake_file.actsets[current_actset].acts[current_act].command
                                send_command(lowest_load_socket, command)
                            else:
                                print("CURRENT COMMAND REQUIRES FILES")
                                print(
                                    f"current command = {rake_file.actsets[current_actset].acts[current_act].command}")
                                print("Sending to socket %i\n",
                                      lowest_load_socket)
                                for file in rake_file.actsets[current_actset].acts[current_act].required_files:
                                    print(f"Sending file {file}")
                                    send_file(lowest_load_socket, file)
                                    file_recieved = False
                                    while file_recieved == False:
                                        event = sel.select(timeout=None)
                                        for key, event_bitmask in event:
                                            if event_bitmask & selectors.EVENT_READ and key.fileobj == lowest_load_socket:
                                                data_type = key.fileobj.recv(
                                                    SIZEOF_INT)
                                                print(
                                                    f"Recieved data = {data_type} while waiting for FILE RECIEVED")
                                                if not data_type:
                                                    break
                                                data_type = int.from_bytes(
                                                    data_type, byteorder='little', signed=False)
                                                if data_type == FILERECIEVED:
                                                    file_recieved = True
                                print(f"File sent, sending command now!")
                                command = rake_file.actsets[current_actset].acts[current_act].command
                                send_command(lowest_load_socket, command)
                            if command_sent[lowest_load_socket.fileno()] == False:
                                total_sock_recv_command += 1
                                command_sent[lowest_load_socket.fileno()] = True
                            
                            current_act += 1
                            if current_act == rake_file.actsets[current_actset].total_actions:
                                all_act_sent = True
                            lowest_load_socket = None
                            lowest_load = __INT32_MAX__
                            for key in load_quried:
                                load_quried[key] = False
    except KeyboardInterrupt:
        print("Caught keyboard interrupt, exiting")
    finally:
        sel.close()


if __name__ == '__main__':
    main()
