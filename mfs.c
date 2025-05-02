/* Zubair Rashaad
   1002051693
*/




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
#include <ctype.h> //needed for toupper

#define MAX_NUM_ARGUMENTS 3

#define WHITESPACE " \t\n"      // We want to split our command line up into tokens
                                // so we need to define what delimits our tokens.
                                // In this case  white space
                                // will separate the tokens on our command line

#define MAX_COMMAND_SIZE 255    // The maximum command-line size

//defining global variables
FILE *fp = NULL;  //file pointer to the FAT32 image
int is_open = 0;  // we set a flag to keep track of file currently being opened
uint32_t current_cluster; // this is to track the cwd

uint16_t BPB_BytesPerSec;
uint8_t  BPB_SecPerClus;
uint16_t BPB_RsvdSecCnt;
uint8_t  BPB_NumFATs;
uint32_t BPB_FATSz32;
uint32_t BPB_RootClus;


void format_filename_8_3(char *input, char *output) 
{
  memset(output, ' ', 11);
  int i = 0, j = 0;
  while (input[i] != '\0' && j < 11) 
  {
      if (input[i] == '.') 
      {
          j = 8;  // jump to extension part
          i++;
          continue;
      }
      output[j++] = toupper(input[i++]);
  }
}

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
      free(working_root);
      continue;
    }

    //open command
    if (strcmp(token[0], "open") == 0) 
    {
      if (is_open) //stops new file from opening if one already open
      {
          printf("Error: File system image already open.\n");
      } 
      else if (token[1] == NULL) // if no filename is given, we stop 
      {
          printf("Error: No filename provided.\n");
      } 
      else 
      {
          fp = fopen(token[1], "rb"); //here we open the file in binary read mode
          if (!fp) 
          {
              printf("Error: File system image not found.\n");
          } 
          else 
          {
              is_open = 1;  //if the open was successful, we set the flag
              current_cluster = BPB_RootClus; //here we set the current directory to the root cluster

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
      } 
      else 
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
      } 
      else 
      {
          printf("BPB_BytesPerSec: %d (0x%X)\n", BPB_BytesPerSec, BPB_BytesPerSec);
          printf("BPB_SecPerClus: %d (0x%X)\n", BPB_SecPerClus, BPB_SecPerClus);
          printf("BPB_RsvdSecCnt: %d (0x%X)\n", BPB_RsvdSecCnt, BPB_RsvdSecCnt);
          printf("BPB_NumFATs: %d (0x%X)\n", BPB_NumFATs, BPB_NumFATs);
          printf("BPB_FATSz32: %u (0x%X)\n", BPB_FATSz32, BPB_FATSz32);
      }
    }

    //get command
    else if (strcmp(token[0], "get") == 0) 
    {
      if (!is_open) //image needs to be open for us to access files
      {
          printf("Error: File system image must be opened first.\n");
      } else if (token[1] == NULL) //check if get is entered without a file name
      {
          printf("Error: File not found.\n");
      } else //converting the input to the needed format
      {
          char formatted[12];
          format_filename_8_3(token[1], formatted);

          // here we calculate the root directory location using the formulas from the specification
          uint32_t FirstDataSector = BPB_RsvdSecCnt + (BPB_NumFATs * BPB_FATSz32);
          uint32_t root_cluster = BPB_RootClus;
          uint32_t root_sector = ((root_cluster - 2) * BPB_SecPerClus) + FirstDataSector;
          uint32_t root_offset = root_sector * BPB_BytesPerSec;

          fseek(fp, root_offset, SEEK_SET);

          int found = 0;
          //we loop over the directory entries to find the file
          for (int i = 0; i < 16; i++) 
          {
              unsigned char dir[32];
              fread(dir, 32, 1, fp);

              if (dir[0] == 0x00 || dir[0] == 0xE5) continue; // if it is an unused or deleted entry we skip over

              //we compare the first 11 bytes of the entry, and if it matches we are sure the file is found
              if (memcmp(dir, formatted, 11) == 0) 
              {
                  found = 1;
                  
                  uint8_t attr = dir[11];
                  // if what we find is actually a directory, we reject it because we are looking for a file
                  if (attr == 0x10) 
                  {
                      printf("Error: File not found.\n");  // it’s a directory
                      break;
                  }

                  //here the full 32 bit starting cluster number is reconstructed
                  //we also read the file size here 
                  uint16_t high = *(uint16_t *)&dir[20];
                  uint16_t low  = *(uint16_t *)&dir[26];
                  uint32_t first_cluster = (high << 16) | low;
                  uint32_t filesize = *(uint32_t *)&dir[28];

                  //here we open a file to write to
                  //we make a file with the same name in cwd, if fails we give an error
                  FILE *out = fopen(token[1], "wb");
                  if (!out) 
                  {
                      printf("Error: Could not create output file.\n");
                      break;
                  }

                  //we read the file data by following the cluster chain
                  uint32_t cluster = first_cluster;
                  uint32_t bytes_remaining = filesize;

                  //we keep on reading until we reach the end
                  while (cluster < 0x0FFFFFF8) 
                  { 
                      //we calculate the sector and offset for the current cluster
                      uint32_t first_sector = ((cluster - 2) * BPB_SecPerClus) + FirstDataSector;
                      uint32_t data_offset = first_sector * BPB_BytesPerSec;

                      fseek(fp, data_offset, SEEK_SET);

                      //we read the sectors in the current cluster and copy them to the new output file
                      for (int s = 0; s < BPB_SecPerClus; s++) 
                      {
                          unsigned char buffer[512];
                          int bytes_to_read = (bytes_remaining > 512) ? 512 : bytes_remaining;

                          fread(buffer, 1, bytes_to_read, fp);
                          fwrite(buffer, 1, bytes_to_read, out);

                          bytes_remaining -= bytes_to_read;
                          if (bytes_remaining == 0) break;
                      }

                      // here we get the next cluster from the FAT
                      uint32_t fat_offset = BPB_RsvdSecCnt * BPB_BytesPerSec + (cluster * 4);
                      fseek(fp, fat_offset, SEEK_SET);
                      fread(&cluster, 4, 1, fp);
                  }
                  //we close the output file when we finish successfully
                  fclose(out);
                  printf("File '%s' extracted successfully.\n", token[1]);
                  break;
              }
          }
          //displaying error if file was never found
          if (!found) 
          {
              printf("Error: File not found.\n");
          }
      }
    }

    //cd command
    else if (strcmp(token[0], "cd") == 0)
    {
        if (!is_open)
        {
            printf("Error: File system image must be opened first.\n");
        }
        else if (token[1] == NULL)
        {
            printf("Error: Directory name required.\n");
        }
        else
        {
            // if cd is entered it goes to the root directory
            if (strcmp(token[1], "/") == 0)
            {
                current_cluster = BPB_RootClus;  // go to root
            }
            //here we go up a level if cd .. is entered
            else if (strcmp(token[1], "..") == 0)
            {
                // For simplicity, we'll treat cd .. as root for now
                current_cluster = BPB_RootClus;
            }
            else
            {
                // here we convert to the needed format for FAT
                char formatted[12];
                format_filename_8_3(token[1], formatted);
    
                // here we calculate the offset to current directory
                uint32_t FirstDataSector = BPB_RsvdSecCnt + (BPB_NumFATs * BPB_FATSz32);
                uint32_t sector = ((current_cluster - 2) * BPB_SecPerClus) + FirstDataSector;
                uint32_t offset = sector * BPB_BytesPerSec;

                //here we scan the directory entries
                fseek(fp, offset, SEEK_SET);
    
                int found = 0;
                for (int i = 0; i < 16; i++)
                {
                    unsigned char dir[32];
                    fread(dir, 32, 1, fp);
    
                    if (dir[0] == 0x00 || dir[0] == 0xE5) continue;
                    //here we find matching entries and check if its a directory
                    if (memcmp(dir, formatted, 11) == 0 && (dir[11] & 0x10)) // must be a directory
                    { 
                        //here we get the new directorys cluster number
                        uint16_t high = *(uint16_t *)&dir[20];
                        uint16_t low = *(uint16_t *)&dir[26];
                        current_cluster = (high << 16) | low;
                        found = 1;
                        break;
                    }
                }
    
                if (!found)
                {
                    printf("Error: Directory not found.\n");
                }
            }
        }
    }

    // ls command
    else if (strcmp(token[0], "ls") == 0)
    {
        if (!is_open) //check if the image is open first
        {
            printf("Error: File system image must be opened first.\n");
        }
        else
        { 
            // we calculate the offsets to the current directory clusters
            uint32_t FirstDataSector = BPB_RsvdSecCnt + (BPB_NumFATs * BPB_FATSz32);
            uint32_t sector = ((current_cluster - 2) * BPB_SecPerClus) + FirstDataSector;
            uint32_t offset = sector * BPB_BytesPerSec;
    
            fseek(fp, offset, SEEK_SET);

            // we loop over the first directories, i just used 16
            for (int i = 0; i < 16; i++)
            {   
                //each one os 32 bytes
                unsigned char dir[32];
                fread(dir, 32, 1, fp);
                //debug
                printf("Raw entry: %02X %02X %02X\n", dir[0], dir[11], dir[1]);


                //here we skip entries that aren't supposed to be listed
                if (dir[0] == 0x00) break; //this means the rest of the directory is empty
                if (dir[0] == 0xE5) continue; //this is a deleted file
                if (dir[11] == 0x0F) continue;  //this isn't an actual file name
                if (dir[11] & 0x08) continue; //this also isnt user files

                //here we display the file name with proper formatting
                for (int j = 0; j < 11; j++)
                {
                    if (j == 8 && dir[j] != ' ')
                        printf(".");
                    if (dir[j] != ' ')
                        printf("%c", dir[j]);
                }
    
                if (dir[11] & 0x10)
                    printf("/");
    
                printf("\n");
            }
        }
    }

    //read command
    else if (strcmp(token[0], "read") == 0)
    {
        if (!is_open) //verify the image is open
        {
            printf("Error: File system image must be opened first.\n");
        }
        //here we validate the arguments
        else if (token[1] == NULL || token[2] == NULL || token[3] == NULL)
        {
            printf("Error: Invalid arguments.\n");
        }
        else
        {
            //here we convert the filename to the proper format
            char formatted[12];
            format_filename_8_3(token[1], formatted);
            //we convert these to integers
            uint32_t position = atoi(token[2]);
            uint32_t num_bytes = atoi(token[3]);

            //we find current directory cluster and calculate the offsets where the cwd starts
            uint32_t FirstDataSector = BPB_RsvdSecCnt + (BPB_NumFATs * BPB_FATSz32);
            uint32_t sector = ((current_cluster - 2) * BPB_SecPerClus) + FirstDataSector;
            uint32_t offset = sector * BPB_BytesPerSec;
    
            fseek(fp, offset, SEEK_SET);
    
            int found = 0;
            //here we look for the file and read the entries from the cwd
            for (int i = 0; i < 16; i++)
            {
                unsigned char dir[32];
                fread(dir, 32, 1, fp);

                //check if there are no more files or if the file is deleted
                if (dir[0] == 0x00 || dir[0] == 0xE5) continue;
                //we match the character name
                if (memcmp(dir, formatted, 11) == 0)
                {
                    found = 1;

                    //here we get the file cluster and size 
                    uint16_t high = *(uint16_t *)&dir[20];
                    uint16_t low = *(uint16_t *)&dir[26];
                    uint32_t cluster = (high << 16) | low;
                    uint32_t file_size = *(uint32_t *)&dir[28];

                    //we verify the position, we can't read past the end of a file
                    if (position >= file_size)
                    {
                        printf("Error: Position out of range.\n");
                        break;
                    }
                    
                    //we traverse the cluster chain to read position
                    uint32_t skip = position;
                    uint32_t bytes_read = 0;
    
                    while (cluster < 0x0FFFFFF8)
                    {
                        uint32_t cluster_bytes = BPB_BytesPerSec * BPB_SecPerClus;
                        //if the position we want isnt there, we subtract and move on
                        if (skip >= cluster_bytes)
                        {
                            skip -= cluster_bytes;
                        }
                        else
                        {   
                            //we calculate the exact byte offset into the file
                            uint32_t read_offset = ((cluster - 2) * BPB_SecPerClus + FirstDataSector) * BPB_BytesPerSec + skip;
                            fseek(fp, read_offset, SEEK_SET);

                            //here we read up to the size of the file and print each byte as a character
                            for (int i = 0; i < num_bytes && position + i < file_size; i++)
                            {
                                unsigned char c;
                                fread(&c, 1, 1, fp);
                                printf("%c", c);
                                bytes_read++;
                            }
                            printf("\n");
                            break;
                        }
                        //if we need to then we move to the next cluster in the chain
                        uint32_t fat_offset = BPB_RsvdSecCnt * BPB_BytesPerSec + cluster * 4;
                        fseek(fp, fat_offset, SEEK_SET);
                        fread(&cluster, 4, 1, fp);
                    }
                    break;
                }
            }
    
            if (!found)
            {
                printf("Error: File not found.\n");
            }
        }
    }

    //delete command
    else if (strcmp(token[0], "del") == 0)
    {
        if (!is_open) //verify the image is open
        {
            printf("Error: File system image must be opened first.\n");
        }
        else if (token[1] == NULL) //the filename has to be given
        {
            printf("Error: File not found.\n");
        }
        else
        {   
            //again we convert to the proper format
            char formatted[12];
            format_filename_8_3(token[1], formatted);
            
            //we locate the current directory cluster and then calculate where the cwd starts
            uint32_t FirstDataSector = BPB_RsvdSecCnt + (BPB_NumFATs * BPB_FATSz32);
            uint32_t sector = ((current_cluster - 2) * BPB_SecPerClus) + FirstDataSector;
            uint32_t offset = sector * BPB_BytesPerSec;
    
            fseek(fp, offset, SEEK_SET);
    
            int found = 0;
            //we loop over all the entries
            for (int i = 0; i < 16; i++)
            {   
                //we track where the entry starts
                long entry_offset = ftell(fp);
                //we read the next directory entry
                unsigned char dir[32];
                fread(dir, 32, 1, fp);

                //we skip over the invalid and deleted entries
                if (dir[0] == 0x00 || dir[0] == 0xE5) continue;

                //we check the match
                if (memcmp(dir, formatted, 11) == 0)
                {   
                    //we mark the file as deleted here and make sure FAT32 treats it as a deletion
                    fseek(fp, entry_offset, SEEK_SET);
                    unsigned char deleted = 0xE5;
                    fwrite(&deleted, 1, 1, fp);
                    found = 1;
                    break;
                }
            }
    
            if (!found)
            {
                printf("Error: File not found.\n");
            }
        }
    }

    //undelete command
    else if (strcmp(token[0], "undel") == 0)
    {
        if (!is_open) //verify the image is open
        {
            printf("Error: File system image must be opened first.\n");
        }
        else if (token[1] == NULL)  //a file name needs to be entered
        {
            printf("Error: File not found.\n");
        }
        else
        { 
            //we convert to the FAT format
            char formatted[12];
            format_filename_8_3(token[1], formatted);

            //we calculate the cwd location
            uint32_t FirstDataSector = BPB_RsvdSecCnt + (BPB_NumFATs * BPB_FATSz32);
            uint32_t sector = ((current_cluster - 2) * BPB_SecPerClus) + FirstDataSector;
            uint32_t offset = sector * BPB_BytesPerSec;
    
            fseek(fp, offset, SEEK_SET);
    
            int restored = 0;
            //we loop through the directory entries
            for (int i = 0; i < 16; i++)
            {   
                //we store the current entrys location
                long entry_offset = ftell(fp);  
                //we read one directory entry
                unsigned char dir[32];
                fread(dir, 32, 1, fp);

                //we skip the entries that were not deleted
                if (dir[0] != 0xE5) continue;

                //we match the remaining bytes of file name
                if (memcmp(&dir[1], &formatted[1], 10) == 0)
                {   
                    //we restore the first byte and overwrite the deleted mark at the beginning with the original character
                    fseek(fp, entry_offset, SEEK_SET);
                    fwrite(&formatted[0], 1, 1, fp);
                    //we mark it as restored if successful
                    restored = 1;
                    break;
                }
            }
    
            if (!restored)
            {
                printf("Error: File not found.\n");
            }
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
