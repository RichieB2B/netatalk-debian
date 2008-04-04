/*
 * $Id: comm.h,v 1.1.4.4 2004/01/09 21:05:50 lenneis Exp $
 *
 * Copyright (C) Joerg Lenneis 2003
 * All Rights Reserved.  See COPYING.
 */

#ifndef CNID_DBD_COMM_H
#define CNID_DBD_COMM_H 1


#include <atalk/cnid_dbd_private.h>


extern int      comm_init  __P((struct db_param *, int, int));
extern int      comm_rcv  __P((struct cnid_dbd_rqst *));
extern int      comm_snd  __P((struct cnid_dbd_rply *));
extern int      comm_nbe  __P((void));

#endif /* CNID_DBD_COMM_H */

