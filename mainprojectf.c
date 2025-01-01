#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#define TOTAL_BLOCKS 100
#define BF 3
#define MAX_USERNAME 30
#define MAX_PASSWORD 30
#define USER_FILE "users.data"
// Define ANSI color codes for terminal output
#define GREEN "\033[1;32m"
#define RED "\033[1;31m"
#define RESET "\033[0m"
#define BLOCKS_PER_ROW 5 
#define BLOCK_WIDTH 12    // Width of each block in characters
#define BLOCK_HEIGHT 3    // Height of each block in lines
typedef struct record record;
struct record {
	int key;
	char info[30];
};
typedef struct block {
    record trec[BF];
    int nr;     // Number of records in this block
    int next;   // For chained organization: index of the next block
} block;

typedef struct meta {
    int adresse;         // Starting block address
    int nombreRecords;   // Total number of records
    int taille;          // Size of the file in blocks
    int orgGlobal;       // Global organization mode (0: Contiguous, 1: Chained)
    int orgInternal;     // Internal organization mode (0: Unsorted, 1: Sorted)
    char fileName[30];   // Name of the file
}meta;

typedef struct {
    char username[MAX_USERNAME];
    char password[MAX_PASSWORD];
} User;
void instializems(FILE *ms ){
   	       // to write the allocation table in the first bloc
   	       int allocationTable[TOTAL_BLOCKS];
   	      int i;
   	      for (i=1 ; i<TOTAL_BLOCKS; i++ ){
   	      	   allocationTable[i]=0;
			 }
   	       block firstblock;
   	      memset(&firstblock,0,sizeof(block));
   	      fwrite(allocationTable,sizeof(int),TOTAL_BLOCKS,ms);
   	      
		   // intialiser tout les blocs
		   block emptyb={.nr = 0 };
		   for (i=1;i<TOTAL_BLOCKS;i++){
		   	fwrite(&emptyb,sizeof(block),1,ms);
		   }
		   printf("the disque has been intialized with %d blocks .\n", TOTAL_BLOCKS);
}


int allocationTable[TOTAL_BLOCKS]; // 0 for free, 1 for occupied

int findFreeBlock(FILE *ms) {
	int i ,compacted;
    for ( i = 1; i < TOTAL_BLOCKS; i++) {
        if (allocationTable[i] == 0) {
            allocationTable[i] = 1; // Mark block as occupied
            fseek(ms,0,SEEK_SET);
		   	fwrite(allocationTable,sizeof(int), TOTAL_BLOCKS,ms);
            return i;
        }
    }
    
    return -1; // No free blocks available
}

void updateallocationtable(FILE*ms,int blocindex,int status){
	int allocationTable[TOTAL_BLOCKS]; 
		   	fseek(ms,0,SEEK_SET);
		   	fread (allocationTable,sizeof(int),  TOTAL_BLOCKS,ms);
		   	allocationTable[blocindex]=status;// 1 if full//0 if empty
		   	fseek(ms,0,SEEK_SET);
		   	fwrite(allocationTable,sizeof(int), TOTAL_BLOCKS,ms);
		   	
		   }
	

int allocateContiguousBlocks(FILE *ms,int requiredBlocks) {
	int i;
    for ( i = 1; i <= TOTAL_BLOCKS - requiredBlocks; i++) {
        int isFree = 1;
        int j;
        for ( j = 0; j < requiredBlocks; j++) {
            if (allocationTable[i + j] != 0) {
                isFree = 0;
                break;
            }
        }
        if (isFree) {
        	int j;
            for ( j = 0; j < requiredBlocks; j++) {
                allocationTable[i + j] = 1; // Mark these blocks as occupied
            }
            fseek(ms,0,SEEK_SET);
		   	fwrite(allocationTable,sizeof(int), TOTAL_BLOCKS,ms);
            return i; // Return the starting block index
        }
        
    }

    compactdisk(ms);
      
    for ( i = 1; i <= TOTAL_BLOCKS - requiredBlocks; i++) {
        int isFree = 1;
        int j;
        for ( j = 0; j < requiredBlocks; j++) {
            if (allocationTable[i + j] != 0) {
                isFree = 0;
                break;
            }
        }
        if (isFree) {
        	int j;
            for ( j = 0; j < requiredBlocks; j++) {
                allocationTable[i + j] = 1; // Mark these blocks as occupied
            }
            fseek(ms,0,SEEK_SET);
		   	fwrite(allocationTable,sizeof(int), TOTAL_BLOCKS,ms);
            return i; // Return the starting block index
        }
        
    }
    return -1; // No contiguous blocks available
}
int isNextBlockFree(FILE *ms, int currentBlock) {
    int allocationTable[TOTAL_BLOCKS];

    // Step 1: Read the allocation table from the first block
    fseek(ms, 0, SEEK_SET); // Go to the first block
    fread(allocationTable, sizeof(int), TOTAL_BLOCKS, ms);

    // Step 2: Check if the next block exists and is free
    if (currentBlock + 1 >= TOTAL_BLOCKS) {
        printf("Error: No next block exists beyond block %d.\n", currentBlock);
        return -1; // Indicates no next block exists
    }

    if (allocationTable[currentBlock + 1] == 0) {
        return 1; // The next block is free
    } else {
        return 0; // The next block is occupied
    }
}

