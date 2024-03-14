# What is this
An example of using a software timer to keep track of RX activity. If something was receieved, turn on an LED, turn on the timer, if nothing is seen turn LED off again.

# How....
Using the freeRTOS timer API, it's pretty easy to keep track of what each software timer is doing, and there isn't as much overhead as a task/delay. Each time something is received in the RX buffer, the task will
start the software timer, in which after a certain amount of ticks, the timers callback function will be called and the LED will turn off, but if there is a constant input then the timer will be constantly reset,
this mimics an LCD dimming.

![image](https://github.com/5ilentAlarm/ESP-32-RTOS-Examples/assets/143994622/21677648-1376-48c2-8627-74905d9d75af)

Simple state machine depicting how the program should flow.
