; write to disk
; console is 1
; disk is 2

  org 64

  movb r0,0
  movb r1,2
  port 1
  adr r2,diskid
  ll r2,r2
  eq r1,r2
  bnz lr,hasdisk

  adr r2,nomsg
  bl puts
  b end

diskid: db "disk"
nomsg: db "no disk",10,0

align 2

hasdisk:
  movb r0,2
  movbu r1,255
  port 3
  adr r1,data
  movbu r2,datalen
  port 6

end:
  movb r0,0
  port 0

data: db "WROTETODISK"
datalen = . - data

align 2

puts:
  movb r0,1
puts0:
  lnb r1,r2
  bz r1,puts0
  port 1
  b puts0
puts1:
  mov pc,lr

