#include "fs.h"
#include <cmath>
#include <cstring> // for memcpy
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// std::find for vectors
#include <bits/stdc++.h>

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
	int n_inodeBlocks = std::ceil(diskSize * 0.1);

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

	/* Initialize and setting bitmap as the initial state */
	instantiate_bitmap();

	return 1;
}

void INE5412_FS::fs_debug()
{
	if (!isMounted)
	{
		cout << "Error: File system is not mounted. Cannot delete." << endl;
		return;
	}

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
				cout << "inode " << (i * INODES_PER_BLOCK + j) + 1 << ":" << endl;
				cout << "    size: " << inodeBlock.inode[j].size << " bytes" << endl;

				//////// 2. PRINT INODE DIRECT BLOCKS INFO ////////
				
				bool wasPrinted = false;
				/* Iterates over direct blocks */
				for (int k = 0; k < POINTERS_PER_INODE; k++)
				{
					if (inodeBlock.inode[j].direct[k] != 0)
					{
						if (!wasPrinted) {
							cout << "    direct blocks: ";
						}
						cout << inodeBlock.inode[j].direct[k] << " ";
						wasPrinted = true;
					}
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
							set_bitmap_bit_by_index(1, blockIndex);
						}
					}

					// Iterating in indirect block pointers
					if (inodeBlock.inode[j].indirect != 0)
					{
						int indirectBlockIndex = inodeBlock.inode[j].indirect;
						union fs_block indirectBlock;
						disk->read(indirectBlockIndex, indirectBlock.data);

						/* Setting 1 for indirect block on bitmap */
						set_bitmap_bit_by_index(1, indirectBlockIndex);

						for (int k = 0; k < POINTERS_PER_BLOCK; k++)
						{
							int pointedBlockIndex = indirectBlock.pointers[k];
							if (indirectBlock.pointers[k] != 0) {
								/* Setting 1 for blocks referenced on indirect block on bitmap */
								set_bitmap_bit_by_index(1, pointedBlockIndex);
							}
						}
					}
				}
			}
		}
		/* Setting boolean value as true if the mount was successful, along with returning 1 */	
		isMounted = true;
		return 1;
	} else {
		/* Means that the first block in the disk isn't a superblock with a valid magic number, 
		and therefore it's an unvalid disk*/
		cout << "The disk is invalid!";
		return 0;
	}
}

int INE5412_FS::fs_create()
{
	/* 
	Cria um novo inodo de comprimento zero. Em caso de sucesso, retorna o inúmero (positivo). Em
	caso de falha, retorna zero. (Note que isto implica que zero não pode ser um inúmero válido.)
	*/
	if (!isMounted) {
		cout << "File System is not yet mounted!";
		return 0;
	}

	union fs_block superblock;
	disk->read(0, superblock.data);

	int numberOfInodeBlocks = superblock.super.ninodeblocks;

	/* Searching for the first invalid inode */

	fs_inode inode;
	inode.isvalid = 0;
	/* Setting inumber default number as 0, since it's the failure number */
	int inumber = 0;

	for (int i = 0; i < numberOfInodeBlocks; i++)
	{
		if (inode.isvalid) {
			/* Means that an invalid inode was already found on the inner loop below */
			break;
		}

		union fs_block inodeBlock;
		disk->read(i + 1, inodeBlock.data);
		
		for (int j = 0; j < INODES_PER_BLOCK; j++)
		{
			if (inodeBlock.inode[j].isvalid == 0) {
				inode = inodeBlock.inode[j];
				
				inode.isvalid = 1;
				inode.size = 0;
				inode.indirect = 0;
				inode.size = 0;
				for (int k = 0; k < POINTERS_PER_INODE; k++)
				{
					inode.direct[k] = 0;
				}

				inumber = (i * INODES_PER_BLOCK + j) + 1;
				inodeBlock.inode[j] = inode;

				/* Breaking the loop as soon as we find an invalid inode */
				disk->write(i + 1, inodeBlock.data);

				/* We always update the bitmap number to 1 for the inode block in case of a successful inode creation */
				set_bitmap_bit_by_index(1, i + 1);
				break;
			}
		}
	}

	return inumber;
}

