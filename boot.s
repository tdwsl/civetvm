; load first 1k of first disk then execute

  org 64

  ; r4 = no of ports
  movb r0,0
  port 2
  mov r4,r1

  adr r3,diskid : ll r3,r3
  bl find
  bz r2,nodisk

  mov r0,r2
  movb r1,0
  port 3
  adr r1,code
  ll r1,r1
  movwu r2,1024+64
  sl r1,r2
  mov r3,r2
  addi r2,-64
  movbu r1,64
  mov pc,r3

diskid: db "disk"

nodisk:
  adr r3,conid : ll r3,r3
  bl find
  bz r2,nocon

  mov r0,r2
  adr r2,msg
  movbu r3,msglen

puts:
  lnb r1,r2
  port 1
  addi r3,-1
  bnz r3,puts

nocon:
  movb r0,0
  port 0

conid: db "ccon"
msg:
  db "no disk",10
msglen = . - msg
align 2

code:
  port 5
  movb pc,64

find: ; r3=devid -> r2=foundid|0
  mov r2,r4
find0:
  mov r1,r2
  port 1
  sub r1,r3
  bz r1,find1
  addi r2,-1
  bnz r2,find0
find1:
  mov pc,lr

