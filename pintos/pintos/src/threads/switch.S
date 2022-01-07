#include "threads/switch.h"

#### struct thread *switch_threads (struct thread *cur, struct thread *next);
####
#### Switches from CUR, which must be the running thread, to NEXT,
#### which must also be running switch_threads(), returning CUR in
#### NEXT's context.
####
#### This function works by assuming that the thread we're switching
#### into is also running switch_threads().  Thus, all it has to do is
#### preserve a few registers on the stack, then switch stacks and
#### restore the registers.  As part of switching stacks we record the
#### current stack pointer in CUR's thread structure.

.globl switch_threads
.func switch_threads
switch_threads:
	# Save caller's register state.
	#
	# Note that the SVR4 ABI allows us to destroy %eax, %ecx, %edx,
	# but requires us to preserve %ebx, %ebp, %esi, %edi.  See
	# [SysV-ABI-386] pages 3-11 and 3-12 for details.
	#
	# This stack frame must match the one set up by thread_create()
	# in size.
	# 先4个寄存器压栈保存寄存器状态（保护作用）， 这4个寄存器是switch_threads_frame的成员
	pushl %ebx
	pushl %ebp
	pushl %esi
	pushl %edi

	# Get offsetof (struct thread, stack).
	# 全局变量thread_stack_ofs记录线程和栈之间的间隙， 我们都知道线程切换有个保存现场的过程
.globl thread_stack_ofs
	mov thread_stack_ofs, %edx

	# Save current stack pointer to old thread's stack, if any.
	# 先把当前的线程指针放到eax中
	# 并把线程指针保存在相对基地址偏移量为edx的地址中
	movl SWITCH_CUR(%esp), %eax
	movl %esp, (%eax,%edx,1)

	# Restore stack pointer from new thread's stack.
	# 切换到下一个线程的线程棧指针， 保存在ecx中，
	# 再把这个线程相对基地址偏移量edx地址（上一次保存现场的时候存放的）放到esp当中继续执行
	movl SWITCH_NEXT(%esp), %ecx
	movl (%ecx,%edx,1), %esp

	# 这里ecx, eax起容器的作用， edx指向当前现场保存的地址偏移量。
	# 简单来说就是保存当前线程状态， 恢复新线程之前保存的线程状态

	# Restore caller's register state.
	popl %edi
	popl %esi
	popl %ebp
	popl %ebx
	# 然后再把4个寄存器拿出来， 这个是硬件设计要求的， 必须保护switch_threads_frame里面的寄存器才可以destroy掉eax, edx, ecx
	# 然后注意到现在eax(函数返回值是eax)就是被切换的线程棧指针
        ret
.endfunc

.globl switch_entry
.func switch_entry
switch_entry:
	# Discard switch_threads() arguments.
	addl $8, %esp

	# Call thread_schedule_tail(prev).
	pushl %eax
.globl thread_schedule_tail
	call thread_schedule_tail
	addl $4, %esp

	# Start thread proper.
	ret
.endfunc
