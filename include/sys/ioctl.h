#ifndef _IOCTL_H
#define _IOCTL_H

int ioctl(int fd, uint64_t op, uint64_t arg);
#define TIOCSPGRP 100

#endif