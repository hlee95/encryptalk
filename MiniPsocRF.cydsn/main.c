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
uint8 readRegister(uint8 reg);
void writeRegister(uint8 reg, uint8 data);
void adcToDac();
void echoUART();
void setup();
void blink();
void printByte(uint8 byte);
void print(char* string);
void csn(uint8 value);

// Declare global variables
uint8 psocByte;
uint8 statusByte; // status byte from NRF24L01
uint8 adcValue;
char printbuf[8]; // buffer used to print things to the PC

// Begin main program
int main()
{
    CyGlobalIntEnable; /* Enable global interrupts. */

    /* Place your initialization/startup code here (e.g. MyInst_Start()) */
    setup();
    VDAC8_1_Wakeup();
    for(;;)
    {
        // Ferry values from the ADC to the DAC to test the playback system
        // adcToDac();
        VDAC8_1_SetValue(255);
        CyDelay(1);
        VDAC8_1_SetValue(0);
        CyDelay(1);
    }
}

// Reads a register from the NRF24L01
uint8 readRegister(uint8 reg) {
    uint8 temp;
    // Set CSN high to let NRF know we are starting a new command
    csn(0);
    // Write command to read given register from NRF24L01
    SPIM_1_WriteTxData(R_REGISTER | (REGISTER_MASK & reg));
    // Wait for NRF24L01 to respond 
    while (SPIM_1_GetRxBufferSize() < 1);
    SPIM_1_WriteTxData(0xff);
    // First byte is the status byte
    statusByte = SPIM_1_ReadRxData();
    // Second byte is the contents of the register.
    temp = SPIM_1_ReadRxData();
    printByte(temp);
    // Must set CSN high again so that when we call another command
    // the NRF sees a high->low transition
    csn(1);
    return temp;
}

// Writes to a register on the NRF24L01
void writeRegister(uint8 reg, uint8 data) {
    // Pull chip select low to trigger a new command for NRF42L01
    csn(0);
    
    // Write command to read a register
    SPIM_1_WriteTxData(W_REGISTER | (REGISTER_MASK & reg));
    // Wait for SPI master to finish sending
    while((SPIM_1_ReadTxStatus() & SPIM_1_STS_SPI_DONE) != 1);
    // Write the data to that register
    SPIM_1_WriteTxData(data);
    // Wait for SPI master to finish sending
    while((SPIM_1_ReadTxStatus() & SPIM_1_STS_SPI_DONE) != 1)
    
    // Flush buffer because writing a command makes NRF24L01 return status register
    // and we don't want that sitting in the Rx buffer the next time we try to read something.
    SPIM_1_ClearRxBuffer();
    
    // Set chip select high again so the next time we pull it low it triggers a command
    csn(1);
}

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
    Pin_1_Write(1);
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

void blink() {
    Pin_1_Write(0);
    CyDelay(500);
    Pin_1_Write(1);
    CyDelay(500);
    Pin_1_Write(0);
}

void blinkFast() {
    Pin_1_Write(0);
    CyDelay(200);
    Pin_1_Write(1);
    CyDelay(200);
    Pin_1_Write(0);
}

/* [] END OF FILE */
