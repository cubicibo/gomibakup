import wrappers as wrp
import struct

def format(nb=10):
	addr = []
	val = []
	read = []
	b_read, b_addr, b_val = wrp.cpu_get_script()
	for i in range(0,2*nb,2):
		addr.append(struct.unpack('H', b_addr[i:(i+2)])[0])
		val.append(struct.unpack('H', b_val[i:(i+2)])[0])

	for i in range(0, nb):
		read.append(struct.unpack('B', b_read[i:i+1])[0])
	return read, addr, val

def print(nb=10):
	read, addr, val = format(nb)

	for i in range(len(read)):
		if int(read[i]) == 1:
			print("R ",end='')
		else:
			print("W ",end='')

		print(hex(addr[i]), end=' ')
		print(hex(val[i]))