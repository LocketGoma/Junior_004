//#define PRINT_FOR_DEBUG
#define _CRT_SECURE_NO_WARNINGS // �����

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <time.h>
#include "sectormap.h"

AddrMapTab sectormaptbl; // ���� ���̺�
Garbage_list garbagetable; // �߰�.
extern FILE *devicefp;

/****************  prototypes ****************/
void ftl_open();
void ftl_write(int lsn, char *sectorbuf);
void ftl_read(int lsn, char *sectorbuf);
void print_block(int pbn);
void print_sectormaptbl();

/******* Mapping Table *******/
int get_table_area(int lsn); // pbn ����, ��������� -1
int first_empty(int freeblock_pbn); // ����ִ� page ����. ���ľߵ�.
int find_empty_block();


/******* Garbage Table *******/
int make_empty_page(int garbage_pbn, int freeblock_pbn);	// �������� ����
int get_empty_page();										// �ݵ�� ��ĥ��.
int is_garbage(int ppn); // 0 or 1 ����. garbage�� ������ 1, �ƴ� 0
void print_garbage_all(); // ȣ���ϸ� garbage table ����
void print_garbage(int ppn); // �����Լ�

/*************1�� G-C�� �Լ�*************/
//2�������� free_ppn = DATABLKS_PER_DEVICE;
//int free_ppn; // free ppn, ���������̳� �Լ��θ� in/out �Ұ�. ���� : ������ ��ȣ + ��ȯ�� ����. * ���� ȣ������ ���� *
//void set_free(int ppn); // free_ppn ����
//int get_free(); // free_ppn ����


//
// flash memory�� ó�� ����� �� �ʿ��� �ʱ�ȭ �۾�, ���� ��� address mapping table�� ����
// �ʱ�ȭ ���� �۾��� �����Ѵ�
//
void ftl_open()
{
	int i;

	// initialize the address mapping table
	for(i = 0; i < DATAPAGES_PER_DEVICE; i++)
	{
		sectormaptbl.entry[i].ppn = -1; // ���� �ִ°� DATABLKS_PER_DEVICE
		garbagetable.list[i].on_garbage = 0;
		if (i % 4 == 0)
			erase(i / 4);
	}
	//free_ppn = DATAPAGES_PER_DEVICE - 1; // �ϴ� �� ���� ������ �ʱ�ȭ. <- 1�� G-C��.

	//
	// �߰������� �ʿ��� �۾��� ������ ������ ��
	//

	return;
}

//
// file system�� ���� write interface
// 'sectorbuf'�� ����Ű�� �޸��� ũ��� 'SECTOR_SIZE'
//
void ftl_write(int lsn, char *sectorbuf)
{
	int ppn = -1;
	int gppn = -1; // garbage �� ��.
	char spare[SPARE_SIZE];
	char temp[SECTOR_SIZE];

#ifdef PRINT_FOR_DEBUG			// �ʿ� �� ������ block mapping table�� ����� �� �� ����
	print_sectormaptbl_info();
#endif
	/*
	if (sectormaptbl.entry[lsn].ppn != -1) // '���� ���� �־��ٸ�'
	{
		gppn = ppn;
		garbagetable.list[gppn].on_garbage = 1; // garbage ó��.
	}
	else
		;
	*/

	ppn = get_empty_page();
	printf("retrun ppn : %d\n", ppn);
	assert(ppn > -1); // -1�̸� �ߴ�.
	

	_itoa(lsn, spare, 16);

	strcat(sectorbuf,spare);

	sectormaptbl.entry[lsn].ppn = ppn;				// ��� ����.

	printf("[[lsn : %d, ppn : %d]]\n", lsn, sectormaptbl.entry[lsn].ppn);
	write(ppn, sectorbuf);		// �� '����' �����ݾ�?. �� �ٲ۴�.


	return;
}

//
// file system�� ���� read interface
// 'sectorbuf'�� ����Ű�� �޸��� ũ��� 'SECTOR_SIZE'
// 
void ftl_read(int lsn, char *sectorbuf)		
{
#ifdef PRINT_FOR_DEBUG			// �ʿ� �� ������ block mapping table�� ����� �� �� ����
	print_sectormaptbl_info();
#endif
	int ppn = -1;
	ppn = sectormaptbl.entry[lsn].ppn;

	assert(ppn != -1);


	read(ppn, sectorbuf);

	return;
}