void releaseBlock(int blockIndex,FILE *ms) {
    if (blockIndex >= 0 && blockIndex < TOTAL_BLOCKS) {
        allocationTable[blockIndex] = 0; // Mark block as free
        fseek(ms,0,SEEK_SET);
		   	fwrite(allocationTable,sizeof(int), TOTAL_BLOCKS,ms);
    }
}

void createFile(FILE *ms, FILE *metaFile) {
    meta fileMeta;
    block buffer;
    
    // Request file details
    printf("Enter file name: ");
    scanf("%s", fileMeta.fileName);

    printf("Enter the number of records: ");
    scanf("%d", &fileMeta.nombreRecords);

    printf("Choose global organization mode (0: Contiguous, 1: Chained): ");
    scanf("%d", &fileMeta.orgGlobal);

    printf("Choose internal organization mode (0: Unsorted, 1: Sorted): ");
    scanf("%d", &fileMeta.orgInternal);

    // Calculate required blocks
    int requiredBlocks = ceil((double)(fileMeta.nombreRecords) / BF); // Ceiling division
    fileMeta.taille = requiredBlocks;

    // Assign starting block

     fseek(metaFile, 0, SEEK_END);
    fwrite(&fileMeta, sizeof(meta), 1, metaFile);

    printf("File '%s' created successfully.\n", fileMeta.fileName);
}



void loadFile(FILE *ms, FILE *metaFile) {
    block buffer;
    meta fileMeta;
    int totalRecords = 0;
    int currentBlock, prevBlock = -1;

    // Step 1: Retrieve metadata for the file to load
    printf("Enter the file name to load: ");
    char fileName[30];
    scanf("%s", fileName);

    rewind(metaFile);
    while (fread(&fileMeta, sizeof(meta), 1, metaFile)) {
        if (strcmp(fileMeta.fileName, fileName) == 0) {
            break;
        }
    }

    if (strcmp(fileMeta.fileName, fileName) != 0) {
        printf("Error: File '%s' not found.\n", fileName);
        return;
    }

    int requiredBlocks = fileMeta.taille;
    int i,j;
    // Step 2: Start loading the records
    for ( i = 0; i < requiredBlocks; i++) {
        buffer.nr = 0;

        // Fill the block with records
        for ( j = 0; j < BF && totalRecords < fileMeta.nombreRecords; j++) {
            printf("Enter details for record %d:\n", totalRecords + 1);
            printf("ID: ");
            scanf("%d", &buffer.trec[j].key);
            printf("info: ");
            scanf(" %[^\n]", buffer.trec[j].info);
            buffer.nr++;
            totalRecords++;
        }

        // Determine the current block based on organization type
        if (fileMeta.orgGlobal == 0) { // Contiguous organization
            if (i == 0) {
                currentBlock = allocateContiguousBlocks(ms,requiredBlocks);
                if (currentBlock == -1) {
                    printf("Error: No contiguous blocks available.\n");
                    return;
                }
                fileMeta.adresse = currentBlock; // Store first block address in metadata
            }
            currentBlock = fileMeta.adresse + i;
        } else if (fileMeta.orgGlobal == 1) { // Chained organization
            currentBlock = findFreeBlock(ms);
            if (currentBlock == -1) {
                printf("Error: No free blocks available.\n");
                return;
            }

            // Link the previous block to the current block
            if (prevBlock != -1) {
                fseek(ms, prevBlock * sizeof(block), SEEK_SET);
                fread(&buffer, sizeof(block), 1, ms);
                buffer.next = currentBlock;
                fseek(ms, prevBlock * sizeof(block), SEEK_SET);
                fwrite(&buffer, sizeof(block), 1, ms);
            } else {
                fileMeta.adresse = currentBlock; // First block for chained organization
            }
        }

        // Update block metadata
        buffer.next = -1; // Default for the last block
        prevBlock = currentBlock;

        // Write the block to secondary memory
        fseek(ms, currentBlock * sizeof(block), SEEK_SET);
        fwrite(&buffer, sizeof(block), 1, ms);
    }

    // Step 3: Update file metadata
	
    fseek(metaFile, -sizeof(meta), SEEK_CUR);
    fwrite(&fileMeta, sizeof(meta), 1, metaFile);

    printf("File '%s' loaded successfully.\n", fileName);
    
}


    
void displayBlock(const char *status, const char *info, const char *color) {
    printf("%s", color); // Apply color
    printf("+------------+\n");
    printf("| %-10s |\n", status); // Display block status (e.g., "Free" or "Occupied")
    printf("| %-10s |\n", info);   // Display additional information
    printf("+------------+" RESET "\n"); // Reset color after the block
}



