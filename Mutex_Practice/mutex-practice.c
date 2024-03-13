/*
    Newest version of freeRTOS does not allow two different threads to take and release a mutex, this will trigger an assert error
    Meaning that, the solution of app_main taking the mutes, and then the blink_led function releasing the mutex, will not work. 
    The mutex must be taken and given in that same thread

    Alternate solution:
    Add a delay after creating the task, then have the task take the mutex, copy the argument, then release the mutex.
*/
#include <stdio.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "string.h"
#include "freertos/semphr.h"

//globals for peripherals
#define TXD_PIN (GPIO_NUM_43)
#define RXD_PIN (GPIO_NUM_44)
#define LED_PIN 4
static const int RX_BUF_SIZE = 1024; //Rx input buffer size

//globals for tasks
static SemaphoreHandle_t mutex; //mutex to help with critical sections

void vTask_BlinkLED(void *param) {
    int local_var;
    if(xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) { //if the mutex is avaiable, take it, run the code, then give it back and cont
        //This is where issues arise, by the time the task gets here, app_main would have left, so this would not save
        local_var = *(int *)param; //dereference the passed in reference, which is cast as an int pointer. Cast as int pointer -> deference -> int
        printf("Local variable: %d\n", local_var);
        xSemaphoreGive(mutex);
    }
    else {
        local_var = 0;
    }


    while(1) {
            //blink LED here
            gpio_set_level(LED_PIN, 1);
            vTaskDelay(local_var / portTICK_PERIOD_MS);
            gpio_set_level(LED_PIN, 0);
            vTaskDelay(local_var / portTICK_PERIOD_MS);
        }
}

void init(void) { //initialize UART and GPIO pins for the led ( can also be used to initailize other things). UART initialized to read serial input from terminal, which is why I'm using UART 0
    const uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    uart_driver_install(UART_NUM_0, RX_BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(UART_NUM_0, &uart_config);
    uart_set_pin(UART_NUM_0, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    //configure GPIO
    gpio_reset_pin(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
}

int receiveDelay(void) {
    uint8_t* data = (uint8_t*)malloc(RX_BUF_SIZE + 1); //Data will be the pointer to our allocated recieving buffer, itll hold the data
    while(1) { //wait until we receive something, before moving on
        const int rxBytes = uart_read_bytes(UART_NUM_0, data, RX_BUF_SIZE, 1000 / portTICK_PERIOD_MS); //read from input stream copy to data pointer, return bytes received
        if(rxBytes > 0) { //if there was actually data in there
            data[rxBytes] = 0; //set last byte
            //Convert received "string" into an integer 
            int delay = 0;
            delay = atoi((const char*)data);
            return delay;
        }
    }
    return 0;
}

void app_main(void)
{
    const char* currTaskName = "Main app";
    ESP_LOGI(currTaskName, "Starting Main!\n");

    //initialize 
    init();

    //create the mutex
    mutex = xSemaphoreCreateMutex();

    //Wait for input, store in local variable
    int delay_arg = 0;
    delay_arg = receiveDelay();
    printf("Delay is: %d\n", delay_arg);

    xTaskCreate(&vTask_BlinkLED, "Blink LED", 1024 * 2, (void *)&delay_arg, tskIDLE_PRIORITY, NULL); //A delay after this creation would work too, giving the task time to copy the value
    vTaskDelay(50 / portTICK_PERIOD_MS); //Used in place of waiting for a mutex to be given, due to reasons stated at the top of this file
}
