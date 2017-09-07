#ifndef _DIRENT_H
#define _DIRENT_H

#define NAME_MAX 255

struct dirent {
	unsigned long d_ino;                 /* inode number */
    unsigned long d_off;                /* offset to this dirent */
    unsigned short d_reclen;    /* length of this d_name */
    char d_name [];   /* filename (null-terminated) */
};

typedef struct DIR DIR;

DIR *opendir(const char *name);
struct dirent *readdir(DIR *dirp);
int readdir_fd(unsigned int fd, struct dirent *dirp, unsigned int count);
int getdents(unsigned int fd, struct dirent *dirp, unsigned int count);
int closedir(DIR *dirp);

#endif
