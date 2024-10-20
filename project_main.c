/* C Standard library */
#include <sensors/opt3001.h>
#include <stdio.h>

/* XDCtools files */
#include <xdc/std.h>
#include <xdc/runtime/System.h>

/* BIOS Header files */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/drivers/PIN.h>
#include <ti/drivers/pin/PINCC26XX.h>
#include <ti/drivers/I2C.h>
#include <ti/drivers/Power.h>
#include <ti/drivers/power/PowerCC26XX.h>
#include <ti/drivers/UART.h>

/* Board Header files */
#include "Board.h"

/* Task */
#define STACKSIZE 2048
Char sensorTaskStack[STACKSIZE];
Char uartTaskStack[STACKSIZE];

// JTKJ: Teht�v� 3. Tilakoneen esittely
// JTKJ: Exercise 3. Definition of the state machine
enum state { WAITING=1, DATA_READY=2};
enum state programState = WAITING;

// JTKJ: Teht�v� 3. Valoisuuden globaali muuttuja
// JTKJ: Exercise 3. Global variable for ambient light
double ambientLight = -1000.0;

// JTKJ: Teht�v� 1. Lis�� painonappien RTOS-muuttujat ja alustus
// JTKJ: Exercise 1. Add pins RTOS-variables and configuration here
static PIN_Handle buttonHandle;
static PIN_State buttonState;
static PIN_Handle ledHandle;
static PIN_State ledState;

PIN_Config buttonConfig[] = {
   Board_BUTTON0  | PIN_INPUT_EN | PIN_PULLUP | PIN_IRQ_NEGEDGE,
   PIN_TERMINATE // The configuration table is always terminated with this constant
};

// The constant Board_LED0 corresponds to one of the LEDs
PIN_Config ledConfig[] = {
   Board_LED0 | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX,
   PIN_TERMINATE // The configuration table is always terminated with this constant
};


void buttonFxn(PIN_Handle handle, PIN_Id pinId) {

    // JTKJ: Teht�v� 1. Vilkuta jompaa kumpaa ledi�
    // JTKJ: Exercise 1. Blink either led of the device
   uint_t pinValue = PIN_getOutputValue(Board_LED0);
   pinValue = !pinValue;
   PIN_setOutputValue(ledHandle, Board_LED0, pinValue);
}

/* Task Functions */
Void uartTaskFxn(UArg arg0, UArg arg1) {

    // JTKJ: Teht�v� 4. Lis�� UARTin alustus: 9600,8n1
    // JTKJ: Exercise 4. Setup here UART connection as 9600,8n1
    UART_Handle handle;
    UART_Params params;

    UART_Params_init(&params);
    params.writeDataMode = UART_DATA_TEXT;
    params.readDataMode = UART_DATA_TEXT;
    params.readEcho = UART_ECHO_OFF;
    params.readMode = UART_MODE_BLOCKING;
    params.baudRate = 9600; // 9600 baud rate
    params.dataLength = UART_LEN_8; // 8
    params.parityType = UART_PAR_NONE; // n
    params.stopBits = UART_STOP_ONE; // 1
    params.readTimeout = 100000/Clock_tickPeriod;// 100ms read timeout

    handle = UART_open(Board_UART0, &params);
   if (handle == NULL) {
      System_abort("Error opening the UART");
   }

   char uartmsg[15];

    while (1) {

        // JTKJ: Teht�v� 3. Kun tila on oikea, tulosta sensoridata merkkijonossa debug-ikkunaan
        //       Muista tilamuutos
        // JTKJ: Exercise 3. Print out sensor data as string to debug window if the state is correct
        //       Remember to modify state
        if (programState == DATA_READY) {
            sprintf(uartmsg, "\n\rlux:%8.2f", ambientLight);
            System_printf(uartmsg);
            System_flush();
            UART_write(handle, uartmsg, sizeof(uartmsg));
            programState = WAITING;
        }

        // JTKJ: Teht�v� 4. L�het� sama merkkijono UARTilla
        // JTKJ: Exercise 4. Send the same sensor data string with UART


        // Just for sanity check for exercise, you can comment this out
        //System_printf("uartTask\n");
        //System_flush();

        // Once per second, you can modify this
        Task_sleep(250000 / Clock_tickPeriod);
    }
}

