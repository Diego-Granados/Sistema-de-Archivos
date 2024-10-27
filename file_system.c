#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#define DISK_SIZE 1048576
#define BLOCK_SIZE 510
#define MAX_FILES 100
#define NUM_BLOCKS 2048


// Vamos a usar para los archivos lo de la lista enlazada y para los bloques libres el counting

// Structure for the node that contains the key and the value
// Struct para los nodos de las listas.
struct File
{
    char *name; // El nombre del archivo
    int location;       // Dirección
    int size;          // Tamaño
    uint16_t firstBlock; // Puntero al próximo bloque
    struct File *prev; // Puntero al nodo anterior
    struct File *next; // Puntero al próximo nodo
};

// Struct para la lista
struct FileList
{
    uint8_t quantity;
    int size;
    struct File *head; // Puntero a la cabeza de la lista
    struct File *tail; // Puntero a la cola de la lista
};

// Función para crear una lista
static struct FileList* createFileList()
{
    struct FileList* list = (struct FileList *)malloc(sizeof(struct FileList)); // Asignar memoria para la lista
    if (list == NULL)
    {
        printf("Error de asignación de memoria para la lista!\n");
        exit(1);
    }
    (list)->head = NULL;
    (list)->tail = NULL;
    return list;
}

struct Block
{
    int location;       // Dirección
    struct Block *prev; // Puntero al nodo anterior
    struct Block *next; // Puntero al próximo nodo
};

// Struct para la lista
struct BlockList
{
    uint16_t quantity;
    struct Block *head;
    struct Block *tail;
};

// // Variables globales
static struct FileList *directory = NULL;
static struct FileList *openFiles = NULL;
static struct BlockList *freeBlocks = NULL;
static uint16_t FAT[NUM_BLOCKS];

// Open the file
FILE *disk; 
FILE *FATFile;
FILE * directoryFile;
// Función para crear una lista
static struct BlockList* createBlockList()
{
    struct BlockList *list = (struct BlockList *)malloc(sizeof(struct BlockList)); // Asignar memoria para la lista
    if (list == NULL)
    {
        printf("Error de asignación de memoria para la lista!\n");
        exit(1);
    }
    list->quantity = 0;
    list->head = NULL;
    list->tail = NULL;
    return list;
}

// Función para crear un nodo
static struct File *createFileRef(const char *name, int address, int size, uint16_t firstBlock)  
{
    // Asignar memoria al nodo
    struct File *newFile = (struct File *)malloc(sizeof(struct File));
    if (newFile == NULL)
    {
        printf("Error de asignación de memoria para el nodo!\n");
        exit(1);
    }
    // Inicializar el nodo
    newFile->name = malloc(strlen(name) + 1); // Allocate memory for the string + null terminator
    if (newFile->name == NULL) {
        printf("Error de asignación de memoria para el nodo!\n");
        exit(1);
    }
    strcpy(newFile->name, name);
    newFile->location = address; // Todavía no hay una dirección asignada
    newFile->size = size;
    newFile->firstBlock = firstBlock;
    newFile->next = NULL;
    newFile->prev = NULL;
    return newFile;
}

static struct Block *createBlockRef(int address)
{
    // Asignar memoria al nodo
    struct Block *newBlock = (struct Block *)malloc(sizeof(struct Block));
    if (newBlock == NULL)
    {
        printf("Error de asignación de memoria para el nodo!\n");
        exit(1);
    }
    // Inicializar el nodo
    newBlock->location = address; // Todavía no hay una dirección asignada
    newBlock->next = NULL;
    newBlock->prev = NULL;
    return newBlock;
}

// Función para añadir un valor a la lista
static void *addFileToList(struct File *newNode)
{
    // Si la lista está vacía, se añade el nodo al principio
    if (directory->head == NULL)
    {
        directory->head = newNode;
        directory->tail = newNode;
    }
    else
    {
        // Se inserta el archivo al final de lista del directorio
        newNode->prev = directory->tail;
        directory->tail->next = newNode;
        directory->tail = newNode;
        
    }
}

