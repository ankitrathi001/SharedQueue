/******************************************************************************
 *
 * File Name: main_1.c
 *
 * Author: Ankit Rathi (ASU ID: 1207543476)
 *
 * Date: 21-SEP-2014
 *
 * Description: A test program that initiates multiple threads to access the 
 * shared queues consisting of 3 senders, 1 bus daemon and 3 receivers.
 * 
 *****************************************************************************/
 
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>

#define NUMBER_OF_SENDERS 3
#define NUMBER_OF_RECEIVERS 3
#define CPU_CLOCK_SPEED 400000000 //400 MHz

/**
 * Comment Below Line to run code to enable printing to output console
 */
//#define STATIC

/**
 * Declaration of global variables
 */ 
unsigned int STR_MAX_LEN=80;
unsigned int STR_MIN_LEN=10;
unsigned int GLOBAL_SEQUENCE_NUM = 1;
unsigned int GLOBAL_BUS_IN_Q_COUNTER = 0;
unsigned int GLOBAL_BUS_OUT_Q_COUNTER = 0;
unsigned int GLOBAL_BUS_OUT_Q1_COUNTER = 0;
unsigned int GLOBAL_BUS_OUT_Q2_COUNTER = 0;
unsigned int GLOBAL_BUS_OUT_Q3_COUNTER = 0;
unsigned int GLOBAL_SENDER_FLAG = 0;

/**
 * mutex for protecting GLOBAL_SEQUENCE_NUM
 */ 
pthread_mutex_t mutex;

/**
 * Function Declaration
 */
char *getRandomString(unsigned int str_min_length, unsigned int str_max_length);

/**
 * Message Token
 */
typedef struct MessageToken_Tag
{
	int msgID;
	int senderID;
	int receiverID;
	char str_msg[80];
	unsigned long timeStamp1;
	unsigned long timeStamp2;
}MessageToken;

/**
 * Thread Arguments
 */
typedef struct 
{
	int threadId;
	int fd_bus_in_q;
	int fd_bus_out_q1;
	int fd_bus_out_q2;
	int fd_bus_out_q3;
}ThreadParams;
 
/**
 * Function called by sender/transmitting threads to send data.
 */
void *thread_transmit(void *data)
{
	char random_str[80];
	int random_receiver;
	int res;
	int i;
	int sleep_interval;
	time_t endTime = time(0) + 10;
	ThreadParams *tparams = (ThreadParams*)data;
	MessageToken tok;
	while(time(0) < endTime)
	{
		/*Begininning of mutex to lock GLOBAL_SEQUENCE_NUM*/
		pthread_mutex_lock(&mutex);
		tok.msgID = GLOBAL_SEQUENCE_NUM++;
		pthread_mutex_unlock (&mutex);
		/*End of mutex to lock GLOBAL_SEQUENCE_NUM*/
		
		/*Sender thread generating a random receiver for the message*/
		random_receiver = rand() % NUMBER_OF_RECEIVERS;
		strcpy(random_str, getRandomString(STR_MIN_LEN, STR_MAX_LEN));
		tok.senderID = (tparams->threadId % 100 ) + 1;
		tok.receiverID = random_receiver + 1;
		sprintf(tok.str_msg, "%s", random_str);
		res = -1;
		
		/*
		 * write() is used to send message to driver program and it keeps 
		 * trying till a success message is obtained
		 */
		do
		{
			res = write(tparams->fd_bus_in_q, &tok, sizeof(MessageToken));
			usleep((rand() % 10 ) * 1000);
		}while(res==-1);
		
		if(res == sizeof(MessageToken))
		{
			printf("Can not write to the bus_in_q device file.\n");
			close(tparams->fd_bus_in_q);
			exit(-1);
		}
		else
		{
			/*Begininning of mutex to lock GLOBAL_BUS_IN_Q_COUNTER*/
			pthread_mutex_lock(&mutex);
			GLOBAL_BUS_IN_Q_COUNTER++;
			pthread_mutex_unlock(&mutex);
			/*End of mutex to lock GLOBAL_BUS_IN_Q_COUNTER*/
		}
	}
	//printf("main_1.c ThreadID: %d thread_transmit() Ends\n",tparams->threadId);
	pthread_exit(0);
}

