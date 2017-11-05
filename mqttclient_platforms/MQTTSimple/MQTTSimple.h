/*-
 * Copyright (c) 2017, Jeff Kletsky
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *
 *     * Neither the name of the copyright holder nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * MQTTSimple -- Just the basics for IPv4 TCP
 * 
 * Copyright (c) 2017, Jeff Kletsky
 * All rights reserved.
 */

#ifndef _MQTTSIMPLE_H_
#define _MQTTSIMPLE_H_

#include "FreeRTOS.h"
#include <task.h>

#include <lwip/sockets.h>

#if !defined LWIP_SOCKET || !defined LWIP_DNS
#error  LWIP_SOCKET and LWIP_DNS required
#endif

/* #include <semphr.h> */ /* conditionally included below */

/*
 * By passing a pointer to a struct, the call sequence and signatures
 * can be independent of the mqttclient_platform selected.
 * Only the contents of struct MQTT_NetworkParams need change
 */

struct MQTT_NetworkParams
{
    char *hostname;
    char *port;  /* Yes, STRING; decimal port or name; see getaddrinfo() */
    int   timeout_ms;
};
    
/* 
 * Implement required Network struct and type, and required prototypes
 * as specified by MQTTClient.c -- Opaque to end users
 */

typedef struct Network
{
    int s;
    int (*mqttread)(struct Network *network,
                    unsigned char *read_buffer, int length,
                    int timeout_ms);
    int (*mqttwrite)(struct Network *network,
                     unsigned char *send_buffer, int length,
                     int timeout_ms);
    struct MQTT_NetworkParams *params;
} Network;

/* Initial value for network->s; must be negative */
#define NETWORK_S_INIT -2



/*
 * API for creating, managing and destorying a Network
 * (not specified by MQTTClient-C)
 */

/* Dynamic allocation not supported */
/* Network* MQTT_NetworkNew();  */ 

/*
 * Initialize the data structures for an MQTT network.
 * MQTTSimple does not support dynamic allocation,
 * so the Network should be static, or allocated by the end user
 * 
 * returns: 0 on success
 */
int
MQTT_NetworkInit(Network* network, struct MQTT_NetworkParams *params);
/* should probably return an error STRING */


/*
 * Take an MQTT Network that has been initialized or re-initialized,
 * perform hostname lookup, and try to connect the socket
 *
 * returns: 0 on succcess
 *         -1 on socket() or connect() errors (if POSIX compliant returns)
 *          one of the error codes listed in gai_strerror(); see netdb.h
 */
int
MQTT_NetworkConnect(Network *network);


/*
 * "Gracefully" disconnect network
 */
int
MQTT_NetworkDisconnect(Network *network);


/*
 * Destroy network and free resources
 */
int
MQTT_NetworkDestroy(Network *network);


/*
 * Timer functionality required by MQTTClient
 */

struct Timer
{
    TickType_t ticks_due;
};

typedef struct Timer Timer;

void
TimerInit(Timer *timer);

char
TimerIsExpired(Timer *timer);

void
TimerCountdownMS(Timer *timer, unsigned int ms);

void
TimerCountdown(Timer *timer, unsigned int sec);

int
TimerLeftMS(Timer *timer);


/* 
 * MQTTClient.c appears to be able to use a mutex lock 
 */

/* TODO: Re-evaluate if this needs to be a re-entrant mutex */

#ifdef MQTT_TASK

#include <semphr.h>

typedef SemaphoreHandle_t Mutex;

/* 
 * unless FreeRTOS was compiled with INCLUDE_vTaskSuspend
 * MutexLock() will wait "a very long time" and not "forever"
 * You've got bigger problems if it takes portMAX_DELAY
 */

/* May want to make these first-class functions so can printf/LED watch */

#define Mutex          SemaphoreHandle_t
#define MutexInit(M)   xSemaphoreCreateMutex(M)
#define MutexLock(M)   xSemaphoreTake(M, portMAX_DELAY)
#define MutexUnlock(M) xSemaphoreGive(M)

/* 
 * Catch-22 here; platform-specific header is included before
 * MQTTClient type is defined in MQTTClient.h
 *
 * Called by MQTTStartTask() from MQTTClient.c 
 * which is not called internally by paho-embed-mqtt3c as of v1.1.0
 */

void
ThreadStart(*Thread thread, (*MQTTRun)(void), void *client);

#endif /* MQTT_TASK */



#endif /* _MQTTSIMPLE_H_ */
