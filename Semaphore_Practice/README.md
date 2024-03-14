# Problem

Use semaphores and mutexes to allow for multiple tasks to write to a buffer three times, then exit, and have other tasks read from this buffer until it is empty. Without using these, the semaphores that do the writing(producers) will exit, but since there is no way to signal to the readers(consumers),
They will constantly read from the buffer, even though there are no new writes. 

# Methodology

I first re-drew the diagram to get a better understanding of the structure and tasks at hand. (DrawIO)

![image](https://github.com/5ilentAlarm/ESP-32-RTOS-Examples/assets/143994622/e06be33a-8f85-420b-8450-9dad200ffca6)

I then used paint to figure out where I would need to increment(give) a semaphore, or decrement(take) one. 

![image](https://github.com/5ilentAlarm/ESP-32-RTOS-Examples/assets/143994622/7acddcc7-a7ed-49cb-9731-27bcf2893d29)

The drawing was crucial, as it helps understand that (1) Two semaphores are needed to signal, One to know if the buffer is filled(primarily for the producers) and one to know if the buffer is empty(for the Consumers). 
Once an element in the buffer is written to, the filled semaphore will increment and the empty semaphore will decrement. The reason why it is important to have two semaphores is that after a producer has done its writes, it will exit. 
This means that once all the writers have exited, there will be no use of the filled semaphore, it was only there to let the writers know if the buffer was full or not. The empty semaphore will signal to the readers that if it isn't empty, to increment the count, 
and print the value. Since all of the writers have exited, it is now on the readers to increment the count until it is at its max. Meaning that the writes have been destroyed, and the readers left alone :(


![image](https://github.com/5ilentAlarm/ESP-32-RTOS-Examples/assets/143994622/c23163f1-3575-402a-977a-233a5a55cf0f)
