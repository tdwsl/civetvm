; test some arithmetic
; port 1 is console

  org 64

  movb r0,0
  port 3
  mov sp,r1

  movb r2,8 : bl putn
  movb r2,1 : addi r2,-17 : bl putn
  movb r2,1 : movb r1,17 : sub r2,r1 : bl putn
  movb r2,1 : movb r1,4 : add r2,r1 : bl putn
  movb r2,-13 : divui r2,3 : bl putn
  movb r2,-13 : divi r2,3 : bl putn
  movb r2,9 : mul r2,r2 : bl putn
  movb r2,123 : bl putn

  movb r0,0
  port 0

putn:
  dsw lr,sp
  mov bp,sp
  movb r0,1
  movb r3,10

  movb r1,0
  lt r2,r1
  bz lr,putn0

  movb r1,"-"
  port 1
  neg r2
putn0:
  mov r1,r2
  rem r1,r3
  addi r1,"0"
  dsb r1,sp
  div r2,r3
  bnz r2,putn0

putn1:
  lnb r1,sp
  port 1
  eq sp,bp
  bz lr,putn1

  movb r1,10
  port 1
  lnw pc,sp

