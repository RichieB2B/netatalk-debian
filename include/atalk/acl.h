/*
   $Id: acl.h,v 1.1 2009/10/14 15:04:01 franklahm Exp $
   Copyright (c) 2009 Frank Lahm <franklahm@gmail.com>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
 
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
*/

#ifndef ATALK_ACL_H
#define ATALK_ACL_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#ifdef HAVE_NFSv4_ACLS
#include <sys/acl.h>
#endif  /* HAVE_NFSv4_ACLS */

/* Solaris NFSv4 ACL stuff */
#ifdef HAVE_NFSv4_ACLS
extern int get_nfsv4_acl(const char *name, ace_t **retAces);
extern int remove_acl(const char *name);
#endif /* HAVE_NFSv4_ACLS */


#endif  /* ATALK_ACL_H */
