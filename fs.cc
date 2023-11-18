#include "fs.h"

int INE5412_FS::fs_format()
{
	return 0;
}

void INE5412_FS::fs_debug()
{
	union fs_block block;

	/* Reads block 0 of disk and puts into block variable. */
	disk->read(0, block.data);

	cout << "superblock:\n";
	cout << "    " << (block.super.magic == FS_MAGIC ? "magic number is valid\n" : "magic number is invalid!\n");
	cout << "    " << block.super.nblocks << " blocks\n";
	cout << "    " << block.super.ninodeblocks << " inode blocks\n";
	cout << "    " << block.super.ninodes << " inodes\n";

	int n_inodeBlocks = block.super.ninodeblocks;

	/* Iterate over blocks reserved to store inodes */
	for (int i = 0; i < n_inodeBlocks; i++)
	{
		/* Reads block i+1 of disk and puts into block variable. */
		disk->read(i + 1, block.data);

		/* Iterave over inodes of the current block */
		for (int j = 0; j < INODES_PER_BLOCK; j++)
		{
			if (block.inode[j].isvalid)
			{
				//////// 1. PRINT INODE INFO ////////
				cout << "inode " << i * INODES_PER_BLOCK + j << ":" << endl;
				cout << "    size: " << block.inode[j].size << " bytes" << endl;

				//////// 2. PRINT INODE DIRECT BLOCKS INFO ////////
				cout << "    direct blocks: ";
				/* Iterate over direct blocks */
				for (int k = 0; k < POINTERS_PER_INODE; k++)
				{
					std::cout << block.inode[j].direct[k] << " ";
				}
				cout << endl;

				//////// 3. PRINT INODE INDIRECT BLOCKS INFO ////////
				if (block.inode->indirect != 0)
				{
					cout << "    indirect block: " << block.inode[j].indirect << endl;
					cout << "    indirect data blocks: ";
					/* Read and iterate over indirect blocks */
					disk->read(block.inode[j].indirect, block.data);
					for (int k = 0; k < POINTERS_PER_BLOCK; k++)
					{
						if (block.pointers[k] != 0)
							cout << block.pointers[k] << " ";
					}
					cout << endl;
				}
			}
		}
	}
}

int INE5412_FS::fs_mount()
{
	return 0;
}

int INE5412_FS::fs_create()
{
	return 0;
}

int INE5412_FS::fs_delete(int inumber)
{
	return 0;
}

int INE5412_FS::fs_getsize(int inumber)
{
	return -1;
}

int INE5412_FS::fs_read(int inumber, char *data, int length, int offset)
{
	return 0;
}

int INE5412_FS::fs_write(int inumber, const char *data, int length, int offset)
{
	return 0;
}
