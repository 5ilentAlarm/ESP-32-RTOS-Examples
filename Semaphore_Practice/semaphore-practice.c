#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "string.h"
#include "freertos/semphr.h"

//Initial code taken from example
enum {BUF_SIZE = 5}; //buffer size
static const int num_prod_tasks = 5; //number of producer tasks, will write to the buffer
static const int num_cons_tasks = 2; //number of consumer tasks, will read from the buffer and remove what was read
static const int num_writes = 3;

static int buf[BUF_SIZE]; //circular buffer that will be written/read 
static int head = 0; //front of buffer
static int tail = 0; //back of buffer
static SemaphoreHandle_t bin_sem; //binary semaphore that waits for the parameter to be read, doesn't allow app_main to exit before argument can be copied

//My semaphores and mutexes NOTE: Give() increments counter, take decrements
static SemaphoreHandle_t filled_sem; //Counting semaphore with 5 max count, start at 0
static SemaphoreHandle_t empty_sem; //Counting semaphore with 5 max count start at 0
static SemaphoreHandle_t print_mutex;

void producer(void *param) { //producer task that will write to the buffer 3 times
    int num = *(int*)param; //cast param as an int pointer and dereference it

    xSemaphoreGive(bin_sem); //Release the semaphore

    for(int i = 0; i < num_writes; i++) { //write to the front of the list, making sure to update the head 
        xSemaphoreTake(empty_sem, portMAX_DELAY); //decrement the amount of empty slots there are
        buf[head] = num; //set the current empty slot as the number
        head = (head + 1) % BUF_SIZE; //move the head right by 1(circular so mod 5)
        xSemaphoreGive(filled_sem); //incrememnt the fact that the filled slot has been taken
    }
    
    vTaskDelete(NULL); //delete the task
}

void consumer(void *param) {
    int val;

    while(1) {
        xSemaphoreTake(filled_sem, portMAX_DELAY);
        val = buf[tail]; //Read from the back
        tail = (tail + 1) % BUF_SIZE; //move tail 
        xSemaphoreGive(empty_sem);

        xSemaphoreTake(print_mutex, portMAX_DELAY);
        printf("Val: %d\n", val); 
        xSemaphoreGive(print_mutex);
    }
}

void app_main(void)
{
    const char* currTaskName = "Main App";
    ESP_LOGI(currTaskName, "Starting main!\n");

    char task_name[32];

    bin_sem = xSemaphoreCreateBinary(); //create the binary semaphore 
    filled_sem = xSemaphoreCreateCounting(5, 0); //5 is max count, 0 is inital count
    empty_sem = xSemaphoreCreateCounting(5, 5); //5 is max, 0 is initial, may change
    print_mutex = xSemaphoreCreateMutex();

    for(int i = 0; i < num_prod_tasks; i++) {
        sprintf(task_name, "Producer %i", i);
        printf("P: %s\n", task_name);
        xTaskCreate(&producer, task_name, 1024 * 2, (void *)&i, tskIDLE_PRIORITY, NULL); //The data sent to the task is its current number
        xSemaphoreTake(bin_sem, portMAX_DELAY); //Wait for a semaphore to be obtained(Which is after the data has been copied over)
    }

    for(int i = 0; i < num_cons_tasks; i++) {
        sprintf(task_name, "Consumer %i", i);
        printf("C: %s\n", task_name);
        xTaskCreate(&consumer, task_name, 1024 * 2, NULL, tskIDLE_PRIORITY, NULL); //The data sent to the task is its current number
    }

    printf("All tasks spawned!\n");
}
