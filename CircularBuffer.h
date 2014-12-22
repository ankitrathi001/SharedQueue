/******************************************************************************
 *
 * File Name: CircularBuffer.h
 *
 * Author: Ankit Rathi (ASU ID: 1207543476)
 *
 * Date: 21-SEP-2014
 *
 * Description: Header file for driver Squeue.c to perform basic operation
 * of enquing and dequing data from Circular Buffer. It includes both dynamic
 * and static implementation of allocating buffers.
 * 
 *****************************************************************************/
 
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <asm/uaccess.h>
/**
 * Maximum Queue Size
 */ 
#define MAX_QUEUE_SIZE 10

/**
 * Comment Below Line to run code as Dynamic Memory Allocate Code
 * If not commented then code runs as a Static Memory Allocated Code
 */
#define STATIC

/**
 * Message Token Structure
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
 * Circular Buffer Structure
 */
typedef struct CircularBuffer_Tag
{
#ifdef STATIC
	MessageToken msg[MAX_QUEUE_SIZE+1];
#else
	MessageToken *msg[MAX_QUEUE_SIZE];
#endif
	unsigned int frontIndex;
	unsigned int rearIndex;
	int size;
}CircularBuffer;

/**
 * Function Declaration
 */
int isCircularBuffer_Full(CircularBuffer *cb);
int isCircularBuffer_Empty(CircularBuffer *cb);
void init_CircularBuffer(CircularBuffer *cb);
int enqueue_CircularBuffer(CircularBuffer *cb, MessageToken *msgtoken);
int dequeue_CircularBuffer(CircularBuffer *cb, MessageToken *msgtoken);
void clean_CircularBuffer(CircularBuffer *cb);
void display_CircularBuffer(CircularBuffer *cb);

/**
 * Function to check if Circular Buffer is Full
 */
inline int isCircularBuffer_Full(CircularBuffer *cb)
{
    return ((cb->rearIndex + 1) % cb->size == cb->frontIndex);
}

/**
 * Function to check if Circular Buffer is Empty
 */
inline int isCircularBuffer_Empty(CircularBuffer *cb)
{
    return (cb->rearIndex == cb->frontIndex);
}

/**
 * Function to initialize Circular Buffer
 */
void init_CircularBuffer(CircularBuffer *cb)
{
#ifdef STATIC
	cb->size  = MAX_QUEUE_SIZE+1;
#else
	cb->size = MAX_QUEUE_SIZE;
#endif
    cb->frontIndex = 0;
    cb->rearIndex = 0;
}

/**
 * Function to Enqueue/Write/Add data into Circular Buffers
 */
inline int enqueue_CircularBuffer(CircularBuffer *cb, MessageToken *msgtoken)
{
	int retValue = -1;
	if(isCircularBuffer_Full(cb))
	{
		return -1;
	}
#ifdef STATIC
    cb->msg[cb->rearIndex] = *msgtoken;
    retValue = cb->rearIndex;
    cb->rearIndex = (cb->rearIndex + 1) % cb->size;
#else
	cb->msg[cb->rearIndex] = kmalloc(sizeof(MessageToken), GFP_KERNEL);
	memcpy(cb->msg[cb->rearIndex],msgtoken,sizeof(MessageToken));
    retValue = cb->rearIndex;
    cb->rearIndex = (cb->rearIndex + 1) % cb->size;
    
#endif
    return retValue;
}

/** 
 * Function to Dequeue/Read/Remove data from Circular Buffer
 */
inline int dequeue_CircularBuffer(CircularBuffer *cb, MessageToken *msgtoken)
{
	int retValue;
	if(isCircularBuffer_Empty(cb))
	{
		return -1;
	}
#ifdef STATIC
    *msgtoken = cb->msg[cb->frontIndex];
    retValue = cb->frontIndex;
    cb->frontIndex = (cb->frontIndex + 1) % cb->size;
#else
	memcpy(msgtoken,cb->msg[cb->frontIndex],sizeof(MessageToken));
	retValue = cb->frontIndex;
	kfree(cb->msg[cb->frontIndex]);
	cb->frontIndex = (cb->frontIndex + 1) % cb->size;
#endif
    return retValue;
}
