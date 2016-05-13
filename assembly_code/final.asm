; This is the final assembly file for the project.
; When the button is pressed, I send values from the ADC to
; the PSOC through serial.
; When the button is not pressed, I'm waiting for values from
; the PSOC and writing those values to the DAC
; This program is fairly simple; the bulk of the coding challenge
; of this project is in getting the communication going
; between the NRF24L01 wireless chips.

; Note that I modified getchr to jmp back to the loop
; instead of waiting forever for a character

.org 0000h
ljmp start

.org 100h
start:
  lcall init
  loop:
    ; Check for button press
    jb p1.0, nobutton
    lcall transmit ; There is a button, so send a value
    sjmp loop

    nobutton: ; If no button pressed, wait to receive a value
    ljmp getchr
    continue: ; Only gets to continue if we received a character
      cpl p1.3
      lcall writedac

  sjmp loop


transmit:
    lcall readadc
    lcall sndchr
ret

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
   jnb  ri, loop          ; wait till character received
   mov  a,  sbuf          ; get character
   clr  ri                ; clear serial status bit
ljmp continue

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