// Función para añadir un valor a la lista
static void *addBlockToList(struct Block *newNode)
{
    // Si la lista está vacía, se añade el nodo al principio
    if (freeBlocks->head == NULL)
    {
        freeBlocks->head = newNode;
        freeBlocks->tail = newNode;
    }
    else
    {
        // Se recorre la lista para insertar el nodo con base en el ordenamiento
        struct Block *temp = freeBlocks->head;
        while (temp != NULL)
        {
            if (newNode->location < temp->location)
            { // Si el nodo debe ir antes que el nodo actual
                newNode->next = temp;
                newNode->prev = temp->prev;
                if (temp->prev == NULL)
                {
                    freeBlocks->head = newNode;
                }
                else
                {
                    temp->prev->next = newNode;
                }
                temp->prev = newNode;
                break;
            }
            else
            {
                temp = temp->next;
            }
        }
        if (temp == NULL)
        { // Si el nodo debe ir al final de la lista
            newNode->prev = freeBlocks->tail;
            freeBlocks->tail->next = newNode;
            freeBlocks->tail = newNode;
        }
    }
    freeBlocks->quantity++;
}

// Función para borrar un nodo de una lista
static void deleteBlock(struct Block *nodeToDelete)
{
    if (nodeToDelete == NULL)
    {
        printf("El nodo a eliminar no es válido.\n");
        return;
    }

    // Si el nodo es la cabeza
    if (nodeToDelete == freeBlocks->head)
    {
        // Si la lista solo tiene un nodo
        if (freeBlocks->tail == freeBlocks->head)
        {
            freeBlocks->tail = NULL;
        }
        freeBlocks->head = nodeToDelete->next; // Se actualiza la cabeza

        // Si la lista no está vacía, se actualiza el puntero previo de la nueva cabeza
        if (freeBlocks->head != NULL)
        {
            freeBlocks->head->prev = NULL;
        }
    }
    // Si el nodo es la cola
    else if (nodeToDelete == freeBlocks->tail)
    {
        freeBlocks->tail = nodeToDelete->prev; // Update the tail
        if (freeBlocks->tail != NULL)
        {
            freeBlocks->tail->next = NULL; // Set the next pointer of the new tail to NULL
        }
    }
    // If the node to delete is in the middle
    else
    {
        nodeToDelete->prev->next = nodeToDelete->next;
        nodeToDelete->next->prev = nodeToDelete->prev;
    }
    free(nodeToDelete); // Se libera la memoria
}

void initializeFAT() {
    for (int i = 0; i < NUM_BLOCKS; i++) {
        FAT[i] = 0x5F5F;
        struct Block *newBlock = createBlockRef(i);
        addBlockToList(newBlock);
    }
}

static void writeFAT(uint16_t block, uint16_t value) {
    if (fseek(FATFile, block, SEEK_SET) != 0) {
        perror("Error seeking in file");
        exit(EXIT_FAILURE);
    }

    if (fwrite(&value, sizeof(uint16_t), 1, FATFile) != 1) {
        perror("Error writing to file");
        exit(EXIT_FAILURE);
    }
}

static void readFAT() {
    if (fseek(FATFile, 0, SEEK_SET) != 0) {
        perror("Error seeking in file");
        exit(EXIT_FAILURE);
    }

    if (fread(FAT, sizeof(uint16_t), NUM_BLOCKS, FATFile) != NUM_BLOCKS) {
        perror("Error reading from file");
        exit(EXIT_FAILURE);
    }
}

static void resetFAT(){
    // Write the FAT to the file
    initializeFAT();

    if (fwrite(FAT, sizeof(uint16_t), NUM_BLOCKS, FATFile) != NUM_BLOCKS) {
        perror("Error writing to file");
        exit(EXIT_FAILURE);
    }
}

static void saveFile(struct File *temp){
    if (fseek(directoryFile, 0, SEEK_END) != 0) {
        perror("Error seeking to the end of file");
    }
    
    fprintf(directoryFile, "%s %d %d %d\n", temp->name, temp->location, temp->size, temp->firstBlock);
}

