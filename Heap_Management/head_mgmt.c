#include <stdio.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "string.h"

#define TXD_PIN (GPIO_NUM_43)
#define RXD_PIN (GPIO_NUM_44)

static const int RX_BUF_SIZE = 1024; //Rx input buffer size

static volatile bool newline_received = 0; //global bool to use as signal
static  unsigned char* message = NULL;

//initialize UART ports, as well as GPIO ports
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
}

//Task to recive characters through UART, and save them into (data)
static void vTask_recieve(void *param) {
    static const char *currTaskName = "RX task\n";
    ESP_LOGI(currTaskName, "Starting!\n");
    uint8_t* data = (uint8_t*)malloc(RX_BUF_SIZE + 1); //allocate the size of our buffer into a data pointer, this will hold our receieving data
    for(;;) {
        const int rxBytes = uart_read_bytes(UART_NUM_0, data, RX_BUF_SIZE, 1000 / portTICK_PERIOD_MS); //read from input stream
        if(rxBytes > 0) { //rxBytes holds how many bytes were input
            data[rxBytes] = 0;  //set the last character to zero
            printf("Received: %s\n", data);
            printf("Last Charachter: %c\n", data[rxBytes - 1]);
            message = (uint8_t*)malloc(RX_BUF_SIZE + 1);
            message = data;
            message[rxBytes] = '\0';
            printf("Pointer contents: %s\n", message);
            if(data[rxBytes - 1] == '\r') {
                printf("Nice.\n");
                newline_received = 1;
            }
        }
    }
    free(data);
}

static void vTask_ackNewline(void *param) {
    static const char *currTaskName = "RX task\n";
    ESP_LOGI(currTaskName, "Starting!\n");

    for(;;) {
        if(newline_received) {
            printf("Newline acknowledged!\n");
            newline_received = 0;
        }
    }
    free(message);
}

void app_main(void)
{
    char *ourTaskName = pcTaskGetName(NULL); //create pointer to task name so that LOGI can tell us which function we are in
    ESP_LOGI(ourTaskName, "Hello, starting up!\n"); //"App_main "hello starting up"

    init();

    xTaskCreate(&vTask_recieve, "Rx_Task", 1024 *4, NULL, tskIDLE_PRIORITY, NULL);
    xTaskCreate(&vTask_ackNewline, "Newline_Task", 1024 *2, NULL, tskIDLE_PRIORITY, NULL);
}