/**
 * Function called by bus daemon thread to receive and send data.
 */
void *thread_transmit_receive(void *data)
{
	int res;
	ThreadParams *tparams = (ThreadParams*)data;
	MessageToken tok;
	//printf("main_1.c ThreadID: %d thread_transmit_receive() Start\n",tparams->threadId);
	while(1)
	{
		usleep((rand() % 10 ) * 1000);
		res = read(tparams->fd_bus_in_q, &tok, sizeof(MessageToken));

		//IF failed while reading
		if(res == -1)
		{
			usleep((rand() % 10 ) * 1000);
		}
		else
		{
			if(tok.receiverID == 1)
			{
				res = write(tparams->fd_bus_out_q1, &tok, sizeof(MessageToken));
				if(res == -1)
				{
					while(res == -1)
					{
						usleep((rand() % 10 ) * 1000);
						res = write(tparams->fd_bus_out_q1, &tok, sizeof(MessageToken));
					}
				}
				else if(res == sizeof(MessageToken))
				{
					printf("Can not write to the bus_out_q device file.\n");
					close(tparams->fd_bus_in_q);
					exit(-1);
				}
			}
			else if(tok.receiverID == 2)
			{
				res = write(tparams->fd_bus_out_q2, &tok, sizeof(MessageToken));
				if(res == -1)
				{
					while(res == -1)
					{
						usleep((rand() % 10 ) * 1000);
						res = write(tparams->fd_bus_out_q2, &tok, sizeof(MessageToken));
					}
				}
				else if(res == sizeof(MessageToken))
				{
					printf("Can not write to the bus_out_q device file.\n");
					close(tparams->fd_bus_in_q);
					exit(-1);
				}
			}
			else if(tok.receiverID == 3)
			{
				res = write(tparams->fd_bus_out_q3, &tok, sizeof(MessageToken));
				if(res == -1)
				{
					while(res == -1)
					{
						usleep((rand() % 10 ) * 1000);
						res = write(tparams->fd_bus_out_q3, &tok, sizeof(MessageToken));
					}
				}
				else if(res == sizeof(MessageToken))
				{
					printf("Can not write to the bus_out_q device file.\n");
					close(tparams->fd_bus_in_q);
					exit(-1);
				}
			}
		}
		pthread_mutex_lock(&mutex);
		GLOBAL_BUS_OUT_Q_COUNTER++;
		pthread_mutex_unlock(&mutex);
		if((GLOBAL_BUS_IN_Q_COUNTER == GLOBAL_BUS_OUT_Q_COUNTER) && (GLOBAL_SENDER_FLAG==1))
		{
			break;
		}
	}
	//printf("main_1.c ThreadID: %d thread_transmit_receive() Ends\n",tparams->threadId);
	pthread_exit(0);
}

/**
 * Function called by receiver threads to receive data.
 */
