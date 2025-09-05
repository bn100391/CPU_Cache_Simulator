#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <limits.h>
#define MAX_LINE_LENGTH 250

//parameters
int mapValue = 0;
int numOffsetBits = 0;
int numIndexBits = 0;
char allocatePolicy[MAX_LINE_LENGTH];
char writePolicy[MAX_LINE_LENGTH]; //Dirty Or Not.

//accesses
struct Access{
  char type;
  unsigned int address;
};
struct Access *accesses;
int numAccesses = 0;
int numSets = 0;

struct DirectMappedEntry{
  int validBit;
  int dirtyBit;
  int tag;
};
struct DirectMappedEntry *DirectMappedCache;

struct TwoWayEntry{
  int validBit[2];
  int dirtyBit[2];
  int tag[2];
  int putIn[2];
};
struct TwoWayEntry *TwoWayCache;

struct FourWayEntry{
  int validBit[4];
  int dirtyBit[4];
  int tag[4];
  int putIn[2];
};
struct FourWayEntry *FourWayCache;


//output stats
int rhits = 0;
int whits = 0;
int rmisses = 0;
int wmisses = 0;
double hrate = 0.0;
int wb = 0;
int wt = 0;

int offsetMask = 0;
int indexMask = 0;


unsigned int* decodeInstruction(unsigned int address);
void processAsDirectCache(){
  for(int i=0; i<numAccesses; i++){
    char* outcome;
    char type = accesses[i].type;
    unsigned int* accessFields = decodeInstruction(accesses[i].address);
    int offset = accessFields[0];
    int index = accessFields[1];
    int tag = accessFields[2];

    if(DirectMappedCache[index].validBit == 1 && DirectMappedCache[index].tag == tag ){ //hit
        if(type == 'r'){ //Read - Hit
          rhits++;

        }else if(type == 'w'){//Write - Hit
          whits++;
          if(strcmp(writePolicy, "wt") == 0){
            wt++;
          }
          if(strcmp(writePolicy, "wb") == 0){
            DirectMappedCache[index].dirtyBit = 1;
          }
        }

    }else{ //miss
      if(type == 'r'){ //Read - Miss
        rmisses++;
        if(strcmp(writePolicy, "wb") == 0 && DirectMappedCache[index].dirtyBit == 1){
          wb++;
        }
        DirectMappedCache[index].tag = tag;
        DirectMappedCache[index].validBit = 1;
        DirectMappedCache[index].dirtyBit = 0;

      }else if(type == 'w'){ //Write - Miss
          wmisses++;
          if(strcmp(writePolicy, "wt") == 0){
            wt++;
          }
          if(strcmp(writePolicy, "wb") == 0 && strcmp(allocatePolicy, "wna") == 0){
            wt++;
          }
          if(strcmp(writePolicy, "wb") == 0 && DirectMappedCache[index].dirtyBit == 1){
            wb++;
          }
          if(strcmp(allocatePolicy, "wa") == 0){
            DirectMappedCache[index].tag = tag;
            DirectMappedCache[index].validBit = 1;
            DirectMappedCache[index].dirtyBit = 1;
          }

      }


    }

  }
}