static void readDirectory() {
    if (fseek(directoryFile, 0, SEEK_SET) != 0) {
        perror("Error seeking in file");
        exit(EXIT_FAILURE);
    }

    char name[50];
    int location;
    int size;
    int firstBlock;
    directory->quantity = 0;
    directory->size = 0;
    while (fscanf(directoryFile, "%s %d %d %d\n", name, &location, &size, &firstBlock) != EOF) {
        struct File *temp = createFileRef(name, location, size, firstBlock);
        addFileToList(temp);
        directory->quantity++;
        directory->size += size;
    }
}

// (i * BLOCK_SIZE + sizeof(uint16_t) * i + i)
int resetDisk() {
    // Create a buffer with 512 '_'s
    char buffer[BLOCK_SIZE+sizeof(uint16_t)];
    for (int i = 0; i < (BLOCK_SIZE+sizeof(uint16_t)); i++) {
        buffer[i] = '_';
    }

    // Write 512 0xFF and a newline 2048 times
    for (int i = 0; i < NUM_BLOCKS; i++) {
        fwrite(buffer, sizeof(char), (BLOCK_SIZE+sizeof(uint16_t)), disk);
        fputc('\n', disk);
    }
}

// Función para resetear el directorio
void resetDirectory() {
    fClose(directoryFile);
    directoryFile = fopen("directory.txt", "w");
    if (directoryFile == NULL) {
        perror("Error opening file");
        return;
    }
    fclose(directoryFile);
    directoryFile = fopen("directory.txt", "r+");
    if (directoryFile == NULL) {
        perror("Error opening file");
        return;
    }
}


// Función para encontrar si hay un hueco en la memoria donde cabe el valor
static void createFile(const char *name, int size)
{
    struct Block *freeBlock = freeBlocks->head;

    if (size > (freeBlocks->quantity * BLOCK_SIZE) || directory->quantity == MAX_FILES || (directory->size + size) > DISK_SIZE || freeBlock == NULL)
    {
        printf("No hay suficiente espacio para el archivo", name);
        return;
    }

    // Se revisa si el archivo ya existe
    struct File *file = directory->head;
    while (file != NULL)
    {
        if (strcmp(file->name, name) == 0)
        {
            printf("El archivo ya existe", name);
            return;
        }
        file = file->next;
    }

    struct File *newNode = createFileRef(name, freeBlock->location, size, freeBlock->location / BLOCK_SIZE);
    uint16_t previousBlock = 0;
    int sizeSave = size;
    struct Block *temp = NULL;
    while (freeBlock != NULL && size > 0)
    {
        size -= BLOCK_SIZE;
        if (size > 0) {
            FAT[freeBlock->location] = freeBlock->next->location;
            writeFAT(freeBlock->location, freeBlock->next->location);
        } else {
            FAT[freeBlock->location] = 0x5F5F;
            writeFAT(freeBlock->location, 0x5F5F);
        }
        
        previousBlock = (freeBlock->location * BLOCK_SIZE + sizeof(uint16_t) * freeBlock->location + freeBlock->location) + BLOCK_SIZE;
        temp = freeBlock;
        freeBlock = freeBlock->next;

        // Seek to the calculated position
        if (fseek(disk, previousBlock, SEEK_SET) != 0) {
            perror("Error seeking in file");
            exit(EXIT_FAILURE);
        }

        // Write the 2-byte number to the file
        if (fwrite(&freeBlock->location, sizeof(uint16_t), 1, disk) != 1) {
            perror("Error writing to file");
            exit(EXIT_FAILURE);
        }
        
        deleteBlock(temp);
    }
    addFileToList(newNode);
    saveFile(newNode);
    directory->quantity++;
    directory->size += sizeSave;
    printf("Archivo creado con éxito", name);
}



