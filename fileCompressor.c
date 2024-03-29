#include "fileCompressor.h"

int main(int argc, char* argv[])
{
    if(argc < 3 || argc > 5)
    {
        printf("Fatal Error: invalid number of arguments\n");
        return 0;
    }

    if(argc == 3)
    {
        if(strcmp(argv[1], "-b") == 0) // if build codebook is selected
        {
            node* root = NULL; //create root 
            char* huffmanPath = "./HuffmanCodebook"; //create huffman codebook path
            root = buildAVLFromFile(prePendDotSlash(argv[2]), root); //build AVL tree of nodes with tokens
            build(root, huffmanPath); //build huffman codebook
        }
        else
        {
            printf("Fatal Error: flags are not valid/incorrect order\n");
            return 0;
        }  
    }

    if(argc == 4)
    {
        if(strcmp(argv[1], "-R") == 0 && strcmp(argv[2], "-b") == 0 ) // if recursive build is selected
        {
            node* root = NULL;
            recursiveBuild(root, prePendDotSlash(argv[3]));
        }
        
        else if(strcmp(argv[1], "-c") == 0) //if compress is selected
        { 
            node* root = NULL;
            compress(root, prePendDotSlash(argv[3]), prePendDotSlash(argv[2]));
        }

        else if(strcmp(argv[1], "-d") == 0)//if decompress  
        {
            node* root = NULL;
            root = (node*) malloc(sizeof(node));
            root->encoding = NULL;
            root->token = NULL;
            root->left = NULL;
            root->right = NULL;
            root = buildHuffmanFromFile(prePendDotSlash(argv[3]), root); //create huffman tree of nodes
            if(root == NULL)
            {
                printf("Error: file can't be decompressed\n");
                return 0;
            }
            decompress(root, prePendDotSlash(argv[3]), prePendDotSlash(argv[2]));
            free(root);
        }

        else
        {
            printf("Fatal Error: flags are not valid/incorrect order\n");
            return 0;
        }
        
    }

    if(argc == 5)
    {
        if(strcmp(argv[1], "-R") == 0 && strcmp(argv[2], "-c") == 0 ) //if recursive compress is selected 
        {
            node* root = NULL;
            root = buildAVLFromHuffman(prePendDotSlash(argv[4]), root);
            if(root == NULL)
            {
                printf("Error: file can't be compressed\n");
                return 0;
            }
            recursiveCompress(prePendDotSlash(argv[3]), prePendDotSlash(argv[4]), root);
            freeTree(root);
        }

        else if(strcmp(argv[1], "-R") == 0 && strcmp(argv[2], "-d") == 0 ) //if recursive decompress  R d path codebook
        {
            node* root = NULL;
            root = (node*) malloc(sizeof(node));
            root->encoding = NULL;
            root->token = NULL;
            root->left = NULL;
            root->right = NULL;
            root = buildHuffmanFromFile(prePendDotSlash(argv[4]), root); //create huffman tree of nodes
            if(root == NULL)
            {
                printf("Error: file can't be decompressed\n");
                return 0;
            }
            recursiveDecompress(root, prePendDotSlash(argv[4]), prePendDotSlash(argv[3]));
            free(root);
        }
        else
        {
            printf("Fatal Error: flags are not valid/incorrect order\n");
            return 0;            
        }
        
    }

    return 0;
}

/**
 * Recursive decompress function
 * */
void recursiveDecompress(node* root, char* codebookPath, char* dirPath)
{
    DIR* dir = opendir(dirPath); //open directory

    if(dir == NULL)
    {
        printf("Fatal Error: directory: %s does not exist\n", dirPath);
        return;
    }

    readdir(dir); //skip first two (becuase of . and ..)
    readdir(dir);

    char path[20000];
    memset(path, '\0', 20000);
    struct dirent* entry;

    while((entry = readdir(dir)) != NULL)
    {
        strcpy(path, dirPath);
        if(path[strlen(path)-1] != '/') //concatinate a / to create path for file/directory
            strcat(path, "/");
        strcat(path, entry->d_name);

        if(isDirectory(path))
            recursiveDecompress(root, codebookPath, path);
        else if(strcmp("hcz", getFileExtension(path)) == 0)
        {
            decompress(root, codebookPath, path);       
        }
    }

    closedir(dir);
}

/**
 * Decompress function
 * Creates Huffman Tree from codebook and walks through compressed file
 * Converts compressed file to decompressed form
 * */
void decompress(node* root, char* codebookPath, char* oldFilePath)
{
    if(strcmp("hcz", getFileExtension(oldFilePath)) != 0) //check if file is a compressed file
    {
        printf("Error: can only decompress hcz files\n");
        return;
    }
    char* newFilePath = getDecompFileName(oldFilePath); //create new file path
    decompressFile(oldFilePath, newFilePath, root);
    free(newFilePath);
}

/**
 * Recursive build function
 * */
void recursiveBuild(node* root, char* path)
{
    char* huffmanPath = "./HuffmanCodebook";
    root = buildAVLRecursive(path, root); //create tree after walking through every file in directory
    build(root, huffmanPath); 
}

/**
 * Base build function
 * */
