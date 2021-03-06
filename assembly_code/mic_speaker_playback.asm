; This program is an endless loop that ferries values from
; the ADC to the DAC.
; The goal is to be able to speak into the microphone and
; playback the sound through the speakers automatically.

; Big part will be figuring out how to remove noise.

.org 0000h
ljmp start

.org 0100h
start:
    loop:
        lcall readadc
        lcall writedac
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

end
