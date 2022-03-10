#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define CHAR_LEN_16         16          /* string length:16  */
#define CHAR_LEN_64         64          /* string length:64  */
#define CHAR_LEN_1024       1024        /* string length:16  */

#define MARK_COLON          ":"         /* colon */
#define MARK_AT             "@"         /* @ */
#define MARK_SPACE          " "         /* space */
#define MARK_LINE_BREAK     '\\'        /* line break */

#define CMD_DISCONNECT      "BYE"       /* disconnect to the server */
#define CMD_QUIT            "QUIT"      /* quit the client */
#define CMD_BROADCAST       "all"       /* broadcase to all clients */
#define CMD_OPTION_NAME     "--name"    /* command option: mc's name */

#endif /* COMMON_H */
