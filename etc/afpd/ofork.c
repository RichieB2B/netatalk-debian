/*
 * $Id: ofork.c,v 1.32 2010-03-12 15:16:49 franklahm Exp $
 *
 * Copyright (c) 1996 Regents of The University of Michigan.
 * All Rights Reserved.  See COPYRIGHT.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <string.h>
#include <sys/stat.h> /* works around a bug */
#include <sys/param.h>
#include <atalk/logger.h>
#include <errno.h>

#include <atalk/util.h>

#include "globals.h"
#include "volume.h"
#include "directory.h"
#include "fork.h"

/* we need to have a hashed list of oforks (by dev inode). just hash
 * by first letter. */
#define OFORK_HASHSIZE  64
static struct ofork     *ofork_table[OFORK_HASHSIZE];

static struct ofork	**oforks = NULL;
static int	        nforks = 0;
static u_short		lastrefnum = 0;


/* OR some of each character for the hash*/
static unsigned long hashfn(const struct file_key *key)
{
#if 0
    unsigned long i = 0;
    while (*name) {
        i = ((i << 4) | (8*sizeof(i) - 4)) ^ *name++;
    }
#endif    
    return key->inode & (OFORK_HASHSIZE - 1);
}

static void of_hash(struct ofork *of)
{
    struct ofork **table;

    table = &ofork_table[hashfn(&of->key)];
    if ((of->next = *table) != NULL)
        (*table)->prevp = &of->next;
    *table = of;
    of->prevp = table;
}

static void of_unhash(struct ofork *of)
{
    if (of->prevp) {
        if (of->next)
            of->next->prevp = of->prevp;
        *(of->prevp) = of->next;
    }
}

#ifdef DEBUG1
void of_pforkdesc( FILE *f)
{
    int	ofrefnum;

    if (!oforks)
        return;

    for ( ofrefnum = 0; ofrefnum < nforks; ofrefnum++ ) {
        if ( oforks[ ofrefnum ] != NULL ) {
            fprintf( f, "%hu <%s>\n", ofrefnum, of_name(oforks[ ofrefnum ]));
        }
    }
}
#endif

int of_flush(const struct vol *vol)
{
    int	refnum;

    if (!oforks)
        return 0;

    for ( refnum = 0; refnum < nforks; refnum++ ) {
        if (oforks[ refnum ] != NULL && (oforks[refnum]->of_vol == vol) &&
                flushfork( oforks[ refnum ] ) < 0 ) {
            LOG(log_error, logtype_afpd, "of_flush: %s", strerror(errno) );
        }
    }
    return( 0 );
}

int of_rename(const struct vol *vol,
              struct ofork *s_of,
              struct dir *olddir, const char *oldpath _U_,
              struct dir *newdir, const char *newpath)
{
    struct ofork *of, *next, *d_ofork;
    int done = 0;

    if (!s_of)
        return AFP_OK;
        
    next = ofork_table[hashfn(&s_of->key)];
    while ((of = next)) {
        next = next->next; /* so we can unhash and still be all right. */

        if (vol == of->of_vol && olddir == of->of_dir &&
	         s_of->key.dev == of->key.dev && 
	         s_of->key.inode == of->key.inode ) {
	    if (!done) {
	        strlcpy( of_name(of), newpath, of->of_ad->ad_m_namelen);
	        done = 1;
	    }
            if (newdir != olddir) {
                of->of_d_prev->of_d_next = of->of_d_next;
                of->of_d_next->of_d_prev = of->of_d_prev;
                if (of->of_dir->d_ofork == of) {
                    of->of_dir->d_ofork = (of == of->of_d_next) ? NULL : of->of_d_next;
                }	    
                of->of_dir = newdir;
                if (!(d_ofork = newdir->d_ofork)) {
                    newdir->d_ofork = of;
                    of->of_d_next = of->of_d_prev = of;
                } else {
                    of->of_d_next = d_ofork;
                    of->of_d_prev = d_ofork->of_d_prev;
                    of->of_d_prev->of_d_next = of;
                    d_ofork->of_d_prev = of;
                }
            }
        }
    }

    return AFP_OK;
}

#define min(a,b)	((a)<(b)?(a):(b))

struct ofork *
of_alloc(struct vol *vol,
    struct dir	   *dir,
    char	   *path,
    u_int16_t	   *ofrefnum,
    const int      eid,
    struct adouble *ad,
    struct stat    *st)
{
    struct ofork        *of, *d_ofork;
    u_int16_t		refnum, of_refnum;

    int			i;

    if (!oforks) {
        nforks = getdtablesize() - 10;
	/* protect against insane ulimit -n */
        nforks = min(nforks, 0xffff);
        oforks = (struct ofork **) calloc(nforks, sizeof(struct ofork *));
        if (!oforks)
            return NULL;
    }