void *thread_receive(void *data)
{
	ThreadParams *tparams = (ThreadParams*)data;
	MessageToken tok;
	//printf("main_1.c ThreadID: %d thread_receive() Start\n",tparams->threadId);
	int stopFlag=0;
	int res,ret;
	int sleep_interval;
	int threadid;
	while(stopFlag != 1)
	{
		usleep((rand() % 10 ) * 1000);
		threadid = (tparams->threadId) % 300;
		if(threadid == 0)
		{
			res = read(tparams->fd_bus_out_q1, &tok,  sizeof(MessageToken));
			if(res != -1)
			{
				GLOBAL_BUS_OUT_Q1_COUNTER++;
#ifdef STATIC
#else
				printf("%d          %d          %d          %ld         %lu mS    %s\n",tok.msgID,tok.senderID,tok.receiverID,tok.timeStamp1 + tok.timeStamp2, (tok.timeStamp1 + tok.timeStamp2) * 1000 / CPU_CLOCK_SPEED, tok.str_msg);
#endif
			}
		}
		else if(threadid == 1)
		{
			res = read(tparams->fd_bus_out_q2, &tok,  sizeof(MessageToken));
			if(res != -1)
			{
				GLOBAL_BUS_OUT_Q2_COUNTER++;
#ifdef STATIC
#else
				printf("%d          %d          %d          %ld         %lu mS     %s\n",tok.msgID,tok.senderID,tok.receiverID,tok.timeStamp1 + tok.timeStamp2,(tok.timeStamp1 + tok.timeStamp2) * 1000 / CPU_CLOCK_SPEED, tok.str_msg);
#endif
			}
		}
		else if(threadid == 2)
		{
			res = read(tparams->fd_bus_out_q3, &tok,  sizeof(MessageToken));
			if(res != -1)
			{
				GLOBAL_BUS_OUT_Q3_COUNTER++;
#ifdef STATIC
#else
				printf("%d          %d          %d          %ld         %lu mS     %s\n",tok.msgID,tok.senderID,tok.receiverID,tok.timeStamp1 + tok.timeStamp2,(tok.timeStamp1 + tok.timeStamp2) * 1000 / CPU_CLOCK_SPEED , tok.str_msg);
#endif
			}
		}
		if(res == -1)
		{
			if(((GLOBAL_BUS_OUT_Q1_COUNTER + GLOBAL_BUS_OUT_Q2_COUNTER + GLOBAL_BUS_OUT_Q3_COUNTER) == GLOBAL_BUS_IN_Q_COUNTER) && (GLOBAL_SENDER_FLAG==1))
			{
				
				stopFlag = 1;
			}
			else
			{
				usleep((rand() % 10) * 1000 );
			}
		}
	}
	//printf("main_1.c ThreadID: %d thread_receive() Ends\n",tparams->threadId);
	pthread_exit(0);
}

/**
 * Main Function
 */
