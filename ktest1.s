; list devices
; port 1 is console

  org 64

  movb r0,0
  port 2
  mov r5,r1
  movb r4,0

l0:
  movb r0,0
  mov r1,r4
  port 1
  mov r2,r1
  bl putd
  addi r4,1
  eq r4,r5
  bz lr,l0

  movb r0,0
  port 0

putd:
  movb r0,1
  movb r3,4
putd0:
  mov r1,r2
  andi r1,0xff
  port 1
  lsri r2,8
  addi r3,-1
  bnz r3,putd0

  movb r1,10
  port 1
  mov pc,lr

