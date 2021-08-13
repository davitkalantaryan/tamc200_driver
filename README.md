# tamc200_driver  
This driver is devoted to handling TAMC200 devices. 
Currently PITZ uses driver [this](https://github.com/davitkalantaryan/drivers/blob/master/sources/timer_drv_main.c). 
The problem with this driver is that this driver uses exported functions of the driver 
[mtcagen](https://github.com/davitkalantaryan/mtcagen) and this driver is currently out of support.  
The hierarchy of the old driver was following tamc200=>mtcagen=>upciedev. Now we are going to get rid of 
intermediate driver.  
By getting rid of dependency from unsupported driver we will be able to maintain and support `tamc200` driver 
with regular `DESY` supported drivers.  
  
## Steps to prepare dkms pakage  
see: https://docs.01.org/clearlinux/latest/guides/kernel/kernel-modules-dkms.html  
  1. copy etire repository into '/usr/src' directory. The directory should ve 'tamc200-version'.   
     Current vesion is 2.0.1 . So the directory path, where content of the repository   
     should be is following:  '/usr/src/tamc200-2.0.1'  
  2. $sudo dkms add -m tamc200  # to add tamc200 to dkms tree  
  3. $sudo dkms build -m tamc200 -v 2.0.1     # to build driver using dkms  
  4. $sudo dkms mkdeb -m tamc200 -v 2.0.1     #  to prepare debian package from compile driver  
  5. Debian file one ca find here: '/var/lib/dkms/tamc200/2.0.1'  
  
