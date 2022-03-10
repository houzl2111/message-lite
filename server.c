#include "common.h"

/**************************************************Define**************************************************/
#define SERVER_IP       "127.0.0.1"      /* server ip address    */
#define SERVER_PORT     12345            /* server port          */
#define MC_MAX_NUM      100              /* Maximum number of MC */
#define CMD_QUIT        "QUIT"           /* quit the client      */


/**************************************************Struct**************************************************/
/* MC information */
typedef struct {
	int     sockfd;                 /* sockfd       */
    char    mc_name[CHAR_LEN_64];   /* client name  */
} MC_INFO;

/* connected MC list */
typedef struct{
	MC_INFO         mc_info;    /* client information */
    struct MC_LIST  *next;      /* next client        */
} MC_LIST;


/*************************************************Variable*************************************************/
MC_LIST        *mc_list;                                    /* client list              */
static int      mc_num      = 0;                            /* number of MC             */
pthread_mutex_t mutex       = PTHREAD_MUTEX_INITIALIZER;    /* mutex to protect MC list */


/***********************************************Declaration***********************************************/
void mclist_init(MC_LIST *head);
void mclist_add(MC_LIST *head, MC_INFO mcinfo);
void mclist_remove(MC_LIST *head, int sockfd);
void mclist_destory(MC_LIST *head);
int mclist_find_sockfd_by_name(MC_LIST *head, char *name);
int parse_message(char *buf, char *name, char *msg);
void *mchandler_start(void *arg);


/*************************************************Function************************************************/
/* initialize list */
void mclist_init(MC_LIST *head)
{
    if(head == NULL)
    {
        return;
    }
	
    head->mc_info.sockfd = 0;
    bzero(head->mc_info.mc_name, CHAR_LEN_64);
    head->next = NULL;
}


/* Add a client at the end. */
void mclist_add(MC_LIST *head, MC_INFO mcinfo)
{
    if(head == NULL)
    {
        return;
    }

    /* go to the tail */
    MC_LIST *curNode = head;
    MC_LIST *preNode = NULL;
    while(curNode)
    {
        preNode = curNode;
        curNode = curNode->next;
    }

    /* add a MC at the end */
    MC_LIST *add_node = (MC_LIST *)malloc(sizeof(MC_LIST));
    if (add_node == NULL)
    {
        return;
    }
    
    add_node->mc_info = mcinfo;
    add_node->next = NULL;
    preNode->next = add_node;
}


/* Remove a client by sockfd. */
void mclist_remove(MC_LIST *head, int sockfd)
{
    if(head == NULL)
    {
        return;
    }

    MC_LIST *curNode = head;
    MC_LIST *preNode = NULL;
    MC_LIST *delNode = NULL;
    while(curNode && (curNode->mc_info.sockfd != sockfd))
    {
        preNode = curNode;
        curNode = curNode->next;
    }

    preNode->next = curNode->next;
	free(curNode);
}


/* Destroy mc list */
void mclist_destory(MC_LIST *head)
{
    if(head == NULL)
    {
        return;
    }
    MC_LIST *curNode = head;
    MC_LIST *nextNode = NULL;
    while(curNode)
    {
        nextNode = curNode->next;
        free(curNode);
        curNode = nextNode;
    }

    head->next = NULL;

    return;
}


/* Find specified client by name. */
int mclist_find_sockfd_by_name(MC_LIST *head, char *name)
{
    int clientfd = -1;
    if((head == NULL) || (name == NULL))
    {
        return -1;
    }

    MC_LIST *curNode = head;
    while(curNode)
    {
        if(strcmp(curNode->mc_info.mc_name, name) == 0)
        {
            clientfd = curNode->mc_info.sockfd;
            break;
        }
        curNode = curNode->next;
    }

    return clientfd;
}


/* parse name and message from received message */
int parse_message(char *buf, char *name, char *msg)
{
	char *pcstart = NULL;
	char *pcend = NULL;
 
	pcstart = strstr(buf, MARK_AT);     /* find the the @ */
	pcend = strstr(buf, MARK_SPACE);    /* find the the space after @name */
 
	if((pcstart == NULL) || (pcend == NULL) || (pcstart > pcend))
	{
		printf("The format of buf is wrong!\n");
        return -1;
	}
	else
	{
        /* get the name */
		pcstart += strlen(MARK_AT);
		memcpy(name, pcstart, pcend-pcstart);

        /* get the message */
        pcend += strlen(MARK_SPACE);
        strcpy(msg, pcend);
	}
 
	return 0;
}


