/**
 *  @file tamc200_io.h
 *
 *  @brief PCI Express tamc200 device driver
 *
 *  @author Davit Kalantaryan, Ludwig Petrosyan
**/

/**
 *Copyright 2015-  DESY (Deutsches Elektronen-Synchrotron, www.desy.de)
 *
 *This file is part of TAMC200 driver.
 *
 *TAMC200 is free software: you can redistribute it and/or modify
 *it under the terms of the GNU General Public License as published by
 *the Free Software Foundation, either version 3 of the License, or
 *(at your option) any later version.
 *
 *TAMC200 is distributed in the hope that it will be useful,
 *but WITHOUT ANY WARRANTY; without even the implied warranty of
 *MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *GNU General Public License for more details.
 *
 *You should have received a copy of the GNU General Public License
 *along with TAMC200.  If not, see <http://www.gnu.org/licenses/>.
 **/

/*
 *	File: tamc200_io.h
 *
 *	Created on: Sep 15, 2015
 *  Created by: Davit Kalantaryan
 *	Authors (maintainers):
 *      Davit Kalantaryan (davit.kalantaryan@desy.de)
 *      Ludwig Petrosyan  (ludwig.petrosyan@desy.de)
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

typedef struct SWaitInterruptTimeout
{
    u_int32_t     miliseconds;      //
    u_int32_t     carrierNumber;	// 0-2
}SWaitInterruptTimeout;


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
#define IP_TIMER_ACTIVATE_INTERRUPT						_IOW(TAMC200_IOC,32, u_int32_t)
//#define IP_TIMER_WAIT_FOR_EVENT_INF                     _IO(TAMC200_IOC,33)
#define IP_TIMER_WAIT_FOR_EVENT_INF                     _IOW(TAMC200_IOC,33,u_int32_t)
#define IP_TIMER_WAIT_FOR_EVENT_TIMEOUT                 _IOW(TAMC200_IOC,34, SWaitInterruptTimeout)

#endif  // #ifndef TAMC200_IO_H
