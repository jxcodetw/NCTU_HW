default rel
sigsetsize equ 8

struc Timespec
	.sec:  resq 1
	.nsec: resq 1
	.size:
endstruc

struc Jmpbuf
	.rbx:  resq 1
	.rsp:  resq 1
	.rbp:  resq 1
	.r12:  resq 1
	.r13:  resq 1
	.r14:  resq 1
	.r15:  resq 1
	.rip:  resq 1
	.mask: resq 1
endstruc

struc Sigaction
	.sa_handler:  resq 1
	.sa_flags:    resq 1
	.sa_restorer: resq 1
	.sa_mask:     resq 1
	.size:
endstruc

struc SigactionLibc
	.sa_handler:   resq 1
	.sa_sigaction: resq 1
	.sa_mask:      resq 1
	.sa_flags:     resq 1
	.sa_restorer:  resq 1
endstruc

	; functoin call params: RDI, RSI, RDX, RCX, R8, R9 
	section .text

	; exit, rdi = exit code
	global sys_exit:function
sys_exit:
	mov	rax, 60
	syscall
	ret

	global write:function
write:
	; rdi, rsi, rdx is filled by caller (fd, buf, len)
	mov	rax, 1
	syscall
	ret

	global sleep:function
sleep:
	push rbp
	mov rbp, rsp
	sub rsp, Timespec.size

	mov QWORD [rsp+Timespec.nsec], 0
	mov QWORD [rsp+Timespec.sec], rdi
	mov rdi, rsp
	xor rsi, rsi
	mov rax, 35 ; call nanosleep
	syscall

	leave
	ret

	global sys_pause:function
sys_pause:
	mov rax, 34
	syscall
	ret

	global alarm:function
alarm:
	mov rax, 37
	syscall
	ret

	global setjmp:function
setjmp:
	mov QWORD [rdi+Jmpbuf.rbx], rbx
	mov QWORD [rdi+Jmpbuf.rsp], rsp
	mov QWORD [rdi+Jmpbuf.rbp], rbp
	mov QWORD [rdi+Jmpbuf.r12], r12
	mov QWORD [rdi+Jmpbuf.r13], r13
	mov QWORD [rdi+Jmpbuf.r14], r14
	mov QWORD [rdi+Jmpbuf.r15], r15

	mov rax, QWORD [rsp]
	mov QWORD [rdi+Jmpbuf.rip], rax

	; save mask
	lea rdi, [rdi+Jmpbuf.mask]
	mov rsi, 0 ; nset = null
	mov rdi, 1 ;BLOCK but ignored
	call sigprocmask

	xor rax, rax
	ret

	global longjmp:function
longjmp:
	mov rsp, QWORD [rdi+Jmpbuf.rsp]
	pop rax
	mov rax, QWORD [rdi+Jmpbuf.rip]
	push rax

	mov rbx, QWORD [rdi+Jmpbuf.rbx]
	mov rbp, QWORD [rdi+Jmpbuf.rbp]
	mov r12, QWORD [rdi+Jmpbuf.r12]
	mov r13, QWORD [rdi+Jmpbuf.r13]
	mov r14, QWORD [rdi+Jmpbuf.r14]
	mov r15, QWORD [rdi+Jmpbuf.r15]

	; r15 as tmp
	xor rdx, rdx  ; oset = null
	lea rsi, [rdi+Jmpbuf.mask] ; nset
	mov rdi, 2 ; set mask
	call sigprocmask
	
	mov rax, rsi
	ret

	global sigaction:function
sigaction:
	push rbp
	mov rbp, rsp
	sub rsp, Sigaction.size
	
	mov rax, QWORD [rsi+SigactionLibc.sa_handler]
	mov QWORD [rsp+Sigaction.sa_handler], rax

	mov rax, QWORD [rsi+SigactionLibc.sa_flags]
	mov QWORD [rsp+Sigaction.sa_flags], rax
	or  QWORD [rsp+Sigaction.sa_flags], 0x04000000

	lea rax, [rel __mysigret]
	mov QWORD [rsp+Sigaction.sa_restorer], rax

	mov rax, QWORD [rsi+SigactionLibc.sa_mask]
	mov QWORD [rsp+Sigaction.sa_mask], rax

	mov rsi, rsp

	mov r10, sigsetsize
	mov rax, 13 ; sys_rt_sigaction
	syscall

	leave
	ret

__mysigret:
	mov rax, 15
	syscall
	ret

	global sigprocmask:function
sigprocmask:
	mov r10, sigsetsize
	mov rax, 14
	syscall

	ret
