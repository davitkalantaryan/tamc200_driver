# tamc200_driver  
This driver is devoted to handling TAMC200 devices. 
Currently PITZ uses driver [this](https://github.com/davitkalantaryan/drivers/blob/master/sources/timer_drv_main.c). 
The problem with this driver is that this driver uses exported functions of the driver 
[mtcagen](https://github.com/davitkalantaryan/mtcagen) and this driver is currently out of support.  
The hierarchy of the old driver was following tamc200=>mtcagen=>upciedev. Now we are going to get rid of 
intermediate driver.  
By getting rid of dependency from unsupported driver we will be able to maintain and support `tamc200` driver 
with regular `DESY` supported drivers.
