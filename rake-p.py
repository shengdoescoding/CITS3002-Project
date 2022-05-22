import socket
import selectors
import sys
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

CURRENT_ACT_INDEX = 0
CURRENT_ACTSET_INDEX = 0

# def accept_connection(sock):
# 	conn, addr = sock.accept()
# 	print(f"Accepted connection from {addr}")
# 	conn.setblocking(False)
# 	data = types.SimpleNamespace(addr = addr, inb = b"", outb = b"")
# 	permit_events = selectors.EVENT_READ | selectors.EVENT_WRITE	# Permit both read and write with this accepted socket (conn)
# 	sel.register(conn, permit_events, data=data)


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


def main():
	if len(sys.argv) <= 1 or len(sys.argv) > 2:
		print("Fatal: must provide rakefile")
		sys.exit(-1)

	rake_file = Rake_File()
	rake_file_address = sys.argv[1];

	with open(rake_file_address, 'r') as fd:
		lines = fd.readlines()
		for line in lines:
			stripped_line = line.strip()
			if stripped_line.startswith('#'):
				continue
			elif stripped_line.startswith('PORT = ') and line[0] != '\t':
				port_num = stripped_line.split('PORT = ', 1)[1]
				rake_file.port = port_num
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
					rake_file.actsets[CURRENT_ACTSET_INDEX].acts[CURRENT_ACT_INDEX].required_files.append(file)
					rake_file.actsets[CURRENT_ACTSET_INDEX].acts[CURRENT_ACT_INDEX].total_files += 1
	
	print(rake_file)

	# Create selector
	sel = selectors.DefaultSelector()
	# Declare listening socket, and add to selector
	ssock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	ssock.bind((HOST, PORT))
	ssock.listen()
	print(f"Listening on {(HOST, PORT)}")
	ssock.setblocking(False)
	sel.register(ssock, selectors.EVENT_READ, data=None)	# ssock is a listening socket, therefore we want to read events, hense selectors.EVENT_READ
	# Event loop for I/O with client sockets
	try:
		while True:
			aval_sock = sel.select(timeout=None)	# Blocking, waiting for avaliable sockcet for I/O, initially, only waiting with listening socket
			for sel_key, event_mask in aval_sock:
				if sel_key.data is None:
					accept_connection(sel_key.fileobj)
				else:
					service_connection(sel_key, event_mask)
	except KeyboardInterrupt:
		print("Terminated by keyboard, exiting")
	finally:
		sel.close()
				
	
		

if __name__ == '__main__':
	main()