int main(int argc, char **argv)
{
	int fd_bus_in_q, fd_bus_out_q1, fd_bus_out_q2, fd_bus_out_q3;
	pthread_t thread_id_s[NUMBER_OF_SENDERS], thread_id_bd, thread_id_r[NUMBER_OF_RECEIVERS];
	ThreadParams *tp_s[NUMBER_OF_SENDERS],*tp_bd, *tp_r[NUMBER_OF_RECEIVERS];
	
	/*Open Device bus_in_q*/
	fd_bus_in_q = open("/dev/bus_in_q", O_RDWR);
	if (fd_bus_in_q < 0)
	{
		printf("Can not open device file bus_in_q.\n");
		return 0;
	}
	/*Open Device bus_out_q1*/
	fd_bus_out_q1 = open("/dev/bus_out_q1", O_RDWR);
	if (fd_bus_out_q1 < 0)
	{
		printf("Can not open device file bus_in_q.\n");
		return 0;
	}
	/*Open Device bus_out_q2*/
	fd_bus_out_q2 = open("/dev/bus_out_q2", O_RDWR);
	if (fd_bus_out_q2 < 0)
	{
		printf("Can not open device file bus_out_q2.\n");
		return 0;
	}
	/*Open Device bus_out_q3*/
	fd_bus_out_q3 = open("/dev/bus_out_q3", O_RDWR);
	if (fd_bus_out_q3 < 0)
	{
		printf("Can not open device file bus_out_q3.\n");
		return 0;
	}
	
	/* Mutex Initialization*/
	pthread_mutex_init(&mutex, NULL);
	
	int i,ret;
	/* Sender Threads Creation*/
	for(i=0;i<NUMBER_OF_SENDERS;i++)
	{
		tp_s[i] = malloc(sizeof(ThreadParams));
		tp_s[i] -> threadId = 100+i;
		tp_s[i] -> fd_bus_in_q = fd_bus_in_q;
		tp_s[i] -> fd_bus_out_q1 = fd_bus_out_q1;
		tp_s[i] -> fd_bus_out_q2 = fd_bus_out_q2;
		tp_s[i] -> fd_bus_out_q3 = fd_bus_out_q3;
		ret = pthread_create(&thread_id_s[i], NULL, &thread_transmit, (void*)tp_s[i]);
		if(ret)
		{
			printf("ERROR; return code from pthread_create() is %d\n", ret);
			exit(-1);
		}
	}
	//printf("Sender Threads Created\n");
	
	/* Bus Daemon Thread Creation*/
	tp_bd = malloc(sizeof(ThreadParams));
	tp_bd ->  threadId = 200;
	tp_bd -> fd_bus_in_q = fd_bus_in_q;
	tp_bd -> fd_bus_out_q1 = fd_bus_out_q1;
	tp_bd -> fd_bus_out_q2 = fd_bus_out_q2;
	tp_bd -> fd_bus_out_q3 = fd_bus_out_q3;
	ret = pthread_create(&thread_id_bd, NULL, &thread_transmit_receive, (void*)tp_bd);
	if(ret)
	{
		printf("ERROR; return code from pthread_create() is %d\n", ret);
		exit(-1);
	}
	//printf("Bus Daemon Thread Created\n");
#ifdef STATIC
#else
	printf("MessageID  SenderID  ReceiverID  TSCCounter      Time(mS)        Message\n");
	printf("=================================================================================================\n");
#endif
	
	/* Receiver Threads Creation*/
	for(i=0;i<NUMBER_OF_RECEIVERS;i++)
	{
		tp_r[i] = malloc(sizeof(ThreadParams));
		tp_r[i] -> threadId = 300+i;
		tp_r[i] -> fd_bus_in_q = fd_bus_in_q;
		tp_r[i] -> fd_bus_out_q1 = fd_bus_out_q1;
		tp_r[i] -> fd_bus_out_q2 = fd_bus_out_q2;
		tp_r[i] -> fd_bus_out_q3 = fd_bus_out_q3;
		ret = pthread_create(&thread_id_r[i], NULL, &thread_receive, (void*)tp_r[i]);
		if(ret)
		{
			printf("ERROR; return code from pthread_create() is %d\n", ret);
			exit(1);
		}
	}
	//printf("Receiver Threads Created\n");
	
	/* Main thread waits for all threads to execute before closing*/
	for(i=0;i<NUMBER_OF_SENDERS;i++)
	{
		pthread_join(thread_id_s[i], NULL);
	}
	GLOBAL_SENDER_FLAG = 1;
	pthread_join(thread_id_bd, NULL);
	for(i=0;i<NUMBER_OF_RECEIVERS;i++)
	{
		pthread_join(thread_id_r[i], NULL);
	}
#ifdef STATIC
#else
	printf("Number of Messages Sent: %d\n",GLOBAL_BUS_IN_Q_COUNTER);
	printf("Number of Messages Received By Receiver 1: %d\n",GLOBAL_BUS_OUT_Q1_COUNTER);
	printf("Number of Messages Received By Receiver 2: %d\n",GLOBAL_BUS_OUT_Q2_COUNTER);
	printf("Number of Messages Received By Receiver 3: %d\n",GLOBAL_BUS_OUT_Q3_COUNTER);
	printf("Total Number of Messages Received: %d\n",GLOBAL_BUS_OUT_Q1_COUNTER + GLOBAL_BUS_OUT_Q2_COUNTER + GLOBAL_BUS_OUT_Q3_COUNTER);
#endif
	
	/*Close the file descriptors*/
	close(fd_bus_in_q);
	close(fd_bus_out_q1);
	close(fd_bus_out_q2);
	close(fd_bus_out_q3);
	
	return 0;
}

/**
 * Function to generate a Random String given the maximum and minimum size of 
 * random string to be generated.
 */
char *getRandomString(unsigned int str_min_length, unsigned int str_max_length)
{
	unsigned int rand_str_len;
	int i;
	char *ran_str;
	unsigned int MAX_STR_LENGTH;
	MAX_STR_LENGTH = str_max_length - str_min_length;
	rand_str_len = (rand() % (str_max_length - str_min_length)) + str_min_length;
	ran_str = malloc(sizeof(rand_str_len));
	for (i = 0; i < rand_str_len; i++)
	{
		ran_str[i] = (i % 94) + 33;
	}
	ran_str[rand_str_len] = '\0';
	return ran_str;
}
