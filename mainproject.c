#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#define TOTAL_BLOCKS 100
#define BF 3
typedef struct record record;
struct record {
	int key;
	char info[30];
};
typedef struct block block;
typedef struct block {
    record trec[BF];
    int nr;     // Number of records in this block
    int next;   // For chained organization: index of the next block
} ;
typedef struct meta meta;
typedef struct meta {
    int adresse;         // Starting block address
    int nombreRecords;   // Total number of records
    int taille;          // Size of the file in blocks
    int orgGlobal;       // Global organization mode (0: Contiguous, 1: Chained)
    int orgInternal;     // Internal organization mode (0: Unsorted, 1: Sorted)
    char fileName[30];   // Name of the file
} ;


int allocationTable[TOTAL_BLOCKS]; // 0 for free, 1 for occupied

void initializeAllocationTable() {
	int i;
    for ( i = 0; i < TOTAL_BLOCKS; i++) {
        allocationTable[i] = 0; // Mark all blocks as free
    }
}

int findFreeBlock() {
	int i ;
    for ( i = 0; i < TOTAL_BLOCKS; i++) {
        if (allocationTable[i] == 0) {
            allocationTable[i] = 1; // Mark block as occupied
            return i;
        }
    }
    return -1; // No free blocks available
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
            return i; // Return the starting block index
        }
        fseek(ms,0,SEEK_SET);
		   	fwrite(allocationTable,sizeof(int), requiredBlocks,ms);
    }
    return -1; // No contiguous blocks available
}

void releaseBlock(int blockIndex) {
    if (blockIndex >= 0 && blockIndex < TOTAL_BLOCKS) {
        allocationTable[blockIndex] = 0; // Mark block as free
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
            currentBlock = findFreeBlock();
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

void printFileContent(FILE *ms, FILE *metaFile, const char *fileName) {
    meta fileMeta;
    block buffer;
    int currentBlock, totalRecordsPrinted = 0;

    // Search for the file metadata
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

    printf("Content of file '%s':\n", fileName);

    // Access and print records based on the organization
    currentBlock = fileMeta.adresse;
    while (currentBlock != -1 && totalRecordsPrinted < fileMeta.nombreRecords) {
        fseek(ms, currentBlock * sizeof(block), SEEK_SET);
        fread(&buffer, sizeof(block), 1, ms);
        int i;
        for ( i = 0; i < buffer.nr; i++) {
            printf("Record %d:\n", totalRecordsPrinted + 1);
            printf("  ID: %d\n", buffer.trec[i].key);
            printf("  info: %s\n", buffer.trec[i].info);
            totalRecordsPrinted++;

            if (totalRecordsPrinted >= fileMeta.nombreRecords) {
                break;
            }
        }

        currentBlock = buffer.next; // Move to the next block (chained organization)
    }

    printf("End of file '%s'.\n", fileName);
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
        releaseBlock(i);
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


int main() {
	int choice, searchID;
    int found, blockNumber, offset;
    FILE *ms = fopen("MS.bin", "rb+");
    if (!ms) {
        ms = fopen("MS.bin", "wb+");
    }

    FILE *metaFile = fopen("MetaDonnees.data", "rb+");
    if (!metaFile) {
        metaFile = fopen("MetaDonnees.data", "wb+");
    }

    initializeAllocationTable();

    if (ms && metaFile) {
    	createFile(ms, metaFile);
        loadFile(ms, metaFile);
        char fileName[30];
    printf("Enter the name of the file to display its content: ");
    scanf("%s", fileName);

    printFileContent(ms, metaFile, fileName);
        printf("Enter the record ID to search: ");
                scanf("%d", &searchID);

                // Call the search function
                searchRecordByID(ms, metaFile, searchID, &found, &blockNumber, &offset);
        renameFile(ms, metaFile);  // Example: Rename file
        deleteFile(ms, metaFile);  // Example: Delete file
        
        fclose(metaFile);
        fclose(ms);
    } else {
        printf("Error opening files.\n");
    }

    return 0;
}
