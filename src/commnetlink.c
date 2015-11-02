#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <string.h>
#include <linux/netlink.h>
#include <getopt.h>
#include <fcntl.h>

#define MSG_LEN 1024
#define MAX_MSGSIZE 1024+20

struct u_packet_info
{
    struct nlmsghdr hdr;
    char msg[MSG_LEN];
};


static void usage(void);

static void usage(void)
{
	fprintf(stderr,
		"Usage: %s [-c cmd] [-m postmsg][-t timeout]\n       %s [-c cmd] [-g getmsg][-t timeout]\n",
		"commnetlink","commnetlink");
}

int  netlink_init(int net_type)
{
	struct sockaddr_nl local;
    struct sockaddr_nl kpeer;
    int skfd, ret, kpeerlen = sizeof(struct sockaddr_nl);
    char *retval;
    int flags;
    

    skfd = socket(PF_NETLINK, SOCK_RAW, net_type);
    if(skfd < 0){
        //printf("can not create a netlink socket\n");
        return -1;
    }
    memset(&local, 0, sizeof(local));
    local.nl_family = AF_NETLINK;
    local.nl_pid = getpid();
    local.nl_groups = 0;
    if(bind(skfd, (struct sockaddr *)&local, sizeof(local)) != 0){
       // printf("bind() error\n");
        return -1;
    }
     //设置socket为非阻塞模式
    flags = fcntl(skfd, F_GETFL, 0);
    fcntl(skfd, F_SETFL, flags|O_NONBLOCK);
    
    return skfd;
}




int netlink_PostMsg(int linkfd,char *postmsg,int recved,int timeout)
{
	
	fd_set st_write_set,st_read_set;                         //select fd
	int ret, kpeerlen = sizeof(struct sockaddr_nl);
    struct timeval write_time_out = {0, 0};     //
    struct nlmsghdr *message;
    struct sockaddr_nl kpeer;
    struct u_packet_info info;
	int has_send = 0,has_recv = 1;
    int select_timeout = time(0) + timeout;
    
    memset(&kpeer, 0, sizeof(kpeer));
    kpeer.nl_family = AF_NETLINK;
    kpeer.nl_pid = 0;
    kpeer.nl_groups = 0;
    //设置select
    FD_ZERO(&st_write_set);
    FD_SET(linkfd, &st_write_set);
    if(recved)
    {
    		FD_ZERO(&st_read_set);
   			FD_SET(linkfd, &st_read_set);
			has_recv = 0;
    }
    message = (struct nlmsghdr *)calloc(1,NLMSG_SPACE(MAX_MSGSIZE));  
    message->nlmsg_len = NLMSG_LENGTH(strlen(postmsg) + 1);  
    message->nlmsg_flags = 0;
    message->nlmsg_type = 0;
    message->nlmsg_seq = 0;
    message->nlmsg_pid = getpid();
    
	strcpy(NLMSG_DATA(message),postmsg);  
    //printf("message sendto kernel are:%s, len:%d\n", (char *)NLMSG_DATA(message), message->nlmsg_len);
    
    while(!has_recv || !has_send)
    {
   		if(recved)
    	{
    		ret = select(linkfd + 1, &st_read_set, &st_write_set,NULL , &write_time_out);
   		}
   		else
  	 	{
   			ret = select(linkfd + 1, NULL, &st_write_set, NULL, &write_time_out);
   		}
   		if(ret <= 0)
    	{
    		printf("select err");
    	}
    	else
   		{
   			//printf("-----------ret=%d,%d,%d\n",has_recv,FD_ISSET(linkfd, &st_read_set),FD_ISSET(linkfd, &st_write_set));
		   	if(!has_recv )
		   	{
		    	 ret = recvfrom(linkfd, &info, sizeof(struct u_packet_info),0, (struct sockaddr*)&kpeer, &kpeerlen);
		       	 if(ret >=0 )
		      	 {
		         	printf("recv msg:[%s].\n",(char *)info.msg);
		         	 has_recv = 1;
		      	 }
				
		    }
		    if(!has_send&&FD_ISSET(linkfd, &st_write_set) )
		    {
		    			//发送消息
		    	 ret = sendto(linkfd, message, message->nlmsg_len, 0, 
		                      (struct sockaddr*)&kpeer, sizeof(kpeer));
		         if(ret>=0)
		      	 {
		         	 printf("post msg:[%s].\n",postmsg);
		         	 has_send = 1;
		      	 }
				
		    } 
	 	}
	 	if( select_timeout < time(0))
	 	{
	 			printf("timeout\n");	
	 			break;
	 	}
	}
    if(message)
    {
   		free(message);	
   		message = NULL;
   	}
 
}

int main(int argc, const char *argv[])
{
	char *value = NULL;
	int opt;
	int setGet = -1;
	int ncmd = 0;
	int linkfd = -1;
	int timeout = 10;
	
	while ((opt = getopt(argc, argv, "c:m:g:t:")) != -1) {
		switch (opt) {
		case 'c':
			ncmd = atoi(optarg);
			break;
		case 'm':
			value = optarg;
			setGet = 0;
			break;
		case 'g':
			setGet = 1;
			value = optarg;
			break;
		case 't':
			timeout = atoi(optarg);
			break;
		default:
			exit(1);
		}
	}
	if((0 == ncmd) || ( -1 == setGet ) ||(0 >= setGet && value == NULL)) {
		usage();
		return 1;
	}
	linkfd  = netlink_init(ncmd);
	if(linkfd == -1)
	{
		printf("cmd err\n");
		exit(1);
	}
	//printf("netlink_init fd = %d\n",linkfd);
	
	char *msg = calloc(1,sizeof(char)*MSG_LEN);
	strncpy(msg,value,strlen(value) < MSG_LEN?strlen(value) :MSG_LEN);
	//printf("post msg:%s\n",msg);
	netlink_PostMsg(linkfd,msg,setGet,timeout);
			
	if(msg != NULL)
	{
		free(msg);
		msg = NULL;	
	}
	
	
	
	return 0;	
}