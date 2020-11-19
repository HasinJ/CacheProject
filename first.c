#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

int cacheSize, blockSize;

struct CacheLine{
  int valid;
  unsigned long int tag;
  unsigned long int* blocks;
};


void sizes(char* assoc, int* setSize, int* linesPerSet,int n){
  switch(assoc[0]){
    case 'd':
      *setSize = cacheSize/blockSize;
      *linesPerSet = 1;
      break;
    case 'f':
      *setSize = 1;
      *linesPerSet = cacheSize/blockSize;
      break;
    case 'a':
      *setSize = cacheSize/(blockSize*n);
      *linesPerSet=n;
      break;
  }
}

int checkSizes(int num1, int num2){
  if((ceil(log2(num1)) != floor(log2(num1)))) return 0;
  if((ceil(log2(num2)) != floor(log2(num2)))) return 0;
  return 1;
}

void freeEverything(int** cache, int setSize,int linesPerSet, int blockSize){
  for (size_t i = 0; i < setSize; i++) {
    free(cache[i]);
  }
  free(cache);
}

int main(int argc, char const *argv[argc+1]) {
  // ./first 32 assoc:2 fifo 4 trace2.txt
  if (argc<6){
    printf("error\n");
    return EXIT_SUCCESS;
  }

  //cache: 1st blocksize: 4th
  cacheSize = atoi(argv[1]);
  blockSize = atoi(argv[4]);

  if(checkSizes(cacheSize, blockSize)==0){
    printf("error\n");
    return EXIT_SUCCESS;
  }


  char assoc[7];
  int n = 0;
  char policy[5];
  strcpy(policy,argv[3]);

  if(argv[2][0]=='a' && argv[2][5]==':'){
    size_t j = 0;
    strcpy(assoc, "assoc");
    char *temp = malloc((strlen(argv[2])-6)*sizeof(char*));
    for (size_t i = 6; i < strlen(argv[2]); i++) temp[j++]=argv[2][i];
    n = atoi(temp);
    free(temp);
  }
  else if(argv[2][0]=='a')  strcpy(assoc,"full");
  else strcpy(assoc,argv[2]);

  printf("n%d\n",n);
  printf("assoc: %s\n", assoc);

  FILE *f;
  f = fopen(argv[5],"r");
  if (f==0){
    printf("error\n");
    return EXIT_SUCCESS;
  }

  /////////////////////////////////////////
  //NEEDS TO CHECK FOR EACH LINE IN TRACE//
  /////////////////////////////////////////

  int setSize=0, linesPerSet=0;
  sizes(assoc, &setSize, &linesPerSet,n);

  int offsetBits=log2(blockSize);
  int setBits=log2(setSize);
  int tagBits = 48 - setBits - offsetBits;
  printf("tagBits: %d\n",tagBits);
  printf("offsetBits: %d\n",offsetBits);
  printf("setSize: %d\n",setSize);
  printf("setBits: %d\n",setBits);
  printf("linesPerSet: %d\n",linesPerSet);


  char access[2];
  unsigned long int address;
  printf("\n");
  while(fscanf(f,"%s %lx",access,&address)!=EOF){
    printf("access:%s address:%lx\n", access, address);
    unsigned long int offset = address & ((1<<offsetBits)-1);
    unsigned long int setIndex = (address>>offsetBits) & ((1<<setBits)-1);
    unsigned long int tag = (address >> (offsetBits+setBits)) & ((1<<tagBits)-1);
    printf("offset: %ld setIndex: %ld tag: %ld\n", offset, setIndex, tag);

    int **cache=calloc(setSize,sizeof(int*));
    for (size_t i = 0; i < setSize; i++) {
      cache[i]=calloc(linesPerSet,sizeof(struct CacheLine));
    }

    freeEverything(cache,setSize,linesPerSet,blockSize);

    printf("\n");
  }

  return EXIT_SUCCESS;
}
