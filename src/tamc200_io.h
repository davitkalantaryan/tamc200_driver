/*
 *	File: tamc200_io.h
 *
 *	Created on: Sep 15, 2015
 *	Author: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *
 */
#ifndef TAMC200_IO_H
#define TAMC200_IO_H

#include <linux/types.h>
#include <linux/ioctl.h> /* needed for the _IOW etc stuff used later */
#include <pciedev_io.h>


enum EIpCarrierType{IpCarrierNone,IpCarrierDelayGate};

typedef struct STimeTelegram
{
    u_int32_t	gen_event;
    u_int32_t	seconds;	// starting from epoch
    u_int32_t	useconds;	// microseconds starting from epoch
}STimeTelegram;


#define	TAMC200_NR_CARRIERS								3
#define	IP_TIMER_BUFFER_HEADER_SIZE						16
#define	IP_TIMER_RING_BUFFERS_COUNT						4
#define	IP_TIMER_ONE_BUFFER_SIZE						16
#define	IP_TIMER_WHOLE_BUFFER_SIZE						(IP_TIMER_BUFFER_HEADER_SIZE + \
                                                        (IP_TIMER_RING_BUFFERS_COUNT*IP_TIMER_ONE_BUFFER_SIZE))

#define	_OFFSET_TO_SPECIFIC_BUFFER_(_a_buffer_)			(IP_TIMER_BUFFER_HEADER_SIZE + \
                                                        (_a_buffer_)*IP_TIMER_ONE_BUFFER_SIZE)
#define	OFFSET_TO_CURRENT_BUFFER(_a_mem_)				_OFFSET_TO_SPECIFIC_BUFFER_(*((int*)(_a_mem_)))
#define	POINTER_TO_CURRENT_BUFFER(_a_mem_)				((char*)_a_mem_ + _OFFSET_TO_SPECIFIC_BUFFER_(*((int*)(_a_mem_))))


#define	TAMC200_IOC										't'

/*////////////////////////////////////////////////////////////////////////*/
//#define IP_TIMER_ACTIVATE_INTERRUPT						_IOWR(TAMC200_IOC,32, int)
#define IP_TIMER_ACTIVATE_INTERRUPT						_IOW(TAMC200_IOC,32, int)
#define IP_TIMER_WAIT_FOR_EVENT_INF                     _IO(TAMC200_IOC,33)
#define IP_TIMER_WAIT_FOR_EVENT_TIMEOUT                 _IOW(TAMC200_IOC,34, int)

#endif  // #ifndef TAMC200_IO_H