// Función para escribir en el archivo
static void writeFile(const char *name, int offset, const char *data){
    
    struct File *temp = directory->head;
    while (temp != NULL)
    {
        if (strcmp(temp->name, name) == 0)
        {
            int dataSize = strlen(data);
            if ((offset+dataSize) > temp->size)
            {
                printf("Error: El offset es mayor que el tamaño del archivo.\n");
                return;
            }
            if (dataSize == 0)
            {
                printf("Error: No hay datos para escribir.\n");
                return;
            }
            if (dataSize > temp->size - offset)
            {
                printf("Error: No hay suficiente espacio para escribir los datos.\n");
                return;
            }
            int block = temp->firstBlock;
            // El offset puede ser mayor que un bloque, entonces se busca el bloque correcto
            int byteCount = BLOCK_SIZE; // Primero, byteCount se usa para llegar a la posición del bloque en el disco
            int blockOffset = 0;
            while (block != 0x5F5F)
            {
                
                if (byteCount >= offset)
                {
                    
                    blockOffset = BLOCK_SIZE - (byteCount - offset);
                    byteCount -= offset; // Aquí, byteCounts tiene la cantidad de bytes que se pueden escribir en el bloque
                    break;
                }
                block = FAT[block];
                byteCount += BLOCK_SIZE;
            }
            if (block == 0xFFFF)
            {
                printf("Error: El archivo no tiene suficientes bloques asignados.\n");
                return;
            }

            if (dataSize < byteCount) {
                byteCount = dataSize;
            }
            int blockPosition = (block * BLOCK_SIZE + sizeof(uint16_t) * block + block) + blockOffset;
            int dataPosition = 0;
            while (dataSize > 0) {
                if (fseek(disk, blockPosition, SEEK_SET) != 0)
                {
                    perror("Error seeking in file");
                    exit(EXIT_FAILURE);
                }
                if (fwrite(data + dataPosition, sizeof(char), byteCount, disk) != byteCount)
                {
                    perror("Error writing to file");
                    exit(EXIT_FAILURE);
                }
                block = FAT[block];
                blockPosition = block * BLOCK_SIZE + sizeof(uint16_t) * block + block;
                dataPosition += byteCount;
                dataSize -= byteCount;
                if (dataSize < byteCount) {
                    byteCount = dataSize;
                } else {
                    byteCount = BLOCK_SIZE;
                }
            }
            break;
        }
        temp = temp->next;
    }
    if (temp == NULL)
    {
        printf("Error: El archivo no existe.\n");
    }
    
}

// // Función para imprimir la lista
// static void printList(struct FileList *list)
// {
//     struct File *temp = list->head; // Variable local para iterar sobre la lista
//     while (temp != NULL)
//     {
//         printf("Nombre: %c, Desde: %d, Hasta: %d\n", temp->name, temp->address, temp->address + temp->size);
//         temp = temp->next;
//     }
// }



// // Función para dividir la lista en dos mitades. Esto se usa en el mergeSort
// struct File *split(struct File *head)
// {
//     struct File *fast = head, *slow = head;

//     while (fast->next && fast->next->next)
//     {
//         fast = fast->next->next;
//         slow = slow->next;
//     }

//     struct File *second_half = slow->next;
//     slow->next = NULL;
//     if (second_half)
//         second_half->prev = NULL;

//     return second_half;
// }

// // Función para comparar dos nodos y verificar cuál debería ir antes en la lista
// static int compareNodesMerge(struct FileList *list, struct File *node1, struct File *node2)
// {
//     if (list == assignedList)
//     {
//         // Compare addresses
//         if (node1->address < node2->address)
//             return -1;
//         else if (node1->address > node2->address)
//             return 1;
//         else
//             return 0;
//     }
//     else
//     {
//         // Sorting algorithm based on sorting type
//         switch (SORTING_ALGORITHM)
//         {
//         case 1: // First Fit
//             if (node1->address < node2->address)
//                 return -1;
//             else if (node1->address > node2->address)
//                 return 1;
//             else
//                 return 0;
//         case 2: // Best Fit
//             if (node1->size < node2->size)
//                 return -1;
//             else if (node1->size > node2->size)
//                 return 1;
//             else
//                 return 0;
//         case 3: // Worst Fit
//             if (node1->size > node2->size)
//                 return -1;
//             else if (node1->size < node2->size)
//                 return 1;
//             else
//                 return 0;
//         default:
//             return 0;
//         }
//     }
// }