void displaySMStatus(FILE *ms, FILE *metaFile) {
    int allocationTable[TOTAL_BLOCKS]; // Allocation table
    meta fileMeta;
    char blockInfo[30];

    // Read the allocation table from the secondary memory file
    fseek(ms, 0, SEEK_SET);
    fread(allocationTable, sizeof(int), TOTAL_BLOCKS, ms);

    // Display blocks row by row
    printf("Secondary Memory Status:\n");
    int i;
    for ( i = 0; i < TOTAL_BLOCKS; i++) {
        if (allocationTable[i] == 0) {
            // Free block
            snprintf(blockInfo, sizeof(blockInfo), "Free");
            displayBlock("Free", blockInfo, GREEN);
        } else {
            // Occupied block
            rewind(metaFile);
            char fileInfo[30] = "Unknown"; // Default info if no metadata found

            while (fread(&fileMeta, sizeof(meta), 1, metaFile)) {
                if (fileMeta.adresse <= i && i < fileMeta.adresse + fileMeta.taille) {
                    snprintf(fileInfo, sizeof(fileInfo), "%s (%d)", fileMeta.fileName, fileMeta.nombreRecords);
                    break;
                }
            }
            displayBlock("Occupied", fileInfo, RED);
        }

        // Print a newline after every 5 blocks for better layout
        if ((i + 1) % 5 == 0) {
            printf("\n");
        }
    }
    
    printf("\n+-----------------------------------+\n");
}






void showFileMetadata(FILE *metaFile) {
	meta fileMeta;
	rewind(metaFile);
	printf("+--------------------------+------------------+------------+---------------+------------------+----------------+\n");
    printf("| File Name                | Starting Block   | Records    | Size (blocks) | Global Org Mode  | Internal Org   |\n");
    printf("+--------------------------+------------------+------------+---------------+------------------+----------------+\n");
	
    while (fread(&fileMeta, sizeof(meta), 1, metaFile)) {
    	 printf("| %-24s | %-16d | %-10d | %-13d | %-16s | %-14s |\n", 
               fileMeta.fileName,
               fileMeta.adresse,
               fileMeta.nombreRecords,
               fileMeta.taille,
               fileMeta.orgGlobal == 0 ? "Contiguous" : "Chained",
               fileMeta.orgInternal == 0 ? "Unsorted" : "Sorted");
    printf("+--------------------------+------------------+------------+---------------+------------------+----------------+\n");      
    }

}
void searchRecordByID(FILE *ms, FILE *metaFile, int searchID, int *found, int *blockNumber, int *offset) {
    meta fileMeta;
	block buffer;
    *found = 0; // Initialize as not found
    *offset = 0;
    
    
     printf("Enter file name to load: ");
    char fileName[30];
    scanf("%s", fileName);

    // Search for the metadata
    rewind(metaFile);
    while (fread(&fileMeta, sizeof(meta), 1, metaFile)) {
        if (strcmp(fileMeta.fileName, fileName) == 0) {
            break;
        }
    }

    if (strcmp(fileMeta.fileName, fileName) != 0) {
        printf("Error: File '%s' not found in metadata.\n", fileName);
        return;
    }
   printf("Metadata: Address = %d, Size = %d, GlobalOrg = %d, InternalOrg = %d\n", 
       fileMeta.adresse, fileMeta.taille, fileMeta.orgGlobal, fileMeta.orgInternal);


    if (fileMeta.orgGlobal == 0) { // Contiguous Organization
        if (fileMeta.orgInternal == 1) { // Sorted
        int bi = fileMeta.adresse; // First block
            int bs = fileMeta.adresse + fileMeta.taille - 1; // Last block
            int mid;
           while (bi <= bs && !(*found)) {
    mid = (bi + bs) / 2;

    // Read the middle block
    fseek(ms, mid * sizeof(block), SEEK_SET);
    fread(&buffer, sizeof(block), 1, ms);

    // Check if the ID is within this block
    if (searchID >= buffer.trec[0].key && searchID <= buffer.trec[buffer.nr - 1].key) {
        int inf = 0, sup = buffer.nr - 1, j;

        // Binary search within the block
        while (inf <= sup && !(*found)) {
            j = (inf + sup) / 2;

            if (buffer.trec[j].key == searchID) {
                *found = 1;
                *blockNumber = mid;
                *offset = j;
            } else if (buffer.trec[j].key > searchID) {
                sup = j - 1;
            } else {
                inf = j + 1;
            }
        }

        // If not found, inf points to the insertion position
        if (!(*found)) {
            *blockNumber = mid;
            *offset = inf;
        }
    } else if (searchID < buffer.trec[0].key) {
        bs = mid - 1; // Search the left side
    } else {
        bi = mid + 1; // Search the right side
    }
}

// If the record is not found in any block
if (!(*found) && bi > bs) {
    *blockNumber = bi;
    *offset = 0; // Should be the first position of the next block
}
        } else { // Unsorted
            // Sequential search across all blocks
            int currentBlock = fileMeta.adresse;
            int i,j;
            for ( i = 0; i < fileMeta.taille && !(*found); i++) {
                fseek(ms, currentBlock * sizeof(block), SEEK_SET);
                fread(&buffer, sizeof(block), 1, ms);

                for ( j = 0; j < buffer.nr; j++) {
                    if (buffer.trec[j].key == searchID) {
                        *found = 1;
                        *blockNumber = currentBlock;
                        *offset = j;
                        break;
                    }
                }

                currentBlock++;
            }

            // If not found, position is at the end of the last block
            if (!(*found)) {
                *blockNumber = fileMeta.adresse + fileMeta.taille - 1;
                *offset = buffer.nr;
            }
        }
    } else { // Linked Organization
        int currentBlock = fileMeta.adresse;
        int prevBlock = -1;
        int j;
        while (currentBlock != -1 && !(*found)) {
            fseek(ms, currentBlock * sizeof(block), SEEK_SET);
            fread(&buffer, sizeof(block), 1, ms);

            if (fileMeta.orgInternal == 1) { // Ordered
                // Sequential search within the block
                for ( j = 0; j < buffer.nr; j++) {
                    if (buffer.trec[j].key == searchID) {
                        *found = 1;
                        *blockNumber = currentBlock;
                        *offset = j;
                        break;
                    } else if (buffer.trec[j].key > searchID) {
                        *blockNumber = currentBlock;
                        *offset = j; // Potential insertion point
                        return;
                    }
                }
            } else { // Unordered
            int j;
                // Sequential search within the block
                for ( j = 0; j < buffer.nr; j++) {
                    if (buffer.trec[j].key == searchID) {
                        *found = 1;
                        *blockNumber = currentBlock;
                        *offset = j;
                        return;
                    }
                }
            }

            // Move to the next block in chained organization
            prevBlock = currentBlock;
            currentBlock = buffer.next;
        }

        // If not found, position is at the end of the last block
        if (!(*found)) {
            *blockNumber = prevBlock;
            *offset = buffer.nr;
        }
    }

    if (*found) {
        printf("Record found at Block: %d, Offset: %d.\n", *blockNumber, *offset);
    } else {
        printf("Record not found. Should be at Block: %d, Offset: %d.\n", *blockNumber, *offset);
    }
}



