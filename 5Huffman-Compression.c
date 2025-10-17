#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define MAX_TREE_NODES 256
#define MAX_CODE_LENGTH 256

//Huffmancde struc
typedef struct HuffmanNode {
    unsigned char data;
    int frequency;
    struct HuffmanNode *left;
    struct HuffmanNode *right;
} HuffmanNode;

//Priority queue desc
typedef struct PriorityQueue {
    HuffmanNode **nodes;
    int size;
    int capacity;
} PriorityQueue;

//code table
typedef struct CodeTable {
    char code[MAX_CODE_LENGTH];
    int length;
} CodeTable;

//Functions are functioning
HuffmanNode* createNode(unsigned char data, int frequency);
PriorityQueue* createPriorityQueue(int capacity);
void swapNodes(HuffmanNode **a, HuffmanNode **b);
void minHeapify(PriorityQueue *queue, int idx);
HuffmanNode* extractMin(PriorityQueue *queue);
void insertNode(PriorityQueue *queue, HuffmanNode *node);
void buildHuffmanTree(PriorityQueue *queue);
void generateCodes(HuffmanNode *root, CodeTable *codes, char *currentCode, int depth);
void freeHuffmanTree(HuffmanNode *root);
void writeBit(FILE *file, int bit, int *bitPosition, unsigned char *byteBuffer);
void flushBits(FILE *file, int *bitPosition, unsigned char *byteBuffer);
int readBit(FILE *file, int *bitPosition, unsigned char *currentByte);

//Creating a new Huffman node
HuffmanNode* createNode(unsigned char data, int frequency) {
    HuffmanNode *node = (HuffmanNode*)malloc(sizeof(HuffmanNode));
    node->data = data;
    node->frequency = frequency;
    node->left = node->right = NULL;
    return node;
}

//Creating a priority queue
PriorityQueue* createPriorityQueue(int capacity) {
    PriorityQueue *queue = (PriorityQueue*)malloc(sizeof(PriorityQueue));
    queue->nodes = (HuffmanNode**)malloc(capacity * sizeof(HuffmanNode*));
    queue->size = 0;
    queue->capacity = capacity;
    return queue;
}

void swapNodes(HuffmanNode **a, HuffmanNode **b) {
    HuffmanNode *temp = *a;
    *a = *b;
    *b = temp;
}

//Mini-heap priority is kept at constant
void minHeapify(PriorityQueue *queue, int idx) {
    int smallest = idx;
    int left = 2 * idx + 1;
    int right = 2 * idx + 2;

    if (left < queue->size && 
        queue->nodes[left]->frequency < queue->nodes[smallest]->frequency) {
        smallest = left;
    }

    if (right < queue->size && 
        queue->nodes[right]->frequency < queue->nodes[smallest]->frequency) {
        smallest = right;
    }

    if (smallest != idx) {
        swapNodes(&queue->nodes[idx], &queue->nodes[smallest]);
        minHeapify(queue, smallest);
    }
}

//Extract node with min freq
HuffmanNode* extractMin(PriorityQueue *queue) {
    if (queue->size <= 0) return NULL;
    
    HuffmanNode *minNode = queue->nodes[0];
    queue->nodes[0] = queue->nodes[queue->size - 1];
    queue->size--;
    minHeapify(queue, 0);
    
    return minNode;
}


void insertNode(PriorityQueue *queue, HuffmanNode *node) {
    if (queue->size >= queue->capacity) {
        return; 
    }
    
    int i = queue->size;
    queue->size++;
    queue->nodes[i] = node;
    
    while (i != 0 && queue->nodes[(i-1)/2]->frequency > queue->nodes[i]->frequency) {
        swapNodes(&queue->nodes[i], &queue->nodes[(i-1)/2]);
        i = (i-1)/2;
    }
}

//Build Huffman tree from frequency data chosen.
void buildHuffmanTree(PriorityQueue *queue) {
    while (queue->size > 1) {
        HuffmanNode *left = extractMin(queue);
        HuffmanNode *right = extractMin(queue);
        
        HuffmanNode *parent = createNode(0, left->frequency + right->frequency);
        parent->left = left;
        parent->right = right;
        
        insertNode(queue, parent);
    }
}

