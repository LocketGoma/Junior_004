//#define PRINT_FOR_DEBUG
#define _CRT_SECURE_NO_WARNINGS // 지우셈

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <time.h>
#include "sectormap.h"

AddrMapTab sectormaptbl; // 매핑 테이블
Garbage_list garbagetable; // 추가.
extern FILE *devicefp;

/****************  prototypes ****************/
void ftl_open();
void ftl_write(int lsn, char *sectorbuf);
void ftl_read(int lsn, char *sectorbuf);
void print_block(int pbn);
void print_sectormaptbl();

/******* Mapping Table *******/
int get_table_area(int lsn); // pbn 리턴, 비어있으면 -1
int first_empty(int freeblock_pbn); // 비어있는 page 리턴. 고쳐야됨.
int find_empty_block();


/******* Garbage Table *******/
int make_empty_page(int garbage_pbn, int freeblock_pbn);	// 멀쩡하진 않음
int get_empty_page();										// 반드시 고칠것.
int is_garbage(int ppn); // 0 or 1 리턴. garbage가 맞으면 1, 아님 0
void print_garbage_all(); // 호출하면 garbage table 리턴
void print_garbage(int ppn); // 보조함수

/*************1차 G-C용 함수*************/
//2차에서는 free_ppn = DATABLKS_PER_DEVICE;
//int free_ppn; // free ppn, 전역변수이나 함수로만 in/out 할것. 사유 : 데이터 보호 + 변환시 용이. * 직접 호출하지 말것 *
//void set_free(int ppn); // free_ppn 갱신
//int get_free(); // free_ppn 리턴


//
// flash memory를 처음 사용할 때 필요한 초기화 작업, 예를 들면 address mapping table에 대한
// 초기화 등의 작업을 수행한다
//
void ftl_open()
{
	int i;

	// initialize the address mapping table
	for(i = 0; i < DATAPAGES_PER_DEVICE; i++)
	{
		sectormaptbl.entry[i].ppn = -1; // 쓸수 있는건 DATABLKS_PER_DEVICE
		garbagetable.list[i].on_garbage = 0;
		if (i % 4 == 0)
			erase(i / 4);
	}
	//free_ppn = DATAPAGES_PER_DEVICE - 1; // 일단 맨 뒤의 것으로 초기화. <- 1차 G-C용.

	//
	// 추가적으로 필요한 작업이 있으면 수행할 것
	//

	return;
}

//
// file system을 위한 write interface
// 'sectorbuf'가 가리키는 메모리의 크기는 'SECTOR_SIZE'
//
void ftl_write(int lsn, char *sectorbuf)
{
	int ppn = -1;
	int gppn = -1; // garbage 될 것.
	char spare[SPARE_SIZE];
	char temp[SECTOR_SIZE];

#ifdef PRINT_FOR_DEBUG			// 필요 시 현재의 block mapping table을 출력해 볼 수 있음
	print_sectormaptbl_info();
#endif
	/*
	if (sectormaptbl.entry[lsn].ppn != -1) // '뭔가 값이 있었다면'
	{
		gppn = ppn;
		garbagetable.list[gppn].on_garbage = 1; // garbage 처리.
	}
	else
		;
	*/

	ppn = get_empty_page();
	printf("retrun ppn : %d\n", ppn);
	assert(ppn > -1); // -1이면 중단.
	

	_itoa(lsn, spare, 16);

	strcat(sectorbuf,spare);

	sectormaptbl.entry[lsn].ppn = ppn;				// 얘는 정상.

	printf("[[lsn : %d, ppn : %d]]\n", lsn, sectormaptbl.entry[lsn].ppn);
	write(ppn, sectorbuf);		// 얘 '섹터' 버퍼잖아?. 값 바꾼다.


	return;
}

//
// file system을 위한 read interface
// 'sectorbuf'가 가리키는 메모리의 크기는 'SECTOR_SIZE'
// 
void ftl_read(int lsn, char *sectorbuf)		
{
#ifdef PRINT_FOR_DEBUG			// 필요 시 현재의 block mapping table을 출력해 볼 수 있음
	print_sectormaptbl_info();
#endif
	int ppn = -1;
	ppn = sectormaptbl.entry[lsn].ppn;

	assert(ppn != -1);


	read(ppn, sectorbuf);

	return;
}

//
// ftl_write() 내부에서 호출되는 함수
// input : 스스로 결정해서 필요한 parameter 설정
// output : ppn
//
int get_empty_page()
{
	int ppn = -1; // -1 그대로 리턴시 에러. 넣을 '페이지'
	int pbn = -1; // 넣을 블럭.
	//
	// empty page 하나를 찾아서 또는 없으면 만드는 로직 구현(이때 free block 사용) 
	// empty page를 만들 때 아래의 make_empty_page() 함수를 구현해서 사용해도 되고, 아니면 이 함수를 사용하지 않고
	// 새로운 방법을 고안해서 해도 무방
	//

	//make_empty_page() 쓸 조건 = block 단위로 검사 시 모두 비어있는 block이 단 한개도 없을시.
	pbn = find_empty_block();
	if (pbn == -1)
		//make_empty_page(1,get_free()); // gpbn, fpbn 조건 만들기 + 찾기 // fpbn은 완료.
		make_empty_page(1, DATABLKS_PER_DEVICE);
	else
		ppn=first_empty(pbn);

	return ppn;	
}

