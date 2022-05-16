from ast import While
from asyncio import events
import socket
import selectors
import types

HOST = "127.0.0.1"
PORT = 4321

def service_connection(sel_key, event_mask):
	sock = sel_key.fileobj
	data = sel_key.data
	# & is bitwise AND 
	if event_mask & selectors.EVENT_READ:
		recv_data = sock.recv(1024)
		if recv_data:
			data.outb += recv_data
		else:
			print(f"Closing connection to {data.addr}")
			sel.unregister(sock)
			sock.close()
	if event_mask & selectors.EVENT_WRITE:
		if data.outb:
			print(f"Echoing {data.outb!r} to {data.addr}")
			sent = sock.send(data.outb)
			data.outb = data.outb[sent:]	# Python slicing notation, s[m:n], returns a string starting at m up to but not including n, blank means start/end.

def accept_connection(sock):
	conn, addr = sock.accept()
	print(f"Accepted connection from {addr}")
	conn.setblocking(False)
	data = types.SimpleNamespace(addr = addr, inb = b"", outb = b"")
	permit_events = selectors.EVENT_READ | selectors.EVENT_WRITE	# Permit both read and write with this accepted socket (conn)
	sel.register(conn, permit_events, data=data)

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