void insertRecord(FILE *ms, FILE *metaFile, int ID, const char *info) {
    meta fileMeta;
    block buffer,newBuffer;
    int found, blockNumber, offset;
     record newRecord = {ID, ""};
    strcpy(newRecord.info, info);

    // Retrieve metadata for the file
    printf("Enter the file name to insert the record: ");
    char fileName[30];
    scanf("%s", fileName);

    rewind(metaFile);
    while (fread(&fileMeta, sizeof(meta), 1, metaFile)) {
        if (strcmp(fileMeta.fileName, fileName) == 0) {
            break;
        }
    }

    if (strcmp(fileMeta.fileName, fileName) != 0) {
        printf("Error: File '%s' not found in metadata.\n", fileName);
        return;
    }

    // Use the search function to find the insertion point
    searchRecordByID(ms, metaFile, ID, &found, &blockNumber, &offset);

    if (found) {
        printf("Error: Record with ID %d already exists.\n", ID);
        return;
    }
    if (fileMeta.orgGlobal == 0 ){// contagious organization
    	if (fileMeta.orgInternal ==1){// sorted
		
    int continu = 1;
    while (continu && blockNumber <= fileMeta.taille) {
        fseek(ms, blockNumber * sizeof(block), SEEK_SET);
        fread(&buffer, sizeof(block), 1, ms);

        // Save the last record for potential overflow
        record overflowRecord = buffer.trec[buffer.nr - 1];

        // Shift records within the block to make space
        int k;
        for ( k = buffer.nr - 1; k >= offset; k--) {
            buffer.trec[k + 1] = buffer.trec[k];
        }
        buffer.trec[offset] = newRecord;

        if (buffer.nr < BF) {
            // Block is not full, insert the overflow record and stop
            buffer.nr++;
            fseek(ms, blockNumber * sizeof(block), SEEK_SET);
            fwrite(&buffer, sizeof(block), 1, ms);
            continu = 0;
        } else {
            // Block is full, write it and move the overflow record to the next block
            fseek(ms, blockNumber * sizeof(block), SEEK_SET);
            fwrite(&buffer, sizeof(block), 1, ms);
            blockNumber++;
            offset = 0;
            newRecord = overflowRecord;
            
        }
    }

    // Step 3: If we reach the end, allocate a new block for the overflow record
    if (continu) {
    	 int adjacentBlockFree = isNextBlockFree(ms, blockNumber);
        if (adjacentBlockFree == 1) {
        memset(&buffer, 0, sizeof(block));
        buffer.trec[0] = newRecord;
        buffer.nr = 1;

        fseek(ms, blockNumber * sizeof(block), SEEK_SET);
        fwrite(&buffer, sizeof(block), 1, ms);
        updateallocationtable(ms,blockNumber,1);
        fileMeta.taille = blockNumber; // Update metadata to reflect the new block
    }else{
    	printf("there is no free block after this block");
	}
    }
    }else {//unsorted
    	
    // Step 1: Locate the last block
    int lastBlock = fileMeta.adresse + fileMeta.taille - 1;
    fseek(ms, lastBlock * sizeof(block), SEEK_SET);
    fread(&buffer, sizeof(block), 1, ms);

    // Step 2: Insert the record
    if (buffer.nr < BF) {
        // If there's space in the last block, insert the record
        buffer.trec[buffer.nr] = newRecord;
        buffer.nr++;

        fseek(ms, lastBlock * sizeof(block), SEEK_SET);
        fwrite(&buffer, sizeof(block), 1, ms);

        printf("Record with ID %d inserted at Block %d, Offset %d.\n", ID, lastBlock, buffer.nr - 1);
    } else {
        // Step 3: Allocate a new block if the last block is full
        int adjacentBlockFree = isNextBlockFree(ms, blockNumber);
        if (adjacentBlockFree == 1) {
        block newBuffer = {0}; // Initialize a new block
        newBuffer.trec[0] = newRecord;
        newBuffer.nr = 1;

        // Update metadata to include the new block
        int newBlock = lastBlock + 1;
        fseek(ms, newBlock * sizeof(block), SEEK_SET);
        fwrite(&newBuffer, sizeof(block), 1, ms);
        updateallocationtable(ms,newBlock,1);
        fileMeta.taille++; // Increase the number of blocks in the file
    }else{
    		printf("there is no free block after this block");
	}
    }
	}
 }else { //linked organization 
 	 // Step 1: Locate the last block in the chain
 	int currentBlock = fileMeta.adresse;
    int prevBlock = -1;

    while (currentBlock != -1) {
        prevBlock = currentBlock;
        fseek(ms, currentBlock * sizeof(block), SEEK_SET);
        fread(&buffer, sizeof(block), 1, ms);
        currentBlock = buffer.next;
    }

    // Step 2: Insert the record
    if (buffer.nr < BF) {
        // If there's space in the last block, append the record
        buffer.trec[buffer.nr] = newRecord;
        buffer.nr++;

        fseek(ms, prevBlock * sizeof(block), SEEK_SET);
        fwrite(&buffer, sizeof(block), 1, ms);

    } else {
        // Step 3: Allocate a new block if the last block is full
        int newBlock = findFreeBlock(ms);
        if (newBlock == -1) {
            printf("Error: No free blocks available for insertion.\n");
            return;
        }

        // Update the `next` pointer of the last block
        buffer.next = newBlock;
        fseek(ms, prevBlock * sizeof(block), SEEK_SET);
        fwrite(&buffer, sizeof(block), 1, ms);

        // Initialize the new block
        memset(&newBuffer, 0, sizeof(block));
        newBuffer.trec[0] = newRecord;
        newBuffer.nr = 1;
        newBuffer.next = -1;

        fseek(ms, newBlock * sizeof(block), SEEK_SET);
        fwrite(&newBuffer, sizeof(block), 1, ms);
        updateallocationtable(ms,newBlock,1);
 	    fileMeta.taille++;
 }


// Update metadata for the file
    fileMeta.nombreRecords++;
    fseek(metaFile, -sizeof(meta), SEEK_CUR);
    fwrite(&fileMeta, sizeof(meta), 1, metaFile);

    printf("Record with ID %d successfully inserted.\n", ID);
}
}