int find_empty_block() { // empty 'block' 을 찾아주는 함수. 가장 빠른걸 찾는순간 바로 리턴함.		/ 이것도 비정상작동중.
	int is_empty = 0;
	for (int i = 0; i < DATABLKS_PER_DEVICE; i++) {
		for (int j = 0; j < PAGES_PER_BLOCK; j++) {
			if (i*PAGES_PER_BLOCK + j == -1)	// 내용물이 들어있는게 나올시
				j = PAGES_PER_BLOCK;
			else if (j == PAGES_PER_BLOCK - 1)	// continue에 한번도 안걸리고 마지막에 도달시
				is_empty = 1;					// 해당 블럭이 비어있음을 확인
		}
		if (is_empty)							// 해당 블럭이 비어있으면
			return i;							// 블럭 주소 리턴
	}
	return -1; // 못찾으면 -1
}


//
// get_empty_page() 내부에서 호출되는 함수
// input : garbage block들 중에서 선택된 하나의 'garbage block'에 존재하는 유효한 페이지들을 'free block'에 복사
// output : 복사하고 난 후 free block에 empty page가 있으면 그것들 중 첫 번째 empty page의 ppn을, 없으면 ppn=-1의 값을 리턴
// * 코드의 편의성을 위해 free block에 복사 -> 삭제 // -> 다시 free block 돌려주기를 추가 작업함. 즉, free block 위치는 항상 고정됨.

// 1차 가비지컬렉팅 : garbage block 에서 유효 페이지 freeblock로 복사 -> g-block 초기화 -> 이전 g-block이 freeblock으로 갱신
// 2차 가비지컬렉팅 : 1차 -> 이후, 다시 g-block와 freeblock을 뒤집어서 원래 freeblock 위치를 다시 freeblock으로 돌려줌.
int make_empty_page(int garbage_pbn, int freeblock_pbn) // 만들다보니 가비지컬렉팅이 됨....
{
	int garb[PAGES_PER_BLOCK]; //Garbage block 체크. 1 = 정상, 0 = garbage

	int is_empty = 0; // 0 : 비어있지 않음 / 1 : 비어있음
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
			//(freeblock_pbn*PAGES_PER_BLOCK) + i = (garbage_pbn*PAGES_PER_BLOCK) + i; // 데이터가 어디있는질 모르겠당.
		}
	
	for (int i = 0 ; i < PAGES_PER_BLOCK; i++) {
		garbagetable.list[garbage_pbn*PAGES_PER_BLOCK+i].on_garbage = 0;	// 가비지 초기화.
	}

	erase(garbage_pbn);
	//freeblock_pbn = garbage_pbn;						// 1차
	//set_free(freeblock_pbn);							// 1차

	/**************2차 G-C 용 코드*******************/
	char *temp;
	temp = (char *)malloc(sizeof(char)*BLOCK_SIZE);
	 //초기값으로 초기화.
	for (int i = 0; i < PAGES_PER_BLOCK; i++) {								
		read(freeblock_pbn*PAGES_PER_BLOCK + i, temp);			//그리고 읽은 뒤
		if (temp != 0xFF)										//초기값이 아니면 (=데이터가 있으면)
			write(garbage_pbn*PAGES_PER_BLOCK + i, temp);		//쓰기.

		memset(temp, 0xFF, BLOCK_SIZE);
	}
	erase(freeblock_pbn);										//freeblock 재 초기화.

	/**************2차 G-C 용 코드*******************/

	//if (is_empty) return first_empty(freeblock_pbn); // 단순 1차 free block 교환시 사용.

	if (is_empty) return first_empty(freeblock_pbn);	// 2차 free block 교환시 사용
	else return -1;
}
int first_empty(int freeblock_pbn) { // 위 함수 보조.					// 고쳐야됨!
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
	char *pagebuf;					// flash memory 파일 주소.
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

// 통제함수
// 1. Table 통제함수 (테이블 생성/관리)
int get_table_area(int lsn) { // ppn값 리턴.
	return sectormaptbl.entry[lsn].ppn;
}




// 2. Garbage 통제함수
int is_garbage(int ppn) { // 1 : 맞아 / 0 : 아냐
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
/*						//1차 G-C 용 함수
void set_free(int ppn) // free_ppn 갱신
{
	free_ppn = ppn;
}
int get_free() {
	return free_ppn;
}
*/