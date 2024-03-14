#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "string.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"

//Peripheral globals
#define TXD_PIN (GPIO_NUM_43)
#define RXD_PIN (GPIO_NUM_44)
#define LED_PIN 4
static const int RX_BUF_SIZE = 1024; //Rx input buffer size

//Timer handles
TimerHandle_t dim_timer;


void init(void) {
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
    gpio_set_level(LED_PIN, 0); //make sure pin is off at start
}

void vTimerCallback(TimerHandle_t xTimer) {
    printf("Dimming display\n");
    gpio_set_level(LED_PIN, 0);
    xTimerStop(dim_timer, 0);
}

void vTask_readRX(void* param) {
    uint8_t* data = (uint8_t*)malloc(RX_BUF_SIZE + 1); //Allocate space for char*(data)
    while(1) {
        const int rxBytes = uart_read_bytes(UART_NUM_0, data, RX_BUF_SIZE, 1000 / portTICK_PERIOD_MS);
        if(rxBytes > 0) {
            data[rxBytes] = 0;
            printf("Recieved: %s, brightening display!\n", data);
            gpio_set_level(LED_PIN, 1);
            xTimerReset(dim_timer, 0);
        }
    }
}

void app_main(void)
{
    char *currTaskName = "Main";
    ESP_LOGI(currTaskName, "Starting Main!\n");

    init(); //Initialize peripherals

    //TODO: Error check if timer wasn't created
    xTaskCreate(&vTask_readRX, "Receiving task", 1024 *2, NULL, tskIDLE_PRIORITY, NULL); //create task to detect input
    dim_timer = xTimerCreate("Dim Timer", 500, pdTRUE, (void*)0, vTimerCallback);
    xTimerStart(dim_timer, 0);
}
