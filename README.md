A little project I built to have a bit more security when charing unprotected lipo batteries.   
It takes in 6 (limitation by the Atmega 328P) LM35 sensors that are strapped to the batteries and watches the temperatures. If one (or more) reach over 40°C an alarm is triggered (hearable by a buzzer).
Ideally the device would emit a signal to the charger to stop but sadly my ISDT charger doesn't have such an input.  (Although you really shouldn't charge such batteries anyway)  
In the future there will be an updated design that can take in more sensors that are less susceptible to electrical noise. (and not made on a perfboard)

