/* ========================================
 *
 * Copyright YOUR COMPANY, THE YEAR
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION
 * WHICH IS THE PROPERTY OF your company.
 *
 * ========================================
*/
#include <project.h>
#include <nrf.h>
#include <stdio.h>

// Declare functions
void nrfRegisterTest();
uint8 readRegister(uint8 reg);
void writeRegister(uint8 reg, uint8 data);
void nrfSendTest();
void nrfSendByte(uint8 byte);

void spiWrite(uint8 byte);
uint8 spiRead();

void buttonTest();

void adcToDac();
void dacSquareWave();

void echoUART();

void setup();
void blink();
void printByte(uint8 byte);
void print(char* string);
void csn(uint8 value);
void ce(uint8 value);

// Declare global variables
uint8 psocByte;
uint8 statusByte; // status byte from NRF24L01
uint8 rfByte;
uint8 throwaway; // byte we don't care about, returned from NRF24L01 after writing command
uint8 adcValue;
char printbuf[8]; // buffer used to print things to the PC

// Declare constants
const uint8 rxMode = 11; // decimal 11 = binary 00001011
const uint8 txMode = 10; // decimal 10 = binary 00001010

// Begin main program
int main()
{
    CyGlobalIntEnable; /* Enable global interrupts. */

    /* Place your initialization/startup code here (e.g. MyInst_Start()) */
    setup();
    for(;;)
    {
        // Test detecting a button press.
        // buttonTest();
        
        // Echo values over serial to PC
        // echoUART();
        
        // Test reading and writing to registers in NRF24L01
        nrfRegisterTest();
        
        // Test sending values to the other NRF24L01
        // nrfSendTest();
        
        // Ferry values from the ADC to the DAC to test the playback system
        // adcToDac();
        // Write a square wave to the DAC
        // dacSquareWave();
    }
}

// Send a constant stream of bytes over NRF24L01
void nrfSendTest() {
    uint8 data[6] = {'a', 'b', 'c', 'd', 'e', 'f'};
    int i;
    for (i = 0; i ++; i < 6) {
        nrfSendByte(data[i]);
    }  
}

// Transmit a byte
void nrfSendByte(uint8 byte) {
    // Write the byte to the TX payload.
    csn(0); // Set csn low to indicate a new command
    SPIM_1_WriteByte(W_TX_PAYLOAD); // Tell NRF24L01 that we are sending it TX payload
    SPIM_1_WriteByte(byte); // Write the data byte
    csn(1); // Set csn low to indicate end of command
    writeRegister(CONFIG, txMode); // Set up CONFIG by writing PWR_UP high and PRIM_RX low
    ce(1); // Write CE high to go into TX mode
}

// Turns on LED when button is pressed, turns LED off when button not pressed
void buttonTest() {
    // begin in unpressed state
    uint8 pressed = 0;
    while (1) {
        // If we are in the unpressed state and then we get a button press
        // (remember that the button is active low)
        // then turn LED on
        if (!pressed & !Button_Read()) {
            LED_Write(1);
            pressed = 1;
        }
        // If we are in pressed state and then we get button release
        // then turn LED off
        if (pressed & Button_Read()) {
            LED_Write(0);
            pressed = 0;
        }
    }
}

void nrfRegisterTest() {
    uint8 nrfByte;
    // Wait for Rx FIFO to be not empty
    // while(~UART_1_ReadRxStatus() & UART_1_RX_STS_FIFO_NOTEMPTY);
    // psocByte = UART_1_GetChar();
    
    // Read CONFIG register
    print("Reading...");
    nrfByte = readRegister(CONFIG);
    printByte(nrfByte);
    // Write to the CONFIG register
    print("Writing...");
    writeRegister(CONFIG, 0x09);
    // Read CONFIG register again
    print("Reading again...");
    nrfByte = readRegister(CONFIG);
    printByte(nrfByte);
    // print("Echo...");
    // UART_1_PutChar(psocByte);
    print("");
    
}

uint8 readRegister(uint8 reg) {
    // Set CSN high to let NRF know we are starting a new command
    csn(0);
    // Write command to read given register from NRF24L01
    spiWrite(R_REGISTER | (REGISTER_MASK & reg));
    
    // Write anything to signal to NRF24L01 to start sending the contents
    SPIM_1_WriteByte(0xff);
    while((SPIM_1_ReadTxStatus() & SPIM_1_STS_SPI_DONE) != 1);
    
    rfByte = spiRead();
    
    // Must set CSN high again so that when we call another command
    // the NRF sees a high->low transition
    csn(1);
    return rfByte;
}

