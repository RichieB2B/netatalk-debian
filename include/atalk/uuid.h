/*
   $Id: uuid.h,v 1.1 2009-02-02 11:55:01 franklahm Exp $
   Copyright (c) 2008,2009 Frank Lahm <franklahm@gmail.com>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
 
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
 */

#ifndef AFP_UUID_H
#define AFP_UUID_H

#define UUID_BINSIZE 16
#define UUID_STRINGSIZE 36

typedef char *uuidp_t;
typedef char uuid_t[UUID_BINSIZE];

typedef enum {UUID_USER = 1, UUID_GROUP} uuidtype_t;
extern char *uuidtype[];

/* afp_options.c needs these. defined in libatalk/ldap.c */
extern char *ldap_host;
extern int  ldap_auth_method;
extern char *ldap_auth_dn;
extern char *ldap_auth_pw;
extern char *ldap_userbase;
extern char *ldap_groupbase;
extern char *ldap_uuid_attr;
extern char *ldap_name_attr;
extern char *ldap_group_attr;
extern char *ldap_uid_attr;

/******************************************************** 
 * Interface
 ********************************************************/

/*
 *   name: give me his name
 *   type: and type (UUID_USER or UUID_GROUP)
 *   uuid: and I'll try to return you his uuid
 * returns 0 on success !=0 on errror
 */  
extern int getuuidfromname( const char *name, uuidtype_t type, uuidp_t uuid);

/* 
 *   uuidp: give me a pointer to a uuid
 *   name: and I'll allocate a buf with his name and store a pointer to buf
 *   type: returns USER or GROUP
 * return 0 on success !=0 on errror
 */
extern int getnamefromuuid( uuidp_t uuidp, char **name, uuidtype_t *type);

/* 
 * convert 16 byte binary uuid to neat ascii represantation including dashes
 * string is allocated and pointer returned. caller must freee.
 */
extern int uuid_bin2string( uuidp_t uuidp, char **uuidstring);


/* 
 * convert ascii string that can include dashes to binary uuid.
 * caller must provide a buffer.
 */
extern void uuid_string2bin( const char *uuidstring, uuidp_t uuid);

#endif /* AFP_UUID_H */
