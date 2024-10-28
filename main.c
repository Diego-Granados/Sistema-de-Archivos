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
    // Check if the line is empty or contains only a newline character
    if (instruction[0] == '\0' || strlen(instruction) == 0)
    {
        printf(" \n"); // Print a space and a newline for empty lines
        return;
    }

    // Skip lines starting with '#'
    if (instruction[0] == '#')
    {
        return;
    }

    // Print lines that start with '+', excluding the '+'
    if (instruction[0] == '+')
    {
        printf("%s\n", instruction + 1); // Print without the '+'
        return;
    }

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
        instruction[strcspn(instruction, "\n")] = '\0'; // Remove newline character
        processLine(instruction);
    }

    fclose(file); // Cerrar el archivo
}

int main(int argc, char *argv[])
{
    // if (argc < 2)
    // {
    //     printf("Uso: %s <nombre del archivo>\n", argv[0]);
    //     return 1;
    // }

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

    // readLinesFromFile(argv[1]);
    readLinesFromFile("pruebas.txt");
    // resetDirectory();
    // resetFAT();
    // resetDisk();
    // readFAT();
    // readDirectory();

    // createFile("file1.txt", 1500);
    // createFile("file2.txt", 2000);
    // createFile("file3.txt", 1000);
    // deleteFile("file2.txt");
    // createFile("file4.txt", 2500);
    // listFiles();
    // writeFile("file1.txt", 0, "Hola mundo");
    fclose(disk);
    fclose(FATFile);
    fclose(directoryFile);

    return 0;
}