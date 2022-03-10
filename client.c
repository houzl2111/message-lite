#include "common.h"
#include "time.h"

/**************************************************Define**************************************************/
#define CMD_CONNECT_PREFIX  "HI "       /* prifix of mc connection command */
#define CMD_OPTION_NAME_IDX 1           /* command option index: mc's name */
#define CMD_NAME_IDX        2           /* command index: mc's name */
#define MSG_INPUT_PROMPT    "%s:%d|%s > "/* mc input prompt */

#define SENTMSG_ECHO_FORMAT_BROADCAST   "%s broadcast from %s: %s\n"    /* the format of sent message output for broadcase  */
#define SENTMSG_FORMAT_BROADCAST        "%s %s broadcast from %s: %s"   /* the format of sent message for broadcase         */
#define SENTMSG_ECHO_FORMAT_PERSONAL    "%s %s: %s\n"                   /* the format of sent message output for personal   */
#define SENTMSG_FORMAT_PERSONAL         "%s %s %s: %s"                  /* the format of sent message for personal          */

/*************************************************Variable*************************************************/
static char  gcinput_prompt[CHAR_LEN_1024]  = {0};   /* input prompt */

/***********************************************Declaration***********************************************/
int seperate_input_content(char *buf, char *atname, char *msg);
int parse_address(char *address, char *ip, int* port);
void *recv_thread(void *arg);

/*************************************************Function************************************************/
/* seperate name and message from input content */
int seperate_input_content(char *buf, char *atname, char *msg)
{
	char *pcstart   = NULL;
	char *pcend     = NULL;
 
	pcstart = strstr(buf, MARK_AT);
	pcend   = strstr(buf, MARK_SPACE);
 
	if((pcstart == NULL) || (pcend == NULL) || (pcstart > pcend))
	{
		printf("The format of input content is wrong!\n");
        return -1;
	}
	else
	{
		memcpy(atname, pcstart, pcend-pcstart);
        pcend += strlen(MARK_SPACE);
        strcpy(msg, pcend);
	}
 
	return 0;
}


/* parse address form mc connection command */
int parse_address(char *address, char *ip, int* port)
{
	char *pcstart           = NULL;
	char *pcend             = NULL;
    char cport[CHAR_LEN_16] = {0};
 
	pcstart = strstr(address, CMD_CONNECT_PREFIX);
	pcend   = strstr(address, MARK_COLON);
 
    if((pcstart == NULL) || (pcend == NULL) || (pcstart > pcend))
	{
		printf("The format of address is wrong!\n");
        return -1;
	}
	else
	{
        /* parse ip address */
		pcstart += strlen(CMD_CONNECT_PREFIX);
		memcpy(ip, pcstart, pcend-pcstart);

        /* parse port */
        strcpy(cport, pcend+strlen(MARK_COLON));
        *port = atoi(cport);
	}
 
	return 0;
}


/* thread used to receive message */
void *recv_thread(void *arg)
{
    int ret = 0;
    int socketfd = (int)(*((int *)arg));
    while (1)
    {
        char recvmessage[CHAR_LEN_1024];
        bzero(&recvmessage, CHAR_LEN_1024);

        /* receive message */
        ret = recv(socketfd, recvmessage, CHAR_LEN_1024, 0);
        if (ret == 0)
        {
            close(socketfd);
            exit(0);
        }
        
        /* output the message */
        printf("\n%s\n", recvmessage);
        /* input prompt */
        printf("%s", gcinput_prompt);
        fflush(stdout);
    }

    pthread_exit(0);

    return NULL;
}


