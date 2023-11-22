#include "fs.h"
#include <cmath>

int INE5412_FS::fs_format()
{

	/*
	* Rebuild all the blocks: super, inodes and data.
	*/

	/* Checks if the file system is already mounted */
	if (isMounted)
	{
		cout << "Error: File system is already mounted. Cannot format." << endl;
		return 0;
	}

	int diskSize = disk->size();

	/* Calculates the number of blocks reserved for inodes (10% of total blocks),
	* rounded up.
	*/
	int n_inodeBlocks = std::ceil(diskSize * 0.1) + 1;

	/* Initializes and writes the superblock -> first block of the disk */
	fs_superblock superblock;
	superblock.magic = FS_MAGIC;
	superblock.nblocks = diskSize;
	superblock.ninodeblocks = n_inodeBlocks;
	superblock.ninodes = n_inodeBlocks * INODES_PER_BLOCK;

	union fs_block superblockUnion;
	superblockUnion.super = superblock;
	disk->write(0, superblockUnion.data);

	/* Following the n_inodeBlocks, it sets each inode to the default values.
	* Iterates over disk blocks reserved for inodes.
	*/
	for (int i = 1; i <= n_inodeBlocks + 1; ++i)
	{
		union fs_block block;

		/* Iterates over each inode in the current block */
		for (int j = 0; j < INODES_PER_BLOCK; j++)
		{
			block.inode[j].isvalid = 0;
			block.inode[j].size = 0;

			/* Iterates over each pointer in current inode  and sets the direct pointers */
			for (int k = 0; k < POINTERS_PER_INODE; k++)
			{
				block.inode[j].direct[k] = 0;
			}

			/* Sets inderect pointer */
			block.inode[j].indirect = 0;
		}
		disk->write(i, block.data);
	}

	/*TODO: Initialize and set bitmap */

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

/*
Set isValid = false;
Set direct pointers = 0;
Reset indirect vector with 0s;
Set size = 0;
Free all data and indirect blocks data - How to do this?
Update bitmap accordindly - How to do this?
*/
int INE5412_FS::fs_delete(int inumber)
{

	/*TODO: Check if we need any more validations */
	if (!isMounted)
	{
		cout << "Error: File system is not mounted. Cannot delete." << endl;
		return 0;
	}

	if (inumber < 0)
	{
		cout << "Error: Inumber is not valid. " << endl;
		return 0;
	}

	union fs_block block;

	/* Reads and stores superblock to block variable */
	disk->read(0, block.data);

	int n_inodeBlocks = block.super.ninodeblocks;

	/* Iterates over disk blocks reserved to inodes */
	for (int i = 0; i < n_inodeBlocks; i++)
	{
		disk->read(i + 1, block.data);

		/*Iterates over inodes*/
		for (int j = 0; j < INODES_PER_BLOCK; j++)
		{
			if (j == inumber)
			{
				/* Inode is found */
				block.inode[j].isvalid = 0;
				block.inode[j].size = 0; /* QUESTION: Is it a problem to update size here?*/

				/* Iterates over direct pointers in inode and set them to zero */
				for (int k = 0; k < POINTERS_PER_INODE; k++)
				{
					block.inode[j].direct[k] = 0;
				}

				if (block.inode[j].indirect != 0)
				{
					disk->read(block.inode[j].indirect, block.data);

					/* Iterates over indirect blocks and set them to zero */
					for (int k = 0; k < POINTERS_PER_BLOCK; k++)
					{
						block.pointers[k] = 0;
					}
				}
			}
		}
	}
	return 1;
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
