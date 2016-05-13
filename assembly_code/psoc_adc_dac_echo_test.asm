; This program reads the ADC, sends the value to the PSOC
; and then receives the value back from the PSOC and then
; writes it to the DAC.
; Test by using function generator to write a slow square wave
; to the ADC, and seeing if the DAC output is the same.
; Second test is to go through the microphone and speaker circuits,
; still using a function generator.
; Third test is to play a note to the microphone and see if
; I can hear it correctly out of the speakers after going
; through the PSOC.

.org 00h
ljmp start

.org 100h
start:
  lcall init
  loop:
    lcall readadc
    lcall sndchr
    lcall getchr    ; Get the last thing sent from the psoc
    lcall writedac
    mov p1, a       ; Write it to the P1 LEDs
  sjmp loop


readadc:
; This subroutine samples the ADC and puts the value in the accumulator.
    mov dptr, #0fe28h
    movx @dptr, a           ; Write anything to trigger conversion.
    nop                     ; Takes 2.5 micros to do the conversion.
    nop
    nop
    movx a, @dptr
ret

writedac:
; This subroutine writes the value in the accumulator to the DAC.
    mov dptr, #0fe08h
    movx @dptr, a
ret

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