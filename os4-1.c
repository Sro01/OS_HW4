#include <stdio.h>
#include <stdlib.h>

// 명령어
// gcc -Wall os4-1.c && cat tests4-1/test4-2.bin | ./a.out

#define MAX_FILES 16     // 최대 파일이 16개
#define MAX_BLOCKS 208   // 데이터블락 208개
#define BLOCK_SIZE 16    // 데이터 블락 크기 16byte
#define MAX_FILENAME_LEN 24
#define MAX_FILE_BLOCKS 3
#define NONE_BLOCK 255

// #define SUPERBLOCK_SIZE 256

typedef struct {
    /* 모든 바이너리 파일에서 동일하게 주어짐 */
    int fs_size;                         // 파일 시스템 전체 크기 (4B)
    int inode_count;                     // 최대 inode 개수 (4B)
    int block_count;                     // 최대 데이터 블록 개수 (4B)
    int block_size;                      // 블록 크기 (byte) (4B)

    int free_inodes;                     // 가용 inode 개수 (4B)
    int free_blocks;                     // 가용 데이터 블록 개수 (4B)
    char inode_alloc_bitmap[MAX_FILES];  // 사용 중인 inode 관리 배열 (16B)
    char block_alloc_bitmap[MAX_BLOCKS]; // 사용 중인 데이터블록 관리 배열 (208B)
    char padding[8];                     // 남은 공간 패딩 (8B)
} Superblock;                            // 총 256B
         
typedef struct {
    char data[BLOCK_SIZE];  // 16B
} DataBlock;

typedef struct {
    unsigned int size;                      // 256B
    unsigned char indirect_blocks;          // 1B (datablock의 넘버 0 ~ 207)
    unsigned char blocks[MAX_FILE_BLOCKS];  // 3B (datablock의 넘버 0 ~ 207)
    char file_name[MAX_FILENAME_LEN];       // 24B
} Inode;                                    // 총 32B

typedef struct {
    Superblock superblock;          // 256B
    Inode inodes[MAX_FILES];        // 32B * 16 = 512B
    DataBlock blocks[MAX_BLOCKS];    // 16B * 208 = 3328B
} FileSystem;                       // 총 4096B

/*
    // if ((Inode.size / 48) > 0) {
        // Indirect Block 할당
        Datablock datablocks;
        indirect_blocks = datablocks;
        // indirect block table이 가리키는 파일 16개 * 16byte
    }

*/

FileSystem fs;

int calculate_indirect_blocks(int size) {
    if (size <= 48) {
        return 0;
    }

    int ret = (size - 48) / 16;

    if ((size - 48) % 16 > 0) { 
        return ++(ret);
    } else {
        return ret;
    }
}

void print_superblock_info() {
    Superblock superblock = fs.superblock;
    /* Filesystem 상태 출력 */
    printf("Filesystem Status:\n");

    /* Superblock 정보 출력: */
    printf("Superblock Information:\n");
    printf("\tFilesystem Size: %d bytes\n", superblock.fs_size);
    printf("\tBlock Size: %d bytes\n", superblock.block_size);
    printf("\tAvailable Inodes: %d/%d\n", superblock.free_inodes, superblock.inode_count);
    printf("\tAvailable Blocks: %d/%d\n", superblock.free_blocks, superblock.block_count);
}

void print_file_info() {
    printf("Detailed File Information:\n");

    int used_inodes_num = MAX_FILES - fs.superblock.free_inodes;
    for (int i = 0; i < used_inodes_num; i++) {
        Inode inode = fs.inodes[i];

        printf("\tFile: %s\n", inode.file_name);
        printf("\t\tSize: %u\n", inode.size);
        printf("\t\tInode: %d\n", i);
        printf("\t\tDirect blocks: ");
        int none_blocks = 0;
        for (int i = 0; i < MAX_FILE_BLOCKS; i++) {
            if (inode.blocks[i] == NONE_BLOCK) {
                none_blocks++;
            }
        }

        for (int j = 0; j < MAX_FILE_BLOCKS; j++) {
            if (j == MAX_FILE_BLOCKS - none_blocks - 1) {
                printf("%u\n", inode.blocks[j]);
                break;
            }
            else {
                printf("%u, ", inode.blocks[j]);
            }
        }
        
        int indirect_blocks_num = calculate_indirect_blocks(inode.size);

        // Indirect block이 있는 경우에만 출력
        if (indirect_blocks_num > 0) {
            printf("\t\tIndirect block: %u\n", inode.indirect_blocks);
            printf("\t\tIndirect data blocks: ");
            
            int data_idx = inode.indirect_blocks;
            
            unsigned char *indirect_data = (unsigned char *)fs.blocks[data_idx].data;

            for (int j = 0; j < indirect_blocks_num; j++) {
                if (j == indirect_blocks_num - 1) {
                    printf("%u\n", indirect_data[j]);
                }
                else {
                    printf("%u, ", indirect_data[j]);
                }
            }
        }
    }
}

void print_datablock_info() {
    for (int i = 0; i < MAX_BLOCKS; i++) {
        DataBlock db = fs.blocks[i];

        for (int j = 0; j < MAX_FILES; j++) {
            if (db.data[j]) {
                if (db.data[j] == NONE_BLOCK) {
                    // printf("NONE!\n");
                    continue;
                }

                printf("%c", db.data[j]);

                if (j == MAX_FILES - 1) {
                    printf("\n");
                }
            }
        }
    }
}

void read_binary() {
    FILE *fp = stdin;

    /* 1. Super Block - 256B */
    if (fread(&(fs.superblock), sizeof(fs.superblock), 1, fp)) {
        print_superblock_info();
    } else {
        // printf("ERROR!\n");
        exit(1);
    }

    /* 2. Inode Table - 16 * 32B */
    for (int i = 0; i < MAX_FILES; i++) {
        if (!fread(&(fs.inodes[i]), sizeof(fs.inodes[i]), 1, fp)) {
            // printf("ERROR!\n");
            exit(1);
        }
    }
    
    /* 3. DataBlock - 208 * 16B */
    for (int i = 0; i < MAX_BLOCKS; i++) {
        if (!fread(&(fs.blocks[i]), sizeof(fs.blocks[i]), 1, fp)) {
            // printf("ERROR!\n");
            exit(1);
        }
    }
    print_file_info();
}

int main() {
    read_binary();
}