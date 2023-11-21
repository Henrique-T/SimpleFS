#ifndef DISK_H
#define DISK_H

#include <fstream>
#include <iostream>
#include <stdio.h>
#include <vector>

using namespace std;

/*My initial thoughts:
* 1. The bitmap should be declared here.
* 1.1 Every time the disk is mounted, the system should build a new bitmap.
*/
class Disk
{
public:
	static const unsigned short int DISK_BLOCK_SIZE = 4096;
	static const unsigned int DISK_MAGIC = 0xdeadbeef;

	Disk(const char *filename, int nblocks);

	int size();
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