//
// ftl_write() ���ο��� ȣ��Ǵ� �Լ�
// input : ������ �����ؼ� �ʿ��� parameter ����
// output : ppn
//
int get_empty_page()
{
	int ppn = -1; // -1 �״�� ���Ͻ� ����. ���� '������'
	int pbn = -1; // ���� ��.
	//
	// empty page �ϳ��� ã�Ƽ� �Ǵ� ������ ����� ���� ����(�̶� free block ���) 
	// empty page�� ���� �� �Ʒ��� make_empty_page() �Լ��� �����ؼ� ����ص� �ǰ�, �ƴϸ� �� �Լ��� ������� �ʰ�
	// ���ο� ����� ����ؼ� �ص� ����
	//

	//make_empty_page() �� ���� = block ������ �˻� �� ��� ����ִ� block�� �� �Ѱ��� ������.
	pbn = find_empty_block();
	if (pbn == -1)
		//make_empty_page(1,get_free()); // gpbn, fpbn ���� ����� + ã�� // fpbn�� �Ϸ�.
		make_empty_page(1, DATABLKS_PER_DEVICE);
	else
		ppn=first_empty(pbn);

	return ppn;	
}

int find_empty_block() { // empty 'block' �� ã���ִ� �Լ�. ���� ������ ã�¼��� �ٷ� ������.		/ �̰͵� �������۵���.
	int is_empty = 0;
	for (int i = 0; i < DATABLKS_PER_DEVICE; i++) {
		for (int j = 0; j < PAGES_PER_BLOCK; j++) {
			if (i*PAGES_PER_BLOCK + j == -1)	// ���빰�� ����ִ°� ���ý�
				j = PAGES_PER_BLOCK;
			else if (j == PAGES_PER_BLOCK - 1)	// continue�� �ѹ��� �Ȱɸ��� �������� ���޽�
				is_empty = 1;					// �ش� ���� ��������� Ȯ��
		}
		if (is_empty)							// �ش� ���� ���������
			return i;							// �� �ּ� ����
	}
	return -1; // ��ã���� -1
}


//
// get_empty_page() ���ο��� ȣ��Ǵ� �Լ�
// input : garbage block�� �߿��� ���õ� �ϳ��� 'garbage block'�� �����ϴ� ��ȿ�� ���������� 'free block'�� ����
// output : �����ϰ� �� �� free block�� empty page�� ������ �װ͵� �� ù ��° empty page�� ppn��, ������ ppn=-1�� ���� ����
// * �ڵ��� ���Ǽ��� ���� free block�� ���� -> ���� // -> �ٽ� free block �����ֱ⸦ �߰� �۾���. ��, free block ��ġ�� �׻� ������.

