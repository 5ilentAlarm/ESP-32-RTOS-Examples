#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "string.h"

//Pin definitions
#define TXD_PIN (GPIO_NUM_43)
#define RXD_PIN (GPIO_NUM_44)
#define LED_PIN 4

//UART stuff
static const int RX_BUF_SIZE = 1024; //Rx input buffer size

//Globals for Queue
static const uint8_t message_queue_length = 5; //Limit que elements to 5
static QueueHandle_t char_queue; //Queues
static QueueHandle_t int_queue;

//For Tasks
uint8_t blink_count = 0; //keeps track of how many times LED has blinked, set to 0 after 100 times
int t_delay = 0; //Delay that will be changed within blinkLED task

static const char* delay = "delay";
static const char* blinked = "blinked";

int extractDelay(uint8_t* message, int msg_length) {
    uint8_t matched = 0;
    uint8_t* number = (uint8_t*)malloc(RX_BUF_SIZE + 1);
    uint8_t number_index = 0;
    for(int i = 0; i < msg_length; i++) {
        if(message[i] == delay[i]) { //compare letters from message to "delay"
            matched++;
        }
        if(matched == 5) {
            //if delay was in the message, now we extract the delay and return it
            if((message[i] >= 48) && (message[i] <= 57)) {
                //if the character is a number
                number[number_index] = message[i];
                number_index++;
            }
        }
    }
    int delay = atoi((const char*) number);
    free(number);
    return delay; //returns delay number
}



//This task will receive input from the serial monitor, user inputs delay X, then X is sent to int_queue to be received by blinkLED task
//Also prints any message in char_queue
void vTask_receiveDelay(void *param) {
    static const char * currTaskName = "Task A\n";
    ESP_LOGI(currTaskName, "Commencing task A...\n");
    uint8_t* data = (uint8_t*)malloc(RX_BUF_SIZE + 1); //Data will be the pointer to our allocated recieving buffer, itll hold the data
    while(1) {
        const int rxBytes = uart_read_bytes(UART_NUM_0, data, RX_BUF_SIZE, 1000 / portTICK_PERIOD_MS); //read from input stream copy to data pointer, return bytes received
        if(rxBytes > 0) { //if there was actually data in there
            data[rxBytes] = 0; //set last byte
            printf("Got: %s\n", data); //Echo what was receieved

            //See if message contains delay and extract number from that
            int delay = 0;
            delay = extractDelay(data, rxBytes);
            //printf("Delay: %d\n", delay);
            if(xQueueSend(int_queue, (void*)&delay, 0) != pdTRUE) { //Check if queue is full, if not it'll store it
                printf("Queue full!\n");
            }
        }
        //This task also prints any messages in message queue, aka "blinked"
        char* msg;
        if(xQueueReceive(char_queue, (void*)&msg, 0) == pdTRUE) {
            printf("%s\n", msg);
        }
    }
    free(data); //free allocated space when task exits
}

//Receives a number from int queue and uses it to blink the led, led is contantly blinking
//Tracks amount of times LED has blinked, after 100 times "Blinked" is sent to char queue
void vTask_blinkLED(void *param) {
    static const char * currTaskName = "Task B\n";
    ESP_LOGI(currTaskName, "Commencing Task B...\n");
    while(1) {
        if(xQueueReceive(int_queue, (void*)&t_delay, 0) == pdTRUE) {
            printf("Delay from queue: %d\n", t_delay);
        }
        gpio_set_level(LED_PIN, 1);
        vTaskDelay(t_delay / portTICK_PERIOD_MS);
        gpio_set_level(LED_PIN, 0);
        vTaskDelay(t_delay / portTICK_PERIOD_MS);
        blink_count++;
        if(blink_count >= 100) {
            xQueueSend(char_queue, (void*)&blinked, 0); //send the message blinked to the queue
            blink_count = 0; //reset blink_count
        }
    }
}

void createQueues(void) {
    int_queue = xQueueCreate(message_queue_length, sizeof(int)); //create the queue with mql elements, each with a 32-bit integer size
    char_queue = xQueueCreate(message_queue_length, sizeof(char*)); //queue that will hold the "blinked" message from task b, takes in a pointer****
}

//Intialize UART to receive
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
}

void app_main(void)
{
    char *currTaskName = pcTaskGetName(NULL);
    ESP_LOGI(currTaskName, "Starting up!\n"); //"App_main "hello starting up"

    init();
    createQueues(); //Allocate queues

    xTaskCreate(&vTask_receiveDelay, "Recieving Delay", 1024 * 2, NULL, tskIDLE_PRIORITY, NULL);
    xTaskCreate(&vTask_blinkLED, "Blinking LED", 1024 * 2, NULL, tskIDLE_PRIORITY, NULL);
}
