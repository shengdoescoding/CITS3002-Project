import ctypes
import socket
import subprocess

HOST = "127.0.0.1"
PORT = 4321
ALL_COMMANDS = []

SIZEOF_INT = 4	# bytes

ISCOMMAND = 1
ISFILE = 2
ISLOADQUERY = 3
ISLOAD = 4


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
				conn.sendall(data)
				data = int.from_bytes(data, 'big')	# Replace ntoh
				print(f"Recieved data = {data}")
				if data == ISCOMMAND:
					print("IN COMMAND RECIEVING")
					command_size = conn.recv(SIZEOF_INT)
					command_size = int.from_bytes(command_size, 'big')

					print(f"Recieved size of command = {command_size}")
					command = conn.recv(command_size)
					command = command.decode('utf-8')

					print(f"{command}")
					command_list = command.split()
					# subprocess.run(command_list)
					ALL_COMMANDS.append(command_list)
					# EXECUTE ALL COMMANDS in ALL_COMMANDS IN PARALLEL
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
		for command in ALL_COMMANDS:
			print(command)


if __name__ == '__main__':
		main()		