//Huffman codes recursively defined
void generateCodes(HuffmanNode *root, CodeTable *codes, char *currentCode, int depth) {
    if (root == NULL) return;
    
    if (root->left == NULL && root->right == NULL) {
        currentCode[depth] = '\0';
        strcpy(codes[root->data].code, currentCode);
        codes[root->data].length = depth;
        return;
    }
    
    if (root->left != NULL) {
        currentCode[depth] = '0';
        generateCodes(root->left, codes, currentCode, depth + 1);
    }
    
    if (root->right != NULL) {
        currentCode[depth] = '1';
        generateCodes(root->right, codes, currentCode, depth + 1);
    }
}

//Free Huffman tree memory
void freeHuffmanTree(HuffmanNode *root) {
    if (root == NULL) return;
    
    freeHuffmanTree(root->left);
    freeHuffmanTree(root->right);
    free(root);
}

void writeBit(FILE *file, int bit, int *bitPosition, unsigned char *byteBuffer) {
    if (bit) {
        *byteBuffer |= (1 << (7 - *bitPosition));
    }
    
    (*bitPosition)++;
    
    if (*bitPosition == 8) {
        fwrite(byteBuffer, 1, 1, file);
        *byteBuffer = 0;
        *bitPosition = 0;
    }
}

//Flushing remaining bits out
void flushBits(FILE *file, int *bitPosition, unsigned char *byteBuffer) {
    if (*bitPosition > 0) {
        fwrite(byteBuffer, 1, 1, file);
    }
}

//Reading single bit off file
int readBit(FILE *file, int *bitPosition, unsigned char *currentByte) {
    if (*bitPosition == 0) {
        if (fread(currentByte, 1, 1, file) != 1) {
            return -1; // EOF or error
        }
    }
    
    int bit = (*currentByte >> (7 - *bitPosition)) & 1;
    *bitPosition = (*bitPosition + 1) % 8;
    
    return bit;
}

void compressFile(const char *inputFilename, const char *outputFilename) {
    FILE *inputFile = fopen(inputFilename, "rb");
    FILE *outputFile = fopen(outputFilename, "wb");
    
    if (!inputFile || !outputFile) {
        printf("Error: Cannot open files\n");
        return;
    }
    
    int frequencies[256] = {0};
    int totalChars = 0;
    
    int ch;
    while ((ch = fgetc(inputFile)) != EOF) {
        frequencies[ch]++;
        totalChars++;
    }

    PriorityQueue *queue = createPriorityQueue(256);
    
    for (int i = 0; i < 256; i++) {
        if (frequencies[i] > 0) {
            HuffmanNode *node = createNode((unsigned char)i, frequencies[i]);
            insertNode(queue, node);
        }
    }
    
    //Building Huffman Tree
    buildHuffmanTree(queue);
    HuffmanNode *root = extractMin(queue);
    
    CodeTable codes[256];
    char currentCode[MAX_CODE_LENGTH];
    generateCodes(root, codes, currentCode, 0);
    
    fwrite(&totalChars, sizeof(int), 1, outputFile);
    fwrite(frequencies, sizeof(int), 256, outputFile);
    
    rewind(inputFile);
    
    unsigned char byteBuffer = 0;
    int bitPosition = 0;
    
    while ((ch = fgetc(inputFile)) != EOF) {
        char *code = codes[ch].code;
        int codeLength = codes[ch].length;
        
        for (int i = 0; i < codeLength; i++) {
            writeBit(outputFile, code[i] - '0', &bitPosition, &byteBuffer);
        }
    }
    
    flushBits(outputFile, &bitPosition, &byteBuffer);
    
    // Clean up
    fclose(inputFile);
    fclose(outputFile);
    freeHuffmanTree(root);
    free(queue->nodes);
    free(queue);
    
    printf("Compression completed!\n");
}