    for ( refnum = ++lastrefnum, i = 0; i < nforks; i++, refnum++ ) {
        /* cf AFP3.0.pdf, File fork page 40 */
        if (!refnum)
            refnum++;
        if ( oforks[ refnum % nforks ] == NULL ) {
            break;
        }
    }
    /* grr, Apple and their 'uniquely identifies'
          the next line is a protection against 
          of_alloc()
             refnum % nforks = 3 
             lastrefnum = 3
             oforks[3] != NULL 
             refnum = 4
             oforks[4] == NULL
             return 4
         
          close(oforks[4])
      
          of_alloc()
             refnum % nforks = 4
             ...
             return 4
         same if lastrefnum++ rather than ++lastrefnum. 
    */
    lastrefnum = refnum;
    if ( i == nforks ) {
        LOG(log_error, logtype_afpd, "of_alloc: maximum number of forks exceeded.");
        return( NULL );
    }

    of_refnum = refnum % nforks;
    if (( oforks[ of_refnum ] =
                (struct ofork *)malloc( sizeof( struct ofork ))) == NULL ) {
        LOG(log_error, logtype_afpd, "of_alloc: malloc: %s", strerror(errno) );
        return NULL;
    }
    of = oforks[of_refnum];

    /* see if we need to allocate space for the adouble struct */
    if (!ad) {
        ad = malloc( sizeof( struct adouble ) );
        if (!ad) {
            LOG(log_error, logtype_afpd, "of_alloc: malloc: %s", strerror(errno) );
            free(of);
            oforks[ of_refnum ] = NULL;
            return NULL;
        }

        /* initialize to zero. This is important to ensure that
           ad_open really does reinitialize the structure. */
        ad_init(ad, vol->v_adouble, vol->v_ad_options);

        ad->ad_m_namelen = 255 +1;
        /* here's the deal: we allocate enough for the standard mac file length.
         * in the future, we'll reallocate in fairly large jumps in case
         * of long unicode names */
        if (( ad->ad_m_name =(char *)malloc( ad->ad_m_namelen )) == NULL ) {
            LOG(log_error, logtype_afpd, "of_alloc: malloc: %s", strerror(errno) );
            free(ad);
            free(of);
            oforks[ of_refnum ] = NULL;
            return NULL;
        }
        strlcpy( ad->ad_m_name, path, ad->ad_m_namelen);
    } else {
        /* Increase the refcount on this struct adouble. This is
           decremented again in oforc_dealloc. */
        ad->ad_refcount++;
    }

    of->of_ad = ad;
    of->of_vol = vol;
    of->of_dir = dir;

    if (!(d_ofork = dir->d_ofork)) {
        dir->d_ofork = of;
        of->of_d_next = of->of_d_prev = of;
    } else {
        of->of_d_next = d_ofork;
        of->of_d_prev = d_ofork->of_d_prev;
        d_ofork->of_d_prev->of_d_next = of;
        d_ofork->of_d_prev = of;
    }

    *ofrefnum = refnum;
    of->of_refnum = refnum;
    of->key.dev = st->st_dev;
    of->key.inode = st->st_ino;
    if (eid == ADEID_DFORK)
        of->of_flags = AFPFORK_DATA;
    else
        of->of_flags = AFPFORK_RSRC;

    of_hash(of);
    return( of );
}

struct ofork *of_find(const u_int16_t ofrefnum )
{
    if (!oforks || !nforks)
        return NULL;

    return( oforks[ ofrefnum % nforks ] );
}

/* -------------------------- */
int of_stat  (struct path *path)
{
int ret;
    path->st_errno = 0;
    path->st_valid = 1;
    if ((ret = lstat(path->u_name, &path->st)) < 0)
    	path->st_errno = errno;
   return ret;
}

#ifdef HAVE_RENAMEAT
int of_fstatat(int dirfd, struct path *path)
{
    int ret;

    path->st_errno = 0;
    path->st_valid = 1;

    if ((ret = fstatat(dirfd, path->u_name, &path->st, AT_SYMLINK_NOFOLLOW)) < 0)
    	path->st_errno = errno;

   return ret;
}
#endif /* HAVE_RENAMEAT */

/* -------------------------- 
   stat the current directory.
   stat(".") works even if "." is deleted thus
   we have to stat ../name because we want to know if it's there
*/
int of_statdir  (struct vol *vol, struct path *path)
{
static char pathname[ MAXPATHLEN + 1] = "../";
int ret;

    if (*path->m_name) {
        /* not curdir */
        return of_stat (path);
    }
    path->st_errno = 0;
    path->st_valid = 1;
    /* FIXME, what about: we don't have r-x perm anymore ? */
    strlcpy(pathname +3, path->d_dir->d_u_name, sizeof (pathname) -3);

    if (!(ret = lstat(pathname, &path->st)))
        return 0;
        
    path->st_errno = errno;
    /* hmm, can't stat curdir anymore */
    if (errno == EACCES && curdir->d_parent ) {
       if (movecwd(vol, curdir->d_parent)) 
           return -1;
       path->st_errno = 0;
       if ((ret = lstat(path->d_dir->d_u_name, &path->st)) < 0) 
           path->st_errno = errno;
    }
    return ret;
}