/* main function */
int main(int argc, char *argv[])
{    
    struct sockaddr_in socket_ms_addr = {0};
    int         socketfd        = -1;
    pthread_t   tid_recv;
    char        mc_name[CHAR_LEN_64]     = {0};
    char        ms_address[CHAR_LEN_64]  = {0};    
    char        sendmsg[CHAR_LEN_1024]   = {0};
    char        inputmsg[CHAR_LEN_1024]  = {0};
    char        ip[CHAR_LEN_16] = {0};
    int         port            = 0;
    int         ret             = 0;

    /* get the parameter: client name */
    if(strcmp(argv[CMD_OPTION_NAME_IDX], CMD_OPTION_NAME) != 0)
    {
        printf("The parameter is wrong!\n");
        return -1;
    }
    strcpy(mc_name, argv[CMD_NAME_IDX]);
    
    /* input the command of mc connection */
    do
    {
        printf("%s > ", mc_name);
        fgets(ms_address, CHAR_LEN_64, stdin);
        /*parse address*/
        ret = parse_address(ms_address, ip, &port);
        if(ret != -1)
        {
            break;
        }
    }while(1);

    /* create socket */
    socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if (socketfd < 0)
    {
        printf("Failed to create the Socket!\n");
        return -1;
    }

    socket_ms_addr.sin_family = AF_INET;
    socket_ms_addr.sin_port = htons(port);
    socket_ms_addr.sin_addr.s_addr = inet_addr(ip);

    /* connect to server */
    ret = connect(socketfd, (void *)&socket_ms_addr, sizeof(socket_ms_addr));
    if (ret < 0)
    {
        printf("Failed to connect!\n");
    }
    else
    {
        /* send MC name to MS once */
        char send_name[CHAR_LEN_1024] = {0};
        strcpy(send_name, CMD_OPTION_NAME);
        strcat(send_name, mc_name);
        send(socketfd, send_name, CHAR_LEN_1024, 0);

        /* start a thread to receive message */
        pthread_create(&tid_recv, NULL, recv_thread, (void *)&socketfd);
        pthread_detach(tid_recv);

        /* send message to server */
        while (1)
        {
            bzero(&inputmsg, CHAR_LEN_1024);
            char input[CHAR_LEN_1024] = {0};
            /* input prompt */
            sprintf(gcinput_prompt, MSG_INPUT_PROMPT, ip, port, mc_name);
            printf("%s", gcinput_prompt);
            while(1)
            {
                fgets(input, CHAR_LEN_1024, stdin);
                strcat(inputmsg, input);
                /* if the end of line is not line break mark, end the input */
                if (input[strlen(input)-2] != MARK_LINE_BREAK)
                {
                    break;
                }
            }

            /* if it is not multiple lines message, replace the mark in disconnect or quit command */
            char *pEnd = NULL;
            if(((pEnd = strchr(inputmsg, MARK_LINE_BREAK)) == NULL)
                && ((pEnd = strchr(inputmsg, '\n')) != NULL))
            {
                 *pEnd = '\0';
            }

            char atname[CHAR_LEN_64] = {0};
            char msg[CHAR_LEN_1024]  = {0};
            /* send message to a MC  */
            if(strstr(inputmsg, MARK_AT) != NULL)
            {
                /* get the time */
                char ctime[CHAR_LEN_64] = {0};
                time_t timep;
                struct tm *ptm;
                time(&timep);
                ptm = gmtime(&timep);
                strftime(ctime, sizeof(ctime), "%Y/%m/%d %H:%M:%S", ptm);

                ret = seperate_input_content(inputmsg, atname, msg);
                if(ret == -1)
                {
                    continue;
                }

                if(strcmp((char*)(&atname[1]), CMD_BROADCAST) == 0)
                {
                    /* echo of the sent message */
                    printf(SENTMSG_ECHO_FORMAT_BROADCAST, ctime, mc_name, msg);
                    /* @name + sent content */
                    sprintf(sendmsg, SENTMSG_FORMAT_BROADCAST, atname, ctime, mc_name, msg);
                }
                else
                {
                    /* echo of the sent message */
                    printf(SENTMSG_ECHO_FORMAT_PERSONAL, ctime, mc_name, msg);
                    /* @name + sent content */
                    sprintf(sendmsg, SENTMSG_FORMAT_PERSONAL, atname, ctime, mc_name, msg);
                }

                /* send message */
                send(socketfd, sendmsg, CHAR_LEN_1024, 0);
            }
            /* execute command */
            else{
                /* disconnect command:BYE */
                if(strcmp(inputmsg, CMD_DISCONNECT) == 0)
                {
                    /* send message */
                    send(socketfd, inputmsg, CHAR_LEN_1024, 0);
                    close(socketfd);
                    pthread_cancel(tid_recv);
                }
                /* quit command:QUIT */
                else if(strcmp(inputmsg, CMD_QUIT) == 0)
                {
                    /* send message */
                    send(socketfd, inputmsg, CHAR_LEN_1024, 0);
                    close(socketfd);
                    pthread_cancel(tid_recv);
                    exit(0);
                }
                else
                {
                    printf("The input format is Wrong.\nSample:@xxx Hello.\n");
                }
            }
        }
    }

    close(socketfd);

    return 0;
}
