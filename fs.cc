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
			block.inode[j].indirect = 0; // Shouldn't this be a vector?
		}
		disk->write(i, block.data);
	}

	/* Initialize and setting bitmap as the initial state */
	instantiate_bitmap();

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
		/* Reads block i+1 of disk and puts into inode block variable. */
		union fs_block inodeBlock;
		disk->read(i + 1, inodeBlock.data);

		/* Iterates over inodes of the current block */
		for (int j = 0; j < INODES_PER_BLOCK; j++)
		{
			if (inodeBlock.inode[j].isvalid)
			{
				//////// 1. PRINT INODE INFO ////////
				cout << "inode " << i * INODES_PER_BLOCK + j << ":" << endl;
				cout << "    size: " << inodeBlock.inode[j].size << " bytes" << endl;

				//////// 2. PRINT INODE DIRECT BLOCKS INFO ////////
				cout << "    direct blocks: ";
				/* Iterates over direct blocks */
				for (int k = 0; k < POINTERS_PER_INODE; k++)
				{
					if (inodeBlock.inode[j].direct[k] != 0)
					std::cout << inodeBlock.inode[j].direct[k] << " ";
				}
				cout << endl;
				//////// 3. PRINT INODE INDIRECT BLOCKS INFO ////////
				if (inodeBlock.inode[j].indirect != 0)
				{
					cout << "    indirect block: " << inodeBlock.inode[j].indirect << endl;
					cout << "    indirect data blocks: ";
					// /* Reads and iterates over indirect blocks */
					union fs_block indirectBlock;
					disk->read(inodeBlock.inode[j].indirect, indirectBlock.data);

					for (int k = 0; k < POINTERS_PER_BLOCK; k++)
					{
						if (indirectBlock.pointers[k] != 0)
							cout << indirectBlock.pointers[k] << " ";
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
	/*
	Examina o disco para um sistema de arquivos. Se um está presente, lê o superbloco, constrói um
	bitmap de blocos livres, e prepara o sistema de arquivos para uso. Retorna um em caso de sucesso, zero
	caso contrário. Note que uma montagem bem-sucedida é um pré-requisito para as outras chamadas
	*/
	if (isMounted) {
		/* Stop mount since the disk is already mounted */
		cout << "The disk is already mounted!";
		return 0;
	}


	union fs_block superblock;
	/* Reads block 0 of disk and puts into superblock variable. */
	disk->read(0, superblock.data);

	/* Mounting the disk since the superblock has a valid magic number, therefore it's a valid disk */
	if (superblock.super.magic == FS_MAGIC) {
		/* Instantiates initial bitmap */
		instantiate_bitmap();

		/* Starting of inode loop to set the bitmap at the current state*/
		int n_inodeBlocks = superblock.super.ninodeblocks;
		/* Iterates over blocks reserved to store inodes */
		for (int i = 0; i < n_inodeBlocks; i++)
		{
			union fs_block inodeBlock;
			/* Reads block i+1 of disk and puts into block variable. */
			disk->read(i + 1, inodeBlock.data);

			/* Iterates over inodes of the current block */
			for (int j = 0; j < INODES_PER_BLOCK; j++)
			{
				if (inodeBlock.inode[j].isvalid)
				{
					for (int k = 0; k < POINTERS_PER_INODE; k++)
					{
						int blockIndex = inodeBlock.inode[j].direct[k];
						if (blockIndex != 0) {
							/* Setting 1 for direct blocks referenced on inode on bitmap */
							set_bitmap_bit_by_index(blockIndex);
						}
					}

					// Iterating in indirect block pointers
					if (inodeBlock.inode[j].indirect != 0)
					{
						int indirectBlockIndex = inodeBlock.inode[j].indirect;
						union fs_block indirectBlock;
						disk->read(indirectBlockIndex, indirectBlock.data);

						/* Setting 1 for indirect block on bitmap */
						set_bitmap_bit_by_index(indirectBlockIndex);

						for (int k = 0; k < POINTERS_PER_BLOCK; k++)
						{
							int pointedBlockIndex = indirectBlock.pointers[k];
							if (indirectBlock.pointers[k] != 0) {
								/* Setting 1 for blocks referenced on indirect block on bitmap */
								set_bitmap_bit_by_index(pointedBlockIndex);
							}
						}
					}
				}
			}
		}
		/* TODO: REMOVE DEBUGGING PRINTS */

		cout << "AFTER MOUNT BITMAP\n";
		// print all elements of vector
		for(bool elem : bitmap) {
			std::cout<<elem << ", ";
		}
		std::cout<< std::endl;
		/* TODO: Check if we have to do anything else in this call besides setting the bitmap up */
		/* Setting boolean value as true if the mount was successful, along with returning 1 */	
		isMounted = true;
		return 1;
	} else {
		/* Means that the first block in the disk isn't a superblock with a valid magic number, 
		and therefore it's an unvalid disk*/
		cout << "The disk is invalid!";
		return 0;
	}


	/* Returning 0 in case of failure */
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

void INE5412_FS::instantiate_bitmap()
{
	/* Always setting the first bit as 1 for the superblock. 
	This can be done without any checks because this function is only called 
	where the presence and validity of superblock is already checked */
	union fs_block superblock;
	disk->read(0, superblock.data);

	/* Setting bitmap as a vector of 0`s */
	bitmap =  std::vector<bool>(superblock.super.nblocks, 0);

	/* TODO: REMOVE DEBUGGING PRINTS */
	cout << "PREVIOUS BITMAP\n";
	// print all elements of vector
    for(bool elem : bitmap) {
        std::cout<<elem << ", ";
    }
    std::cout<< std::endl;

	/* Setting the superblock as 1 (index 0)*/
	set_bitmap_bit_by_index(0);

	int numberOfInodeBlocks = superblock.super.ninodeblocks;

	/* Setting 1's for inode blocks */
	for (int i = 0; i < numberOfInodeBlocks; i++)
	{
		set_bitmap_bit_by_index(i + 1);
	}
    
	/* TODO: REMOVE DEBUGGING PRINTS */
	cout << "NEW BITMAP\n";
	// print all elements of vector
    for(bool elem : bitmap) {
		std::cout<<elem << ", ";
    }
    std::cout<< std::endl;
}

void INE5412_FS::set_bitmap_bit_by_index(int index)
{
	bitmap.at(index) = 1;
}