from base64 import decode
import socket
import subprocess

HOST = "127.0.0.1"
PORT = 4321

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
				data = conn.recv(1024) # we will read at most 1024 bytes
				if not data:
					break
				else:
					decoded = data.decode('utf-8')
					if "COMMAND" in decoded:
						command = decoded.removeprefix("COMMAND")
						command_list = command.split()
						# print(command_list)
						subprocess.run(command_list)

				# 
				# print(f"{decoded}")
				# # conn.sendall(b"local host")


if __name__ == '__main__':
		main()		
