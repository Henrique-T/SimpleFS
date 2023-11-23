#ifndef FS_H
#define FS_H

#include "disk.h"

class INE5412_FS
{
public:
	static const unsigned int FS_MAGIC = 0xf0f03410;
	static const unsigned short int INODES_PER_BLOCK = 128;
	static const unsigned short int POINTERS_PER_INODE = 5;
	static const unsigned short int POINTERS_PER_BLOCK = 1024;

	class fs_superblock /*A total of 16 bytes, 4 bytes each.*/
	{
	public:
		/*magic = FS_MAGIC:  rotina de formatação coloca este número nos primeiros bytes do su-
		* perbloco como um tipo de “assinatura” do sistema de arquivos.
		* Quando o sistema de arquivos é montado, o
		* SO procura por este número mágico. Se estiver correto, então assume-se que o disco 
		* contém um sistema de arquivos correto. Se algum outro número estiver presente, 
		* então a montagem falha, talvez porque o disco não esteja formatado 
		* ou contém algum outro tipo de dado.
		*
		* ninodeblocks: A rotina de formatação é responsável por escolher ninodeblocks: 
		* isto deve ser sempre 10 por cento de nblocks, arredondando pra cima.
		*/
		unsigned int magic;
		int nblocks;	  /*Total number of blocks*/
		int ninodeblocks; /*Number of blocks reserved to store inodes.*/
		int ninodes;	  /*Number of inodes in these blocks*/
	};

	class fs_inode
	{
	public:
		int isvalid; /*1 for valid*/
		int size;	 /*bytes*/
		int direct[POINTERS_PER_INODE];
		int indirect;
	};

	union fs_block
	{
	public:
		fs_superblock super;
		fs_inode inode[INODES_PER_BLOCK];
		int pointers[POINTERS_PER_BLOCK];
		char data[Disk::DISK_BLOCK_SIZE];
	};

public:
	INE5412_FS(Disk *d)
	{
		disk = d;
	}

	void fs_debug();
	int fs_format();
	int fs_mount();

	int fs_create();
	int fs_delete(int inumber);
	int fs_getsize(int inumber);

	int fs_read(int inumber, char *data, int length, int offset);
	int fs_write(int inumber, const char *data, int length, int offset);

	/* Helper functions */
	void instantiate_bitmap();
	void set_bitmap_bit_by_index(bool bit, int index);

private:
	Disk *disk;
	bool isMounted = false;
	std::vector<bool> bitmap;
};

#endif