from pwn import *


elf = ELF('./BubbleSort')
def addr_str(addr):
    print hex(addr)
    print elf.string(addr)

print 'functions'
print '-' * 20
for key, _ in elf.functions.iteritems():
    print key
print '-' * 20

print 'in dark soul'
addr_str(0x8048898)
addr_str(0x804889b)
