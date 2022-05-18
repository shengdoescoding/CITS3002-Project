import subprocess
import ctypes
import socket
import tempfile
import pathlib
import os

HOST = "127.0.0.1"
PORT = 4321
ALL_COMMANDS = []
ALL_REQUIRED_FILES = []

SIZEOF_INT = 4	# bytes
BUFFSIZE = 1024

ISCOMMAND = 1
ISFILE = 2
FILERECIEVED = 3
ISLOADQUERY = 4
ISLOAD = 5
ALLCOMMANDSSENT = 6
ALLCOMMANDEXECUTED = 7

def execute_all_commands():
	all_chilps = []
	for command in ALL_COMMANDS:
		childp = subprocess.Popen(command, shell=True)
		all_chilps.append(childp)
	exit_status = [childp.wait() for childp in all_chilps]
	print(f"exit status = {exit_status}")
	ALL_COMMANDS.clear()
	for fd in ALL_REQUIRED_FILES:
		os.remove(fd.name)
	ALL_REQUIRED_FILES.clear()

def main():
	# AF_INET = IPv4, SOCK_STREM = TCP
	with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
		s.bind((HOST, PORT))
		# s.listen() is optional, if omitted, default "backlog value" will be used
		# Max backlog value is system dependent, on Linux see "https://serverfault.com/questions/518862/will-increasing-net-core-somaxconn-make-a-difference/519152"
		s.listen()
		# conn is a new socket object used to communicate with client, not the same as s
		# addr contains (host, port) of the client
		conn, addr = s.accept()
		with conn:
			#print(f"...") is basically auto formatting for variables in {}, makes life easier
			print(f"Connected by {addr}")
			while True:
				data = conn.recv(SIZEOF_INT)
				if not data:
					break
				data = int.from_bytes(data, 'big')	# Replace ntoh
				print(f"Recieved data = {data}")
				if data == ISCOMMAND:
					print("IN COMMAND RECIEVING")
					# Recieve size of incoming command
					command_size = conn.recv(SIZEOF_INT)
					command_size = int.from_bytes(command_size, 'big')
					# Recieve command
					print(f"Recieved size of command = {command_size}")
					command = conn.recv(command_size)
					command = command.decode('utf-8')
					
					# Append command to ALL_COMMANDS
					print(f"{command}")
					command_list = command.split()
					ALL_COMMANDS.append(command_list)
				elif data == ISFILE:
					print("IN FILE RECIEVING")
					# Recieve size of file name of incoming file
					file_name_len = conn.recv(SIZEOF_INT)
					file_name_len = int.from_bytes(file_name_len, 'big')
					print(f"File name len = {file_name_len}")

					# Recieve name of file
					file_name = conn.recv(file_name_len)
					file_name = file_name.decode('utf-8')
					print(f"File name = {file_name}")

					# Recieve size of file
					file_size = conn.recv(SIZEOF_INT)
					file_size = int.from_bytes(file_size, 'big')
					print(f"File size = {file_size}")

					# Create temp file in cwd
					with open(file_name, 'wb') as fd:
						# Recieve file
						file_bytes = conn.recv(file_size)
						fd.write(file_bytes)
					# Store file desc so it can be deleted later
					ALL_REQUIRED_FILES.append(fd);

					# Inform client file has been recieved
					conn.sendall(bytes(ctypes.c_uint32(FILERECIEVED)))
				elif data == ISLOADQUERY:
					print("IN LOAD QUERY")
					load_head = ctypes.c_uint32(ISLOAD)  
					load_head = bytes(load_head)	# Big endian
					print(f"Sending load head bytes = {load_head} , int = {int.from_bytes(load_head, 'little')}")
					conn.sendall(load_head)
					
					load = ctypes.c_uint32(len(ALL_COMMANDS))
					load = bytes(load)
					print(f"Sending load bytes = {load} , int = {int.from_bytes(load, 'little')}")
					conn.sendall(load)
				elif data == ALLCOMMANDSSENT:
					print("IN ALL COMMANDS SENT")
					execute_all_commands()
					# Inform client all commands have been executed.
					conn.sendall(bytes(ctypes.c_uint32(ALLCOMMANDEXECUTED)))
					

		for command in ALL_COMMANDS:
			print(command)


if __name__ == '__main__':
		main()		
