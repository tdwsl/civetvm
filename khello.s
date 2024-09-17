; hello world for kernel mode
; assumes console to be port 1

  org 0x40

  adr r2,hellomsg
  bl puts

  movb r0,0
  port 0 ; quit

hellomsg:
  db "Hello, world!",10,0
  align 2

puts:
  movb r0,1
puts0:
  lnb r1,r2
  bz r1,puts1
  port 1      ; putc
  b puts0
puts1:
  mov pc,lr

