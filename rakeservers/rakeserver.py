import subprocess
import ctypes
import socket
import os
import pathlib

HOST = ""
PORT = 0
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
FAILEDCOMMANDEXECUTION = 8

def diff(list1, list2):
	return list(set(list1) - set(list2)) + list(set(list2) - set(list1))

def get_file_paths_in_cwd():
	files = []
	for path in os.listdir(os.getcwd()):
		if os.path.isfile(os.path.join(os.getcwd(), path)):
			files.append(os.path.join(os.getcwd(), path))
	return files

def execute_all_commands():
	all_chilps = []
	for command in ALL_COMMANDS:
		childp = subprocess.Popen(command)
		all_chilps.append(childp)
	exit_status = [childp.wait() for childp in all_chilps]
	print(f"exit status = {exit_status}")
	if 1 in exit_status:
		return False
	return True

def main():
	current_files = get_file_paths_in_cwd()
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
					bytes_remaining = file_size
					with open(file_name, 'wb') as fd:
						while bytes_remaining > 0:
							data_chunk = min(bytes_remaining, BUFFSIZE)
							file_bytes = conn.recv(data_chunk)
							fd.write(file_bytes)
							bytes_remaining -= len(file_bytes)
					ALL_REQUIRED_FILES.append(fd);

					# Inform client file has been recieved
					print(f"Sending FILERECIEVED")
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

					files_pre_exec = get_file_paths_in_cwd()
					no_error = execute_all_commands()
					files_post_exec = get_file_paths_in_cwd()

					if no_error == True:
						if len(files_post_exec) - len(files_pre_exec) != 0:
							new_files = diff(files_pre_exec, files_post_exec)
							print("Some file created, sending back to client, number of new files = ")
							print(*new_files, sep = ", ")
							for path in new_files:
								print(f"Sending created file {path} to client")
								# First send ISFILE to inform client a file is coming
								print(f"Sending ISFILE")
								conn.sendall(bytes(ctypes.c_uint32(ISFILE)))

								# Second send size of file name
								file_name = pathlib.PurePath(path).name
								print(f"Sending size of file name = {len(file_name)}")
								conn.sendall(bytes(ctypes.c_uint32(len(file_name))))

								# Third send name of file
								print(f"Sending file name = {file_name}")
								conn.sendall(file_name.encode('ascii'))	# Hopefully c will convert from ascii to char auto
				
								# Fourth send size of file
								file_size = pathlib.Path(file_name).stat().st_size
								print(f"Sending file size = {file_size}, byte form:")
								print(bytes(ctypes.c_uint32(file_size)))
								conn.sendall(bytes(ctypes.c_uint32(file_size)))

								# print("Testing python primative int with to_bytes()")
								# file_size_bytes = file_size.to_bytes(4, byteorder='little')
								# print(f"Sending file size = {file_size}, byte form with to_bytes(4, byteorder='little'): {file_size_bytes}")
								# conn.sendall(file_size_bytes)

								# Send file
								with open(file_name, "rb") as fd:
									while True:
										bytes_read = fd.read(file_size)
										if not bytes_read:
											print("File transfer complete")
											break
										conn.sendall(bytes_read)

								# Wait for FILERECIEVED
								while(True):
									data = conn.recv(SIZEOF_INT)
									print(f"Recieved data = {data} while waiting for FILE RECIEVED")
									if not data:
										break
									data = int.from_bytes(data, 'big')	# Replace ntoh
									if(data == FILERECIEVED):
										break

						# Inform client all commands have been executed.
						print("SENDING NO ERROR")
						conn.sendall(bytes(ctypes.c_uint32(ALLCOMMANDEXECUTED)))

					else:
						# Inform client an error occured
						print("SENDING ERROR")
						conn.sendall(bytes(ctypes.c_uint32(FAILEDCOMMANDEXECUTION)))

					# Clean Up
					ALL_COMMANDS.clear()
					for fd in ALL_REQUIRED_FILES:
						os.remove(fd.name)
					ALL_REQUIRED_FILES.clear()

					final_files = get_file_paths_in_cwd()
					new_files = diff(current_files, final_files)
					for path in new_files:
						os.remove(path)

		for command in ALL_COMMANDS:
			print(command)


if __name__ == '__main__':
		main()		
