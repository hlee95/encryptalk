; This program receives bytes from the psoc
; and writes them to the DAC (at location #0fe08h)
; and writes them to Port 1

.org 00h
ljmp start

.org 100h
start:
  lcall init
  mov dptr, #0fe08h ; Location of DAC
  loop:
    lcall getchr    ; Get the last thing sent from the psoc
    movx @dptr, a   ; Write it to the DAC
    mov p1, a       ; Write it to the P1 LEDs
  sjmp loop



;=================================================================
; subroutine init
; this routine initializes the hardware
; set up serial port with a 11.0592 MHz crystal,
; use timer 1 for 9600 baud serial communications
;=================================================================
init:
   mov   tmod, #20h       ; set timer 1 for auto reload - mode 2
   mov   tcon, #41h       ; run counter 1 and set edge trig ints
   mov   th1,  #0fdh      ; set 9600 baud with xtal=11.059mhz
   mov   scon, #50h       ; set serial control reg for 8 bit data
                          ; and mode 1
   ret

;===============================================================
; subroutine getchr
; this routine reads in a chr from the serial port and saves it
; in the accumulator.
;===============================================================
getchr:
   jnb  ri, getchr        ; wait till character received
   mov  a,  sbuf          ; get character
   anl  a,  #7fh          ; mask off 8th bit
   clr  ri                ; clear serial status bit
   ret

;===============================================================
; subroutine sndchr
; this routine takes the chr in the acc and sends it out the
; serial port.
;===============================================================
sndchr:
   clr  scon.1            ; clear the tx buffer full flag.
   mov  sbuf,a            ; put chr in sbuf
txloop:
   jnb  scon.1, txloop    ; wait till chr is sent
   ret

end