#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "file_system.c"

// Funcion para procesar cada linea
// void processLine(const char *line)
// {
//   char command[10]; // Buffer para almacenar el comando
//   char name;        // Variable para el nombre (carácter)

//   size_t size; // Variable para el tamaño

//   // Parsea la linea (con ALLOC, REALLOC y FREE)
//   if (sscanf(line, "%s %c %zu", command, &name, &size) == 3) // Usa &name para pasar la dirección de name
//   {
//     if (strcmp(command, "ALLOC") == 0)
//     {
//       allocateMemory(name, size);
//     }
//     else if (strcmp(command, "REALLOC") == 0)
//     {
//       reallocateMemory(name, size);
//     }
//   }
//   else if (sscanf(line, "%s %c", command, &name) == 2)
//   {
//     if (strcmp(command, "FREE") == 0)
//     {
//       freeMemory(name);
//     }
//   }
//   // Caso de PRINT que no requiere más argumentos
//   else if (strcmp(command, "PRINT") == 0)
//   {
//     printf("Lista de memoria asignada:\n");
//     printList(assignedList);
//     printf("Lista de memoria no asignada o huecos:\n");
//     printList(unassignedList);
//     printMemory();
//     // printMemory();
//   }
//   else if (strcmp(command, "RESET") == 0)
//   {
//     reset();
//   }
//   else
//   {
//     printf("Invalid command: %s\n", line); // Mensaje de error si el comando no es válido
//   }
// }

// // Funcion para leer las lineas del archivo
// void readLinesFromFile(const char *filename)
// {
//   FILE *file = fopen(filename, "r");
//   if (file == NULL)
//   {
//     perror("Error opening the file");
//     exit(EXIT_FAILURE);
//   }

//   char line[50]; // Buffer para almacenar cada línea

//   // Leer línea por línea del archivo
//   while (fgets(line, sizeof(line), file))
//   {
//     if (line[0] == '#' || line[0] == '\n')
//     {
//       continue; // Ignorar comentarios
//     }
//     else if (line[0] == '+')
//     {
//       printf("\n%s", line);
//       continue; // Ignorar líneas en blanco
//     }
//     processLine(line); // Procesar la línea
//   }

//   fclose(file); // Cerrar el archivo
// }

int main(int argc, char *argv[])
{
  
  disk = fopen("disk.txt", "r+b"); // Use "r+b" to read/write in binary mode
  if (disk == NULL) {
      perror("Error opening file");
      return EXIT_FAILURE;
  }
  FATFile = fopen("FAT.txt", "r+b");
  if (FATFile == NULL) {
      perror("Error opening file");
      return EXIT_FAILURE;
  }
  directoryFile = fopen("directory.txt", "r+");
  if (directoryFile == NULL) {
      perror("Error opening file");
      return EXIT_FAILURE;
  }

  directory = createFileList();
  openFiles = createFileList();
  freeBlocks = createBlockList();
  resetDirectory();
  resetFAT();
  resetDisk();
  readFAT();
  readDirectory();

  createFile("file1.txt", 1500);
  createFile("file2.txt", 2000);
  createFile("file3.txt", 1000);
  deleteFile("file2.txt");
  createFile("file4.txt", 2500);
  listFiles();
  // writeFile("file1.txt", 0, "Hola mundo");
  fclose(disk);
  fclose(FATFile);
  fclose(directoryFile);
  
  // if (argc < 2)
  // {
  //   printf("Uso: %s <nombre del archivo>\n", argv[0]);
  //   return 1;
  // }

  // // Crear las listas globales
  // createList(&assignedList);
  // createList(&unassignedList);
  // startMemory();
  // addValue(unassignedList, '0', 0, MEMORY_SIZE);

  // readLinesFromFile(argv[1]);

  // return 0;
}