// static int compareNodes(struct FileList *list, struct File *node1, struct File *node2)
// {
//     if (list == assignedList)
//     {
//         return node1->address > node2->address;
//     }
//     else
//     {
//         switch (SORTING_ALGORITHM)
//         {
//         case 1: // First Fit
//             return node1->address > node2->address;
//             break;
//         case 2: // Best Fit
//             return node1->size > node2->size;
//             break;
//         case 3: // Worst Fit
//             return node1->size < node2->size;
//             break;
//         default:
//             break;
//         }
//     }
// }

// // Función para fusionar dos listas
// struct File *merge(struct FileList *list, struct File *first, struct File *second)
// {
//     // Si la primera lista está vacía
//     if (!first)
//         return second;

//     // Si la segunda lista está vacía
//     if (!second)
//         return first;

//     // Se comparan los nodos y se fusionan en orden
//     if (compareNodesMerge(list, first, second) <= 0)
//     {
//         first->next = merge(list, first->next, second);
//         if (first->next)
//             first->next->prev = first;
//         first->prev = NULL;
//         return first;
//     }
//     else
//     {
//         second->next = merge(list, first, second->next);
//         if (second->next)
//             second->next->prev = second;
//         second->prev = NULL;
//         return second;
//     }
// }

// // Función para ordenar la lista
// struct File *mergeSort(struct FileList *list, struct File *head)
// {
//     if (!head || !head->next)
//         return head;

//     // Split the list into two halves
//     struct File *second = split(head);

//     // Recursively sort the sublists
//     head = mergeSort(list, head);
//     second = mergeSort(list, second);

//     // Merge the sorted halves
//     return merge(list, head, second);
// }

// // Function para ordenar la lista
// void sortList(struct FileList *list)
// {
//     if (!list->head)
//         return;

//     // Se ordena la lista
//     list->head = mergeSort(list, list->head);

//     // Se actualiza la cola
//     struct File *temp = list->head;
//     while (temp->next)
//         temp = temp->next;
//     list->tail = temp;
// }



// // Función para borrar un nodo de una lista
// static void deleteNode(struct FileList *list, struct File *nodeToDelete)
// {
//     if (nodeToDelete == NULL)
//     {
//         printf("El nodo a eliminar no es válido.\n");
//         return;
//     }

//     // Si el nodo es la cabeza
//     if (nodeToDelete == list->head)
//     {
//         // Si la lista solo tiene un nodo
//         if (list->tail == list->head)
//         {
//             list->tail = NULL;
//         }
//         list->head = nodeToDelete->next; // Se actualiza la cabeza

//         // Si la lista no está vacía, se actualiza el puntero previo de la nueva cabeza
//         if (list->head != NULL)
//         {
//             list->head->prev = NULL;
//         }
//     }
//     // Si el nodo es la cola
//     else if (nodeToDelete == list->tail)
//     {
//         list->tail = nodeToDelete->prev; // Update the tail
//         if (list->tail != NULL)
//         {
//             list->tail->next = NULL; // Set the next pointer of the new tail to NULL
//         }
//     }
//     // If the node to delete is in the middle
//     else
//     {
//         nodeToDelete->prev->next = nodeToDelete->next;
//         nodeToDelete->next->prev = nodeToDelete->prev;
//     }
//     free(nodeToDelete); // Se libera la memoria
// }



// // Función para asignar memoria
// static void allocateMemory(char name, int size)
// {
//     // Caso cuando no hay espacio en la memoria
//     if (unassignedList == NULL || unassignedList->head == NULL)
//     {
//         printf("No hay espacio disponible para asignar memoria a %c.\n", name);
//     }
//     // Cuando hay huecos en la memoria
//     else
//     {
//         // Se busca el hueco
//         signed int address = findHole(name, size);
//         if (address != -1) // Si se encontró el hueco
//         {
//             for (int i = 0; i < size; i++) // Se escribe en la memoria
//             {
//                 simulatedMemory[address + i] = name;
//             }
//         }
//         else
//         { // Si no, no hay espacio suficiente
//             printf("No hay espacio contiguo suficiente para asignar memoria a %c.\n", name);
//         }
//     }
// }