int INE5412_FS::fs_delete(int inumber)
{
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

	union fs_block superblock;
	/* Reads and stores superblock to block variable */
	disk->read(0, superblock.data);

	int numberOfInodeBlocks = superblock.super.ninodeblocks;

	if (inumber > superblock.super.ninodes) {
		cout << "Inumber is invalid. (bigger than the amount of inodes.)" << endl;
		return 0;
	}

	bool wasInodeFound = false;
	/* Iterates over disk blocks reserved to inodes */
	for (int i = 0; i < numberOfInodeBlocks; i++)
	{
		if (wasInodeFound) {
			/* Avoiding extra iterations after the inode block has been already deleted */
			break;
		}
		union fs_block inodeBlock;
		disk->read(i + 1, inodeBlock.data);

		/*Iterates over inodes*/
		for (int j = 0; j < INODES_PER_BLOCK; j++)
		{
			int currentINumber = (i * INODES_PER_BLOCK + j) + 1;
			if (currentINumber == inumber)
			{
				if (!inodeBlock.inode[j].isvalid) 
				{
					cout << "Inode doesn't exist." << endl;
					return 0;
				}
				/* Inode is found */
				inodeBlock.inode[j].isvalid = 0;
				inodeBlock.inode[j].size = 0; 

				/* Iterates over direct pointers in inode and set them to zero */
				for (int k = 0; k < POINTERS_PER_INODE; k++)
				{
					if (inodeBlock.inode[j].direct[k] != 0)
					{
						int directBlockIndex = inodeBlock.inode[j].direct[k];
						set_bitmap_bit_by_index(0, directBlockIndex);
						/* Erasing diect pointers from inode */
						inodeBlock.inode[j].direct[k] = 0;
					}
				}

				if (inodeBlock.inode[j].indirect != 0)
				{
					int indirectBlockIndex = inodeBlock.inode[j].indirect;
					union fs_block indirectBlock;
					disk->read(indirectBlockIndex, indirectBlock.data);

					/* Iterates over indirect blocks and set them to zero */
					for (int k = 0; k < POINTERS_PER_BLOCK; k++)
					{
						if (indirectBlock.pointers[k] != 0)
						{
							int pointedIndirectBlock = indirectBlock.pointers[k];
							set_bitmap_bit_by_index(0, pointedIndirectBlock);
						}
					}
					/* Erasing indirect pointer from inode */
					inodeBlock.inode[j].indirect = 0;
					
					/* Freeing indirect blocks from bitmap */
					set_bitmap_bit_by_index(0, indirectBlockIndex);
				}
				disk->write(i + 1, inodeBlock.data);
				wasInodeFound = true;
				break;
			} 
		}
	}
	return 1;
}

int INE5412_FS::fs_getsize(int inumber)
{
	if (!isMounted) {
		cout << "File System is not yet mounted!";
		return -1;
	}

	union fs_block superblock;
	/* Reads and stores superblock to block variable */
	disk->read(0, superblock.data);

	if (inumber > superblock.super.ninodes || inumber == 0) {
		cout << "Inumber is invalid. (bigger than the amount of inodes or equals to 0.)" << endl;
		return -1;
	}

	/* 
	Finds the exact block and inode index inside of this block
	This operation will always be valid since we already checked if inumber is valid
	 */
	int blockIndex = 1 + inumber / INODES_PER_BLOCK;
	/* Adding one to the modulo since inumbers always start in 1 */
	int inodeIndexInBlock = (inumber - 1) % INODES_PER_BLOCK;

	/* Reads the block and stores in block.data */
	union fs_block blockWithInode;
	disk->read(blockIndex, blockWithInode.data);

	/* Gets the exact inode requested by the inumber */
	fs_inode inode = blockWithInode.inode[inodeIndexInBlock];
	if (inode.isvalid)
	{
		return inode.size;
	}
	return -1;
}

