
%macro	PUSH_ALL 0	; zero parameters
	push	rax
	push	rbx
	push	rcx
	push	rdx
	push	rsi
	push	rdi	; rdi must be the last
%endmacro

%macro	POP_ALL 0	; zero parameters
	pop	rdi	; rdi must be the first
	pop	rsi
	pop	rdx
	pop	rcx
	pop	rbx
	pop	rax
%endmacro

	section .data

SA_RESTORER EQU 0x04000000

struc Jmpbuf
	; RBX, RSP, RBP, R12, R13, R14, R15, return addr
	.rbx: resq 1
	.rsp: resq 1
	.rbp: resq 1
	.r12: resq 1
	.r13: resq 1
	.r14: resq 1
	.r15: resq 1
	.rip: resq 1
	.mask: resq 1
endstruc

struc act  ; sigaction structure
	.sa_handler: resq 1
	.sa_sigaction: resq 1
	.sa_mask: resq 1
	.sa_flags: resq 1
	.sa_restorer: resq 1
endstruc

struc k_act
	.sa_handler: resq 1
	.sa_flags: resq 1
	.sa_restorer: resq 1
	.sa_mask: resq 1
	.size:
endstruc

struc siginfo
	.si_signo:	resq 1
	.si_errno:	resq 1
	.si_code:	resq 1
	.si_trapno:	resq 1
	.si_pid:	resq 1
	.si_uid:	resq 1
	.si_status:	resq 1
	.si_utime:	resq 1
	.si_stime:	resq 1
	.si_value:	resq 1
	.si_int:	resq 1
	.si_ptr:	resq 1
	.si_overrun:resq 1
	.si_timerid:resq 1
	.si_addr:	resq 1
	.si_band:	resq 1
	.si_fd:		resq 1
	.si_addr_lsb:resq 1
	.si_lower:	resq 1
	.si_upper:	resq 1
	.si_pkey:	resq 1
	.si_call_addr:resq 1
	.si_syscall:resq 1
	.si_arch:	resq 1
endstruc

struc timespec
	.tv_sec:	resq 1
	.tv_nsec:	resq 1
	.size:
endstruc

	; functoin call params: RDI, RSI, RDX, RCX, R8, R9 
	; system call params: RDI, RSI, RDX, R10, R8, R9
	section .text

	; exit, rdi = exit code
	global exit
exit:
	mov	rax, 60
	syscall
	ret
	
	global sigreturn
sigreturn
	mov rax, 15
	syscall
	ret

	global sigaction:function
sigaction
    ; long sigaction(int how, const struct sigaction *nact, struct sigaction *oact)
	push rbp
	mov rbp, rsp
	sub rsp, k_act.size	

	mov rax, [rsi+act.sa_handler]
	mov [rsp+k_act.sa_handler], rax

	mov rax, [rsi+act.sa_flags]
	or rax, SA_RESTORER
	mov [rsp+k_act.sa_flags], rax

	mov QWORD [rsp+k_act.sa_restorer], sigreturn

	mov rax, [rsi+act.sa_mask]
	mov [rsp+k_act.sa_mask], rax

	lea rsi, [rsp]
	
	mov r10, 8   ; r10: size_t sigsetsize
	mov rax, 13  ; sys_rt_sigaction
	syscall
	leave
	ret


	global sigprocmask
sigprocmask
	; int sigprocmask(int how, const sigset_t *set, sigset_t *oldset);
	push rbp
	mov rbp, rsp
	push r10
	mov r10, 8  ; r10: size_t sigsetsize
	mov rax, 14  ; sys_rt_sigprocmask
	syscall
	pop r10
	leave
	ret


	global longjmp
longjmp:
	; void longjmp(jmp_buf env, int val)
	push rbp
	mov rbp, rsp

	mov rbx, [rdi+Jmpbuf.rbx]
	mov rsp, [rdi+Jmpbuf.rsp]
	mov rbp, [rdi+Jmpbuf.rbp]
	mov r12, [rdi+Jmpbuf.r12]
	mov r13, [rdi+Jmpbuf.r13]
	mov r14, [rdi+Jmpbuf.r14]
	mov r15, [rdi+Jmpbuf.r15]

	mov rax, rsi
	leave  ; mov rsp, rbp; pop rbp
	pop r15  ; pop long jump's next instruction
	mov r15, [rdi+Jmpbuf.rip]
	push r15 ; push new rip
	mov r15, [rdi+Jmpbuf.r15]
	ret  ; pop rip

	global setjmp
setjmp:
	; int setjmp(jmp_buf env)
	; RBX, RSP, RBP, R12, R13, R14, R15, return addr
	push rbp
	mov rbp, rsp
	mov [rdi+Jmpbuf.rbx], rbx
	mov [rdi+Jmpbuf.rsp], rsp
	mov [rdi+Jmpbuf.rbp], rbp
	mov [rdi+Jmpbuf.r12], r12
	mov [rdi+Jmpbuf.r13], r13
	mov [rdi+Jmpbuf.r14], r14
	mov [rdi+Jmpbuf.r15], r15
	; mov [rdi+Jmpbuf.mask], rsi
	mov rax, 0  ; return value
	leave
	pop r15  ; pop the return address (rip)
	mov [rdi+Jmpbuf.rip], r15
	push r15
	mov r15, [rdi+Jmpbuf.r15]
	ret  ; pop rip

	global alarm
alarm:
	push rbp
	mov rbp, rsp
	mov rax, 37
	syscall
	leave
	ret

 	global _pause
_pause:
 	mov rax, 34
 	syscall
 	ret

	global write
write:
	push	rbp
	mov	rbp, rsp
	mov	rax, 1
	; rdi, rsi, rdx is filled by caller (fd, buf, len)
	syscall
	leave  ; mov rsp, rbp; pop rbp
	ret

	global sleep
sleep:
	; void sys_nanosleep(struct timespec *rqtp, struct timespec *rmtp)
	push rbp
	mov rbp, rsp
	sub rsp, timespec.size
	mov [rsp+timespec.tv_sec], rdi
	mov QWORD [rsp+timespec.tv_nsec], 0
	lea rdi, [rsp]
	xor rsi, rsi
	mov rax, 35 ; call nanosleep
	syscall
	leave
	ret