// // Función para fusionar la memoria
// static void mergeMemory(struct File *current)
// {
//     struct File *afterHoleNode = NULL;
//     struct File *beforeHoleNode = NULL;
//     struct File *unassignedNode = unassignedList->head;
//     while (unassignedNode != NULL)
//     {
//         if (unassignedNode->address == (current->address + current->size))
//         {
//             afterHoleNode = unassignedNode; // Se encuentra si hay un hueco después del que se intenta fusionar.
//         }
//         else if (current->address == (unassignedNode->address + unassignedNode->size))
//         {
//             beforeHoleNode = unassignedNode; // Se encuentra is hay un hueco antes del que se intenta fusionar.
//         }
//         unassignedNode = unassignedNode->next;
//     }
//     // Si hay un hueco antes y después
//     if (afterHoleNode != NULL && beforeHoleNode != NULL)
//     {
//         // Se eliminan los dos nodos de la derecha, se deja solo el de la izquierda y se suma al tamaño el tamaño de los borrados
//         beforeHoleNode->size = current->size + afterHoleNode->size + beforeHoleNode->size;
//         deleteNode(unassignedList, afterHoleNode);
//         deleteNode(unassignedList, current);
//     } // Si solo hay un hueco después
//     else if (afterHoleNode != NULL)
//     {
//         // Se elimina el nodo después y se suma su tamaño al hueco actual
//         current->size = current->size + afterHoleNode->size;
//         deleteNode(unassignedList, afterHoleNode);
//     }
//     // Si solo hay un hueco antes
//     else if (beforeHoleNode != NULL)
//     {
//         // Se elimina el hueco actual y se añade su tamaño al hueco anterior
//         beforeHoleNode->size = current->size + beforeHoleNode->size;
//         deleteNode(unassignedList, current);
//     }
//     sortList(unassignedList); // Se ordena la lista
// }

// // Función para eliminar nodos no asignados y escribir '_' en la memoria
// static void freeNodeMemory(struct File *unassignedNode, int size, int address)
// {
//     if (unassignedNode->size == 0)
//     {
//         deleteNode(unassignedList, unassignedNode);
//         return;
//     }
//     mergeMemory(unassignedNode);
//     for (int i = 0; i < size; i++)
//     {
//         simulatedMemory[address + i] = '_';
//     }
// }

// static struct File* findBeforeNode(struct File *current)
// {
//     struct File *beforeHoleNode = unassignedList->head;
//     while (beforeHoleNode != NULL)
//     {
//         if (current->address == (beforeHoleNode->address + beforeHoleNode->size))
//         {
//             return beforeHoleNode;
//         }
//         beforeHoleNode = beforeHoleNode->next;
//     }
//     return beforeHoleNode;
// }

// struct File* findAfterNode(struct File *current)
// {
//     struct File *afterHoleNode = unassignedList->head;
//     while (afterHoleNode != NULL)
//     {
//         if (afterHoleNode->address == (current->address + current->size))
//         {
//             return afterHoleNode;
//         }
//         afterHoleNode = afterHoleNode->next;
//     }
//     return afterHoleNode;
// }		