void build(node* root, char* huffmanPath)
{
    int heapSize = getSize(root);
    if(heapSize == 0)
    {
        printf("Fatal Error: no valid directory/file(s) to build from\n");
        return;
    }
    if(access(huffmanPath, F_OK) != -1)
    {
        printf("Warning: Codebook already exists in directory, will be deleted and/or replaced\n");
        remove(huffmanPath);
    }
    node** heap = (node**) malloc(sizeof(node*) * heapSize);
    createHeap(heap, root, 0);
    buildHeap(heap, heapSize);
    buildHuffmanTree(heap, &heapSize);
    int huffmanSize = getSize(heap[0]);
    int* huffmanCodeArr = (int*) malloc(sizeof(int) * huffmanSize);
    int lengthOfEncoding = 0;
    encode(heap[0], huffmanCodeArr, lengthOfEncoding);
    int codebookFD = open(huffmanPath, O_RDWR | O_CREAT, 00600);
    writeHuffmanCodebook(codebookFD, heap[0]);
    close(codebookFD);
    freeHuffman(heap[0]);
    free(heap);
    free(huffmanCodeArr);
}

/**
 * Go through each directory/file and send to compressFile function of IO.c
 * */
void recursiveCompress(char* basePath, char* huffmanPath, node* root)
{
    DIR* dir = opendir(basePath);

    if(dir == NULL)
    {
        printf("Fatal Error: directory: %s does not exist\n", basePath);
        return;
    }

    readdir(dir);
    readdir(dir);

    char path[20000];
    memset(path, '\0', 20000);
    struct dirent* entry;

    while((entry = readdir(dir)) != NULL)
    {
        strcpy(path, basePath);
        if(path[strlen(path)-1] != '/')
            strcat(path, "/");
        strcat(path, entry->d_name);

        if(isDirectory(path))
            recursiveCompress(path, huffmanPath, root);
        else if((strcmp(path, huffmanPath)) != 0 && (strcmp("hcz", getFileExtension(path))) != 0)
        {
            char* compFile = getCompressedFileName(path);
            compressFile(path, compFile, root);   
            free(compFile);       
        }
    }

    closedir(dir);
}

/**
 * Create AVL tree based off Huffman Tree, and send file to be compressed to IO.c function
 * */
void compress(node* root, char* huffmanPath, char* oldFile)
{
    if(strcmp("hcz", getFileExtension(oldFile)) == 0)
    {
        printf("Error: cannot compress an hcz file\n");
        return;
    }
    root = buildAVLFromHuffman(huffmanPath, root);
    if(root == NULL)
    {
        printf("Error: file can't be compressed\n");
        return;
    }
    char* compFile = getCompressedFileName(oldFile);
    compressFile(oldFile, compFile, root);
    freeTree(root);
    free(compFile);
}

/**
 * Check if path is a directory
 * */
int isDirectory(char* path)
{
    int fd = open(path, O_RDWR);

    if(fd == -1 && errno == EISDIR)
        return 1;

    close(fd);
    return 0;
}

/**
 * Recurse through all files/directories and construct one AVL tree
 * */
node* buildAVLRecursive(char* basePath, node* root)
{
    DIR* dir = opendir(basePath); 
    if(dir == NULL)
    {
        printf("Fatal Error: directory can't be accessed\n");
        return root;
    }
    // 2 reads to skip past parent and current directory
    readdir(dir);
    readdir(dir);

    char path[10000];

    struct dirent* entry;

    while((entry = readdir(dir)) != NULL)
    { // still an entry to be read from the directory
        strcpy(path, basePath);
        if(path[strlen(path)-1] != '/')
            strcat(path, "/");
        strcat(path, entry->d_name);

        if(isDirectory(path))
            root = buildAVLRecursive(path, root);
        else if(strcmp("hcz", getFileExtension(path)) != 0)
            root = buildAVLFromFile(path, root);   
    }

    closedir(dir);

    return root;
}

/**
 * Get file extension
 * */
char* getFileExtension(char* fileName)
{
    char* ext = strrchr(fileName, '.');
    if(ext == NULL)
        return "";
    return ext+1;
}

/**
 * Create compressed path
 * Adds .hcz to end of path
 * */
char* getCompressedFileName(char* path)
{
    char* compFileName = (char*) malloc(sizeof(char) * (strlen(path) + 5));
    memset(compFileName, '\0', strlen(path) + 5);
    strcpy(compFileName, path);
    strcat(compFileName, ".hcz");
    return compFileName;
}

/**
 * Get file name without .hcz
 * Example: file.txt.hcz -> file.txt
 * */
char* getDecompFileName(char* path)
{
    int index = strlen(path)-1;
    while(index != 0 && path[index] != '.')
    {
        index--;
    }
    char* decompFile = (char*) malloc(sizeof(char) * index+1);
    memset(decompFile, '\0', index+1);
    memcpy(decompFile, path, index);
    return decompFile;
}

/**
 * Added so if any path arguments do not have './', it is prepended to it
 * This is in case of error in user input
 * */
char* prePendDotSlash(char* arg)
{
    if(arg[0] == '/')
        return arg;
    char* newArg;
    if(strlen(arg) >= 2) //relative path
    {
        if(arg[0] != '.' || arg[1] != '/')
        {
            newArg = (char*) malloc(sizeof(char) * strlen(arg) + 3);
            memset(newArg, '\0', strlen(arg) + 3);
            newArg[0] = '.';
            newArg[1] = '/';
            strcpy(&newArg[2], arg);
            return newArg;
        }
        else
        {
            return arg;
        }
    }
    else
    {
        newArg = (char*) malloc(sizeof(char) * strlen(arg) + 3);
        memset(newArg, '\0', strlen(arg) + 3);
        newArg[0] = '.';
        newArg[1] = '/';
        strcpy(&newArg[2], arg);
        return newArg;        
    }    
}