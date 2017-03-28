#pragma once
#include "yotypes.h"

//������������ݽṹ
struct spinlock {
	uint s_locked;

	//������Ϣ����
	char *l_name;	//��������
	struct cpu *l_cpu;//���������CPU
	uint l_pcs[10];	//��������ջ
};

//��ʼ��һ����
void initlock(struct spinlock *slk, char *lname);

//�˶Ը�CPU�Ƿ������
//��Ϊ�棬�򷵻�1
int ishold(struct spinlock *lock);

//��֤�����������ԭ����
//���ƻ��ָ���е�cli������ֹ�жϷ���
void forbidtrap(void);

//�����жϷ���
void promisetrap(void);

//�����
//ѭ���ҵ����õ���
void acquirelock(struct spinlock *slk);

//�ͷ���
void releaselock(struct spinlock *slk);

//Ӳ����ز���
//��¼�µ�ǰ��������ջ
void getcallerpcs(void *v, uint pcs[]);
