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

/* 
 * The address-resolution and socket-opening code in MQTT_NetworkConnect()
 * is based on the FreeBSD getaddrinfo(3) man page
 * 
 * Copyright (C) 2004  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 2000, 2001  Internet Software Consortium.
 *  
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 * 
 * and, as a derivative work, is additionally subject to terms of that license
 */

#include "MQTTSimple.h"

#include <errno.h>
#include <string.h>

#include <lwip/inet.h>
#include <lwip/netdb.h>

/*
 * Utility functions and macros
 */


#define LOG_DEBUGT(...) printf("%0.3f %s(): ",((double)xTaskGetTickCount())/100, __func__); printf(__VA_ARGS__)



#define ticks2ms(t)     ((t) * portTICK_PERIOD_MS)
#define ms2ticks(ms)    ((ms) / portTICK_PERIOD_MS)  /* floor; close enough */

/*
 * IPv4/IPv6 agnostic conversion of sockaddr to IP-address string
 */
static const char*
sockaddr2p(struct sockaddr *sa, char *buf, size_t buflen)
{
    switch (sa->sa_family)
    {
    case AF_INET:
        return (inet_ntop(AF_INET, &(((struct sockaddr_in *)sa)->sin_addr),
                          buf, buflen));
        break;
#if LWIP_IPV6
    case AF_INET6:
        return (inet_ntop6(AF_INET6, &(((struct sockaddr_in6 *)sa)->sin6_addr),
                          buf, buflen));
        break;
#endif
    default:
        return (NULL);

    }
}


/* Forward declarations to allow API functions to lead in this file */

static int
mqttread(Network *network, unsigned char *read_buffer, int length,
         int timeout_ms);

static int
mqttwrite(Network *network, unsigned char *send_buffer, int length,
          int timeout_ms);


/*
 * API for creating, managing and destorying a Network
 * (not specified by MQTTClient-C)
 */

/* Dynamic allocation not supported at this time */
/* Network* MQTT_NetworkNew();  */ 

int
MQTT_NetworkInit(Network *network, struct MQTT_NetworkParams *params)
{
    network->s = NETWORK_S_INIT;    /* Will be created after getaddrinfo() */
    network->mqttread = mqttread;
    network->mqttwrite = mqttwrite;
    network->params = params;

    return (0);
}


int
MQTT_NetworkConnect(Network *network)
{
    int rv;
    int retval = -1;  /* quell compiler warning */

#if LWIP_IPV6
    char addrstr[INET6_ADDRSTRLEN];
#else
    char addrstr[INET_ADDRSTRLEN];
#endif

    struct addrinfo *res0;  /* returned by getaddrinfo() */
    struct addrinfo *res;   /* pointer within *res0 chain */

    const struct addrinfo hints = {
        .ai_flags = AI_ADDRCONFIG,
        .ai_family = AF_UNSPEC,
        .ai_socktype = SOCK_STREAM
    };

    rv = lwip_getaddrinfo(network->params->hostname,
                          network->params->port,
                          &hints, &res0);
    if (rv)
        goto ERR_GETADDRINFO;

    network->s = NETWORK_S_INIT;
    for (res = res0; res; res = res->ai_next) {

        sockaddr2p(res->ai_addr, addrstr, sizeof(addrstr));
        LOG_DEBUGT("Trying %s\n",addrstr);

        network->s = lwip_socket(res->ai_family, res->ai_socktype,
                                 res->ai_protocol);
        if (network->s < 0)
            continue;

        rv = lwip_connect(network->s, res->ai_addr, res->ai_addrlen);
        if (rv < 0) {
            lwip_close(network->s);
            network->s = NETWORK_S_INIT;
            continue;
        }

        sockaddr2p(res->ai_addr, addrstr, sizeof(addrstr));
        LOG_DEBUGT("Connected to %s\n",addrstr);

        retval = 0;
        goto CLEANUP;  /* okay we got one */
    }
    if (network->s < 0)
        retval = -1;
    goto CLEANUP;


 ERR_GETADDRINFO:
    retval = rv;
    goto CLEANUP;

 CLEANUP:
    lwip_freeaddrinfo(res0);
    return (retval);

}

int
MQTT_NetworkDisconnect(Network *network)
{
    return (lwip_close(network->s));
}

int
MQTT_NetworkDestroy(Network *network)
{
    return (lwip_close(network->s));  /* close() frees the socket resources */
}

    

/*
 * Network read and write functions, as required by MQTTClient.c
 */