// 1�� �������÷��� : garbage block ���� ��ȿ ������ freeblock�� ���� -> g-block �ʱ�ȭ -> ���� g-block�� freeblock���� ����
// 2�� �������÷��� : 1�� -> ����, �ٽ� g-block�� freeblock�� ����� ���� freeblock ��ġ�� �ٽ� freeblock���� ������.
int make_empty_page(int garbage_pbn, int freeblock_pbn) // ����ٺ��� �������÷����� ��....
{
	int garb[PAGES_PER_BLOCK]; //Garbage block üũ. 1 = ����, 0 = garbage

	int is_empty = 0; // 0 : ������� ���� / 1 : �������
	for (int i = 0; i < PAGES_PER_BLOCK; i++) {
		if (is_garbage((garbage_pbn*PAGES_PER_BLOCK)+i)) {
			garb[i] = 0;
			is_empty = 1;
		}
		else
			garb[i] = 1;
	}

	for (int i = 0 ; i < PAGES_PER_BLOCK ; i++)
		if (garb[i]) {
			//(freeblock_pbn*PAGES_PER_BLOCK) + i = (garbage_pbn*PAGES_PER_BLOCK) + i; // �����Ͱ� ����ִ��� �𸣰ڴ�.
		}
	
	for (int i = 0 ; i < PAGES_PER_BLOCK; i++) {
		garbagetable.list[garbage_pbn*PAGES_PER_BLOCK+i].on_garbage = 0;	// ������ �ʱ�ȭ.
	}

	erase(garbage_pbn);
	//freeblock_pbn = garbage_pbn;						// 1��
	//set_free(freeblock_pbn);							// 1��

	/**************2�� G-C �� �ڵ�*******************/
	char *temp;
	temp = (char *)malloc(sizeof(char)*BLOCK_SIZE);
	 //�ʱⰪ���� �ʱ�ȭ.
	for (int i = 0; i < PAGES_PER_BLOCK; i++) {								
		read(freeblock_pbn*PAGES_PER_BLOCK + i, temp);			//�׸��� ���� ��
		if (temp != 0xFF)										//�ʱⰪ�� �ƴϸ� (=�����Ͱ� ������)
			write(garbage_pbn*PAGES_PER_BLOCK + i, temp);		//����.

		memset(temp, 0xFF, BLOCK_SIZE);
	}
	erase(freeblock_pbn);										//freeblock �� �ʱ�ȭ.

	/**************2�� G-C �� �ڵ�*******************/

	//if (is_empty) return first_empty(freeblock_pbn); // �ܼ� 1�� free block ��ȯ�� ���.

	if (is_empty) return first_empty(freeblock_pbn);	// 2�� free block ��ȯ�� ���
	else return -1;
}
int first_empty(int freeblock_pbn) { // �� �Լ� ����.					// ���ľߵ�!
	char temp[PAGE_SIZE];

	for (int i = 0; i < PAGES_PER_BLOCK; i++) {
		read(freeblock_pbn*PAGES_PER_BLOCK + i, temp);
		printf("temp_spare:%s", temp);
		//if (temp == ) {
			return freeblock_pbn*PAGES_PER_BLOCK + i;
		}
	//}
	//return -1;
}


void print_block(int pbn)
{
	char *pagebuf;					// flash memory ���� �ּ�.
	SpareData *sdata;
	int i;
	
	pagebuf = (char *)malloc(PAGE_SIZE);
	sdata = (SpareData *)malloc(SPARE_SIZE);

	printf("Physical Block Number: %d\n", pbn);

	for(i = pbn*PAGES_PER_BLOCK; i < (pbn+1)*PAGES_PER_BLOCK; i++)
	{
		read(i, pagebuf);
		memcpy(sdata, pagebuf+SECTOR_SIZE, SPARE_SIZE);
		printf("\t   %5d-[%7d][%7d]\n", i, sdata->lpn, sdata->is_invalid);
	}

	free(pagebuf);
	free(sdata);

	return;
}

void print_sectormaptbl()
{
	int i;

	printf("Address Mapping Table: \n");
	for(i = 0; i < DATAPAGES_PER_DEVICE; i++)
	{
		if(sectormaptbl.entry[i].ppn >= 0)
		{
			printf("[%d %d]\n", i, sectormaptbl.entry[i].ppn);
		}
	}
}

// �����Լ�
// 1. Table �����Լ� (���̺� ����/����)
int get_table_area(int lsn) { // ppn�� ����.
	return sectormaptbl.entry[lsn].ppn;
}




// 2. Garbage �����Լ�
int is_garbage(int ppn) { // 1 : �¾� / 0 : �Ƴ�
	if (garbagetable.list[ppn].on_garbage)
		return 1;
	else
	return 0;
}
void print_garbage_all() {
	printf("Garbage Table\n [PPN] \n");
	for (int i = 0; i < DATABLKS_PER_DEVICE; i++) {
		for (int j=0;j<PAGES_PER_BLOCK;j++)
		print_garbage(PAGES_PER_BLOCK*i+j);
	}
}

void print_garbage(int ppn) {
	if (garbagetable.list[ppn].on_garbage == 1) {
		printf("[%d]\n", ppn);
	}
}
/*						//1�� G-C �� �Լ�
void set_free(int ppn) // free_ppn ����
{
	free_ppn = ppn;
}
int get_free() {
	return free_ppn;
}
*/