#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#define DISK_SIZE 1048576
#define BLOCK_SIZE 510
#define MAX_FILES 100
#define NUM_BLOCKS 2048
#define MAX_LINE_LENGTH 1024
// El máximo tamaño de un archivo es de 510 * 2048 = 1044480 bytes

// Vamos a usar para los archivos lo de la lista enlazada y para los bloques libres el counting

// Structure for the node that contains the key and the value
// Struct para los nodos de las listas.
struct File
{
    char *name;          // El nombre del archivo
    unsigned int size;   // Tamaño
    uint16_t firstBlock; // Puntero al próximo bloque
    struct File *prev;   // Puntero al nodo anterior
    struct File *next;   // Puntero al próximo nodo
};

// Struct para la lista
struct FileList
{
    uint8_t quantity;
    unsigned int size;
    struct File *head; // Puntero a la cabeza de la lista
    struct File *tail; // Puntero a la cola de la lista
};

// Función para crear una lista
static struct FileList *createFileList()
{
    struct FileList *list = (struct FileList *)malloc(sizeof(struct FileList)); // Asignar memoria para la lista
    if (list == NULL)
    {
        printf("Error de asignación de memoria para la lista!\n");
        exit(1);
    }
    (list)->head = NULL;
    (list)->tail = NULL;
    list->quantity = 0;
    list->size = 0;
    return list;
}
// Struct para el bloque
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
FILE *directoryFile;
// Función para crear una lista
static struct BlockList *createBlockList()
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
static struct File *createFileRef(const char *name, int size, uint16_t firstBlock)
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
    if (newFile->name == NULL)
    {
        printf("Error de asignación de memoria para el nodo!\n");
        exit(1);
    }
    strcpy(newFile->name, name); // Todavía no hay una dirección asignada
    newFile->size = size;
    newFile->firstBlock = firstBlock;
    newFile->next = NULL;
    newFile->prev = NULL;
    return newFile;
}

// Función para crear un nodo
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
                // Si el nodo debe ir al principio de la lista
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
    if (nodeToDelete == NULL) // Si el nodo a eliminar no es válido
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