int INE5412_FS::fs_read(int inumber, char *data, int length, int offset)
{
	if (!isMounted)
	{
		cout << "File System is not yet mounted!";
		return 0;
	}

	union fs_block superblock;
	/* Reads and stores superblock to block variable */
	disk->read(0, superblock.data);

	if (inumber > superblock.super.ninodes || inumber == 0)
	{
		cout << "Inumber is invalid. (bigger than the amount of inodes or equals to 0.)" << endl;
		return 0;
	}

	/* 
	Finds the exact block and inode index inside of this block
	This operation will always be valid since we already checked if inumber is valid
	 */
	int blockIndex = 1 + inumber / INODES_PER_BLOCK;
	/* Adding one to the modulo since inumbers always start in 1 */
	int inodeIndexInBlock = (inumber - 1) % INODES_PER_BLOCK;

	/* Reads the block and stores in block.data */
	union fs_block blockWithInode;
	disk->read(blockIndex, blockWithInode.data);

	/* Gets the exact inode requested by the inumber */
	fs_inode inode = blockWithInode.inode[inodeIndexInBlock];

	if (!inode.isvalid)
	{
		cout << "Inode is invalid." << endl;
		return 0;
	}

	if (offset > inode.size)
	{
		cout << "Offset is invalid (bigger than inode size)." << endl;
		return 0;
	}

	/*
	* Offset + length cannot be bigger then inode size.
	* If that's the case, we adjust length to include everything until the end
	* of the inode data.
	*/

	if (offset + length > inode.size)
	{
		length = inode.size - offset;
	}

	/* Total read bytes */
	int readBytes = 0;
	/* Calculates the starting block for data */
	int startBlock = offset / Disk::DISK_BLOCK_SIZE;
	/* Calculates the starting offset within the first block */
	int startOffset = offset % Disk::DISK_BLOCK_SIZE;

	/* Iterates over direct blocks, starting from the start block index calculated above */
	for (int i = startBlock; i < POINTERS_PER_INODE; ++i)
	{
		if (readBytes == length)
		{
			break;
		}
		/* 
		* Reads the inode direct block and stores in blockWithInode.data.
		*/
		int pointedBlockIndex = blockWithInode.inode[inodeIndexInBlock].direct[i];

		union fs_block pointedBlock;
		disk->read(pointedBlockIndex, pointedBlock.data);

		/*
		* Calculates the number of bytes to copy in this block.
		* The minimum value between the remaining bytes to read and 
		* the available space in the current data block.
		*/
		int bytesToCopy = min(length - readBytes, Disk::DISK_BLOCK_SIZE - startOffset);

		/* Copy data from the block to the output buffer */
		memcpy(data + readBytes, pointedBlock.data + startOffset, bytesToCopy);

		/* Updates counter and resets startOffset for subsequent blocks */
		readBytes += bytesToCopy;
		startOffset = 0;
		/* We add the start block amount for the second iteration, where it will read a mix of one indirect block and 3 indirect blocks
		(4 total blocks = 16384 bytes, buffer's size)*/
		startBlock++;
	}

	/* Checks if more data needs to be read from the indirect block */
	if (readBytes < length && blockWithInode.inode[inodeIndexInBlock].indirect != 0)
	{
		/* 
		* Reads the inode indirect block and stores in blockWithInode.data.
		*/
		int indirectBlockIndex = blockWithInode.inode[inodeIndexInBlock].indirect;
		union fs_block indirectBlock;
		disk->read(indirectBlockIndex, indirectBlock.data);

		/* Iterates over indirect blocks */
		for (int i = (startBlock - POINTERS_PER_INODE); i < POINTERS_PER_BLOCK; ++i)
		{
			if (readBytes == length)
			{
				break;
			}
			/* 
			* Reads the inode data block and stores in blockWithInode.data.
			*/
			int indirectPointedBlockIndex = indirectBlock.pointers[i];
			union fs_block indirectPointedBlock;
			disk->read(indirectPointedBlockIndex, indirectPointedBlock.data);

			/* 
			* Calculates the number of bytes to copy in this block.
			* Added the -0 to adjust the type. Too lazy to search for a better solution rn.
			*/
			int bytesToCopy = min(length - readBytes, Disk::DISK_BLOCK_SIZE - 0);

			/* Copies data from the block to the output buffer */
			memcpy(data + readBytes, indirectPointedBlock.data, bytesToCopy);

			/*  Update counters */
			readBytes += bytesToCopy;
		}
	}

	return readBytes;
}

