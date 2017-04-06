#pragma once

typedef struct qnode
{
	void * p;		//存储结点内容
	qnode* next;	//存储下一个结点的指针
	qnode* pre;		//存储当前结点的上一个结点指针
}qnode;

typedef struct queue
{
	qnode* head;	//队列的头指针
	qnode* tail;	//队列的尾指针
	int size;
}queue;
