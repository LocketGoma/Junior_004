#ifndef	_SECTOR_MAPPING_H_
#define	_SECTOR_MAPPING_H_



#define TRUE			1
#define	FALSE			0
#define	INVALID			1

#define	SECTOR_SIZE		512			
#define	SPARE_SIZE		16			
#define	PAGE_SIZE		(SECTOR_SIZE+SPARE_SIZE)
#define SECTORS_PER_PAGE	1
#define	PAGES_PER_BLOCK		4					// 수정 가능
#define SECTORS_PER_BLOCK	(SECTORS_PER_PAGE*PAGES_PER_BLOCK)
#define	BLOCK_SIZE		(PAGE_SIZE*PAGES_PER_BLOCK)
#define	BLOCKS_PER_DEVICE	3					// 수정 가능 - 테스트를 위해 3으로 감소.
#define	DEVICE_SIZE		(BLOCK_SIZE*BLOCKS_PER_DEVICE)
#define DATABLKS_PER_DEVICE	(BLOCKS_PER_DEVICE - 1)			// free block로서 예비 블록을 하나 둠
#define DATAPAGES_PER_DEVICE	(DATABLKS_PER_DEVICE*PAGES_PER_BLOCK)	// 실제 데이터를 저장할 수 있는 전체 페이지의 수
//
// 필요하면 상수를 추가해서 사용 가능함
// 
/****************함수 정의********************/
void ftl_open();
void ftl_write(int lsn, char *sectorbuf);
void ftl_read(int lsn, char *sectorbuf);
void print_block(int pbn);
void print_sectormaptbl();
void print_garbage_all(); // 호출하면 garbage table 리턴

void test_garbage();

int read(int ppn, char *pagebuf); //file에서 read
//int write(int ppn, char *pagebuf); //file에 write. lpn값도 쓸듯.
//int write(int ppn, char *pagebuf, char *lsn); //file에 write. lpn값도 쓸듯.

int erase(int pbn); // erase는 한방에 싸그리.


//
// flash memory의 spare area를 다루기 위한 구조체
//
typedef struct
{							// ??????
	int lpn;				// page에 데이터를 저장할 때 spare area에 lpn도 같이 저장함
	int is_invalid;			// page가 유효한지 그렇지 않은지 판별할 때 사용		0 : 에러 / 1 : 정상
	char dummy[SPARE_SIZE - 8];

} SpareData;
int write(int ppn, char *pagebuf, SpareData s_data);

//
// FTL이 관리하는 address mapping table의 각 entry 구조체로서, 각 entry는 (lpn, ppn) 쌍으로 이루어지지만,
// 굳이 lpn을 둘 필요는 없음
//
typedef struct 
{
	//int lpn;			// not necessary
	int ppn;
} AddrMapTblEntry;

//
// FTL이 관리하는 address mapping table을 위한 구조체
//
typedef struct
{
	AddrMapTblEntry entry[DATAPAGES_PER_DEVICE];
} AddrMapTab;

//
// 필요하면(예를 들면, garbage collection) 구조체를 추가할 수 있음
//
typedef struct  // GC - GC 판정법 : 해당 ppn 자리에 저장된 lpn값을 테이블에 비교시 ppn값과 다를시 (연결이 해제될 시) Garbage 판정.
{
	int on_garbage; // 0 : 아님 / 1 : 맞음

}Garbage;

typedef struct { // ppn값으로 놉니다.
	Garbage list[DATAPAGES_PER_DEVICE];
}Garbage_list;

#endif