int INE5412_FS::fs_write(int inumber, const char *data, int length, int offset)
{
	if (!isMounted) {
		cout << "File System is not yet mounted!";
		return 0;
	}

	union fs_block superblock;
	/* Reads and stores superblock to block variable */
	disk->read(0, superblock.data);

	if (inumber > superblock.super.ninodes || inumber == 0) {
		cout << "Inumber is invalid. (bigger than the amount of inodes or equals to 0.)" << endl;
		return 0;
	}

	/* 
	Finds the exact block and inode index inside of this block
	This operation will always be valid since we already checked if inumber is valid
	 */
	int blockWithInodeIndex = 1 + inumber / INODES_PER_BLOCK;
	/* Adding one to the modulo since inumbers always start in 1 */
	int inodeIndexInBlock = (inumber - 1) % INODES_PER_BLOCK;

	/* Reads the block and stores in block.data */
	union fs_block blockWithInode;
	disk->read(blockWithInodeIndex, blockWithInode.data);

	/* Gets the exact inode requested by the inumber */
	fs_inode inode = blockWithInode.inode[inodeIndexInBlock];

	if (!inode.isvalid) {
		cout << "Inode is invalid. Aborting write..." << endl;
		return 0;
	}

	/* After beginning overriding the current inode (if there's any data anyway), we free all its blocks to the bitmap */
	if (offset == 0)
	{
		/* We only erase the inode on the first iteration, that is, when offset equals to 0. */
		erase_entire_inode(inumber);
	}

	int writeOffset = offset;
	
	/* Reads the block and stores in block.data */
	disk->read(blockWithInodeIndex, blockWithInode.data);

	/* Gets the exact inode requested by the inumber */
	inode = blockWithInode.inode[inodeIndexInBlock];

	int writtenBytes = 0;

	/* 
	Iterates over direct blocks only in the first iteration.
	This is because starting from the next iterations (offset > 0), 
	the direct blocks are already gonna be set and cannot be overriden 
	*/
	if (writtenBytes < length)
	{
		for (int i = 0; i < POINTERS_PER_INODE; ++i)
		{
			if (writtenBytes == length)
			{
				break;
			}

			if (inode.direct[i] == 0)
			{
				/* 
				If the current pointer of inode points to a null block, we are allocating a free block from the bitmap, 
				replacing the null pointer to the former free block that will be now used.
				*/
				int freeBlockIndex = find_first_free_block();
				if (freeBlockIndex != -1 )
				{
					inode.direct[i] = freeBlockIndex;
					/* Updating direct pointer of inode from block with the inode */

					union fs_block freeBlock;

					/*
					* Calculates the number of bytes to copy to this block.
					* The minimum value between the remaining bytes to read and 
					* the available space in the current data block.
					*/
					int bytesToCopy = min(length - writtenBytes, Disk::DISK_BLOCK_SIZE - 0);

					/* Copy data from the data pointer to the block */
					memcpy(freeBlock.data, data + writtenBytes, bytesToCopy);

					/* Updates counter and resets startOffset for subsequent blocks */
					writtenBytes += bytesToCopy;
					/* After the first write, the next blocks won't have any offset */
					writeOffset = 0;

					/* Updating the allocated block and updating it on the bitmap */
					set_bitmap_bit_by_index(1, freeBlockIndex);
					disk->write(freeBlockIndex, freeBlock.data);
				} else {
					cout << "DISK FULL!!!!" << endl;
					break;
				}
			} 
		}
	}

	/* Checks if more data needs to be written in indirect blocks */
	if (writtenBytes < length && offset != 0)
	{
		/* 
		If the current pointer of inode points to a null block, we are allocating a free block from the bitmap, 
		replacing the null pointer to the former free block that will be now used.
		*/
		int indirectBlockIndex;

		if (inode.indirect == 0)
		{
			indirectBlockIndex = find_first_free_block();
			if (indirectBlockIndex == -1 || find_first_free_block() == -1) {
				cout << "DISK FULL!!!!" << endl;
				/* If it wasn't possible to allocate any other further free blocks,  then free the indirect block, since it points to nothing */
			} else {
				/* If there are further free blocks, continue with the operation, erasing previous pointers in the allocated indirect block */
				erase_indirect_block(indirectBlockIndex);
				inode.indirect = indirectBlockIndex;
			}
			
		} else {
			indirectBlockIndex = inode.indirect;
		}
		
		if (indirectBlockIndex == -1 ) {
			cout << "DISK FULL!!!!" << endl;
		} else {
			inode.indirect = indirectBlockIndex;

			fs_block indirectBlock;
			/* Allocating certain free block as an indirect block, and blocking from the bitmap */		
			disk->read(indirectBlockIndex, indirectBlock.data);

			set_bitmap_bit_by_index(1, indirectBlockIndex);

			/* Iterates over indirect blocks */
			for (int i = 0; i < POINTERS_PER_BLOCK; ++i)
			{
				if (writtenBytes == length)
				{
					break;
				}
				if (indirectBlock.pointers[i] == 0)
				{
					int freeBlockIndex = find_first_free_block();
					if (freeBlockIndex == -1)
					{
						cout << "DISK FULL!!!!" << endl;
						break;
					}
					erase_indirect_block(freeBlockIndex);

					/* Setting allocated block to pointer of indirect block */
					indirectBlock.pointers[i] = freeBlockIndex;

					union fs_block indirectPointedBlock;

					set_bitmap_bit_by_index(1, freeBlockIndex);
					int bytesToCopy = min(length - writtenBytes, Disk::DISK_BLOCK_SIZE - 0);

					/* Copies data from the output buffer to the block */
					memcpy(indirectPointedBlock.data, data + writtenBytes, bytesToCopy);

					/* Updating block with new written data in disk */
					disk->write(freeBlockIndex, indirectPointedBlock.data);

					/*  Update counters */
					writtenBytes += bytesToCopy;
				}
			}
			/* Updating indirect block with new pointers */
			disk->write(indirectBlockIndex, indirectBlock.data);
		}
	}

	/* We only override the inode size on the first iteration, that is, when offset equals to 0.*/
	if (offset == 0)
	{
		inode.size = writtenBytes;
	} else {
		inode.size += writtenBytes;
	}
	
	blockWithInode.inode[inodeIndexInBlock] = inode;
	disk->write(blockWithInodeIndex, blockWithInode.data);

	return writtenBytes;
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

	/* Setting the superblock as 1 (index 0)*/
	set_bitmap_bit_by_index(1, 0);

	int numberOfInodeBlocks = superblock.super.ninodeblocks;

	/* Setting 1's for inode blocks */
	for (int i = 0; i < numberOfInodeBlocks; i++)
	{
		union fs_block inodeBlock;
		disk->read(i + 1, inodeBlock.data);
		for (int j = 0; j < INODES_PER_BLOCK; j++)
		{
			if (inodeBlock.inode[j].isvalid) {
				/* If at least one inode in the inode block is valid, then the block is in use */
				set_bitmap_bit_by_index(1, i + 1);
				break;
			}
		}
	}
}

