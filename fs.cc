#include "fs.h"

int INE5412_FS::fs_format()
{
	bool isMounted = true;
	int diskSize = disk->size();

	/* Checks if the file system is already mounted */
	if (isMounted)
	{
		cout << "Error: File system is already mounted. Cannot format." << endl;
		return 0;
	}

	/* Calculates the number of blocks reserved for inodes (10% of total blocks) */
	int n_inodeBlocks = diskSize * 0.1;

	/* Clears data on the disk (write zeros to all blocks) */
	for (int i = 0; i < diskSize; ++i)
	{
		disk->write(i, 0);
	}

	/* Reserves blocks for inodes (mark as allocated) */
	for (int i = 0; i <= n_inodeBlocks; ++i)
	{
		//disk->markBlockAllocated(i);
	}

	/* Initializes and writes the superblock */
	fs_superblock superblock;
	superblock.magic = FS_MAGIC;
	superblock.nblocks = diskSize;
	superblock.ninodeblocks = n_inodeBlocks;
	superblock.ninodes = n_inodeBlocks * INODES_PER_BLOCK;

	union fs_block superblockUnion;
	superblockUnion.super = superblock;
	disk->write(0, superblockUnion.data);

	/*WARNING: This is only the initial sketch for how we'd handle the bitmap.
	* The bitmap should be declared in the disk files
	*/

	/* Builds a new bitmap and writes it to the disk */
	// int bitmapSize = diskSize / 8;		   // Assuming one byte per block
	// char *bitmap = new char[bitmapSize](); // Initialize with zeros

	/*  Marks allocated blocks in the bitmap */
	// for (int i = 0; i <= n_inodeBlocks; ++i)
	// {
	// 	//setBit(bitmap, i);
	// }

	/* Writes the bitmap to the disk */
	// disk->write(1, bitmap); // Assuming the bitmap starts from block 1

	/* Cleans up allocated memory*/
	// delete[] bitmap;

	// Return success
	return 1;
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

	/* Iterates over blocks reserved to store inodes */
	for (int i = 0; i < n_inodeBlocks; i++)
	{
		/* Reads block i+1 of disk and puts into block variable. */
		disk->read(i + 1, block.data);

		/* Iterates over inodes of the current block */
		for (int j = 0; j < INODES_PER_BLOCK; j++)
		{
			if (block.inode[j].isvalid)
			{
				//////// 1. PRINT INODE INFO ////////
				cout << "inode " << i * INODES_PER_BLOCK + j << ":" << endl;
				cout << "    size: " << block.inode[j].size << " bytes" << endl;

				//////// 2. PRINT INODE DIRECT BLOCKS INFO ////////
				cout << "    direct blocks: ";
				/* Iterates over direct blocks */
				for (int k = 0; k < POINTERS_PER_INODE; k++)
				{
					std::cout << block.inode[j].direct[k] << " ";
				}
				cout << endl;

				//////// 3. PRINT INODE INDIRECT BLOCKS INFO ////////
				if (block.inode[j].indirect != 0)
				{
					cout << "    indirect block: " << block.inode[j].indirect << endl;
					cout << "    indirect data blocks: ";
					// /* Reads and iterates over indirect blocks */
					// WARNING: this read() makes inode 3 to not showup in the logs anymore
					disk->read(block.inode[j].indirect, block.data);
					for (int k = 0; k < POINTERS_PER_BLOCK; k++)
					{
						if (block.pointers[k] != 0)
							cout << block.pointers[k] << " ";
					}
					cout << endl;
				}
				cout << endl;
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
	// QUESTION: do we need to verify if the disk is not mounted?

	/* Gets the number of the block that holds the inode */
	int blockNumber = 1 + inumber / INODES_PER_BLOCK;

	/* Gets the index of the inode inside the block 
	* The % operation simulates a circular list.
	*/
	int inodeIndex = inumber % INODES_PER_BLOCK;

	/* Reads the block and stores in block.data */
	union fs_block block;
	disk->read(blockNumber, block.data);

	/* Gets inode and does validation */
	fs_inode inode = block.inode[inodeIndex];
	return (inode.isvalid != 1) ? -1 : inode.size;
}

int INE5412_FS::fs_read(int inumber, char *data, int length, int offset)
{
	return 0;
}

int INE5412_FS::fs_write(int inumber, const char *data, int length, int offset)
{
	return 0;
}
