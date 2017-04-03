//BORROWED from xv6

// x86 trap and interrupt constants.

// Processor-defined:
#define T_DIVIDE         0      // divide error ��0����
#define T_DEBUG          1      // debug exception �����г��򵥲�����ʱ�����ñ�־�Ĵ���eflag��T��־ʱ�������ж�
#define T_NMI            2      // non-maskable interrupt ���������ж�
#define T_BRKPT          3      // breakpoint �ɶϵ�ָ��int3����
#define T_OFLOW          4      // overflow eflag�������־OF����
#define T_BOUND          5      // bounds check Ѱַ����Ч��ַ����ʱ����
#define T_ILLOP          6      // illegal opcode CPUִ��ʱ����һ����Ч��ָ�������
#define T_DEVICE         7      // device not available �豸�����ڣ�ָЭ������(CPU���õ���߸������������Ĵ�����)
#define T_DBLFLT         8      // double fault ˫���ϳ��� ��CPU�ڵ���ǰһ���쳣�Ĵ��������ּ�⵽һ���µ��쳣ȴ���޷����д���ʱ����
// #define T_COPROC      9      // reserved (not used since 486) Э�������γ��������ж���ڵ� ��ֹ���������󳬳����ݶεĸ���ֵ
#define T_TSS           10      // invalid task switch segment ��Ч������״̬��TSS
#define T_SEGNP         11      // segment not present ��������ָ���Ķβ�����
#define T_STACK         12      // stack exception ��ջ�β����ڻ�ѰַԽ����ջ��
#define T_GPFLT         13      // general protection fault û�з���80386�������ƣ���Ȩ�����Ĳ�������
#define T_PGFLT         14      // page fault ҳ���� ҳ�����ڴ���
// #define T_RES        15      // reserved ����intel�����ж���ڵ�
#define T_FPERR         16      // floating point error 
#define T_ALIGN         17      // aligment check �߽��������� ���ڴ�߽���ʱ��
#define T_MCHK          18      // machine check
#define T_SIMDERR       19      // SIMD floating point error

// These are arbitrarily chosen, but with care not to overlap
// processor defined exceptions or interrupt vectors.
#define T_SYSCALL       64      // system call
#define T_DEFAULT      500      // catchall

#define T_IRQ0          32      // IRQ 0 corresponds to int T_IRQʱ���ж�

#define IRQ_TIMER        0		//ʱ���ж�����
#define IRQ_KBD          1		//���������豸�ж�����
#define IRQ_COM1         4		//�����豸
#define IRQ_PRI			 7		//��ӡ�������������
#define IRQ_IDE0        14		//IDE0�����������
#define IRQ_IDE1		15		//IDE1�����������
#define IRQ_ERROR       19		//��������
#define IRQ_SPURIOUS    31		

