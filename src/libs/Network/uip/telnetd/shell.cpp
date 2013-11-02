/*
* Copyright (c) 2003, Adam Dunkels.
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
* 1. Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
* 2. Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the distribution.
* 3. The name of the author may not be used to endorse or promote
*    products derived from this software without specific prior
*    written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
* OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
* GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
* WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
* This file is part of the uIP TCP/IP stack.
*
* $Id: shell.c,v 1.1 2006/06/07 09:43:54 adam Exp $
*
*/

#include "stdlib.h"
#include "shell.h"
#include "uip.h"
#include <string.h>
#include "checksumm.h"
#include "utils.h"
#include "stdio.h"
#include "stdlib.h"

#include "deque"

struct ptentry {
    uint16_t command_cs;
    void (* pfunc)(char *str);
};

static std::deque<char*> *command_q= NULL;

#define SHELL_PROMPT "> "

/*---------------------------------------------------------------------------*/
static bool parse(register char *str, struct ptentry *t)
{
    struct ptentry *p;
    for (p = t; p->command_cs != 0; ++p) {
        if (get_checksum(str) == p->command_cs) {
            break;
        }
    }

    p->pfunc(str);

    return p->command_cs != 0;
}
/*---------------------------------------------------------------------------*/
static void
help(char *str)
{
    shell_output("Available commands: All others are passed on\n");
    shell_output("net         - show network info\n");
    shell_output("?           - show network help\n");
    shell_output("help        - show command help\n");
    shell_output("exit, quit  - exit shell\n");
}

/*---------------------------------------------------------------------------*/
static void connections(char *str)
{
    char istr[128];
    struct uip_conn* uip_connr;
    snprintf(istr, sizeof(istr), "Initial MSS: %d, MSS: %d\n", uip_initialmss(), uip_mss());
    shell_output(istr);
    shell_output("Current TCP connections: \n");
    for(uip_connr = &uip_conns[0]; uip_connr <= &uip_conns[UIP_CONNS - 1]; ++uip_connr) {
        if(uip_connr->tcpstateflags != UIP_CLOSED) {
            snprintf(istr, sizeof(istr), "%d - %d.%d.%d.%d\n", HTONS(uip_connr->lport),
                uip_ipaddr1(uip_connr->ripaddr), uip_ipaddr2(uip_connr->ripaddr),  uip_ipaddr3(uip_connr->ripaddr), uip_ipaddr4(uip_connr->ripaddr));
            shell_output(istr);
        }
    }
}

//#include "clock.h"
static void shell_test(char *str)
{
    printf("In Test\n");
    // struct timer t;
    // u16_t ticks=  CLOCK_SECOND*5;
    // timer_set(&t, ticks);
    // printf("Wait....\n");
    // while(!timer_expired(&t)) {

    // }
    // printf("Done\n");
/*
    const char *fn= "/sd/test6.txt";
    uint16_t *buf= (uint16_t *)malloc(200*2);
    int cnt= 0;
    FILE *fp;
    for(int i=0;i<10;i++) {
        fp= fopen(fn, i == 0 ? "w" : "a");
        if(fp == NULL) {
            printf("failed to open file\n");
            return;
        }
        for (int x = 0; x < 200; ++x) {
            buf[x]= x+cnt;
        }
        cnt+=200;
        int n= fwrite(buf, 2, 200, fp);
        printf("wrote %d, %d\n", i, n);
        fclose(fp);
    }

    fp= fopen(fn, "r");
    if(fp == NULL) {
        printf("failed to open file for read\n");
        return;
    }
    printf("Opened file %s for read\n", fn);
    do {
        int n= fread(buf, 2, 200, fp);
        if(n <= 0) break;
        for(int x=0;x<n;x++) {
            printf("%04X, ", buf[x]);
        }
    }while(1);
    fclose(fp);
    free(buf);
    */
}

/*---------------------------------------------------------------------------*/
static void
unknown(char *str)
{
    // its some other command, so queue it for mainloop to find
    if (strlen(str) > 0) {
        command_q->push_back(strdup(str));
    }
}
/*---------------------------------------------------------------------------*/
static struct ptentry parsetab[] = {
    {CHECKSUM("net"), connections},
    {CHECKSUM("exit"), shell_quit},
    {CHECKSUM("quit"), shell_quit},
    {CHECKSUM("test"), shell_test},
    {CHECKSUM("?"), help},

    /* Default action */
    {0, unknown}
};
/*---------------------------------------------------------------------------*/
void
shell_init(void)
{
}
/*---------------------------------------------------------------------------*/
void
shell_start()
{
    if(command_q != NULL){
        for (std::deque<char*>::iterator i = command_q->begin(); i != command_q->end(); ++i) {
            free(*i);
        }
        command_q->clear();
        delete command_q;
    }

    command_q= new std::deque<char*>();

    shell_output("Smoothie command shell\n> ");
}
void shell_stop()
{
    if(command_q != NULL){
        for (std::deque<char*>::iterator i = command_q->begin(); i != command_q->end(); ++i) {
            free(*i);
        }
        command_q->clear();
        delete command_q;
        command_q= NULL;
    }
}
int shell_queue_size()
{
    if(command_q == NULL || command_q->empty()) return 0;
    return command_q->size();
}
/*---------------------------------------------------------------------------*/
void
shell_input(char *cmd)
{
    if(parse(cmd, parsetab)) {
        shell_prompt(SHELL_PROMPT);
    }
}
/*---------------------------------------------------------------------------*/
const char *shell_get_command()
{
    if(command_q == NULL || command_q->empty()) return NULL;
    return command_q->front();
}

void shell_got_command()
{
    if(command_q != NULL && !command_q->empty()){
        free(command_q->front());
        command_q->pop_front();
    }
}

void shell_response(const char *resp)
{
    if(resp == NULL) {
        // only prompt when command is completed
        shell_prompt(SHELL_PROMPT);
    }else{
        shell_output(resp);
    }
}