/* -------------------------- */
struct ofork *of_findname(struct path *path)
{
    struct ofork *of;
    struct file_key key;
    
    if (!path->st_valid) {
       of_stat(path);
    }
    	
    if (path->st_errno)
        return NULL;

    key.dev = path->st.st_dev;
    key.inode = path->st.st_ino;

    for (of = ofork_table[hashfn(&key)]; of; of = of->next) {
        if (key.dev == of->key.dev && key.inode == of->key.inode ) {
            return of;
        }
    }

    return NULL;
}

/*!
 * @brief Search for open fork by dirfd/name
 *
 * Function call of_fstatat with dirfd and path and uses dev and ino
 * to search the open fork table.
 *
 * @param dirfd     (r) directory fd
 * @param path      (rw) pointer to struct path
 */
#ifdef HAVE_RENAMEAT
struct ofork *of_findnameat(int dirfd, struct path *path)
{
    struct ofork *of;
    struct file_key key;
    
    if ( ! path->st_valid) {
        of_fstatat(dirfd, path);
    }
    	
    if (path->st_errno)
        return NULL;

    key.dev = path->st.st_dev;
    key.inode = path->st.st_ino;

    for (of = ofork_table[hashfn(&key)]; of; of = of->next) {
        if (key.dev == of->key.dev && key.inode == of->key.inode ) {
            return of;
        }
    }

    return NULL;
}
#endif

void of_dealloc( struct ofork *of)
{
    if (!oforks)
        return;

    of_unhash(of);

    /* detach ofork */
    of->of_d_prev->of_d_next = of->of_d_next;
    of->of_d_next->of_d_prev = of->of_d_prev;
    if (of->of_dir->d_ofork == of) {
        of->of_dir->d_ofork = (of == of->of_d_next) ? NULL : of->of_d_next;
    }

    oforks[ of->of_refnum % nforks ] = NULL;

    /* decrease refcount */
    of->of_ad->ad_refcount--;

    if ( of->of_ad->ad_refcount <= 0) {
        free( of->of_ad->ad_m_name );
        free( of->of_ad);
    } else {/* someone's still using it. just free this user's locks */
        ad_unlock(of->of_ad, of->of_refnum);
    }

    free( of );
}

/* --------------------------- */
int of_closefork(struct ofork *ofork)
{
    struct timeval      tv;
    int			adflags, doflush = 0;
    int                 ret;

    adflags = 0;
    if ((ofork->of_flags & AFPFORK_DATA) && (ad_data_fileno( ofork->of_ad ) != -1)) {
            adflags |= ADFLAGS_DF;
    }
    if ( (ofork->of_flags & AFPFORK_OPEN) && ad_reso_fileno( ofork->of_ad ) != -1 ) {
        adflags |= ADFLAGS_HF;
        /*
         * Only set the rfork's length if we're closing the rfork.
         */
        if ((ofork->of_flags & AFPFORK_RSRC)) {
            ad_refresh( ofork->of_ad );
            if ((ofork->of_flags & AFPFORK_DIRTY) && !gettimeofday(&tv, NULL)) {
                ad_setdate(ofork->of_ad, AD_DATE_MODIFY | AD_DATE_UNIX,tv.tv_sec);
            	doflush++;
            }
            if ( doflush ) {
                 ad_flush( ofork->of_ad );
            }
        }
    }
    ret = 0;
    if ( ad_close( ofork->of_ad, adflags ) < 0 ) {
        ret = -1;
    }
 
    of_dealloc( ofork );
    return ret;
}

/* ----------------------

*/
struct adouble *of_ad(const struct vol *vol, struct path *path, struct adouble *ad)
{
    struct ofork        *of;
    struct adouble      *adp;

    if ((of = of_findname(path))) {
        adp = of->of_ad;
    } else {
        ad_init(ad, vol->v_adouble, vol->v_ad_options);
        adp = ad;
    }
    return adp;
}

/* ---------------------- 
   close all forks for a volume
*/
void of_closevol(const struct vol *vol)
{
    int	refnum;

    if (!oforks)
        return;

    for ( refnum = 0; refnum < nforks; refnum++ ) {
        if (oforks[ refnum ] != NULL && oforks[refnum]->of_vol == vol) {
            if (of_closefork( oforks[ refnum ]) < 0 ) {
                LOG(log_error, logtype_afpd, "of_closevol: %s", strerror(errno) );
            }
        }
    }
    return;
}

