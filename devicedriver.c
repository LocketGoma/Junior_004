// �갡 FTL ����
#include <stdio.h>
#include <string.h>
//#include "blkmap.h"
#include "sectormap.h"
int read(int ppn, char *pagebuf); //file���� read
//int write(int ppn, char *pagebuf); //file�� write. lpn���� ����.
//int write(int ppn, char *pagebuf, char *lsn);
int write(int ppn, char *pagebuf, SpareData s_data);

int erase(int pbn); // erase�� �ѹ濡 �α׸�.

FILE *devicefp;				// flash device file



int read(int ppn, char *pagebuf)
{
	int	ret;

	fseek(devicefp, PAGE_SIZE*ppn, SEEK_SET);
	ret = fread((void *)pagebuf, PAGE_SIZE, 1, devicefp);
	if(ret == PAGE_SIZE) {
		return 0;
	}
	else {
		if(feof(devicefp)) {
			memset((void*)pagebuf, (int)'\0', PAGE_SIZE);
			return 0;
		}
		else {
			return -1;
		}
	}
}

//int write(int ppn, char *pagebuf, char *lsn)					//lsn �޾ƿ���.	
int write(int ppn, char *pagebuf, SpareData s_data)					//lsn �޾ƿ���.	
{
	int ret;

	fseek(devicefp, PAGE_SIZE*ppn, SEEK_SET);
	ret = fwrite((void *)pagebuf, SECTOR_SIZE, 1, devicefp);
	//ret = fwrite((void *)lsn, SPARE_SIZE, 1, devicefp);
	ret = fwrite(&s_data, SPARE_SIZE, 1, devicefp);

	if(ret == PAGE_SIZE) {			
		return 0;
	}
	else {
		return -1;
	}
}
/*
int write(int ppn, char *pagebuf)				//����
{
	int ret;

	fseek(devicefp, PAGE_SIZE*ppn, SEEK_SET);
	ret = fwrite((void *)pagebuf, PAGE_SIZE, 1, devicefp);


	if (ret == PAGE_SIZE) {
		return 0;
	}
	else {
		return -1;
	}
}*/

int erase(int pbn)
{
	char blockbuf[BLOCK_SIZE];
	int	ret;

	memset((void*)blockbuf, (char)0xFF, BLOCK_SIZE);
	
	fseek(devicefp, BLOCK_SIZE*pbn, SEEK_SET);
	
	ret = fwrite((void *)blockbuf, BLOCK_SIZE, 1, devicefp);
	
	if(ret == BLOCK_SIZE) { 
		return 0;
	}
	else {
		return -1;
	}
}
