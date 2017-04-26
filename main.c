#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "sectormap.h"
#pragma warning(disable:4996) // use VS

//이 파일은 없어도 됩니다. 즉, 여기에 종속되는 코드를 만들지 말것.

extern FILE *devicefp;


void initialize_flash_memory();
/***************테스트코드******************/
void data_write();
void data_read();
void read_table();
void read_garbage();
void controller();

int main(int argc, char *argv[])
{
	// 아래 세 개의 변수는 테스트할 때 필요하면 사용하기 바람
	FILE *workloadfp;
	char sectorbuf[SECTOR_SIZE];
	int lpn;

	// 가상 flash memory의 파일명은 'flashmemory'을 가정함
	devicefp = fopen("flashmemory", "w+b");	

	if(devicefp == NULL)
	{
		assert(0);
		printf("file open error\n");
		exit(1);
	}

	initialize_flash_memory();
	ftl_open();



	//
	// ftl_write() 및 ftl_read() 테스트를 위한 코드를 자유자재로 만드세요
	//
	controller();


	fclose(devicefp);
	//assert(0);
	return 0;
}

//
// initialize flash memory where each byte are set to 'OxFF'
// 
void initialize_flash_memory()
{
    	char *blockbuf;
	int i;

	blockbuf = (char *)malloc(BLOCK_SIZE);
	memset(blockbuf, 0xFF, BLOCK_SIZE);

	for(i = 0; i < BLOCKS_PER_DEVICE; i++)
	{
		fwrite(blockbuf, BLOCK_SIZE, 1, devicefp);
	}

	free(blockbuf);

	return;
}


void controller() {
	int lever=0;
	while (1) {
		printf("------------------------------\n");
		printf("메뉴를 눌러주세요 \n");
		printf("1:데이터쓰기 / 2:데이터읽기 / 3:데이터테이블확인 / 4:가비지테이블확인 / 0:종료\n");
		scanf("%d", &lever);
		switch (lever) {
		case 1:	data_write();	break;
		case 2:	data_read();	break;
		case 3: read_table();	break;
		case 4: read_garbage();	break;
		case 0:	return 0;		break;
		default:				break;
		}
	}
}

void data_write() {
	char temp[SECTOR_SIZE];
	int lsn;
	printf("입력할 데이터를 입력하세요\n");
	scanf("%s", temp);
	printf("입력할 주소를 입력하세요\n");
	scanf("%d", &lsn);


	ftl_write(lsn, temp);
}
void data_read() {
	char temp[SECTOR_SIZE];
	int lsn;
	printf("읽을 주소를 입력하세요\n");
	scanf("%d", &lsn);
		ftl_read(lsn, temp);
		printf("[%d]::%s\n",lsn ,temp);
}
void read_table() {
	print_sectormaptbl();
}
void read_garbage() {
	print_garbage_all();
}