/* handler for each connected MC */
void *mchandler_start(void *arg)
{
    int  clientfd               = (int)(*((int *)arg));
    char buf[CHAR_LEN_1024]     = {0};
    char message[CHAR_LEN_1024] = {0};
    char dest_mc_name[CHAR_LEN_64] = {0};
    int tempfd = -1;

    while (1)
    {
        /* receive message from MC */
        int len = recv(clientfd, buf, CHAR_LEN_1024, 0);
        if (len <= 0)
        {
            /* MC closed or lost connection */
            printf("The client(Clientfd=%d) has quitted!\n", clientfd);

            /* remove the client from list */
            pthread_mutex_lock(&mutex);
            mclist_remove(mc_list, clientfd);
            pthread_mutex_unlock(&mutex);
            break;
        }
        else
        {
            if (strlen(buf) == 0)
            {
                continue;
            }

            /* firstly get the MC name */
            char *pctmp = NULL;
            pctmp = strstr(buf, CMD_OPTION_NAME);
            if(pctmp != NULL)
            {
                pctmp += strlen(CMD_OPTION_NAME);
                /* add MC into client list */
                MC_INFO mc;
                mc.sockfd = clientfd;
                strcpy(mc.mc_name, pctmp);
                pthread_mutex_lock(&mutex);
                mclist_add(mc_list, mc);
                pthread_mutex_unlock(&mutex);
                continue;
            }

            /* If it is quit or disconnect, end the processing. */
            if ((strcmp(buf, CMD_QUIT) == 0)
                || (strcmp(buf, CMD_DISCONNECT) == 0))
            {
                break;
            }
            
            /* parse the name of destination and message */
            bzero(&dest_mc_name, CHAR_LEN_64);
            bzero(&message, CHAR_LEN_1024);
            parse_message(buf, dest_mc_name, message);

            /* broadcast to all client */
            if(strcmp(dest_mc_name, CMD_BROADCAST) == 0)
            {
                MC_LIST *curNode = mc_list;
                while(curNode)
                {
                    if((strcmp(curNode->mc_info.mc_name, "") != 0)
                        && (curNode->mc_info.sockfd != clientfd))
                    {
                        tempfd = curNode->mc_info.sockfd;
                        if (send(tempfd, message, CHAR_LEN_1024, 0) < 0)
                        {
                            printf("Failed to send message to %s\n", curNode->mc_info.mc_name);
                        }
                    }
                    
                    curNode = curNode->next;
                }
            }
            /* send to specified client */
            else
            {
                tempfd = mclist_find_sockfd_by_name(mc_list, dest_mc_name);
                if(tempfd == -1)
                {
                    printf("The MC %s doesn't exist!\n", dest_mc_name);
                    fflush(stdout);
                    continue;
                }
                if (send(tempfd, message, CHAR_LEN_1024, 0) < 0)
                {
                    printf("Failed to send message to %s\n", dest_mc_name);
                }
            }
        }
    }

    close(clientfd);
    mclist_remove(mc_list, clientfd);
    pthread_exit(0);

    return NULL;
}


/* main function */
int main(int argc, char *argv[])
{
    int socketfd = -1;
    struct sockaddr_in socket_ms_addr;
    pthread_t tid;
    int ret = 0;

    /* MC list */
    mc_list = (MC_LIST *)malloc(sizeof(MC_LIST));
    mclist_init(mc_list);

    /* set server address */
    bzero(&socket_ms_addr, sizeof(socket_ms_addr));
    socket_ms_addr.sin_family       = AF_INET;                  /* IPV4 */
    //socket_ms_addr.sin_addr.s_addr  = htonl(INADDR_ANY);      /*INADDR_ANY means local ip */
    socket_ms_addr.sin_addr.s_addr  = inet_addr(SERVER_IP);     /* specify ip */
    socket_ms_addr.sin_port         = htons(SERVER_PORT);       /* port */

    /* create socket */
    socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if (socketfd < 0)
    {
        printf("Failed to create the Socket!\n");
        return -1;
    }

    /* bind address */
    ret = bind(socketfd, (void *)&socket_ms_addr, sizeof(socket_ms_addr));
    if(ret < 0)
    {
        printf("Failed to bind!\n");
        return -1;
    }

    /* listen */
    ret = listen(socketfd, 10);
    if(ret < 0)
    {
        printf("Failed to listen!\n");
        return -1;
    }
    printf("Listenning connection from MC......\n");

    /* wait the CONNECT requests from MC */
    while (1)
    {
        if (mc_num < MC_MAX_NUM)
        {
            struct sockaddr_in ms_address;
            socklen_t client_addrLength = sizeof(struct sockaddr_in);
            int clientfd = accept(socketfd, (struct sockaddr_in *)&ms_address, &client_addrLength);
            if (clientfd == -1)
            {
                printf("MC connect failed！\n");
                break;
            }

            printf("MC(IP:%s:%d, clientfd=%d) connected successfully!\n", inet_ntoa(ms_address.sin_addr), ntohs(ms_address.sin_port), clientfd);
            /* create MC handler */
            ret = pthread_create(&tid, NULL, mchandler_start, (void *)&clientfd);
            if (ret != 0)
            {
                break;
            }
            pthread_detach(tid); 
        }
    }

    close(socketfd);
    mclist_destory(mc_list);

    return 0;
}
