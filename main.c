#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "file_system.c"

#define MAX_FILES 100
#define MAX_FILENAME 50
#define MAX_CONTENT_SIZE 1000

// Funcion para procesar cada linea
void processLine(const char *instruction)
{
    // Revisa si la linea esta vacia
    if (instruction[0] == '\0' || strlen(instruction) == 0)
    {
        printf(" \n");
        return;
    }

    // Salta las lienas que empiezan con '#'
    if (instruction[0] == '#')
    {
        return;
    }

    // Imprime la lineas que empiezan con '+'
    if (instruction[0] == '+')
    {
        printf("%s\n", instruction + 1);
        return;
    }

    // Realiza los comandos en base a la instruccion del archivo
    char command[10], name[MAX_FILENAME], data[MAX_CONTENT_SIZE];
    int offset, size, length;

    if (sscanf(instruction, "CREATE %s %d", name, &size) == 2)
    {
        createFile(name, size);
    }
    else if (sscanf(instruction, "WRITE %s %d \"%[^\"]\"", name, &offset, data) == 3)
    {
        writeFile(name, offset, data);
    }
    else if (sscanf(instruction, "READ %s %d %d", name, &offset, &length) == 3)
    {
        readFile(name, offset, length);
    }
    else if (strncmp(instruction, "LIST", 4) == 0)
    {
        listFiles();
    }
    else if (strncmp(instruction, "RESET", 5) == 0)
    {
        reset();
    }
    else if (strncmp(instruction, "READ", 4) == 0)
    {
        read();
    }
    else if (sscanf(instruction, "DELETE %s", name) == 1)
    {
        deleteFile(name);
    }
    else
    {
        printf("Error: Unknown instruction '%s'\n", instruction);
    }
}

// Funcion para leer las lineas del archivo
void readLinesFromFile(const char *filename)
{
    FILE *file = fopen(filename, "r");
    if (file == NULL)
    {
        perror("Error opening the file");
        exit(EXIT_FAILURE);
    }

    char instruction[256];
    while (fgets(instruction, sizeof(instruction), file))
    {
        instruction[strcspn(instruction, "\n")] = '\0';
        processLine(instruction);
    }

    fclose(file); // Cerrar el archivo
}

int main(int argc, char *argv[])
{
    disk = fopen("disk.txt", "r+b"); // Use "r+b" to read/write in binary mode
    if (disk == NULL)
    {
        perror("Error opening file");
        return EXIT_FAILURE;
    }
    FATFile = fopen("FAT.txt", "r+b");
    if (FATFile == NULL)
    {
        perror("Error opening file");
        return EXIT_FAILURE;
    }
    directoryFile = fopen("directory.txt", "r+");
    if (directoryFile == NULL)
    {
        perror("Error opening file");
        return EXIT_FAILURE;
    }

    directory = createFileList();
    openFiles = createFileList();
    freeBlocks = createBlockList();
    readFAT();
    readDirectory();

    readLinesFromFile("pruebas.txt");

    fclose(disk);
    fclose(FATFile);
    fclose(directoryFile);

    return 0;
}