// // Función para reasignar memoria
// static void reallocateMemory(char name, int newSize)
// {
//     // Se valida que el tamaño sea válido
//     if (newSize <= 0)
//     {
//         printf("Error: El tamaño de la memoria debe ser mayor a 0.\n");
//         return;
//     }
//     struct File *current = assignedList->head;
//     while (current != NULL)
//     {
//         if (current->name == name)
//         {
//             // Hay seis casos posibles:
//             // 1. La nueva asignacion de tamaño es menor a la actual
//             if (newSize <= current->size)
//             {
//                 struct File *unassignedNode = unassignedList->tail;
//                 while (unassignedNode != NULL) // Se busca si hay un hueco después del nodo asignado
//                 {
//                     if (unassignedNode->address == (current->address + current->size))
//                     {
//                         // Si hay un hueco después, se le añade el tamaño restante
//                         int remainderSize = current->size - newSize;
//                         current->size = newSize;                  // Se actualiza el tamaño
//                         unassignedNode->size += remainderSize;    // Se actualiza el tamaño del hueco
//                         unassignedNode->address -= remainderSize; // Se actualiza la dirección del hueco
//                         // mergeMemory(unassignedNode);                                     // Se fusiona la memoria
//                         freeNodeMemory(unassignedNode, current->size, current->address); // Se llama a esta función para escribir '_' en la memoria
//                         for (int i = 0; i < current->size; i++)                          // Se escribe en la memoria
//                         {
//                             simulatedMemory[current->address + i] = current->name;
//                         }
//                         return;
//                     }
//                     else
//                     {
//                         unassignedNode = unassignedNode->prev;
//                     }
//                 }
//                 // Si no hay hueco después del nodo asignado
//                 if (unassignedNode == NULL)
//                 {
//                     // Se añade un hueco después del nodo asignado
//                     addValue(unassignedList, '_', current->address + newSize, current->size - newSize);
//                     // Se escribe '_' en la memoria
//                     for (int i = 0; i < current->size - newSize; i++)
//                     {
//                         simulatedMemory[current->address + newSize + i] = '_';
//                     }
//                     current->size = newSize;                // Se actualiza el tamaño
//                     for (int i = 0; i < current->size; i++) // Se escribe en la memoria
//                     {
//                         simulatedMemory[current->address + i] = current->name;
//                     }
//                     printf("Memoria reasignada para %c.\n", current->name);
//                     return;
//                 }
//             }
//             // Se va a buscar si hay un hueco antes y después del nodo asignado
//             struct File *beforeHoleNode = findBeforeNode(current);                 // Hueco que está antes del hueco actual
//             struct File *afterHoleNode = findAfterNode(current); // Hueco actual

//             // 2. Hay un hueco después de la asignación del cual se le puede asignar más memoria
//             if (afterHoleNode != NULL && newSize <= (afterHoleNode->size + current->size))
//             {
//                 int remainderSize = newSize - current->size;
//                 current->size = newSize;                                         // Se actualiza el tamaño
//                 afterHoleNode->size -= remainderSize;                           // Se actualiza el tamaño del hueco
//                 afterHoleNode->address += remainderSize;                        // Se actualiza la dirección del hueco
//                 freeNodeMemory(afterHoleNode, current->size, current->address); // Se escribe un 0 en la memoria
//                 for (int i = 0; i < remainderSize; i++)
//                 {
//                     simulatedMemory[current->address + newSize - i] = current->name;
//                 }
//                 return;

//             } // 3. Hay un hueco antes de la asignación del cual se le puede asignar más memoria
//             else if (beforeHoleNode != NULL && newSize <= (beforeHoleNode->size + current->size))
//             {

//                 int remainderSize = newSize - current->size;
//                 current->size = newSize;
//                 current->address -= remainderSize;

//                 beforeHoleNode->size -= remainderSize;
//                 freeNodeMemory(beforeHoleNode, current->size, current->address); // Function to free memory and merge
//                 for (int i = 0; i < current->size; i++)
//                 {
//                     simulatedMemory[current->address + i] = current->name;
//                 }
//                 return;
//             }
//             // 4. Hay huecos antes y después de la asignación de los cuales se le puede asignar más memoria
//             else if (beforeHoleNode != NULL && // Validación de que beforeHoleNode no sea nulo
//                         afterHoleNode != NULL &&
//                         newSize <= (afterHoleNode->size + current->size + beforeHoleNode->size))
//             {
//                 // El tamaño disponible es la suma del hueco antes, el nodo actual y el hueco después
//                 int remainderSize = current->size + beforeHoleNode->size + afterHoleNode->size - newSize;

//                 // Mover la dirección actual al comienzo del hueco anterior
//                 current->address = beforeHoleNode->address;
//                 current->size = newSize;

//                 // Eliminar el hueco anterior
//                 deleteNode(unassignedList, beforeHoleNode);

//                 // Ajustar el hueco siguiente (después del bloque actual)
//                 afterHoleNode->size = remainderSize;
//                 afterHoleNode->address = current->address + newSize;

