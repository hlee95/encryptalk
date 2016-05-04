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
#include <stdio.h>
#include <nrf.h>

uint8 psocByte;
uint8 rfByte;
uint8 statusByte;

char printbuf[8];

// Declare Functions
void setup();
uint8 readRegister(uint8 reg);
void writeRegister(uint8 reg, uint8 data);
uint8 nrfEcho(uint8 byte);
void nrfReceive(uint8 byte);
uint8 nrfSend(uint8 byte);
void ce(uint8 byte);
void csn(uint8 byte);
void blink();
void stop();
void printByte(uint8 byte);
void print(char *string);
void printRxBuffer();

// Main function
// Sends bytes from PC -> PSOC -> NRF24L01 -> PSOC -> PC
int main()
{
    CyGlobalIntEnable; /* Enable global interrupts. */

    /* Place your initialization/startup code here (e.g. MyInst_Start()) */  
    setup();
    
    for(;;)
    {
        /* Place your application code here. */
        
        // Receive byte from the PC
        // Wait for Rx FIFO to be not empty
        while(~UART_1_ReadRxStatus() & UART_1_RX_STS_FIFO_NOTEMPTY);
        psocByte = UART_1_GetChar();

        // Send byte to NRF24L01 and echo it back
        // rfByte = nrfEcho(psocByte);
        
        //print("Reading config register.");
        //readRegister(CONFIG);
        //uint8 testReg = RX_PW_P0;
        //print("Writing psocByte to test register.");
        //writeRegister(testReg, psocByte);
        //print("Reading test register.");
        //readRegister(testReg);
        
        // Display rfByte on LCD
        //LCD_Char_1_ClearDisplay();
        //LCD_Char_1_PutChar(rfByte);
        // Echo original byte back to PC
        print("echoing");
        UART_1_PutChar(rfByte);
        // Wait for transmission to complete
        while(~UART_1_ReadTxStatus() & UART_1_TX_STS_COMPLETE);
        print("\r\n");
        print("end of loop");
    }
}

void setup() {
     // Initialize LCD, UART, and SPI Master
    LCD_Char_1_Start();
    LCD_Char_1_ClearDisplay();
    UART_1_Start();
    SPIM_1_Start();
    // Clear buffers
    UART_1_ClearRxBuffer();
    UART_1_ClearTxBuffer();
    SPIM_1_ClearRxBuffer();
    SPIM_1_ClearTxBuffer();
    // Enable certain interrupts for UART so we can tell when
    // we've received a byte or when a transmission is complete
    UART_1_SetRxInterruptMode(UART_1_RX_STS_FIFO_NOTEMPTY);
    UART_1_SetTxInterruptMode(UART_1_TX_STS_COMPLETE);
    
    ce(0);
    csn(1); // Default state should be high, then write low to indicate command
    CyDelay(5); // Give RF time to set up
    
    // Turn on the LED, write to the LCD
    Pin_1_Write(1);
    LCD_Char_1_PrintString("Welcome!"); 
    print("Initializing");
}

uint8 nrfEcho(uint8 byte) {
        // Tell NRF to receive from its RxBuffer.
        nrfReceive(byte);
        
        // Tell NRF24L01 to send a byte
        return nrfSend(byte);

}

uint8 readRegister(uint8 reg) {
    // Set CSN high to let NRF know we are starting a new command
    csn(0);
    // Write command to read given register from NRF24L01
    SPIM_1_WriteTxData(R_REGISTER | (REGISTER_MASK & reg));
    // Wait for NRF24L01 to respond 
    while (SPIM_1_GetRxBufferSize() < 1);
    SPIM_1_WriteTxData(0xff);
    // First byte is the status byte
    statusByte = SPIM_1_ReadRxData();
    printByte(statusByte);
    // Second byte is the contents of the register.
    rfByte = SPIM_1_ReadRxData();
    printByte(rfByte);
    
    // Must set CSN high again so that when we call another command
    // the NRF sees a high->low transition
    csn(1);
    return rfByte;
}

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

// Writes the commands to have NRF24L01 receive the byte in RX FIFO
void nrfReceive(uint8 byte) {
    UART_1_PutString("nrfReceive\n");
    csn(1);
    csn(0);
    SPIM_1_WriteByte(R_RX_PAYLOAD);
    UART_1_PutString("sending R_RX_PAYLOAD command\n");
    while((SPIM_1_ReadTxStatus() & 1) != 1);
    UART_1_PutString("finished sending\n");
    SPIM_1_WriteByte(byte);
    UART_1_PutString("sending R_RX_PAYLOAD data\n");
    while((SPIM_1_ReadTxStatus() & 1) != 1);
    UART_1_PutString("finished sending\n");
}

// Writes commands to make NRF24L01 send a byte
uint8 nrfSend(uint8 byte) {
    UART_1_PutString("nrfSend");
    csn(1);
    csn(0);
    // Send SPI command telling NRF24L01 to write to TxBuffer
    SPIM_1_WriteByte(W_TX_PAYLOAD);
    UART_1_PutString("sending W_TX_PAYLOAD command\n");
    while((SPIM_1_ReadTxStatus() & 1) != 1);
    UART_1_PutString("finished sending\n");
    SPIM_1_WriteByte(byte);
    UART_1_PutString("sending W_TX_PAYLOAD data\n");
    while((SPIM_1_ReadTxStatus() & 1) != 1);
    UART_1_PutString("finished sending\n");
    // Wait until SPI Master RxFIFO is not empty
    UART_1_PutString("waiting for SPI Master Rx FIFO not empty\n");
    while((SPIM_1_ReadRxStatus() & (1 << 5)) != 1) {
        UART_1_PutChar(SPIM_1_ReadRxStatus());
    };
    UART_1_PutString("reading byte\n");
    return SPIM_1_ReadByte();
    UART_1_PutString("done reading\n");
}



/***********/
/* HELPERS */
/***********/

// Prints everything in SPI Master's Rx buffer
// Used for debugging to see what's there
void printRxBuffer() {
    print("printing rx buffer");
    while (SPIM_1_GetRxBufferSize() > 0) {
        print("received");
        printByte(SPIM_1_ReadRxData());
    }
    print("done printing rx buffer");
}

// Generic print function that prints a string
void print(char* string) {
    UART_1_PutString(string);
    UART_1_PutString("\n\r");
    // Wait for it to finish sending
    while(~UART_1_ReadTxStatus() & UART_1_TX_STS_COMPLETE);
}

// Prints a byte to the PC over serial
void printByte(uint8 byte) {
    snprintf(printbuf, 8, "%u", byte);
    print(printbuf);
}

// Sets chip enable high or low for NRF24L01
void ce(uint8 b) {
    // chip enable is Pin_6
    Pin_6_Write(b);
}

// Sets chip select high or low for NRF24L01
void csn(uint8 b) {
    // chip select is Pin_5
    Pin_5_Write(b);
}

// Used for debugging
// Makes LED blink once
void blink() {
    Pin_1_Write(0);
    CyDelay(500);
    Pin_1_Write(1);
    CyDelay(500);
    Pin_1_Write(0);
}

// Used for debugging
// Stops program and blinks LED indefinitely
void stop() {
    for (;;) {
        blink();
    }
}

/* [] END OF FILE */
