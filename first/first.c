#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

unsigned long int cacheSize, blockSize;
unsigned long int memread=0, memwrite=0, cachehit=0, cachemiss=0;
char policy[5];
char assoc[7];
unsigned long int t;

struct CacheLine{
  unsigned long int tag;
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

void insertBeginning(struct CacheLine* current,unsigned long int tag){
  struct CacheLine* temp = malloc(sizeof(struct CacheLine));
  temp->tag=tag;
  temp->next=current->next;
  current->next=temp;
}

void insertEnd(struct CacheLine* end, unsigned long int tag){
  end->next=malloc(sizeof(struct CacheLine));
  end->next->tag=tag;
  end->next->next=0;
}

void removeAfterThis(struct CacheLine* current){
  struct CacheLine* temp = current->next;
  current->next=current->next->next;
  free(temp);
}

void fifo(struct CacheLine** cache, struct CacheLine* current, unsigned long int tag, int setIndex, int linesPerSet){
  removeAfterThis(cache[setIndex]);
  if(linesPerSet==1){
    struct CacheLine* temp = malloc(sizeof(struct CacheLine));
    temp->tag=tag;
    temp->next=0;
    cache[setIndex]->next=temp;
    return;
  }
  insertEnd(current,tag);
  return;
}

void read(struct CacheLine** cache, int setIndex, int linesPerSet, unsigned long int tag){
  struct CacheLine* current = cache[setIndex];
  struct CacheLine* before=0;
  size_t i = 0;
  if(current->next==0){
    current->next = malloc(sizeof(struct CacheLine));
    current->next->tag=tag;
    current->next->next=0;
    memread++;
    cachemiss++;
    return;
  }

  while(current->next!=0){
    if(current->next->tag==tag){ //finds location of matching tag
      cachehit++;
      //printf("hitting\n");
      if(policy[0]=='l' && linesPerSet!=1){
        removeAfterThis(current);
        insertBeginning(cache[setIndex],tag);
        return;
      }
      return;
    }
    before=current;
    current=current->next;
    i++;
    if(i==linesPerSet) break;
  }
  memread++;
  cachemiss++;

  //if it isnt found and set not full
  //printf("i: %ld\n",i);
  if(i!=linesPerSet){
    if(policy[0]=='l' && linesPerSet!=1) {
      insertBeginning(cache[setIndex],tag);
      return;
    }
    insertEnd(current,tag);
    return;
  }

  //if it's not found use policy
  //printf("not found: use policy\n");
  if (policy[0]=='l' && linesPerSet!=1) {
    removeAfterThis(before);
    insertBeginning(cache[setIndex],tag);
    return;
  }
  fifo(cache, current, tag, setIndex, linesPerSet);
  return;
}

void write(struct CacheLine** cache, int setIndex, int linesPerSet, unsigned long int tag){
  struct CacheLine* current = cache[setIndex];
  struct CacheLine* before=0;
  size_t i = 0;
  if(current->next==0){
    current->next = malloc(sizeof(struct CacheLine));
    current->next->tag=tag;
    current->next->next=0;
    memwrite++;
    memread++;
    cachemiss++;
    return;
  }

  while(current->next!=0){
    if (current->next->tag==tag){ //finds location of matching tag
      memwrite++;
      cachehit++;
      //printf("hitting\n");
      if(policy[0]=='l' && linesPerSet!=1){
        removeAfterThis(current);
        insertBeginning(cache[setIndex],tag);
        return;
      }
      return;
    }
    before=current;
    current=current->next;
    i++;
    if(i==linesPerSet) break;
  }
  memread++;
  cachemiss++;
  memwrite++;

  //if it isnt found and set not full
  //printf("i: %ld\n",i);
  if(i!=linesPerSet){
    if(policy[0]=='l' && linesPerSet!=1) {
      insertBeginning(cache[setIndex],tag);
      return;
    }
    insertEnd(current,tag);
    return;
  }

  //if it's not found use policy
  //printf("not found: use policy\n");
  if (policy[0]=='l' && linesPerSet!=1) {
    removeAfterThis(before);
    insertBeginning(cache[setIndex],tag);
    return;
  }
  fifo(cache, current, tag, setIndex, linesPerSet);
  return;

}

void printList(struct CacheLine** cache, int setSize){
  for (size_t i = 0; i < setSize; i++) {
    struct CacheLine* current=cache[i];
    printf("set: %ld\n",i);
    while (current->next!=0) {
      current=current->next;
      printf("%ld-->",current->tag);
    }
    printf("\n");
  }
}

int main(int argc, char const *argv[argc+1]) {
  // ./first 32 assoc:2 fifo 4 trace2.txt
  if (argc!=6){
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
/*
  printf("\nn%ld\n",n);
  printf("assoc: %s\n", assoc);
  printf("tagBits: %ld\n",tagBits);
  printf("offsetBits: %ld\n",offsetBits);
  printf("setSize: %ld\n",setSize);
  printf("setBits: %ld\n",setBits);
  printf("linesPerSet: %ld\n",linesPerSet);
*/
  char access[2];
  unsigned long int address;
  struct CacheLine **cache=calloc(setSize,sizeof(struct CacheLine));
  for (size_t i = 0; i < setSize; i++) {
    cache[i]=calloc(1,sizeof(struct CacheLine));
    cache[i]->next=0;
  }
  //printf("\n");

  while(fscanf(f,"%s %lx",access,&address)!=EOF){

    //unsigned long int offset = address & ((1lu<<offsetBits)-1lu);
    unsigned long int setIndex = (address>>offsetBits) & ((1lu<<setBits)-1lu);
    unsigned long int tag = (address >> (offsetBits+setBits)) & ((1lu<<tagBits)-1lu);

    //printf("access:%s address:%lx\n", access, address);
    //printf("offset: %ld setIndex: %ld tag: %ld access: %s\n", offset, setIndex, tag, access);


    if(access[0]=='R'){
      read(cache,setIndex,linesPerSet,tag);
    }

    else if(access[0]=='W'){
      write(cache,setIndex,linesPerSet,tag);
    }

    //printf("\n");
  }
  //printList(cache,setSize);
  freeEverything(cache,setSize,linesPerSet,blockSize);
  printf("memread:%ld\nmemwrite:%ld\ncachehit:%ld\ncachemiss:%ld\n", memread,memwrite,cachehit,cachemiss);
  return EXIT_SUCCESS;
}