void writeRegister(uint8 reg, uint8 data) {
    // Pull chip select low to trigger a new command for NRF42L01
    csn(0);
    
    // Write command to read a register
    spiWrite(W_REGISTER | (REGISTER_MASK & reg));
    
    // Write the data to that register
    spiWrite(data);
    
    // Set chip select high again so the next time we pull it low it triggers a command
    csn(1);
}

// Transmits something from SPI Master to the slave,
// then waits for transission to complete
// and throws away the automatically returned status byte
void spiWrite(uint8 byte) {
    SPIM_1_WriteByte(byte);
    while((SPIM_1_ReadTxStatus() & SPIM_1_STS_SPI_DONE) != 1);
    while (SPIM_1_GetRxBufferSize() < 1);
    throwaway = SPIM_1_ReadByte();
}

// Waits for RX buffer to be nonempty, then reads one byte and returns it
uint8 spiRead() {
    while (SPIM_1_GetRxBufferSize() < 1);
    return SPIM_1_ReadByte();
}

/***************/
/* ADC and DAC */
/***************/

// Reads values from the ADC and sends it to the DAC
void adcToDac() {
    VDAC8_1_Wakeup(); // Wake the DAC up
    while(1) {
        // Wait for ADC to finish a conversion
        while(!ADC_DelSig_1_IsEndConversion(ADC_DelSig_1_WAIT_FOR_RESULT));
        adcValue = ADC_DelSig_1_GetResult8();
        // Write DAC value
        VDAC8_1_SetValue(adcValue);
    }  
}

// Writes a square wave to the DAC
void dacSquareWave() {
    VDAC8_1_Wakeup();
    while(1) {
        VDAC8_1_SetValue(255);
        CyDelay(1);
        VDAC8_1_SetValue(0);
        CyDelay(1);   
    }
}

// Waits for and echoes one character from the PC
void echoUART() {
    // Wait until we receive a character
    // Wait for Rx FIFO to be not empty
    while(~UART_1_ReadRxStatus() & UART_1_RX_STS_FIFO_NOTEMPTY);
    psocByte = UART_1_GetChar();
    
    UART_1_PutChar(psocByte);
    // Wait for transmission to complete
    while(~UART_1_ReadTxStatus() & UART_1_TX_STS_COMPLETE);
}

void setup() {
    // Set up UART component
    UART_1_Start();
    UART_1_SetRxInterruptMode(UART_1_RX_STS_FIFO_NOTEMPTY);
    UART_1_SetTxInterruptMode(UART_1_TX_STS_COMPLETE);
    // Clear buffers
    UART_1_ClearRxBuffer();
    UART_1_ClearTxBuffer();
    // Enable certain interrupts for UART so we can tell when
    // we've received a byte or when a transmission is complete
    UART_1_SetRxInterruptMode(UART_1_RX_STS_FIFO_NOTEMPTY);
    UART_1_SetTxInterruptMode(UART_1_TX_STS_COMPLETE);
    
    // Set up SPI Master
    SPIM_1_Start();
    // Clear buffers
    SPIM_1_ClearRxBuffer();
    SPIM_1_ClearTxBuffer();
    
    // Set up ADC
    ADC_DelSig_1_Start();				
	ADC_DelSig_1_StartConvert();
    
    // Set up DAC
    VDAC8_1_Start();
    VDAC8_1_Sleep(); // Don't want DAC to do anything yet
    
    // Turn on LED
    LED_Write(1);
}

// Prints a byte to the PC via UART
void printByte(uint8 byte) {
    snprintf(printbuf, 8, "%u", byte);
    print(printbuf);
}

// Prints a string to the PC
void print(char * string) {
    UART_1_PutString(string);
    UART_1_PutString("\n\r");
    // Wait for it to finish sending
    while(~UART_1_ReadTxStatus() & UART_1_TX_STS_COMPLETE);
}

// Write to chip select of SPI Master
void csn(uint8 value) {
    CSN_Write(value);
}

// Write to chip enable of SPI Master
// Used to put NRF24L01 into an active mode e.g. either RX or TX mode
void ce(uint8 value) {
    CE_Write(value);   
}

void blink() {
    LED_Write(0);
    CyDelay(500);
    LED_Write(1);
    CyDelay(500);
    LED_Write(0);
}

void blinkFast() {
    LED_Write(0);
    CyDelay(200);
    LED_Write(1);
    CyDelay(200);
    LED_Write(0);
}

/* [] END OF FILE */