static int
mqttread(Network *network, unsigned char *read_buffer, int length,
         int timeout_ms)
{
    int rv;
    int rcvd;
    int remaining_ms;

    Timer tmr;
    struct timeval this_timeout;

    TimerInit(&tmr);

    rcvd = 0;
    
    TimerCountdownMS(&tmr, timeout_ms);
    
    do {
        
        remaining_ms = TimerLeftMS(&tmr);

        this_timeout.tv_sec =   remaining_ms / 1000;
        this_timeout.tv_usec = (remaining_ms % 1000) * 1000;

        rv = lwip_setsockopt(network->s,
                             SOL_SOCKET, SO_RCVTIMEO,
                             &this_timeout, sizeof(this_timeout));
        if (rv != 0)
            goto ERR_SETSOCKOPT;

        /* TODO: de-lwip_ */
        rv = lwip_recv(network->s,
                       read_buffer + rcvd, length - rcvd,
                       0);

        if (rv > 0) {
            rcvd += rv;
#if EWOULDBLOCK == EAGAIN
        } else if ((rv < 0) && !(errno == EAGAIN)) {
#else
        } else if ((rv < 0) && !((errno == EAGAIN) || errno == EWOULDBLOCK)) {
#endif
            goto ERR_RECV;
        }

    } while ((rcvd < length) && !TimerIsExpired(&tmr));

    return rcvd;

 ERR_SETSOCKOPT:
    LOG_DEBUGT("setsockopt(): %s\n", strerror(errno));
    goto CLEANUP;
    
 ERR_RECV:
    LOG_DEBUGT("recv(): %s\n", strerror(errno));
    goto CLEANUP;

 CLEANUP:
    return rv;

}

static int
mqttwrite(Network *network, unsigned char *send_buffer, int length,
          int timeout_ms)
{
    int rv;
    int sent;
    int remaining_ms;

    Timer tmr;
    struct timeval this_timeout;

    TimerInit(&tmr);

    sent = 0;
    
    TimerCountdownMS(&tmr, timeout_ms);
    remaining_ms = TimerLeftMS(&tmr);

    do {
        
        remaining_ms = TimerLeftMS(&tmr);

        this_timeout.tv_sec =   remaining_ms / 1000;
        this_timeout.tv_usec = (remaining_ms % 1000) * 1000;

        /* TODO: de-lwip_ */
        rv = lwip_setsockopt(network->s,
                             SOL_SOCKET, SO_SNDTIMEO,
                             &this_timeout, sizeof(this_timeout));
        if (rv != 0)
            goto ERR_SETSOCKOPT;
        
        rv = lwip_send(network->s,
                       send_buffer + sent, length - sent,
                       0);

        if (rv > 0) {
            sent += rv;
        } else if (rv < 0) {
            /* Assumes socket should be open and not block on send */
            goto ERR_SEND;
        }

    } while ((sent < length) && !TimerIsExpired(&tmr));

    return sent;

 ERR_SETSOCKOPT:
    LOG_DEBUGT("setsockopt(): %s\n", strerror(errno));
    goto CLEANUP;
    
 ERR_SEND:
    LOG_DEBUGT("send(): %s\n", strerror(errno));
    goto CLEANUP;

 CLEANUP:
    return rv;

}

/*
 * Timer functions, as required by MQTTClient.c
 * 
 * No callback is required, so just use tick count
 */

void
TimerInit(Timer* timer)
{
    timer->ticks_due = xTaskGetTickCount() - 1;
}

char
TimerIsExpired(Timer* timer)
{
    return TimerLeftMS(timer) == 0;
}

void
TimerCountdownMS(Timer* timer, unsigned int ms)
{
    timer->ticks_due = xTaskGetTickCount() + ms2ticks(ms);

}

void
TimerCountdown(Timer* timer, unsigned int sec)
{
    TimerCountdownMS(timer,sec * 1000);
}

int
TimerLeftMS(Timer* timer)
{
    /*
     * Assume that the difference between two unsigned int values
     * with a signed int result is handled properly by the compiler,
     * and that TickType_t is an uint32_t or equivalent. Otherwise
     * should "do the dance" of explicitly handling counter wrap.
     */

    int32_t dt = ticks2ms(timer->ticks_due - xTaskGetTickCount());
    if (dt < 0)
        dt = 0;

    return dt;
}


#ifdef MQTT_TASK
/*
 * Mutex functions, if enabled by #define MQTT_TASK
 * are implemented as macros in MQTTSimple.h
 */


/*
 * ThreadStart needs to be implemented if #define MQTT_TASK
 * as it wil be called by int MQTTStartTask() in MQTTClient.c
 * 
 * Whle "nothing" calls MQTTStartTask() within MQTTClient.c,
 * link errors are likely to occur if not defined
 */

#endif /* MQTT_TASK */