unsigned int* decodeInstruction(unsigned int address);
void processAsTwoWayCache(){
  for(int i=0; i<numAccesses; i++){
    char* outcome;
    char type = accesses[i].type;
    unsigned int* accessFields = decodeInstruction(accesses[i].address);
    int offset = accessFields[0];
    int index = accessFields[1];
    int tag = accessFields[2];

    for(int block = 0; block < 2; block++){
      if(TwoWayCache[index].validBit[block] == 1 && TwoWayCache[index].tag[block] == tag ){ //hit
          if(type == 'r'){ //Read - Hit
            rhits++;

          }else if(type == 'w'){//Write - Hit
            whits++;
            if(strcmp(writePolicy, "wt") == 0){
              wt++;
            }
            if(strcmp(writePolicy, "wb") == 0){
              TwoWayCache[index].dirtyBit[block] = 1;
              TwoWayCache[index].putIn[block] = i;
            }
            continue; //Continue if there was a hit.
          }
      //If you made it to here, this is a miss.

      }else{ //miss

        int replacementBlock; //Compute block for replacement (FIFO)
        for(int a = 0; a < 2; a++){
          int threshold = INT_MAX;
          if(TwoWayCache[i].putIn[a] < threshold){
            replacementBlock = a;
          }
        }

        if(type == 'r'){ //Read - Miss
          rmisses++;
          if(strcmp(writePolicy, "wb") == 0 && TwoWayCache[index].dirtyBit[block] == 1){
            wb++;
          }
          TwoWayCache[index].tag[replacementBlock] = tag;
          TwoWayCache[index].validBit[replacementBlock] = 1;
          TwoWayCache[index].dirtyBit[replacementBlock] = 0;
          TwoWayCache[index].putIn[replacementBlock] = i;

        }else if(type == 'w'){ //Write - Miss
            wmisses++;
            if(strcmp(writePolicy, "wt") == 0){
              wt++;
            }
            if(strcmp(writePolicy, "wb") == 0 && strcmp(allocatePolicy, "wna") == 0){
              wt++;
            }
            if(strcmp(writePolicy, "wb") == 0 && TwoWayCache[index].dirtyBit[block] == 1){
              wb++;
            }
            if(strcmp(allocatePolicy, "wa") == 0){
              TwoWayCache[index].tag[replacementBlock] = tag;
              TwoWayCache[index].validBit[replacementBlock] = 1;
              TwoWayCache[index].dirtyBit[replacementBlock] = 1;
              TwoWayCache[index].putIn[replacementBlock] = i;
            }
        }
      }
    }
  }
}

unsigned int* decodeInstruction(unsigned int address);
  void processAsFourWayCache(){
  for(int i=0; i<numAccesses; i++){
    char* outcome;
    char type = accesses[i].type;
    unsigned int* accessFields = decodeInstruction(accesses[i].address);
    int offset = accessFields[0];
    int index = accessFields[1];
    int tag = accessFields[2];

    for(int block = 0; block < 4; block++){
      if(FourWayCache[index].validBit[block] == 1 && FourWayCache[index].tag[block] == tag ){ //hit
          if(type == 'r'){ //Read - Hit
            rhits++;

          }else if(type == 'w'){//Write - Hit
            whits++;
            if(strcmp(writePolicy, "wt") == 0){
              wt++;
            }
            if(strcmp(writePolicy, "wb") == 0){
              FourWayCache[index].dirtyBit[block] = 1;
              FourWayCache[index].putIn[block] = i;
            }
            continue; //Continue if there was a hit.
          }
      //If you made it to here, this is a miss.

      }else{ //miss

        int replacementBlock; //Compute block for replacement (FIFO)
        for(int a = 0; a < 4; a++){
          int threshold = INT_MAX;
          if(FourWayCache[i].putIn[a] < threshold){
            replacementBlock = a;
          }
        }

        if(type == 'r'){ //Read - Miss
          rmisses++;
          if(strcmp(writePolicy, "wb") == 0 && FourWayCache[index].dirtyBit[block] == 1){
            wb++;
          }
          FourWayCache[index].tag[replacementBlock] = tag;
          FourWayCache[index].validBit[replacementBlock] = 1;
          FourWayCache[index].dirtyBit[replacementBlock] = 0;
          FourWayCache[index].putIn[replacementBlock] = i;

        }else if(type == 'w'){ //Write - Miss
            wmisses++;
            if(strcmp(writePolicy, "wt") == 0){
              wt++;
            }
            if(strcmp(writePolicy, "wb") == 0 && strcmp(allocatePolicy, "wna") == 0){
              wt++;
            }
            if(strcmp(writePolicy, "wb") == 0 && FourWayCache[index].dirtyBit[block] == 1){
              wb++;
            }
            if(strcmp(allocatePolicy, "wa") == 0){
              FourWayCache[index].tag[replacementBlock] = tag;
              FourWayCache[index].validBit[replacementBlock] = 1;
              FourWayCache[index].dirtyBit[replacementBlock] = 1;
              FourWayCache[index].putIn[replacementBlock] = i;
            }
        }
      }
    }
  }
}


void readAccesses(){
  FILE *file;
  char fileName[] = "accesses.txt";
  char line[MAX_LINE_LENGTH];
  char *endptr;

  file = fopen(fileName, "r");
   if (file == NULL) {
     perror("Error opening file");
   }


  int arrayIndex = 0;
  accesses = malloc(sizeof(struct Access) * (arrayIndex + 1));

  while (fgets(line, MAX_LINE_LENGTH, file) != NULL) {
    accesses = realloc(accesses, sizeof(struct Access) * (arrayIndex + 1));
    accesses[arrayIndex].type = line[0];
    char address_string[8];
    strncpy(address_string, line + 3, 8);
    accesses[arrayIndex].address = strtoul(address_string, &endptr, 16);
    arrayIndex++;
    numAccesses++;
  }

  fclose(file);
}