// Decompress file using Huffman coding
void decompressFile(const char *inputFilename, const char *outputFilename) {
    FILE *inputFile = fopen(inputFilename, "rb");
    FILE *outputFile = fopen(outputFilename, "wb");
    
    if (!inputFile || !outputFile) {
        printf("Error: Cannot open files\n");
        return;
    }
    
    int totalChars;
    int frequencies[256];
    
    fread(&totalChars, sizeof(int), 1, inputFile);
    fread(frequencies, sizeof(int), 256, inputFile);
    
    PriorityQueue *queue = createPriorityQueue(256);
    
    for (int i = 0; i < 256; i++) {
        if (frequencies[i] > 0) {
            HuffmanNode *node = createNode((unsigned char)i, frequencies[i]);
            insertNode(queue, node);
        }
    }
    
    buildHuffmanTree(queue);
    HuffmanNode *root = extractMin(queue);
    
    //Decompressing  the data
    unsigned char currentByte = 0;
    int bitPosition = 0;
    int charsDecoded = 0;
    
    HuffmanNode *currentNode = root;
    
    while (charsDecoded < totalChars) {
        int bit = readBit(inputFile, &bitPosition, &currentByte);
        
        if (bit == -1) {
            break; // EOF
        }
        
        if (bit == 0) {
            currentNode = currentNode->left;
        } else {
            currentNode = currentNode->right;
        }
        
        if (currentNode->left == NULL && currentNode->right == NULL) {
            fputc(currentNode->data, outputFile);
            charsDecoded++;
            currentNode = root;
        }
    }
    
    fclose(inputFile);
    fclose(outputFile);
    freeHuffmanTree(root);
    free(queue->nodes);
    free(queue);
    
    printf("Decompression completed successfully!\n");
}

// Compare two files to verify data integrity
int compareFiles(const char *file1, const char *file2) {
    FILE *f1 = fopen(file1, "rb");
    FILE *f2 = fopen(file2, "rb");
    
    if (!f1 || !f2) {
        printf("Error: Cannot open files for comparison\n");
        return 0;
    }
    
    int ch1, ch2;
    int isEqual = 1;
    
    while (1) {
        ch1 = fgetc(f1);
        ch2 = fgetc(f2);
        
        if (ch1 != ch2) {
            isEqual = 0;
            break;
        }
        
        if (ch1 == EOF) break;
    }
    
    fclose(f1);
    fclose(f2);
    
    return isEqual;
}

//Get file in bytes, thats byte SIZED!
long getFileSize(const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (!file) return -1;
    
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fclose(file);
    
    return size;
}

int main() {
    char inputFilename[100];
    char compressedFilename[] = "compressed.txt";
    char decompressedFilename[] = "decompressed.txt";
    
    printf("!!!!! Hospital Medical Records Compression Tool !!!!!\n");
    printf("Enter the input filename: ");
    scanf("%99s", inputFilename);
    
    //Compression time
    printf("\n!!!!! Compression Phase !!!!!\n");
    compressFile(inputFilename, compressedFilename);
    
    long originalSize = getFileSize(inputFilename);
    long compressedSize = getFileSize(compressedFilename);
    
    printf("Original file size: %ld bytes\n", originalSize);
    printf("Compressed file size: %ld bytes\n", compressedSize);
    printf("Compression ratio: %.2f%%\n", 
           (1.0 - (double)compressedSize / originalSize) * 100);
    
    printf("\n!!!!! Decompression Phase !!!!!\n");
    decompressFile(compressedFilename, decompressedFilename);
    
    //DATA VERIFICATION
    printf("\n!!!!! Data Integrity Check !!!!!!\n");
    if (compareFiles(inputFilename, decompressedFilename)) {
        printf("YESSS SUCCESS: Decompressed file matches original exactly!\n");
        printf("YESSS Lossless compression verified!\n");
    } else {
        printf("NAW ERROR: Decompressed file does not match original!\n");
        printf("NAW Data integrity check failed!\n");
    }
    
    return 0;
}