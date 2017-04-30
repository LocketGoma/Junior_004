#ifndef	_SECTOR_MAPPING_H_
#define	_SECTOR_MAPPING_H_



#define TRUE			1
#define	FALSE			0
#define	INVALID			1

#define	SECTOR_SIZE		512			
#define	SPARE_SIZE		16			
#define	PAGE_SIZE		(SECTOR_SIZE+SPARE_SIZE)
#define SECTORS_PER_PAGE	1
#define	PAGES_PER_BLOCK		4					// ���� ����
#define SECTORS_PER_BLOCK	(SECTORS_PER_PAGE*PAGES_PER_BLOCK)
#define	BLOCK_SIZE		(PAGE_SIZE*PAGES_PER_BLOCK)
#define	BLOCKS_PER_DEVICE	3					// ���� ���� - �׽�Ʈ�� ���� 3���� ����.
#define	DEVICE_SIZE		(BLOCK_SIZE*BLOCKS_PER_DEVICE)
#define DATABLKS_PER_DEVICE	(BLOCKS_PER_DEVICE - 1)			// free block�μ� ���� ����� �ϳ� ��
#define DATAPAGES_PER_DEVICE	(DATABLKS_PER_DEVICE*PAGES_PER_BLOCK)	// ���� �����͸� ������ �� �ִ� ��ü �������� ��
//
// �ʿ��ϸ� ����� �߰��ؼ� ��� ������
// 
/****************�Լ� ����********************/
void ftl_open();
void ftl_write(int lsn, char *sectorbuf);
void ftl_read(int lsn, char *sectorbuf);
void print_block(int pbn);
void print_sectormaptbl();
void print_garbage_all(); // ȣ���ϸ� garbage table ����

void test_garbage();

int read(int ppn, char *pagebuf); //file���� read
//int write(int ppn, char *pagebuf); //file�� write. lpn���� ����.
//int write(int ppn, char *pagebuf, char *lsn); //file�� write. lpn���� ����.

int erase(int pbn); // erase�� �ѹ濡 �α׸�.


//
// flash memory�� spare area�� �ٷ�� ���� ����ü
//
typedef struct
{							// ??????
	int lpn;				// page�� �����͸� ������ �� spare area�� lpn�� ���� ������
	int is_invalid;			// page�� ��ȿ���� �׷��� ������ �Ǻ��� �� ���		0 : ���� / 1 : ����
	char dummy[SPARE_SIZE - 8];

} SpareData;
int write(int ppn, char *pagebuf, SpareData s_data);

//
// FTL�� �����ϴ� address mapping table�� �� entry ����ü�μ�, �� entry�� (lpn, ppn) ������ �̷��������,
// ���� lpn�� �� �ʿ�� ����
//
typedef struct 
{
	//int lpn;			// not necessary
	int ppn;
} AddrMapTblEntry;

//
// FTL�� �����ϴ� address mapping table�� ���� ����ü
//
typedef struct
{
	AddrMapTblEntry entry[DATAPAGES_PER_DEVICE];
} AddrMapTab;

//
// �ʿ��ϸ�(���� ���, garbage collection) ����ü�� �߰��� �� ����
//
typedef struct  // GC - GC ������ : �ش� ppn �ڸ��� ����� lpn���� ���̺� �񱳽� ppn���� �ٸ��� (������ ������ ��) Garbage ����.
{
	int on_garbage; // 0 : �ƴ� / 1 : ����

}Garbage;

typedef struct { // ppn������ ��ϴ�.
	Garbage list[DATAPAGES_PER_DEVICE];
}Garbage_list;

#endif