void deleteRecordLogical(FILE *ms, FILE *metaFile, int recordID, const char *fileName) {
    meta fileMeta;
    block buffer;
    int found = 0;


    // Retrieve metadata for the file
    rewind(metaFile);
    while (fread(&fileMeta, sizeof(meta), 1, metaFile)) {
        if (strcmp(fileMeta.fileName, fileName) == 0) {
            break;
        }
    }

    if (strcmp(fileMeta.fileName, fileName) != 0) {
        printf("Error: File '%s' not found in metadata.\n", fileName);
        return;
    }

    // Search for the record
    int i,j;
    for ( i = 0; i < fileMeta.taille; i++) {
        fseek(ms, (fileMeta.adresse + i) * sizeof(block), SEEK_SET);
        fread(&buffer, sizeof(block), 1, ms);

        for ( j = 0; j < buffer.nr; j++) {
            if (buffer.trec[j].key == recordID) {
                buffer.trec[j].key = -1; // Mark as deleted
                fseek(ms, (fileMeta.adresse + i) * sizeof(block), SEEK_SET);
                fwrite(&buffer, sizeof(block), 1, ms);
                printf("Record with ID %d marked as deleted (logical deletion).\n", recordID);
                found = 1;
                break;
            }
        }
        if (found) break;
    }

    if (!found) {
        printf("Error: Record with ID %d not found.\n", recordID);
    }
}