void readParameters(){
  FILE *file;
  char fileName[] = "parameters.txt";
  char line[MAX_LINE_LENGTH];

  file = fopen(fileName, "r");
   if (file == NULL) {
     perror("Error opening file");
   }

  int i = 0;
  while (fgets(line, MAX_LINE_LENGTH, file) != NULL) {
      i++;
      switch(i){
        case 1:
          mapValue = atoi(line);
          break;
        case 2:
          numOffsetBits = atoi(line);
          break;
        case 3:
          numIndexBits = atoi(line);
          break;
        case 4:
          strcpy(allocatePolicy, line);
          size_t length = strlen(allocatePolicy);
          while(length > 0 && isspace(allocatePolicy[length - 1])) {
            allocatePolicy[length - 1] = '\0';
            length--;
          }
          break;
        case 5:
          strcpy(writePolicy, line);
          size_t length2 = strlen(writePolicy);
          while(length2 > 0 && isspace(writePolicy[length2 - 1])) {
            writePolicy[length2 - 1] = '\0';
            length2--;
          }
          break;
      }
  }

  fclose(file);
}

void createCache(){
  numSets = pow(2, numIndexBits);
  int blockSize = pow(2, numOffsetBits);

  if(mapValue == 1){
    int numInit = 0;
    DirectMappedCache = malloc(sizeof(struct DirectMappedEntry) * (1));
    for(int i = 0; i < numSets; i++){
      DirectMappedCache = malloc(sizeof(struct DirectMappedEntry) * (i + 1));
      DirectMappedCache[i].validBit = 0;
      DirectMappedCache[i].dirtyBit = 0;
      DirectMappedCache[i].tag = 0;
    }
  }

  if(mapValue == 2){
    int numInit = 0;
    TwoWayCache = malloc(sizeof(struct TwoWayEntry) * (1));
    for(int i = 0; i < numSets; i++){
      TwoWayCache = malloc(sizeof(struct TwoWayEntry) * (i + 1));
      for(int j = 0; j < 2; j++){
        TwoWayCache[i].validBit[j] = 0;
        TwoWayCache[i].dirtyBit[j] = 0;
        TwoWayCache[i].tag[j] = 0;
    }
  }
}
  if(mapValue == 4){
    int numInit = 0;
    DirectMappedCache = malloc(sizeof(struct FourWayEntry) * (1));
    for(int i = 0; i < numSets; i++){
      FourWayCache = malloc(sizeof(struct FourWayEntry) * (i + 1));
      for(int j = 0; j < 4; j++){
        FourWayCache[i].validBit[j] = 0;
        FourWayCache[i].dirtyBit[j] = 0;
        FourWayCache[i].tag[j] = 0;
    }
  }
}
}


void writeStats(){
  FILE *file;
  file = fopen("statistics.txt", "w");

  if(file == NULL){
    printf("Error creating file");
    exit(1);
  }

  fprintf(file, "rhits: %d\n", rhits);
  fprintf(file, "whits: %d\n", whits);
  fprintf(file, "rmisses: %d\n", rmisses);
  fprintf(file, "wmisses: %d\n", wmisses);
  fprintf(file, "hrate: %f\n", hrate);
  fprintf(file, "wb: %d\n", wb);
  fprintf(file, "wt: %d\n", wt);

}




unsigned int* decodeInstruction(unsigned int address){
  static unsigned int values[3];

  values[0] = address & offsetMask;  //2^numOffsetBits - 1
  address = address >> numOffsetBits;

  values[1] = address & indexMask; //2^numIndexBits - 1
  address = address >> numIndexBits;

  values[2] = address;


  return values; //values[0]=offset | value[1]=index | values[2]=tag
}



int main() {
  readParameters();
  readAccesses();
  createCache();

  offsetMask = pow(2, numOffsetBits) - 1;
  indexMask = pow(2, numIndexBits) - 1;

  if(mapValue == 1){
    processAsDirectCache();
  }else if(mapValue == 2){
    processAsTwoWayCache();
  }else if(mapValue == 4){
    processAsFourWayCache();
  }

  hrate = (((float)rhits + (float)whits) / (float)numAccesses);
  writeStats();



}