// Función para borrar un nodo de una lista
static void deleteFileFromList(struct File *nodeToDelete)
{
    if (nodeToDelete == NULL) // Si el nodo a eliminar no es válido
    {
        printf("El nodo a eliminar no es válido.\n");
        return;
    }

    // Si el nodo es la cabeza
    if (nodeToDelete == directory->head)
    {
        // Si la lista solo tiene un nodo
        if (directory->tail == directory->head)
        {
            directory->tail = NULL;
        }
        directory->head = nodeToDelete->next; // Se actualiza la cabeza

        // Si la lista no está vacía, se actualiza el puntero previo de la nueva cabeza
        if (directory->head != NULL)
        {
            directory->head->prev = NULL;
        }
    }
    // Si el nodo es la cola
    else if (nodeToDelete == directory->tail)
    {
        directory->tail = nodeToDelete->prev; // Update the tail
        if (directory->tail != NULL)
        {
            directory->tail->next = NULL; // Set the next pointer of the new tail to NULL
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

// Inicializar el sistema de archivos que guarda los bloques libres
void initializeFAT()
{
    for (int i = 0; i < NUM_BLOCKS; i++)
    {
        FAT[i] = 0x5F5F;
        struct Block *newBlock = createBlockRef(i);
        addBlockToList(newBlock);
    }
}

// Función para escribir en el FAD para el bloque y el valor
static void writeFAT(uint16_t block, uint16_t value)
{
    int position = (block * sizeof(uint16_t));
    if (fseek(FATFile, position, SEEK_SET) != 0)
    {
        perror("Error seeking in file");
        exit(EXIT_FAILURE);
    }

    if (fwrite(&value, sizeof(uint16_t), 1, FATFile) != 1)
    {
        perror("Error writing to file");
        exit(EXIT_FAILURE);
    }
}

// Funcion para leer el FAT
static void readFAT()
{
    if (fseek(FATFile, 0, SEEK_SET) != 0)
    {
        perror("Error seeking in file");
        exit(EXIT_FAILURE);
    }

    if (fread(FAT, sizeof(uint16_t), NUM_BLOCKS, FATFile) != NUM_BLOCKS)
    {
        perror("Error reading from file");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < NUM_BLOCKS; i++)
    {
        if (FAT[i] == 0x5F5F)
        {
            struct Block *newBlock = createBlockRef(i);
            addBlockToList(newBlock);
        }
    }
}

// Funcion para resetear el FAD
static void resetFAT()
{
    // Write the FAT to the file
    initializeFAT();

    if (fwrite(FAT, sizeof(uint16_t), NUM_BLOCKS, FATFile) != NUM_BLOCKS)
    {
        perror("Error writing to file");
        exit(EXIT_FAILURE);
    }
}

// Función para guardar el archivo en el directorio
static void saveFile(struct File *temp)
{
    fclose(directoryFile);
    directoryFile = fopen("directory.txt", "a+"); // Open in read/append mode
    if (directoryFile == NULL)
    {
        perror("Error opening file");
        return;
    }

    // No need for fseek, since "a+" mode appends by default
    if (fprintf(directoryFile, "%s %d %d\n", temp->name, temp->size, temp->firstBlock) < 0)
    {
        perror("Error writing to file");
        return;
    }

    fclose(directoryFile);
    directoryFile = fopen("directory.txt", "r+"); // Open in read/write mode
    if (directoryFile == NULL)
    {
        perror("Error opening file");
        return;
    }
}

// Funcion para leer el directorio
static void readDirectory()
{
    if (fseek(directoryFile, 0, SEEK_SET) != 0)
    {
        perror("Error seeking in file");
        exit(EXIT_FAILURE);
    }

    char name[50];
    int size;
    int firstBlock;
    directory->quantity = 0;
    directory->size = 0;
    // Lee cada lina del directorio y crea un nodo para cada archivo
    while (fscanf(directoryFile, "%s %d %d\n", name, &size, &firstBlock) != EOF)
    {
        struct File *temp = createFileRef(name, size, firstBlock);
        addFileToList(temp);
        directory->quantity++;
        directory->size += size;
    }
}
// Función para resetear el disco
int resetDisk()
{
    // Asegurarse de que el archivo esté abierto en modo escritura binaria
    if (disk == NULL)
    {
        perror("Error: el archivo 'disk' no está abierto");
        return -1;
    }

    // Crear un buffer lleno de '_'
    char buffer[BLOCK_SIZE + sizeof(uint16_t)];
    for (int i = 0; i < BLOCK_SIZE + sizeof(uint16_t); i++)
    {
        buffer[i] = '_';
    }

    // Posicionar y escribir en cada bloque del disco
    for (int i = 0; i < NUM_BLOCKS; i++)
    {
        // Calcular la posición del bloque en el disco
        long blockPosition = i * (BLOCK_SIZE + sizeof(uint16_t)) + i;

        // Mover el puntero del archivo a la posición calculada
        if (fseek(disk, blockPosition, SEEK_SET) != 0)
        {
            perror("Error al posicionar en el archivo");
            return -1;
        }

        // Escribir el contenido del buffer en la posición actual del archivo
        if (fwrite(buffer, sizeof(char), BLOCK_SIZE + sizeof(uint16_t), disk) != BLOCK_SIZE + sizeof(uint16_t))
        {
            perror("Error al escribir en el archivo");
            return -1;
        }
    }

    fflush(disk); // Asegura que todo se escriba en el archivo
    return 0;
}

// Función para resetear el directorio
void resetDirectory()
{
    fclose(directoryFile);
    directoryFile = fopen("directory.txt", "w");
    if (directoryFile == NULL)
    {
        perror("Error opening file");
        return;
    }
    fclose(directoryFile);
    directoryFile = fopen("directory.txt", "r+");
    if (directoryFile == NULL)
    {
        perror("Error opening file");
        return;
    }
}

// Función para encontrar si hay un hueco en la memoria donde cabe el valor
static void createFile(const char *name, int size)
{
    // Se usa para iterar por los bloques.
    struct Block *freeBlock = freeBlocks->head; // Se obtiene el primer bloque libre

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

    // Se crea un nuevo nodo para el archivo
    struct File *newNode = createFileRef(name, size, freeBlock->location);
    uint16_t previousBlock = 0;
    int sizeSave = size;
    struct Block *temp = NULL;
    // Se recorren los bloques libres para asignar el tamaño del archivo
    while (freeBlock != NULL && size > 0)
    {
        size -= BLOCK_SIZE;
        if (size > 0)
        {
            FAT[freeBlock->location] = freeBlock->next->location;
            writeFAT(freeBlock->location, freeBlock->next->location);
        }
        else
        {
            FAT[freeBlock->location] = 0x5F5F;
            writeFAT(freeBlock->location, 0x5F5F);
        }

        // La posición donde se va a escribir el puntero en el disco es la dirección del bloque * el tamano del bloque + el tamaño de un puntero * el número del bloque + la dirección del bloque
        previousBlock = (freeBlock->location * BLOCK_SIZE + sizeof(uint16_t) * freeBlock->location + freeBlock->location) + BLOCK_SIZE;

        // Seek to the calculated position
        if (fseek(disk, previousBlock, SEEK_SET) != 0)
        {
            perror("Error seeking in file");
            exit(EXIT_FAILURE);
        }

        // Write the 2-byte number to the file
        if (fwrite(&FAT[freeBlock->location], sizeof(uint16_t), 1, disk) != 1)
        {
            perror("Error writing to file");
            exit(EXIT_FAILURE);
        }
        // Se mueve al sigueinte bloque
        temp = freeBlock;
        freeBlock = freeBlock->next;
        deleteBlock(temp);
        freeBlocks->quantity--;
    }
    // Se agrega el archivo al directorio
    addFileToList(newNode);
    saveFile(newNode);
    directory->quantity++;
    directory->size += sizeSave;
    printf("Archivo creado con exito %s\n", name);
}

// Función para escribir en el archivo
static void writeFile(const char *name, int offset, const char *data)
{
    // Se busca el archivo en el directorio
    struct File *temp = directory->head;
    while (temp != NULL)
    {
        if (strcmp(temp->name, name) == 0)
        {
            int dataSize = strlen(data);
            // Se valida si el offset más lo que se va a escribir es mayor que el tamaño del archivo
            if ((offset + dataSize) > temp->size)
            {
                printf("Error: El offset es mayor que el tamano del archivo.\n");
                return;
            }
            // Valida que los datos no vengan vacíos
            if (dataSize == 0)
            {
                printf("Error: No hay datos para escribir.\n");
                return;
            }
            // Se busca el bloque donde se va a escribir
            int block = temp->firstBlock;
            // El offset puede ser mayor que un bloque, entonces se busca el bloque correcto
            int byteCount = BLOCK_SIZE; // Primero, byteCount se usa para llegar a la posición del bloque correcto con base en el offset en el disco
            int blockOffset = 0;        // Es el desplazamiento dentro del bloque correcto para empezar a escribir los datos
            // Se busca el bloque correcto
            while (block != 0x5F5F)
            {
                // Si byteCount es mayor o igual al offset, se encontró el bloque
                if (byteCount >= offset)
                {
                    blockOffset = BLOCK_SIZE - (byteCount - offset);
                    byteCount -= offset; // Aquí, byteCounts tiene la cantidad de bytes que se pueden escribir en el bloque
                    break;
                }
                // Se sigue buscando el bloque correcto
                block = FAT[block];
                byteCount += BLOCK_SIZE;
            }
            // Si el archivo no tiene suficientes bloques asignados
            if (block == 0x5F5F)
            {
                printf("Error: El archivo no tiene suficientes bloques asignados.\n");
                return;
            }
            // Aca se escriben solo los bytes necesarios
            // Aquí, byteCount se usa para la cantidad de bytes que se van a escribir en el bloque
            if (dataSize < byteCount)
            {
                byteCount = dataSize;
            }

            // Se encontro la posición del bloque correcto y se va a escribir los datos
            int blockPosition = (block * BLOCK_SIZE + sizeof(uint16_t) * block + block) + blockOffset;
            int dataPosition = 0; // Es un puntero a la posición de los datos que se van a escribir
            while (dataSize > 0 && block != 0x5F5F)
            {
                // Se mueve el puntero del archivo a la posición del bloque correcto
                if (fseek(disk, blockPosition, SEEK_SET) != 0)
                {
                    perror("Error seeking in file");
                    exit(EXIT_FAILURE);
                }
                // Se escriben los datos en el archivo
                if (fwrite(data + dataPosition, sizeof(char), byteCount, disk) != byteCount)
                {
                    perror("Error writing to file");
                    exit(EXIT_FAILURE);
                }
                // Siguiente bloque
                block = FAT[block];                                                    // Siguiente bloque
                blockPosition = block * BLOCK_SIZE + sizeof(uint16_t) * block + block; // Posición del siguiente bloque
                dataPosition += byteCount;                                             // Actualiza puntero de los datos
                dataSize -= byteCount;                                                 // Resta la cantidad de datos que se escribieron
                if (dataSize < BLOCK_SIZE)
                {
                    byteCount = dataSize;
                }
                else
                {
                    byteCount = BLOCK_SIZE;
                }
            }
            if (dataSize > 0)
            {
                printf("Error: No hay suficiente espacio en el disco para escribir los datos.\n");
                return;
            }
            printf("Datos escritos con exito en %s\n", name);
            break;
        }
        temp = temp->next;
    }
    if (temp == NULL)
    {
        printf("Error: El archivo no existe.\n");
    }
}

// Función para leer el archivo
static void readFile(const char *name, int offset, int size)
{
    struct File *temp = directory->head;
    while (temp != NULL)
    {
        // Se busca el archivo en el directorio en base al nombre
        if (strcmp(temp->name, name) == 0)
        {
            // Se valida si el offset más el tamaño es mayor que el tamaño del archivo
            if ((offset + size) > temp->size)
            {
                printf("Error: El offset es mayor o igual al tamano del archivo.\n");
                return;
            }
            int block = temp->firstBlock;
            int byteCount = BLOCK_SIZE;
            int blockOffset = 0;

            // Navegar hasta el bloque correcto basado en el offset
            while (block != 0x5F5F)
            {
                if (byteCount >= offset)
                {
                    blockOffset = BLOCK_SIZE - (byteCount - offset);
                    break;
                }
                block = FAT[block];
                byteCount += BLOCK_SIZE;
            }

            if (block == 0x5F5F)
            {
                printf("Error: Offset fuera del límite de bloques asignados al archivo.\n");
                return;
            }

            // Leer y mostrar contenido desde el offset hasta el tamaño especificado
            while (size > 0 && block != 0x5F5F)
            {
                int blockPosition = block * BLOCK_SIZE + sizeof(uint16_t) * block + block + blockOffset;

                if (fseek(disk, blockPosition, SEEK_SET) != 0)
                {
                    perror("Error al buscar en el archivo");
                    return;
                }
                // Leer y mostrar el contenido del archivo
                for (int i = blockOffset; i < BLOCK_SIZE && size > 0; i++)
                {
                    int ch = fgetc(disk);
                    if (ch == EOF)
                    {
                        return;
                    }
                    putchar(ch);
                    size--;
                }
                // Reiniciar offset para los bloques siguientes
                blockOffset = 0;
                block = FAT[block];
            }

            printf("\n"); // Nueva línea al final de la lectura del archivo
            return;
        }
        temp = temp->next;
    }

    printf("Error: El archivo no existe.\n");
}

// Funcion para borrar un archivo
static void deleteFile(const char *name)
{

    struct File *temp = directory->head;
    while (temp != NULL)
    {
        // Se busca el archivo en el directorio en base al nombre
        if (strcmp(temp->name, name) == 0)
        {
            uint16_t block = temp->firstBlock;
            // Se recorren los bloques asignados al archivo y se marcan como libres
            while (block != 0x5F5F)
            {
                struct Block *newBlock = createBlockRef(block);
                addBlockToList(newBlock);
                uint16_t oldBlock = block;
                block = FAT[block];
                FAT[oldBlock] = 0x5F5F;
                writeFAT(oldBlock, 0x5F5F);
            }
            // Elimina el archivo del directorios
            deleteFileFromList(temp);

            // Se abre un archivo temporal para escribir el directorio sin el archivo eliminado
            FILE *tempFile = fopen("temp.txt", "w");
            if (tempFile == NULL)
            {
                perror("Error opening temporary file");
                return;
            }

            char line[MAX_LINE_LENGTH];

            // Se lee cada linea del archivo original y se copia al archivo temporal
            while (fgets(line, MAX_LINE_LENGTH, directoryFile) != NULL)
            {
                if (strstr(line, name) == NULL)
                {
                    fputs(line, tempFile);
                }
            }
            fclose(directoryFile);
            fclose(tempFile);
            remove("directory.txt");             // Elimina el archivo original
            rename("temp.txt", "directory.txt"); // Renombra el archivo temporal al original
            directoryFile = fopen("directory.txt", "r+");
            if (directoryFile == NULL)
            {
                perror("Error opening file");
                return;
            }
            printf("Archivo eliminado con exito %s\n", name);
            break;
        }
        temp = temp->next;
    }
    if (temp == NULL)
    {
        printf("Error: El archivo no existe.\n");
    }
}

// Funcion para mostrar los archivos en el directorio
static void listFiles()
{
    if (directory->head == NULL)
    {
        printf("No hay archivos en el directorio.\n");
        return;
    }
    printf("Directorio: \n");
    struct File *temp = directory->head;
    while (temp != NULL)
    {
        printf("Nombre: %s, Tamano: %d bytes, Primer bloque: %d\n", temp->name, temp->size, temp->firstBlock);
        temp = temp->next;
    }
}

// Funcion para resetear el sistema de archivos con el fin de correr todas las pruebas de una vez
static void reset()
{
    freeBlocks = createBlockList();
    resetDirectory();
    resetFAT();
    resetDisk();
    directory = createFileList();
    openFiles = createFileList();
}

// Funcion para el directorio y FAD
static void read()
{
    readDirectory();
    readFAT();
}