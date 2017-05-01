# Context switch
# swtch 并不了解线程，它只是简单地保存和恢复寄存器集合，即上下文
#
#   void swtch(struct context **old, struct context *new);
# 将当前 CPU 的寄存器压入栈中并将栈指针保存在 *old 中。然后 swtch 将 new 拷贝到 %esp 中
#，弹出之前保存的寄存器，然后返回。
# Save current register context in old
# and then load register context from new.
# esp堆栈指针
.globl swtch
swtch:							#从栈中弹出参数，放入寄存器%eax和%edx
  movl 4(%esp), %eax			#将esp中的前四位移到eax寄存器中
  movl 8(%esp), %edx			#将esp中的后四位移到edx寄存器中

  # Save old callee-save registers
  #入栈 在当前栈上建立一个新的上下文结构
  #仅有被调用者保存的寄存器此时需要被保存
  #按照 x86 的惯例即 %ebp %ebx %esi %edi %esp
  pushl %ebp					#保存上下文 显式地压入前四个
  pushl %ebx
  pushl %esi
  pushl %edi
  #%esp是在 struct context* 被写入 old时隐式地保存的
  #程序计数器 %eip在使用 call 调用 swtch 时就保存在栈中 %ebp 之上的位置上了


  # Switch stacks传送			#改变栈的指针
  movl %esp, (%eax)				#交换，将指向新的上下文大的指针放入栈中
  movl %edx, %esp
  #新的栈结构与旧的栈相同，因为新的上下文是之前某次切换的就得上下文


  # Load new callee-save registers出栈弹出上下文
  popl %edi						#返回的上下文，颠倒了顺序，先进后出！！
  popl %esi
  popl %ebx
  popl %ebp
  ret							#popl eip
