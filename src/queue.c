#include "queue.h"

//�½�����
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

//���
queue* in_queue(qnode* qn, queue * q)
{
	//��Ե�һ����ӵĽ��
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

//����
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

//�ж϶����Ƿ�Ϊ��
bool is_queue_empty(queue q)
{
	if (q.head == q.tail == nullptr)
		return true;
	return false;
}