//                 // Si el hueco después ya no tiene tamaño, eliminarlo
//                 if (afterHoleNode->size == 0)
//                 {
//                     deleteNode(unassignedList, afterHoleNode);
//                     mergeMemory(afterHoleNode);
//                 }
//                 else
//                 { // Limpiar la memoria del bloque anterior
//                     for (int i = 0; i < afterHoleNode->size; i++)
//                     {
//                         simulatedMemory[afterHoleNode->address + i] = '_';
//                     }
//                 }
//                 // Escribir en la memoria el valor que había en el bloque anterior
//                 for (int i = 0; i < current->size; i++)
//                 {
//                     simulatedMemory[current->address + i] = current->name;
//                 }
//                 printf("Memoria reasignada para %c.\n", current->name);
//                 return;
//             } // 5. El espacio alrededor del nodo asignado no es suficiente para la reasignación, entonces se tiene que mover a otro lado.
//             else if (afterHoleNode != NULL || beforeHoleNode != NULL)
//             {
//                 // Se busca un hueco donde quepa el nuevo tamaño
//                 int newAddress = findHole(current->name, newSize);
//                 if (newAddress != -1) // Si hay un hueco donde cabe
//                 {
//                     for (int i = 0; i < current->size; i++) // Se reinicia la memoria donde está el nodo actual
//                     {
//                         simulatedMemory[current->address + i] = '_';
//                     }
//                     struct File *newUnassignedNode = addValue(unassignedList, '_', current->address, current->size); // Se añade un hueco donde está el nodo actual
//                     mergeMemory(newUnassignedNode);                                                                  // Se fusiona la memoria
//                     deleteNode(assignedList, current);                                                               // Eliminar de la lista de asignados, ya que se insertó en la nueva posición

//                     for (int i = 0; i < newSize; i++)
//                     { // Se escribe en la memoria el nuevo valor
//                         simulatedMemory[newAddress + i] = current->name;
//                     }
//                     printf("Memoria reasignada para %c.\n", current->name);
//                     return;
//                 }
//                 else
//                 {
//                     // 6. En ningún lado de la memoria hay suficiente espacio, da error.
//                     printf("Error: No hay espacio suficiente para reasignar %c.\n", current->name);
//                     return;
//                 }
//             }
//         }
//         current = current->next;
//     }
// }

// // Esta función libera la memoria de un nodo asignado
// static void freeMemory(char name)
// {
//     struct File *current = assignedList->head;
//     while (current != NULL)
//     {
//         if (current->name == name)
//         {
//             struct File *newUnassignedNode = addValue(unassignedList, '_', current->address, current->size); // Se añade un hueco
//             mergeMemory(newUnassignedNode);                                                                  // Se fusiona la memoria
//             printf("Memoria liberada para %c.\n", current->name);
//             for (int i = 0; i < current->size; i++)
//             {
//                 simulatedMemory[current->address + i] = '_'; // Se reinicia la memoria
//             }
//             deleteNode(assignedList, current); // Eliminar de la lista
//             return;
//         }
//         else
//         {
//             current = current->next;
//         }
//     }
// }

// // Se imprime la memoria
// static void printMemory()
// {
//     printf("Memoria simulada:\n");

//     // Buffer para almacenar hasta 100 caracteres
//     char buffer[101]; // 100 caracteres + 1 para el terminador nulo
//     int bufferIndex = 0;

//     for (int i = 0; i < MEMORY_SIZE; i++)
//     {
//         // Concatenar el carácter actual al buffer
//         buffer[bufferIndex++] = simulatedMemory[i];

//         // Cuando el buffer tiene 100 caracteres, lo imprimimos como string
//         if (bufferIndex == 100)
//         {
//             buffer[bufferIndex] = '\0'; // Asegurar el terminador nulo
//             printf("%s\n", buffer);     // Imprimir el buffer como cadena
//             bufferIndex = 0;            // Reiniciar el índice del buffer
//         }
//     }

//     // Si quedan caracteres en el buffer al final, imprimirlos
//     if (bufferIndex > 0)
//     {
//         buffer[bufferIndex] = '\0'; // Asegurar el terminador nulo
//         printf("%s\n", buffer);     // Imprimir el buffer restante
//     }
// }

// // Función para reiniciar la memoria
// static void reset()
// {
//     unassignedList = createList(&unassignedList);
//     assignedList = createList(&assignedList);
//     startMemory();
//     addValue(unassignedList, '_', 0, MEMORY_SIZE);
// }