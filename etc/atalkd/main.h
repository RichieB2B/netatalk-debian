/*
 * $Id: main.h,v 1.3.14.1 2004/01/15 06:34:15 bfernhomberg Exp $
 */

#ifndef ATALKD_MAIN_H
#define ATALKD_MAIN_H

#include <sys/cdefs.h>
#include "config.h"

int ifconfig __P(( const char *, unsigned long, struct sockaddr_at * ));
void setaddr __P(( struct interface *, u_int8_t, u_int16_t,
        u_int8_t, u_int16_t, u_int16_t ));
void bootaddr __P(( struct interface * ));
void dumpconfig __P(( struct interface * ));

#ifdef linux
int ifsetallmulti __P(( const char *, int));
#endif

#endif /* ATALKD_MAIN_H */
