// The MIT License (MIT)
// 
// Copyright (c) 2020 Trevor Bakker 
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <stdint.h> //needed to use uint

#define MAX_NUM_ARGUMENTS 3

#define WHITESPACE " \t\n"      // We want to split our command line up into tokens
                                // so we need to define what delimits our tokens.
                                // In this case  white space
                                // will separate the tokens on our command line

#define MAX_COMMAND_SIZE 255    // The maximum command-line size

//defining global variables
FILE *fp = NULL;  //file pointer to the FAT32 image
int is_open = 0;  // we set a flag to keep track of file currently being opened

uint16_t BPB_BytesPerSec;
uint8_t  BPB_SecPerClus;
uint16_t BPB_RsvdSecCnt;
uint8_t  BPB_NumFATs;
uint32_t BPB_FATSz32;
uint32_t BPB_RootClus;


int main()
{

  //allocate memory for user input
  char * cmd_str = (char*) malloc( MAX_COMMAND_SIZE );

  //start the shell loop
  while( 1 )
  {
    // Print out the mfs prompt
    printf ("mfs> ");

    // Read the command from the commandline.  The
    // maximum command that will be read is MAX_COMMAND_SIZE
    // This while command will wait here until the user
    // inputs something since fgets returns NULL when there
    // is no input
    while( !fgets (cmd_str, MAX_COMMAND_SIZE, stdin) );

    /* Parse input */
    char *token[MAX_NUM_ARGUMENTS];

    int   token_count = 0;                                 
                                                           
    // Pointer to point to the token
    // parsed by strsep
    char *arg_ptr;                                         
                                                           
    char *working_str  = strdup( cmd_str );                

    // we are going to move the working_str pointer so
    // keep track of its original value so we can deallocate
    // the correct amount at the end
    char *working_root = working_str;

    // Tokenize the input stringswith whitespace used as the delimiter
    while ( ( (arg_ptr = strsep(&working_str, WHITESPACE ) ) != NULL) && 
              (token_count<MAX_NUM_ARGUMENTS))
    {
      token[token_count] = strndup( arg_ptr, MAX_COMMAND_SIZE );
      if( strlen( token[token_count] ) == 0 )
      {
        token[token_count] = NULL;
      }
        token_count++;
    }

    //handle commands
    if (token[0] == NULL) 
    {
      continue;
    }

    //open command
    if (strcmp(token[0], "open") == 0) 
    {
      if (is_open) //stops new file from opening if one already open
      {
          printf("Error: File system image already open.\n");
      } else if (token[1] == NULL) // if no filename is given, we stop 
          printf("Error: No filename provided.\n");
      } else 
      {
          fp = fopen(token[1], "rb"); //here we open the file in binary read mode
          if (!fp) 
          {
              printf("Error: File system image not found.\n");
          } else 
          {
              is_open = 1;  //if the open was successful, we set the flag

              //here we are extracting the values from the BPB
              fseek(fp, 11, SEEK_SET);
              fread(&BPB_BytesPerSec, 2, 1, fp);
              fread(&BPB_SecPerClus, 1, 1, fp);
              fread(&BPB_RsvdSecCnt, 2, 1, fp);
              fread(&BPB_NumFATs, 1, 1, fp);

              fseek(fp, 36, SEEK_SET);
              fread(&BPB_FATSz32, 4, 1, fp);

              //this is the root cluster
              fseek(fp, 44, SEEK_SET);
              fread(&BPB_RootClus, 4, 1, fp);
          }
      }
  }

  //close command
  // we make sure the internal state is reset and that no other operations can be performed until a file is reopened
  else if (strcmp(token[0], "close") == 0) 
  {
    if (!is_open) 
    {
        printf("Error: File system not open.\n");
    } else 
    {
        fclose(fp);
        fp = NULL;
        is_open = 0;
    }
  } 

  //info command
  //we display the fields from BPB in base 10 and hexadecimal and confirm that the image was parsed correctly
  else if (strcmp(token[0], "info") == 0) 
  {
    if (!is_open) 
    {
        printf("Error: File system image must be opened first.\n");
    } else 
    {
        printf("BPB_BytesPerSec: %d (0x%X)\n", BPB_BytesPerSec, BPB_BytesPerSec);
        printf("BPB_SecPerClus: %d (0x%X)\n", BPB_SecPerClus, BPB_SecPerClus);
        printf("BPB_RsvdSecCnt: %d (0x%X)\n", BPB_RsvdSecCnt, BPB_RsvdSecCnt);
        printf("BPB_NumFATs: %d (0x%X)\n", BPB_NumFATs, BPB_NumFATs);
        printf("BPB_FATSz32: %u (0x%X)\n", BPB_FATSz32, BPB_FATSz32);
    }
  }

  //handle false commands
  else 
  {
    printf("Error: Unrecognized command.\n");
  }

  //making sure to free the allocated memory
  for (int i = 0; i < MAX_NUM_ARGUMENTS; i++) 
  {
    if (token[i] != NULL) 
    {
        free(token[i]);
    } 
  }
free(working_root);
}

free(cmd_str);

  
  return 0;
}
