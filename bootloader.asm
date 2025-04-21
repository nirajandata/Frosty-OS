; just for the sake for tradition, writing it from the sample
bits 16
org 0x7c00

mov si, 0 

_print:
  mov ah, 0x0e
  mov al, [myString+si]
  int 0x10
  add si, 1
  cmp byte [myString+si], 0
  jne _print
  
jmp $

myString:
  db "meow meow",0

times 510 -($- $$) db 0
dw 0xAA55

