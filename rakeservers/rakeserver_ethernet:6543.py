import ctypes
import socket
import subprocess
import tempfile

HOST = "192.168.1.14"
PORT = 6543
ALL_COMMANDS = []

SIZEOF_INT = 4	# bytes

ISCOMMAND = 1
ISFILE = 2
ISLOADQUERY = 3
ISLOAD = 4
ISEXECUTEREQUEST = 5


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
					# Recieve name of file
					file_name = conn.recv(file_name_len)
					file_name = file_name.decode('utf-8')
					# Recieve size of file
					file_size = conn.recv(SIZEOF_INT)
					file_size = int.from_bytes(file_size, 'big')

					# Create a temp directory to store all required files
					

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
				elif data == ISEXECUTEREQUEST:
					print("IN EXECUTE REQUEST")
					# Execute all commands in all commands in parallel
					# Empty all commands
					# Send any generated files back to client

		for command in ALL_COMMANDS:
			print(command)


if __name__ == '__main__':
		main()		