void INE5412_FS::set_bitmap_bit_by_index(bool bit, int index)
{
	bitmap.at(index) = bit;
}

int INE5412_FS::find_first_free_block()
{
	union fs_block superblock;
	/* Reads and stores superblock to block variable */
	disk->read(0, superblock.data);

	/* The start index will be the number of inode blocks + 1 (superblock) */
	int startIndex = superblock.super.ninodeblocks + 1;

	int pos = -1;
	for (int i = startIndex; i < bitmap.size(); i++)
	{
		if (bitmap.at(i) == 0)
		{
			pos = i;
			break;
		}
	}

	if (pos == -1)
	{
		cout << "ERROR! There are no free blocks!" << endl;
	}
	return pos;
}

void INE5412_FS::erase_entire_inode(int inumber)
{
	int blockWithInodeIndex = 1 + inumber / INODES_PER_BLOCK;
	/* Adding one to the modulo since inumbers always start in 1 */
	int inodeIndexInBlock = (inumber - 1) % INODES_PER_BLOCK;

	/* Reads the block and stores in block.data */
	union fs_block blockWithInode;
	disk->read(blockWithInodeIndex, blockWithInode.data);

	/* Gets the exact inode requested by the inumber */
	fs_inode inode = blockWithInode.inode[inodeIndexInBlock];

	for (int k = 0; k < POINTERS_PER_INODE; k++)
	{
		if (inode.direct[k] != 0)
		{
			set_bitmap_bit_by_index(0, inode.direct[k]);
			inode.direct[k] = 0;
		}
	}

	if (inode.indirect != 0)
	{
		int indirectBlockIndex = inode.indirect;
		union fs_block indirectBlock;
		disk->read(indirectBlockIndex, indirectBlock.data);

		/* Iterates over indirect blocks and set them to zero */
		for (int k = 0; k < POINTERS_PER_BLOCK; k++)
		{
			if (indirectBlock.pointers[k] != 0)
			{
				set_bitmap_bit_by_index(0, indirectBlock.pointers[k]);
				indirectBlock.pointers[k] = 0;
			}
		}
		set_bitmap_bit_by_index(0, inode.indirect);
		inode.indirect = 0;
	}
	inode.size = 0;

	blockWithInode.inode[inodeIndexInBlock] = inode;
	disk->write(blockWithInodeIndex, blockWithInode.data);
}

void INE5412_FS::erase_indirect_block(int blockIndex)
{
	/* TO BE CALLED WHEN ALLOCATING A BLOCK TO BE AN INDIRECT BLOCK, so we avoid utilizing 'dirty' blocks */
	fs_block emptyBlock;
	for (int i = 0; i < POINTERS_PER_BLOCK; ++i) {
        emptyBlock.pointers[i] = 0;
    }
	disk->write(blockIndex, emptyBlock.data);
}
