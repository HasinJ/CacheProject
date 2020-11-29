#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

struct CacheLine{
  unsigned long int tag;
  unsigned long int address;
  struct CacheLine* next;
};

void freeNodes(struct CacheLine* x){
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

void sizes(char* assoc, unsigned long int* setSize, unsigned long int* linesPerSet,int n, unsigned long int *cacheSize, unsigned long int *blockSize){
  switch(assoc[0]){
    case 'd':
      *setSize = *cacheSize/(*blockSize);
      *linesPerSet = 1;
      break;
    case 'f':
      *setSize = 1;
      *linesPerSet = *cacheSize/(*blockSize);
      break;
    case 'a':
      *setSize = *cacheSize/((*blockSize)*n);
      *linesPerSet=n;
      break;
  }
}

int checkSizes(unsigned long int num1, unsigned long int num2, unsigned long int num3){
  if((ceil(log2(num1)) != floor(log2(num1)))) return 0;
  if((ceil(log2(num2)) != floor(log2(num2)))) return 0;
  if((ceil(log2(num3)) != floor(log2(num3)))) return 0;
  return 1;
}

void read(struct CacheLine** L1cache, int L1linesPerSet, struct CacheLine** L2cache, int L2linesPerSet){

}

void write(struct CacheLine** L1cache, int L1linesPerSet, struct CacheLine** L2cache, int L2linesPerSet){

}

int main(int argc, char const *argv[argc+1]) {
  if (argc!=9){
    printf("error\n");
    return EXIT_SUCCESS;
  }

  unsigned long int L1cacheSize=atoi(argv[1]), L1blockSize=atoi(argv[4]), L2cacheSize=atoi(argv[5]), L2blockSize=L1blockSize;
/*
  printf("L1cacheSize: %ld\n",L1cacheSize);
  printf("L1blockSize: %ld\n",L1blockSize);
  printf("L2cacheSize: %ld\n",L2cacheSize);
  printf("L2blockSize: %ld\n",L2blockSize);
*/
  char L1policy[5], L2policy[5];
  strcpy(L1policy,argv[3]);
  strcpy(L2policy,argv[7]);
/*
  printf("L1policy: %s\n",L1policy);
  printf("L2policy: %s\n",L2policy);
*/
  if(checkSizes(L1cacheSize, L1blockSize, L2cacheSize)==0){
    printf("error\n");
    return EXIT_SUCCESS;
  }

  unsigned long int L1n=0;
  char L1assoc[7];
  if(argv[2][0]=='a' && argv[2][5]==':'){
    size_t j = 0;
    strcpy(L1assoc, "assoc");
    char *temp = malloc((strlen(argv[2])-6)*sizeof(char*));
    for (size_t i = 6; i < strlen(argv[2]); i++) temp[j++]=argv[2][i];
    L1n = atoi(temp);
    free(temp);
  }
  else if(argv[2][0]=='a')  strcpy(L1assoc,"full");
  else strcpy(L1assoc,argv[2]);

  unsigned long int L2n=0;
  char L2assoc[7];
  if(argv[6][0]=='a' && argv[6][5]==':'){
    size_t j = 0;
    strcpy(L2assoc, "assoc");
    char *temp = malloc((strlen(argv[6])-6)*sizeof(char*));
    for (size_t i = 6; i < strlen(argv[6]); i++) temp[j++]=argv[6][i];
    L2n = atoi(temp);
    free(temp);
  }
  else if(argv[6][0]=='a')  strcpy(L2assoc,"full");
  else strcpy(L2assoc,argv[6]);
/*
  printf("L1assoc: %s\n",L1assoc);
  printf("L2assoc: %s\n",L2assoc);
  printf("L1n: %ld\n",L1n);
  printf("L2n: %ld\n",L2n);
*/
  unsigned long int L1setSize=0, L1linesPerSet=0, L2setSize=0, L2linesPerSet=0;
  sizes(L1assoc, &L1setSize, &L1linesPerSet,L1n, &L1cacheSize, &L1blockSize);
  sizes(L2assoc, &L2setSize, &L2linesPerSet,L2n, &L2cacheSize, &L2blockSize);

  unsigned long int L1offsetBits=log2(L1blockSize), L2offsetBits=log2(L2blockSize);
  unsigned long int L1setBits=log2(L1setSize), L2setBits=log2(L2setSize);
  unsigned long int L1tagBits = 48 - L1setBits - L1offsetBits, L2tagBits = 48 - L2setBits - L2offsetBits;


  printf("\nL1tagBits: %ld\n",L1tagBits);
  printf("L1offsetBits: %ld\n",L1offsetBits);
  printf("L1setSize: %ld\n",L1setSize);
  printf("L1setBits: %ld\n",L1setBits);
  printf("L1linesPerSet: %ld\n",L1linesPerSet);

  printf("\nL2tagBits: %ld\n",L2tagBits);
  printf("L2offsetBits: %ld\n",L2offsetBits);
  printf("L2setSize: %ld\n",L2setSize);
  printf("L2setBits: %ld\n",L2setBits);
  printf("L2linesPerSet: %ld\n",L2linesPerSet);

  struct CacheLine **L1cache=calloc(L1setSize,sizeof(struct CacheLine));
  for (size_t i = 0; i < L1setSize; i++) {
    L1cache[i]=calloc(1,sizeof(struct CacheLine));
    L1cache[i]->next=0;
  }

  struct CacheLine **L2cache=calloc(L2setSize,sizeof(struct CacheLine));
  for (size_t i = 0; i < L2setSize; i++) {
    L2cache[i]=calloc(1,sizeof(struct CacheLine));
    L2cache[i]->next=0;
  }

  FILE *f;
  f = fopen(argv[8],"r");
  if (f==0){
    printf("error\n");
    return EXIT_SUCCESS;
  }

  char access[2];
  unsigned long int address;
  while(fscanf(f,"%s %lx",access,&address)!=EOF){

    //should do this in the read() write() functions
    //unsigned long int setIndex = (address>>offsetBits) & ((1lu<<setBits)-1lu);
    //unsigned long int tag = (address >> (offsetBits+setBits)) & ((1lu<<tagBits)-1lu);

    if(access[0]=='R'){
      read(L1cache,L1linesPerSet,L2cache,L2linesPerSet);
    }

    else if(access[0]=='W'){
      write(L1cache,L1linesPerSet,L2cache,L2linesPerSet);
    }

  }

  freeEverything(L1cache,L1setSize,L1linesPerSet,L1blockSize);
  freeEverything(L2cache,L2setSize,L2linesPerSet,L2blockSize);
}
