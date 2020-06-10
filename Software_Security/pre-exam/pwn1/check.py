from pwn import *

elf = ELF('./pwn1')

print 'functions'
print '-' * 20
for key, _ in elf.functions.iteritems():
    print key
print '-' * 20