void deleteRecordPhysical(FILE *ms, FILE *metaFile, int recordID, const char *fileName) {
    meta fileMeta;
    block buffer, nextBuffer;
    int found = 0, currentBlockIndex, currentOffset, i, j;

    // Retrieve metadata for the file
    rewind(metaFile);
    while (fread(&fileMeta, sizeof(meta), 1, metaFile)) {
        if (strcmp(fileMeta.fileName, fileName) == 0) {
            break;
        }
    }

    if (strcmp(fileMeta.fileName, fileName) != 0) {
        printf("Error: File '%s' not found in metadata.\n", fileName);
        return;
    }

    // Search for the record
    for (i = 0; i < fileMeta.taille; i++) {
        fseek(ms, (fileMeta.adresse + i) * sizeof(block), SEEK_SET);
        fread(&buffer, sizeof(block), 1, ms);

        for (j = 0; j < buffer.nr; j++) {
            if (buffer.trec[j].key == recordID) {
                found = 1;
                currentBlockIndex = fileMeta.adresse + i;
                currentOffset = j;
                break;
            }
        }
        if (found) break;
    }

    if (!found) {
    	 printf("Error: Record withID %d not found in file '%s'.\n", recordID, fileName);
    	 return;
    }

    // Step 2: Shift all records after the deleted one
    while (currentBlockIndex < fileMeta.adresse + fileMeta.taille) {
        fseek(ms, currentBlockIndex * sizeof(block), SEEK_SET);
        fread(&buffer, sizeof(block), 1, ms);

        // Shift records within the current block
        int k;
        for ( k = currentOffset; k < buffer.nr - 1; k++) {
            buffer.trec[k] = buffer.trec[k + 1];
        }

        // Handle overflow from the next block
        if (currentBlockIndex < fileMeta.adresse + fileMeta.taille - 1) {
            fseek(ms, (currentBlockIndex + 1) * sizeof(block), SEEK_SET);
            fread(&nextBuffer, sizeof(block), 1, ms);

            // Bring the first record from the next block to the current block
            buffer.trec[buffer.nr - 1] = nextBuffer.trec[0];

            // Shift records in the next block
            int k;
            for ( k = 0; k < nextBuffer.nr - 1; k++) {
                nextBuffer.trec[k] = nextBuffer.trec[k + 1];
            }

            nextBuffer.nr--; // Decrease the record count in the next block

            // Write back the updated next block
            fseek(ms, (currentBlockIndex + 1) * sizeof(block), SEEK_SET);
            fwrite(&nextBuffer, sizeof(block), 1, ms);
        } else {
            // Last block: simply decrease the number of records
            buffer.nr--;
        }

        // Write back the updated current block
        fseek(ms, currentBlockIndex * sizeof(block), SEEK_SET);
        fwrite(&buffer, sizeof(block), 1, ms);

        // Move to the next block
        currentBlockIndex++;
        currentOffset = 0; // Always start at the beginning of the next block
    }

    // Step 3: Handle the last block if it's empty
    if (buffer.nr == 0) {
        fileMeta.taille--; // Decrease the total number of blocks in the file

        // Update the allocation table to mark the block as free
        int allocationTable[TOTAL_BLOCKS];
        fseek(ms, 0, SEEK_SET);
        fread(allocationTable, sizeof(int), TOTAL_BLOCKS, ms);
        allocationTable[fileMeta.adresse + fileMeta.taille] = 0;
        fseek(ms, 0, SEEK_SET);
        fwrite(allocationTable, sizeof(int), TOTAL_BLOCKS, ms);
    }

    // Step 4: Update the metadata
    fileMeta.nombreRecords--; // Decrease the total number of records
    fseek(metaFile, -sizeof(meta), SEEK_CUR);
    fwrite(&fileMeta, sizeof(meta), 1, metaFile);

    printf("Record with ID %d deleted successfully from file '%s'.\n", recordID, fileName);
}



void reorganiserBlocs(FILE *ms, FILE *metaFile, const char *fileName) {
    meta fileMeta;
    block buffer, buffer2;
    buffer2.nr = 0; // Initialize the output block
    int newBlockIndex = 0; // Tracks the position of the new compacted block

    // Retrieve metadata for the file
    rewind(metaFile);
    while (fread(&fileMeta, sizeof(meta), 1, metaFile)) {
        if (strcmp(fileMeta.fileName, fileName) == 0) {
            break;
        }
    }

    if (strcmp(fileMeta.fileName, fileName) != 0) {
        printf("Error: File '%s' not found in metadata.\n", fileName);
        return;
    }

    // Iterate over all blocks of the file
    int i,j;
    for ( i = fileMeta.adresse; i < fileMeta.taille; i++) {
        fseek(ms, (fileMeta.adresse + i) * sizeof(block), SEEK_SET);
        fread(&buffer, sizeof(block), 1, ms);

        // Compact valid records into buffer2
        for ( j = 0; j < buffer.nr; j++) {
            if (buffer.trec[j].key != -1) { // Assuming -1 marks a deleted record
                buffer2.trec[buffer2.nr] = buffer.trec[j];
                buffer2.nr++;

                // Write buffer2 to memory if full
                if (buffer2.nr == BF) {
                    fseek(ms, (fileMeta.adresse + newBlockIndex) * sizeof(block), SEEK_SET);
                    fwrite(&buffer2, sizeof(block), 1, ms);
                    newBlockIndex++;
                    buffer2.nr = 0; // Reset buffer2
                }
            }
        }
    }

    // Write remaining records in buffer2, if any
    if (buffer2.nr > 0) {
        fseek(ms, (fileMeta.adresse + newBlockIndex) * sizeof(block), SEEK_SET);
        fwrite(&buffer2, sizeof(block), 1, ms);
        newBlockIndex++;
    }

    // Update metadata
    fileMeta.taille = newBlockIndex; // New number of blocks
    fseek(metaFile, -sizeof(meta), SEEK_CUR);
    fwrite(&fileMeta, sizeof(meta), 1, metaFile);

    printf("File '%s' defragmented successfully. Total blocks: %d.\n", fileName, newBlockIndex);
}



