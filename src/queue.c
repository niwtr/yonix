#include "queue.h"

//新建队列
queue * queue_create(void)
{
	queue *q;
	q = (queue*)kmalloc(sizeof(queue));
	if (q == nullptr)
		return nullptr;
	else
	{
		q->head = q->tail = nullptr;
		return q;
	}
}

//入队
queue* in_queue(qnode* qn, queue * q)
{
	//针对第一个入队的结点
	if (q->head == nullptr)
	{
		q->head = q->tail = qn;
		qn->next = qn->pre = nullptr;
	}
		
	else
	{
		qn->pre = q->tail;
		q->tail->next = qn;
		q->tail = qn;
		qn->next = nullptr;
	}
	q->size++;
	return q;
}

//出队
queue* out_queue(queue * q)
{
	qnode * qn;
	if (q->head == nullptr)
		painc("out_queue:queue empty! ");
	if (q->head == q->tail != nullptr)
	{
		qn = q->head;
		kmfree(qn);
		q->head = q->tail = nullptr;
	}
	else
	{
		qn = q->head;
		qn->next->pre = nullptr;
		q->head = qn->next;
		kmfree(qn);
	}
	q->size--;
	return q;
}

//判断队列是否为空
bool is_queue_empty(queue q)
{
	if (q.head == q.tail == nullptr)
		return true;
	return false;
}
