#ifndef DISK_H
#define DISK_H

#include <fstream>
#include <iostream>
#include <stdio.h>

using namespace std;

class Disk
{
public:
	static const unsigned short int DISK_BLOCK_SIZE = 4096;
	static const unsigned int DISK_MAGIC = 0xdeadbeef;

	Disk(const char *filename, int nblocks);

	int size();
	/* Toda vez que vocˆe
	 *  invocar um read ou write, vocˆe deve se assegurar 
	 *	de que data aponte para 4 KB completos de mem´oria.
	*/
	void read(int blocknum, char *data);
	void write(int blocknum, const char *data);
	void close();

private:
	void sanity_check(int blocknum, const void *data);

private:
	FILE *diskfile;
	int nblocks;
	int nreads;
	int nwrites;
};

#endif