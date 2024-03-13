# ESP32 FreeRTOS Mutex example
This file is a solution to FreeRTOS's tutorial 6, where a mutex is used to prohibit a calling function from ending before a passed
argument has been copied.

The Digikey solution seems to not work anymore, since newer versions check if a mutex is taken and given by the same task. If the 
task's are not the same, then an assert error is thrown, making the ESP32 reset. 

This solution remedies this by using a delay after the task is created, giving time for the task to take the mutex, thus locking
app_main from returning since this function is also waiting to take the mutex. After the argument has been copied, the mutex is
released and app_main can continue running, and thus, exit.

# How to use
Connect an LED to the ESP32 using a ~220 Ohm resistor, using GPIO pin 4.

After building and flashing the program, open up a serial monitor and type in an integer, this integer will be used as the delay
for the LED blink sequence.
