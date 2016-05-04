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

void setup();
void blink();

uint8 psocByte;

int main()
{
    CyGlobalIntEnable; /* Enable global interrupts. */

    /* Place your initialization/startup code here (e.g. MyInst_Start()) */
    setup();
    
    for(;;)
    {
        /* Place your application code here. */
        // Test that UART is working properly.
        
        // Wait until we receive a character
        // Wait for Rx FIFO to be not empty
        while(~UART_1_ReadRxStatus() & UART_1_RX_STS_FIFO_NOTEMPTY);
        psocByte = UART_1_GetChar();
        
        UART_1_PutChar(psocByte);
        // Wait for transmission to complete
        while(~UART_1_ReadTxStatus() & UART_1_TX_STS_COMPLETE);
    }
}

void setup() {
    // Set up UART component
    UART_1_Start();
    UART_1_SetRxInterruptMode(UART_1_RX_STS_FIFO_NOTEMPTY);
    UART_1_SetTxInterruptMode(UART_1_TX_STS_COMPLETE);
    
    Pin_1_Write(1);
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