void renameFile(FILE *ms, FILE *metaFile) {
    meta fileMeta;
    char oldName[30], newName[30];
    
    // Request the old file name
    printf("Enter the current file name to rename: ");
    scanf("%s", oldName);
    
    // Search for the file in metadata
    rewind(metaFile);
    int found = 0;
    while (fread(&fileMeta, sizeof(meta), 1, metaFile)) {
        if (strcmp(fileMeta.fileName, oldName) == 0) {
            found = 1;
            break;
        }
    }

    if (!found) {
        printf("Error: File '%s' not found in metadata.\n", oldName);
        return;
    }

    // Request new file name
    printf("Enter the new file name: ");
    scanf("%s", newName);
    
    // Update the file name in metadata
    strcpy(fileMeta.fileName, newName);
    fseek(metaFile, -sizeof(meta), SEEK_CUR);
    fwrite(&fileMeta, sizeof(meta), 1, metaFile);

    printf("File renamed from '%s' to '%s'.\n", oldName, newName);
}  

// Function to delete a file
void deleteFile(FILE *ms, FILE *metaFile) {
    meta fileMeta;
    char fileName[30];

    // Request file name to delete
    printf("Enter the file name to delete: ");
    scanf("%s", fileName);

    // Search for the file in metadata
    rewind(metaFile);
    int found = 0;
    while (fread(&fileMeta, sizeof(meta), 1, metaFile)) {
        if (strcmp(fileMeta.fileName, fileName) == 0) {
            found = 1;
            break;
        }
    }

    if (!found) {
        printf("Error: File '%s' not found in metadata.\n", fileName);
        return;
    }
    int i;
    // Free the allocated blocks for this file
    for ( i = fileMeta.adresse; i < fileMeta.adresse + fileMeta.taille-1; i++) {
        releaseBlock(i,ms);
    }

    // Remove the file metadata from the file
    FILE *tempFile = fopen("MetaDonnees.temp", "wb+");
    rewind(metaFile);
    while (fread(&fileMeta, sizeof(meta), 1, metaFile)) {
        if (strcmp(fileMeta.fileName, fileName) != 0) {
            fwrite(&fileMeta, sizeof(meta), 1, tempFile);
        }
    }

    fclose(tempFile);
    fclose(metaFile);
    remove("MetaDonnees.data");
    rename("MetaDonnees.temp", "MetaDonnees.data");

    printf("File '%s' deleted successfully.\n", fileName);
}

void compactdisk(FILE*ms){
	   	block tempblock;
	   	int i = 1,j; // the writing index
	   	// now we need to read the allocation table to know wich block is full
	   	fseek(ms,0,SEEK_SET);
	   	fread(allocationTable,sizeof(int),TOTAL_BLOCKS,ms);
	   	// now we verify the blocks that we are going to compact and"j" is the reading index
	   	for( j=1;j<TOTAL_BLOCKS;j++){
	   		if(allocationTable[j]==1){
	   			if(j!=i){
	   				fseek(ms,sizeof(int)*TOTAL_BLOCKS+(j-1)*sizeof(block),SEEK_SET);
	   				fread(&tempblock,sizeof(block),1,ms);
	   				fseek(ms,sizeof(int)*TOTAL_BLOCKS+(i-1)*sizeof(block),SEEK_SET);
	   				fwrite(&tempblock,sizeof(block),1,ms);
	   				// here we need to update the allocation table
	   				allocationTable[j]=0;
	   				allocationTable[i]=1;
	   				
				   }
				   i++;
			   }
		   }
		   fseek(ms,0,SEEK_SET);
		   fwrite(allocationTable,sizeof(int),TOTAL_BLOCKS,ms);
		   printf("the compact is done !");
    }


void clearms(FILE*ms){
	   	block emptyb={ .nr = 0};
	   	memset(allocationTable,0,TOTAL_BLOCKS);// reintialize the allocation table 
	   	fseek(ms,sizeof(int)*TOTAL_BLOCKS,SEEK_SET);
	   	int i;
	   	for( i=1;i<TOTAL_BLOCKS; i++){
	   		fwrite(&emptyb,sizeof(block),1,ms); // to clear all the blocks 
		   }
		   fseek(ms,0,SEEK_SET);
		    fwrite(allocationTable,sizeof(int),TOTAL_BLOCKS,ms);//update the allocation table 
		    printf("the ms has been cleared");
	   	
	   }




// Function to hash passwords (simple for demonstration, improve for production)
void hashPassword(const char *password, char *hashedPassword) {
    int i;
    for (i = 0; password[i] != '\0'; i++) {
        hashedPassword[i] = password[i] + 3; // Simple Caesar cipher
    }
    hashedPassword[i] = '\0';
}

// Function to signup a new user
int signup() {
    FILE *file = fopen(USER_FILE, "ab+");
    if (!file) {
        printf("Error opening user file.\n");
        return 0;
    }

    User newUser;
    char hashedPassword[MAX_PASSWORD];

    printf("Enter a username: ");
    scanf("%s", newUser.username);

    // Check if username already exists
    User tempUser;
    rewind(file);
    while (fread(&tempUser, sizeof(User), 1, file)) {
        if (strcmp(tempUser.username, newUser.username) == 0) {
            printf("Username already exists. Please try a different username.\n");
            fclose(file);
            return 0; // Username already taken
        }
    }

    // If username is unique, proceed to get the password
    printf("Enter a password: ");
    scanf("%s", newUser.password);

    // Hash the password before saving
    hashPassword(newUser.password, hashedPassword);
    strcpy(newUser.password, hashedPassword);

    // Save the new user to the file
    fwrite(&newUser, sizeof(User), 1, file);
    printf("Signup successful! You can now log in.\n");

    fclose(file);
    return 1; // Signup success
}

