#include "coopos.h"
#include "demo_tasks.h"

// PIC24FJ256DA210 Configuration Bit Settings

// 'C' source line config statements

// CONFIG4

// CONFIG3
#pragma config WPFP = WPFP255           // Write Protection Flash Page Segment Boundary (Highest Page (same as page 170))
#pragma config SOSCSEL = SOSC           // Secondary Oscillator Power Mode Select (Secondary oscillator is in Default (high drive strength) Oscillator mode)
#pragma config WUTSEL = LEG             // Voltage Regulator Wake-up Time Select (Default regulator start-up time is used)
#pragma config ALTPMP = ALPMPDIS        // Alternate PMP Pin Mapping (EPMP pins are in default location mode)
#pragma config WPDIS = WPDIS            // Segment Write Protection Disable (Segmented code protection is disabled)
#pragma config WPCFG = WPCFGDIS         // Write Protect Configuration Page Select (Last page (at the top of program memory) and Flash Configuration Words are not write-protected)
#pragma config WPEND = WPENDMEM         // Segment Write Protection End Page Select (Protected code segment upper boundary is at the last page of program memory; the lower boundary is the code page specified by WPFP)

// CONFIG2
#pragma config POSCMOD = NONE           // Primary Oscillator Select (Primary oscillator is disabled)
#pragma config IOL1WAY = ON             // IOLOCK One-Way Set Enable (The IOLOCK bit (OSCCON<6>) can be set once, provided the unlock sequence has been completed. Once set, the Peripheral Pin Select registers cannot be written to a second time.)
#pragma config OSCIOFNC = ON            // OSCO Pin Configuration (OSCO/CLKO/RC15 functions as port I/O (RC15))
#pragma config FCKSM = CSDCMD           // Clock Switching and Fail-Safe Clock Monitor (Clock switching and Fail-Safe Clock Monitor are disabled)
#pragma config FNOSC = FRCPLL           // Initial Oscillator Select (Fast RC Oscillator with Postscaler and PLL module (FRCPLL))
#pragma config PLL96MHZ = ON            // 96MHz PLL Startup Select (96 MHz PLL is enabled automatically on start-up)
#pragma config PLLDIV = DIV2            // 96 MHz PLL Prescaler Select (Oscillator input is divided by 2 (8 MHz input))
#pragma config IESO = ON                // Internal External Switchover (IESO mode (Two-Speed Start-up) is enabled)

// CONFIG1
#pragma config WDTPS = PS32768          // Watchdog Timer Postscaler (1:32,768)
#pragma config FWPSA = PR128            // WDT Prescaler (Prescaler ratio of 1:128)
#pragma config ALTVREF = ALTVREDIS      // Alternate VREF location Enable (VREF is on a default pin (VREF+ on RA9 and VREF- on RA10))
#pragma config WINDIS = OFF             // Windowed WDT (Standard Watchdog Timer enabled,(Windowed-mode is disabled))
#pragma config FWDTEN = OFF //ON              // Watchdog Timer (Watchdog Timer is enabled)
#pragma config ICS = PGx1               // Emulator Pin Placement Select bits (Emulator functions are shared with PGEC1/PGED1)
#pragma config GWRP = OFF               // General Segment Write Protect (Writes to program memory are allowed)
#pragma config GCP = OFF                // General Segment Code Protect (Code protection is disabled)
#pragma config JTAGEN = OFF             // JTAG Port Disable

task_context_t *uart_task_ptr, *blink_task_ptr;
volatile char recvd_char = 0;
volatile char LED = 0;

void SystemInit(void)
{
    // init oscillator
    CLKDIVbits.CPDIV = 0; // 32 MHz (16 MIPS)
    CLKDIVbits.RCDIV = 0;
    CLKDIVbits.PLLEN = 1;

    // LED
    ANSGbits.ANSG8 = 0; // digital
    TRISGbits.TRISG8 = 0; // output
    
    // init UART - end module dependant
    RPINR18bits.U1RXR = 11u;
    RPOR8bits.RP16R = 3; //3 = U1TX
    U1MODEbits.BRGH = 1; // high speed
    U1BRG = 34u; // 115200 baud @ Fcy = 16 MHz
    U1MODEbits.UARTEN = 1; // enable
    U1STAbits.UTXEN = 1;
    //U1STAbits.OERR = 0; // clear potential overrun error
    IFS0bits.U1RXIF = 0;
    IEC0bits.U1RXIE = 1;
    IFS4bits.U1ERIF = 0;
    IEC4bits.U1ERIE = 1;
    
    T1CON = 0x8000; // Fcy, 1:1 (16MHz)
    PR1 = 15999; // 1 ms
    IFS0bits.T1IF = 0;
    IEC0bits.T1IE = 1;

    /* TODO: other peripherials here */

    LATGbits.LATG8 = ~LATGbits.LATG8;

}

void Send_UART(uint8_t ch)
{
    while(U1STAbits.UTXBF == 1) {}
    U1TXREG = ch;
}

void TasksInit(void)
{
    task_context_t *dummy;

    // task #0 - UART parser
    TaskInit(uart_task_ptr, Task_A, 256);
    // task #1
    TaskInit(dummy, Task_B, 256);
    // task #2 - LED blinking
    TaskInit(blink_task_ptr, Task_C, 256);

    /* TODO: other task here */

}

void Task_A(task_context_t* my_tcon)
{
    uint8_t i = 0;
    
    // task is first-time called during initialization,
    // so simply Yield here for first time
    Yield(0);
    // task has to NEVER end
    while(1)
    {
        if(recvd_char > 0)
        {
            if(recvd_char == 'L')
            {
                LED = 1;
            }
            else if (recvd_char == 'D')
            {
                TaskDisable(blink_task_ptr);
            }
            else if (recvd_char == 'E')
            {
                TaskEnable(blink_task_ptr);
            }
            Send_UART(recvd_char);
            recvd_char = 0;
        }
        i++;
        Yield(-1); // disable this task (will be enabled from UartRx ISR)
    }
}

void Task_B(task_context_t* my_tcon)
{
    Yield(0);
    while(1)
    {
        if(LED > 0)
        {
            LED = 0;
            LATGbits.LATG8 = ~LATGbits.LATG8;
        }
        Yield(0); // stay active
    }
}

void Task_C(task_context_t* my_tcon)
{
    Yield(0);
    while(1)
    {
        LATGbits.LATG8 = ~LATGbits.LATG8;
        Yield(500); // sleep for 500 SysTicks (TMR1-beats)
    }
}

void __attribute__((__interrupt__, __auto_psv__)) _U1RXInterrupt(void)
{
    recvd_char = U1RXREG;
    TaskEnable(uart_task_ptr); // wakeup Task_A()
    IFS0bits.U1RXIF = 0;
}

void __attribute__((__interrupt__, __no_auto_psv__)) _U1ErrInterrupt(void)
{
    U1STAbits.OERR = 0; // clear and flush buffer
    IFS4bits.U1ERIF = 0;
}

void __attribute__((__interrupt__, __auto_psv__)) _T1Interrupt(void)
{
    SysTickISR();
    IFS0bits.T1IF = 0;
}
