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

// Declare constants
const uint8 rxMode = 11; // decimal 11 = binary 00001011
const uint8 txMode = 10; // decimal 10 = binary 00001010

// Declare global variables
uint8 psocByte;
uint8 rfByte;
uint8 statusByte;
uint8 throwaway;
char printbuf[8];

// Declare functions
void setup();

uint8 readRegister(uint8 reg);
void writeRegister(uint8 reg, uint8 data);
void nrfRegisterTest();
void nrfReceiveTest(); // listens for transmissions and prints to UART
uint8 nrfReadByte(); // configures NRF to RX mode and reads a byte out of FIFO queue
void ce(uint8 byte);
void csn(uint8 byte);

void spiWrite(uint8 byte);
uint8 spiRead();

void blink();
void stop();
void printByte(uint8 byte);
void print(char *string);
void printRxBuffer();

void sendSquareWave();
void echoR31JP();

// Interrupt received for new Rx data on UART
// Won't work unless the line rx_int_StartEx(RX_INT) is uncommented in setup()
CY_ISR(RX_INT)
{
    LCD_Char_1_PutChar(UART_1_ReadRxData());     // RX ISR
}

// Main function
// Sends bytes from PC -> PSOC -> NRF24L01 -> PSOC -> PC
int main()
{
    
    /* Place your initialization/startup code here (e.g. MyInst_Start()) */  
    setup();
    
    for(;;)
    {
        /* R31JP Interaction Testing */
        
        // Send a square wave to the R31JP over UART
        // sendSquareWave();
        // Receive and echo bytes back to the R31JP
        // echoR31JP();
       
        /* NRF TESTING */
        
        // nrfReceiveTest();
        
        // Test reading and writing to registers in NRF24L01
        nrfRegisterTest();
        
    }
}

void setup() {
    CyGlobalIntEnable; /* Enable global interrupts. */
    //rx_int_StartEx(RX_INT);
    
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

// Wait for transmission from the other NRF24L01, and then
// echo that information to the screen
void nrfReceiveTest() {
    // Read status register to see if RX FIFO is not empty
    uint8 statusByte;
    uint8 clearRXDR = 0b10111111;
    uint8 receivedByte;
    while(1) {
        statusByte = readRegister(STATUS); 
        print("status byte: ");
        printByte(statusByte);
        // If there's stuff in the RX FIFO, read it
        if (statusByte & (1 << RX_DR)) {
            // Clear the RX_DR interrupt
            writeRegister(STATUS, statusByte & clearRXDR);
            // Get the received byte and print it
            receivedByte = nrfReadByte();
            print("received byte");
            printByte(receivedByte);
        }
    }
}

uint8 nrfReadByte() {
    uint8 receivedByte;
    writeRegister(CONFIG, rxMode); // Enter RX mode
    ce(1); // Enter RX mode
    csn(0); // Write csn low to signal a new command
    SPIM_1_WriteByte(R_RX_PAYLOAD); // Tell NRF24L01 that we want to read RX payload
    while (SPIM_1_GetRxBufferSize() < 1); // Wait for NRF24L01 to respond
    receivedByte = SPIM_1_ReadByte(); // Read the byte
    csn(1); // Signal the end of the command (QUESTION, here or before reading byte?)
    ce(0); // Go back to an idle state, like standby-1 mode
    return receivedByte;
}

void nrfRegisterTest() {
    uint8 nrfByte;
    // Wait for Rx FIFO to be not empty
    while(~UART_1_ReadRxStatus() & UART_1_RX_STS_FIFO_NOTEMPTY);
    psocByte = UART_1_GetChar();
    
    // Read CONFIG register
    print("Reading...");
    nrfByte = readRegister(CONFIG);
    printByte(nrfByte);
    // Write to the CONFIG register
    //print("Writing...");
    writeRegister(CONFIG, 0x09);
    // Read CONFIG register again
    print("Reading again...");
    nrfByte = readRegister(CONFIG);
    printByte(nrfByte);
    print("Echo...");
    UART_1_PutChar(psocByte);
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

/**************************/
/* Interaction with R31JP */
/**************************/

// Loops endlessly and sends a 255 and then a 0 over UART to the R31JP
void sendSquareWave() {
    while (1) {
        UART_1_PutChar(255);
        // Wait for it to send
        while(~UART_1_ReadTxStatus() & UART_1_TX_STS_COMPLETE);
        CyDelay(1000);
        UART_1_PutChar(0);
        while(~UART_1_ReadTxStatus() & UART_1_TX_STS_COMPLETE);
        CyDelay(1000);
    }
}

// Loops endlessly, echoing characters received from R31JP
void echoR31JP() {
    uint8 r31jpByte;
    uint8 maskedByte;
    uint8 mask = 24; // arbitrary
    while (1) {
        // Wait for character
        while(~UART_1_ReadRxStatus() & UART_1_RX_STS_FIFO_NOTEMPTY); 
        r31jpByte = UART_1_GetChar();
        // XOR it twice just to take up (a tiny bit of) time and fake the "encryption"
        maskedByte = r31jpByte ^ mask ^ mask;
        LCD_Char_1_ClearDisplay();
        LCD_Char_1_PrintNumber(r31jpByte);
        LCD_Char_1_PrintString("  ");
        LCD_Char_1_PrintNumber(maskedByte);
        UART_1_PutChar(maskedByte);
    }
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
