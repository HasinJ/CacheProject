#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

unsigned long int cacheSize, blockSize;
unsigned long int memread=0, memwrite=0, cachehit=0, cachemiss=0;
char policy[5];
unsigned long int t;

struct CacheLine{
  unsigned long int tag;
  int valid;
  struct CacheLine* next;
};

void sizes(char* assoc, unsigned long int* setSize, unsigned long int* linesPerSet,int n){
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

void freeNodes(struct Node* x){
  if (x==0) return;
  freeNodes(x->next);
  free(x);
  return;
}

void freeEverything(struct CacheLine** cache, int setSize, int linesPerSet, int blockSize){
  for (size_t i = 0; i < setSize; i++) {
    freeNodes(cache[i]);
  }
  free(cache);
}


void read(struct CacheLine* current,int linesPerSet,int setIndex,unsigned long int tag){
  size_t i = 0;
  setIndex=setIndex-1;


  for (; i < linesPerSet; i++) { //finds location of matching tag
    if (cache[setIndex]->tag==tag){
      cachehit++;
      return;}
    else if (cache[setIndex]->valid==0){
      cache[setIndex]->tag=tag;
      cache[setIndex]->valid=1;
      memread++;
      cachemiss++;
      return;}
  }
  //if it's not found use policy
  memread++;
  cachemiss++;
  printf("not found: use policy\n");
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
  unsigned long int n = 0;
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

  printf("n%ld\n",n);
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

  unsigned long int setSize=0, linesPerSet=0;
  sizes(assoc, &setSize, &linesPerSet,n);

  unsigned long int offsetBits=log2(blockSize);
  unsigned long int setBits=log2(setSize);
  unsigned long int tagBits = 48 - setBits - offsetBits;
  printf("tagBits: %ld\n",tagBits);
  printf("offsetBits: %ld\n",offsetBits);
  printf("setSize: %ld\n",setSize);
  printf("setBits: %ld\n",setBits);
  printf("linesPerSet: %ld\n",linesPerSet);


  char access[2];
  unsigned long int address;
  printf("\n");
  while(fscanf(f,"%s %lx",access,&address)!=EOF){
    printf("access:%s address:%lx\n", access, address);
    unsigned long int offset = address & ((1<<offsetBits)-1);
    unsigned long int setIndex = (address>>offsetBits) & ((1<<setBits)-1);
    unsigned long int tag = (address >> (offsetBits+setBits)) & ((1<<tagBits)-1);
    printf("offset: %ld setIndex: %ld tag: %ld\n", offset, setIndex, tag);

    struct CacheLine** cache=calloc(setSize,sizeof(struct CacheLine));


    if(access[0]=='R'){
      read(cache,linesPerSet,setIndex,tag);
    }

    else if(access[0]=='W'){

    }


    freeEverything(cache,setSize,linesPerSet,blockSize);
    printf("\n");
  }
  printf("memread:%ld\nmemwrite:%ld\ncachehit:%ld\ncachemiss:%ld\n", memread,memwrite,cachehit,cachemiss);
  return EXIT_SUCCESS;
}