Void sensorTaskFxn(UArg arg0, UArg arg1) {
    I2C_Handle      i2c;
    I2C_Params      i2cParams;

    // JTKJ: Teht�v� 2. Avaa i2c-v�yl� taskin k�ytt��n
    // JTKJ: Exercise 2. Open the i2c bus
    I2C_Transaction i2cMessage;

   // Initialize the I2C bus
   I2C_Params_init(&i2cParams);
   i2cParams.bitRate = I2C_400kHz;

   // Open the connection
   i2c = I2C_open(Board_I2C_TMP, &i2cParams);
   if (i2c == NULL) {
      System_abort("Error Initializing I2C\n");
   }

   // Transmit and receive buffers for I2C messages
   uint8_t txBuffer[1]; // Sending one byte
   uint8_t rxBuffer[2]; // Receiving two bytes

   // I2C message structure
   i2cMessage.slaveAddress = Board_OPT3001_ADDR;
   txBuffer[0] = OPT3001_REG_RESULT;      // Register address to the transmit buffer
   i2cMessage.writeBuf = txBuffer; // Set transmit buffer
   i2cMessage.writeCount = 1;      // Transmitting 1 byte
   i2cMessage.readBuf = rxBuffer;  // Set receive buffer
   i2cMessage.readCount = 2;       // Receiving 2 bytes

    // JTKJ: Teht�v� 2. Alusta sensorin OPT3001 setup-funktiolla
    //       Laita enne funktiokutsua eteen 100ms viive (Task_sleep)
    // JTKJ: Exercise 2. Setup the OPT3001 sensor for use
    //       Before calling the setup function, insertt 100ms delay with Task_sleep
   Task_sleep(100000 / Clock_tickPeriod);
   opt3001_setup(&i2c);

    while (1) {

        // JTKJ: Teht�v� 2. Lue sensorilta dataa ja tulosta se Debug-ikkunaan merkkijonona
        // JTKJ: Exercise 2. Read sensor data and print it to the Debug window as string

        // JTKJ: Teht�v� 3. Tallenna mittausarvo globaaliin muuttujaan
        //       Muista tilamuutos
        // JTKJ: Exercise 3. Save the sensor value into the global variable
        //       Remember to modify state
        if (programState==WAITING) {
            if (I2C_transfer(i2c, &i2cMessage)) {
                ambientLight = opt3001_get_data(&i2c);
                if (ambientLight>=0) {programState = DATA_READY;} // accept only valid readings
            }
            else {
                 System_printf("I2C Bus fault\n");
                 System_flush();
             }
        }

        // Once per second, you can modify this
        Task_sleep(100000 / Clock_tickPeriod);
    }
    //I2C_close(i2c);
}

Int main(void) {

    // Task variables
    Task_Handle sensorTaskHandle;
    Task_Params sensorTaskParams;
    Task_Handle uartTaskHandle;
    Task_Params uartTaskParams;

    // Initialize board
    Board_initGeneral();

    
    // JTKJ: Teht�v� 2. Ota i2c-v�yl� k�ytt��n ohjelmassa
    // JTKJ: Exercise 2. Initialize i2c bus
    // JTKJ: Teht�v� 4. Ota UART k�ytt��n ohjelmassa
    // JTKJ: Exercise 4. Initialize UART
    Board_initUART();
    Board_initI2C();

    // JTKJ: Teht�v� 1. Ota painonappi ja ledi ohjelman k�ytt��n
    //       Muista rekister�id� keskeytyksen k�sittelij� painonapille
    // JTKJ: Exercise 1. Open the button and led pins
    //       Remember to register the above interrupt handler for button
    buttonHandle = PIN_open(&buttonState, buttonConfig);
   if(!buttonHandle) {
      System_abort("Error initializing button pins\n");
   }
   ledHandle = PIN_open(&ledState, ledConfig);
   if(!ledHandle) {
      System_abort("Error initializing LED pins\n");
   }
   if (PIN_registerIntCb(buttonHandle, &buttonFxn) != 0) {
     System_abort("Error registering button callback function");
   }

    /* Task */
    Task_Params_init(&sensorTaskParams);
    sensorTaskParams.stackSize = STACKSIZE;
    sensorTaskParams.stack = &sensorTaskStack;
    sensorTaskParams.priority=2;
    sensorTaskHandle = Task_create(sensorTaskFxn, &sensorTaskParams, NULL);
    if (sensorTaskHandle == NULL) {
        System_abort("Task create failed!");
    }

    Task_Params_init(&uartTaskParams);
    uartTaskParams.stackSize = STACKSIZE;
    uartTaskParams.stack = &uartTaskStack;
    uartTaskParams.priority=2;
    uartTaskHandle = Task_create(uartTaskFxn, &uartTaskParams, NULL);
    if (uartTaskHandle == NULL) {
        System_abort("Task create failed!");
    }

    /* Sanity check */
    //System_printf("Hello world!\n");
    //System_flush();

    /* Start BIOS */
    BIOS_start();

    return (0);
}