// Function to log in an existing user
int login() {
    FILE *file = fopen(USER_FILE, "rb");
    if (!file) {
        printf("Error opening user file. Make sure to sign up first.\n");
        return 0;
    }

    char username[MAX_USERNAME];
    char password[MAX_PASSWORD];
    char hashedPassword[MAX_PASSWORD];

    printf("Enter your username: ");
    scanf("%s", username);

    printf("Enter your password: ");
    scanf("%s", password);

    // Hash the entered password to compare with stored hashes
    hashPassword(password, hashedPassword);

    User tempUser;
    rewind(file);
    while (fread(&tempUser, sizeof(User), 1, file)) {
        if (strcmp(tempUser.username, username) == 0 && 
            strcmp(tempUser.password, hashedPassword) == 0) {
            printf("Login successful! Welcome, %s.\n", username);
            fclose(file);
            return 1; // Login success
        }
    }

    printf("Invalid username or password. Please try again.\n");
    fclose(file);
    return 0; // Login failed
}


void menu() {
    printf("\n=== File Management System ===\n");
    printf("1. Initialize secondary memory.\n");
    printf("2. Create a file and load it \n");
    printf("3. Display SM status\n");
    printf("4. Show file metadata\n");
    printf("5. Search for a record in a file\n");
    printf("6. Insert a new record in a file \n");
    printf("7. Delete a record logically in a file\n");
    printf("8. Delete a record physically in a file\n");
    printf("9. Defragment a file\n");
    printf("10. Delete a file\n");
    printf("11. Rename a file\n");
    printf("12. Compact secondary memory\n");
    printf("13. Clear secondary memory\n");
    printf("14. Exit the program\n");
    printf("Choose an option: ");
}






void main() {
	int choice, key;
	int isAuthenticated = 0;
	int  searchID;
	char fileName[30];
	char value[30];
	int offset,found,blockNumber;
	 printf("Welcome to the File Management System!\n");
    printf("1. Log In\n");
    printf("2. Sign Up\n");

    do {
        printf("Choose an option: ");
        scanf("%d", &choice);

        if (choice == 1) {
            if (login()) {
                isAuthenticated = 1;
            }
        } else if (choice == 2) {
            signup();
            if (login()) {
                isAuthenticated = 1;
            }
        } else {
            printf("Invalid option. Please choose 1 to Log In or 2 to Sign Up.\n");
        }
    } while (!isAuthenticated);
	FILE *ms = fopen("MS.bin", "rb+");
    if (!ms) {
        ms = fopen("MS.bin", "wb+");
    }

    FILE *metaFile = fopen("MetaDonnees.data", "rb+");
    if (!metaFile) {
        metaFile = fopen("MetaDonnees.data", "wb+");
    }
	
	
     if (ms && metaFile) {
	   do {
        menu();
        scanf("%d", &choice);

        switch (choice) {
            case 1: // Instiaze secondary memory
                instializems(ms);
                break;

            case 2: // Create file and load it 
                createFile(ms, metaFile);
                loadFile(ms, metaFile);
                break;
                
            case 3: // Display SM status graphically
                displaySMStatus(ms, metaFile);
                break;  
				  
            case 4:
            	showFileMetadata(metaFile);
            	break;
            	
            case 5: // Search for a record
                printf("Enter key to search: ");
                scanf("%d", &key);
                searchRecordByID(ms,metaFile,key,&found,&blockNumber,&offset);
                break;
                
            case 6: // Insert a record
                printf("Enter key: ");
                scanf("%d", &key);
                printf("Enter value: ");
                scanf("%s", value);
                insertRecord(ms,metaFile, key, value);
                printf("Record inserted successfully.\n");
                break;

            case 7: // Delete a record logically
                printf("Enter the file name to delete from: ");
                scanf("%s", fileName);
                printf("Enter key to delete: ");
                scanf("%d", &key);
                deleteRecordLogical(ms,metaFile,key,fileName);
            
                break;

            case 8: // Delete a record physically
            printf("Enter the file name to delete from: ");
                scanf("%s", fileName);
                printf("Enter key to delete: ");
                scanf("%d", &key);
                deleteRecordPhysical(ms,metaFile,key,fileName);
                break;
                
            case 9: // Defragment a file
            printf("Enter the file name to Defragment from: ");
             scanf("%s", fileName);
                reorganiserBlocs(ms, metaFile, fileName);
                break;
                
            case 10: // Delete a file
               deleteFile(ms, metaFile); 
                break;
                
            case 11: // Rename a file
                 renameFile(ms, metaFile);
                break;
                
            case 12:// compact secondary memory
            	compactdisk(ms);
            	break;
            	
            case 13:// clear secondary memory
            	clearms(ms);
            	break;
            	
            case 14: // Exit
                printf("Exiting the program.\n");
                break;
                
            default:
                printf("Invalid choice. Please try again.\n");
        }
    } while (choice != 14);
  }else {
        printf("Error opening files.\n");
  }
    fclose(ms);
    fclose(metaFile);
    return 0;
}

