#pragma once

typedef struct qnode
{
	void * p;		//�洢�������
	qnode* next;	//�洢��һ������ָ��
	qnode* pre;		//�洢��ǰ������һ�����ָ��
}qnode;

typedef struct queue
{
	qnode* head;	//���е�ͷָ��
	qnode* tail;	//���е�βָ��
	int size;
}queue;
