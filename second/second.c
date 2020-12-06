#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

unsigned long int L1offsetBits, L2offsetBits;
unsigned long int L1setBits, L2setBits;
unsigned long int L1tagBits, L2tagBits;
unsigned long int L1setSize=0, L2setSize=0;
unsigned long int L1tag, L2tag;
unsigned long int memread=0, memwrite=0, L1cachehit=0, L1cachemiss=0, L2cachehit=0, L2cachemiss=0;
int checked=0;
char L1policy[5], L2policy[5];

struct CacheLine{
  unsigned long int L1tag;
  unsigned long int L2tag;
  unsigned long int address;
  struct CacheLine* next;
};
//printf("set: %ld\n",i);
//printf("%ld:0x%lx-->",current->L1tag,current->address);

void L1printList(struct CacheLine** cache, int setSize){
  for (size_t i = 0; i < setSize; i++) {
    if(cache[i]->next==0) continue;
    struct CacheLine* current=cache[i];
    printf("set: %ld\n",i);
    while(current->next!=0) {
      current=current->next;
      printf("%ld:0x%lx-->",current->L1tag,current->address);
    }
    printf("\n");
  }
}

void L2printList(struct CacheLine** cache, int setSize){
  for (size_t i = 0; i < setSize; i++) {
    struct CacheLine* current=cache[i];
    printf("set: %ld\n",i);
    while (current->next!=0) {
      current=current->next;
      printf("%ld:0x%lx-->",current->L2tag,current->address);
    }
    printf("\n");
  }
}


void checkDuplicates(struct CacheLine** cache, unsigned long int setSize){
  for (size_t i = 0; i < setSize; i++) {
    if(cache[i]->next==0) continue;
    struct CacheLine* current=cache[i]->next;
    struct CacheLine* current2=current->next;
    while(current->next!=0){
      while(current2!=0) {
        if(current->address==current2->address){
          printf("duplicate found: address: 0x%lx set: %ld\n", current->address, i);
          checked=1;
        }
        current2=current2->next;
      }
      current=current->next;
      current2=current->next;
    }
    //printf("current->address: 0x%lx\n",current->address);

  }
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

void insertBeginning(struct CacheLine* current, unsigned long int address){
  struct CacheLine* temp = malloc(sizeof(struct CacheLine));
  temp->L1tag=L1tag;
  temp->L2tag=L2tag;
  temp->address=address;
  temp->next=current->next;
  current->next=temp;
}

void insertBeginning2(struct CacheLine* current,unsigned long int tag1, unsigned long int tag2, unsigned long int address){
  struct CacheLine* temp = malloc(sizeof(struct CacheLine));
  temp->L1tag=tag1;
  temp->L2tag=tag2;
  temp->address=address;
  temp->next=current->next;
  current->next=temp;
}

void insertEnd(struct CacheLine* end, unsigned long int address){
  end->next=malloc(sizeof(struct CacheLine));
  end->next->L1tag=L1tag;
  end->next->L2tag=L2tag;
  end->next->address=address;
  end->next->next=0;
}

void insertEnd2(struct CacheLine* end,unsigned long int tag1, unsigned long int tag2, unsigned long int address){
  end->next=malloc(sizeof(struct CacheLine));
  end->next->L1tag=tag1;
  end->next->L2tag=tag2;
  end->next->address=address;
  end->next->next=0;
}

void removeAfterThis(struct CacheLine* current){
  struct CacheLine* temp = current->next;
  current->next=current->next->next;
  free(temp);
}

int nospace(struct CacheLine* current, unsigned long int L2linesPerSet){
  if(current==0) return 0;
  unsigned long int i = 1;
  while(current->next!=0){
    i++;
    current=current->next;
  }
  if(i==L2linesPerSet) {
    return 1;
  }
  return 0;
}

void moveIntoL2(struct CacheLine** L2cache, struct CacheLine** L1cache, struct CacheLine* current2,struct CacheLine* current, struct CacheLine* before2, unsigned long int L1setIndex, unsigned long int L2setIndex, size_t j, int L2linesPerSet, unsigned long int address) {
    unsigned long int setIndex = ((current->address)>>L2offsetBits) & ((1lu<<L2setBits)-1lu);

    if( nospace(L2cache[setIndex]->next,L2linesPerSet) ) {
      //printf("current->address:%lx\n",current->address);
      //printf("setIndex:%lx\n",setIndex);

      //printf("no space in L2 set, need to evict L2\n");
      if(L2linesPerSet==1)  { //next=0
        printf("L2linesPerSet==1\n");
        removeAfterThis(L2cache[setIndex]);
        if(L1policy[0]=='l'){
          insertBeginning2(L2cache[setIndex],current->L1tag,current->L2tag,current->address);
          return;
        }
        insertBeginning2(L2cache[setIndex],L1cache[L1setIndex]->L1tag,L1cache[L1setIndex]->L2tag,L1cache[L1setIndex]->address);
        return;
      }

      //lru
      if (L2policy[0]=='l' && L2linesPerSet!=1) {
        unsigned long int setIndex = ((current->address)>>L2offsetBits) & ((1lu<<L2setBits)-1lu);
        removeAfterThis(before2);
        insertBeginning2(L2cache[setIndex],current->L1tag,current->L2tag,current->address);
        return;
      }
      //fifo
      setIndex = ((current->address)>>L2offsetBits) & ((1lu<<L2setBits)-1lu);
      removeAfterThis(L2cache[setIndex]);
      if(L2linesPerSet==1){
        struct CacheLine* temp = malloc(sizeof(struct CacheLine));
        temp->L1tag=L1cache[L1setIndex]->next->L1tag;
        temp->L2tag=L1cache[L1setIndex]->next->L2tag;
        temp->address=L1cache[L1setIndex]->next->address;
        temp->next=0;
        L2cache[setIndex]->next=temp;
        return;
      }
      //printf("address: %lx\n\n", address);
      //printf("current2->address: %lx\n\n", current2->address);
      /*
      L1printList(L1cache,L1setSize);
      printf("\n");
      L1printList(L2cache,L2setSize);
      printf("\n");*/
      current2=L2cache[setIndex]->next;
      while(current2->next!=0) current2=current2->next;

      if(L1policy[0]=='f') {
        insertEnd2(current2,L1cache[L1setIndex]->next->L1tag,L1cache[L1setIndex]->next->L2tag,L1cache[L1setIndex]->next->address);
        return;
      }

      insertEnd2(current2,current->L1tag,current->L2tag,current->address);
      return;
    }

    //L2 has space
    if(L1policy[0]=='l'){
      setIndex = ((current->address)>>L2offsetBits) & ((1lu<<L2setBits)-1lu);
    }else setIndex = ((L1cache[L1setIndex]->next->address)>>L2offsetBits) & ((1lu<<L2setBits)-1lu);

    //printf("current->address:%lx\n",current->address);

    //checks if empty
    if(L2cache[setIndex]->next==0){
      //printf("L2set not full, but is empty\n");
      if(L1policy[0]=='l'){
        L2cache[setIndex]->next = malloc(sizeof(struct CacheLine));
        L2cache[setIndex]->next->L1tag=current->L1tag;
        L2cache[setIndex]->next->L2tag=current->L2tag;
        L2cache[setIndex]->next->address=current->address;
        L2cache[setIndex]->next->next=0;
        return;
      }
      L2cache[setIndex]->next = malloc(sizeof(struct CacheLine));
      L2cache[setIndex]->next->L1tag=L1cache[L1setIndex]->next->L1tag;
      L2cache[setIndex]->next->L2tag=L1cache[L1setIndex]->next->L2tag;
      L2cache[setIndex]->next->address=L1cache[L1setIndex]->next->address;
      L2cache[setIndex]->next->next=0;
      return;
    }


    //L2set not full, move the block we want to evict into L2
    //printf("L2set not full\n");
    //if(j!=L2linesPerSet){ //if there are errors earlier then set this

    //insert beginning L2 lru
    if(L2policy[0]=='l' && L2linesPerSet!=1) {
      insertBeginning2(L2cache[setIndex],current->L1tag,current->L2tag,current->address);
      return;
    }

    if(before2->next==0) {
      insertEnd2(before2,L1cache[L1setIndex]->next->L1tag,L1cache[L1setIndex]->next->L2tag,L1cache[L1setIndex]->next->address);
      return;
    }

    current2=L2cache[setIndex]->next;
    while(current2->next!=0) current2=current2->next;

    //insert end of L2 fifo
    if(L1policy[0]=='f') {
      insertEnd2(current2,L1cache[L1setIndex]->next->L1tag,L1cache[L1setIndex]->next->L2tag,L1cache[L1setIndex]->next->address);
      return;
    }

    insertEnd2(current2,current->L1tag,current->L2tag,current->address);
    return;

}

void read(struct CacheLine** L1cache, int L1linesPerSet, struct CacheLine** L2cache, int L2linesPerSet, unsigned long int address){
  /************************************************************
  * L1 CACHE STUFF                                            *
  ************************************************************/

  unsigned long int L1setIndex = (address>>L1offsetBits) & ((1lu<<L1setBits)-1lu);
  L1tag = (address >> (L1offsetBits+L1setBits)) & ((1lu<<L1tagBits)-1lu);
  unsigned long int L2setIndex = (address>>L2offsetBits) & ((1lu<<L2setBits)-1lu);
  L2tag = (address >> (L2offsetBits+L2setBits)) & ((1lu<<L2tagBits)-1lu);
/*
  if(address==0x9cb0114){
    printf("\nL1setIndex: %ld L1tag: %ld L2setIndex: %ld \nL2tag: %ld", L1setIndex, L1tag, L2setIndex, L2tag);
  }
*/
  //printf("address: 0x%lx\n", address);
  //printf("L1setIndex: %ld L1tag: %ld L2setIndex: %ld \nL2tag: %ld\n", L1setIndex, L1tag, L2setIndex, L2tag);


  //printf("address: 0x%lx\n", address);
  //printf("L1setIndex: %ld L1tag: %ld L2setIndex: %ld \nL2tag: %ld\n", L1setIndex, L1tag, L2setIndex, L2tag);

  size_t i = 0;
  struct CacheLine* current = L1cache[L1setIndex];
  struct CacheLine* before=current;
  if(current->next==0){
    current->next = malloc(sizeof(struct CacheLine));
    current->next->L1tag=L1tag;
    current->next->L2tag=L2tag;
    current->next->address=address;
    current->next->next=0;
    memread++;

    L1cachemiss++;
    L2cachemiss++;
    return;
  }

  //finds location of matching L1tag
  while(current->next!=0){
    if(current->next->L1tag==L1tag){
      L1cachehit++;
      //printf("hitting\n");
      if(L1policy[0]=='l' && L1linesPerSet!=1){
        removeAfterThis(current);
        insertBeginning(L1cache[L1setIndex],address);
        return;
      }
      return;
    }
    before=current;
    current=current->next;
    i++;
    if(i==L1linesPerSet) break;
  }

  L1cachemiss++;

  /************************************************************
  * L2 CACHE STUFF                                            *
  ************************************************************/

  //then search inside L2
  size_t j = 0;
  struct CacheLine* current2 = L2cache[L2setIndex];
  struct CacheLine* before2=current2;
  while(current2->next!=0){
    if(current2->next->L2tag==L2tag){
      if(L2linesPerSet!=1){
        before2=current2;
        current2=before2->next;
        j++;
      }
      //printf("before2->address: %lx\n", before2->address);
      //printf("current2->next->L2tag: %lx\n", current2->L2tag);

      L2cachehit++;
      //printf("L2 hit, need to move to L1 and evict L1 to L2\n");
      if(i!=L1linesPerSet)  printf("\n\nERROR BIG PLZ FIX\n\n");

      /************************************************************
      * moveintoL1                                                *
      ************************************************************/
      unsigned long int tempL1tag=current2->L1tag;
      unsigned long int tempL2tag=current2->L2tag;
      unsigned long int tempaddress=current2->address;
      //printf("tempaddress: %lx\n", tempaddress);

      struct CacheLine* temp = current2;
      struct CacheLine* temp2=before2;
      while(temp->next!=0){
        temp2=temp;
        temp=temp->next;
      }
      //printf("j: %ld\n", j);
      //L1printList(L2cache,L2setSize);
      moveIntoL2(L2cache, L1cache, temp, current, temp2, L1setIndex, L2setIndex, j, L2linesPerSet, address);
      removeAfterThis(before2); //remove current2 in L2 cache


      if (L1policy[0]=='l' && L1linesPerSet!=1) {
        removeAfterThis(before);
        insertBeginning(L1cache[L1setIndex],address);
        return;
      }
      removeAfterThis(L1cache[L1setIndex]);
      if(L1linesPerSet==1){
        struct CacheLine* temp = malloc(sizeof(struct CacheLine));
        temp->L1tag=L1tag;
        temp->L2tag=L2tag;
        temp->address=address;
        temp->next=0;
        L1cache[L1setIndex]->next=temp;
        return;
      }
      insertEnd2(current,tempL1tag,tempL2tag,tempaddress);

      return;

    }
    before2=current2;
    current2=current2->next;
    j++;
    if(j==L2linesPerSet) break;
  }

  L2cachemiss++;
  memread++;

  //not found and L1set not full
  if(i!=L1linesPerSet){
    if(L1policy[0]=='l' && L1linesPerSet!=1) {
      insertBeginning(L1cache[L1setIndex],address);
      return;
    }
    insertEnd(current,address);
    return;
  }

  //printf("not found and L1 full: use L1policy and evict into L2\n");
  if (L1policy[0]=='l' && L1linesPerSet!=1) {
    moveIntoL2(L2cache, L1cache, current2, current, before2, L1setIndex, L2setIndex, j, L2linesPerSet, address);
    removeAfterThis(before);
    insertBeginning(L1cache[L1setIndex],address);
    return;
  }

  //fifo
  //L1cache[L1setIndex]->next->address
  //printf("evict L1cache[L1setIndex]->next->L1tag: %ld\n",L1cache[L1setIndex]->next->L1tag);
  moveIntoL2(L2cache, L1cache, current2, current, before2, L1setIndex, L2setIndex, j, L2linesPerSet, address);
  removeAfterThis(L1cache[L1setIndex]);
  if(L1linesPerSet==1){
    struct CacheLine* temp = malloc(sizeof(struct CacheLine));
    temp->L1tag=L1tag;
    temp->L2tag=L2tag;
    temp->address=address;
    temp->next=0;
    L1cache[L1setIndex]->next=temp;
    return;
  }
  insertEnd(current,address);
  return;

}

void write(struct CacheLine** L1cache, int L1linesPerSet, struct CacheLine** L2cache, int L2linesPerSet, unsigned long int address){
  /************************************************************
  * L1 CACHE STUFF                                            *
  ************************************************************/

  unsigned long int L1setIndex = (address>>L1offsetBits) & ((1lu<<L1setBits)-1lu);
  L1tag = (address >> (L1offsetBits+L1setBits)) & ((1lu<<L1tagBits)-1lu);
  unsigned long int L2setIndex = (address>>L2offsetBits) & ((1lu<<L2setBits)-1lu);
  L2tag = (address >> (L2offsetBits+L2setBits)) & ((1lu<<L2tagBits)-1lu);



  //printf("address: 0x%lx\n", address);
  //printf("L1setIndex: %ld L1tag: %ld L2setIndex: %ld \nL2tag: %ld\n", L1setIndex, L1tag, L2setIndex, L2tag);


  //printf("address: 0x%lx\n", address);
  //printf("L1setIndex: %ld L1tag: %ld L2setIndex: %ld \nL2tag: %ld\n", L1setIndex, L1tag, L2setIndex, L2tag);

  size_t i = 0;
  struct CacheLine* current = L1cache[L1setIndex];
  struct CacheLine* before=current;
  if(current->next==0){
    current->next = malloc(sizeof(struct CacheLine));
    current->next->L1tag=L1tag;
    current->next->L2tag=L2tag;
    current->next->address=address;
    current->next->next=0;
    memread++;
    memwrite++;
    L1cachemiss++;
    L2cachemiss++;
    return;
  }
  memwrite++;
  //finds location of matching L1tag
  while(current->next!=0){
    if(current->next->L1tag==L1tag){
      L1cachehit++;
      //printf("hitting\n");
      if(L1policy[0]=='l' && L1linesPerSet!=1){
        removeAfterThis(current);
        insertBeginning(L1cache[L1setIndex],address);
        return;
      }
      return;
    }
    before=current;
    current=current->next;
    i++;
    if(i==L1linesPerSet) break;
  }

  L1cachemiss++;

  /************************************************************
  * L2 CACHE STUFF                                            *
  ************************************************************/

  //then search inside L2
  size_t j = 0;
  struct CacheLine* current2 = L2cache[L2setIndex];
  struct CacheLine* before2=current2;
  while(current2->next!=0){
    if(current2->next->L2tag==L2tag){
      if(L2linesPerSet!=1){
        before2=current2;
        current2=before2->next;
        j++;
      }
      //printf("before2->address: %lx\n", before2->address);
      //printf("current2->next->L2tag: %lx\n", current2->L2tag);

      L2cachehit++;
      //printf("L2 hit, need to move to L1 and evict L1 to L2\n");
      if(i!=L1linesPerSet)  printf("\n\nERROR BIG PLZ FIX\n\n");

      /************************************************************
      * moveintoL1                                                *
      ************************************************************/
      unsigned long int tempL1tag=current2->L1tag;
      unsigned long int tempL2tag=current2->L2tag;
      unsigned long int tempaddress=current2->address;
      //printf("tempaddress: %lx\n", tempaddress);

      struct CacheLine* temp = current2;
      struct CacheLine* temp2=before2;
      while(temp->next!=0){
        temp2=temp;
        temp=temp->next;
      }
      //L1printList(L2cache,L2setSize);
      if(j==L2linesPerSet){
        removeAfterThis(before2); //remove current2 in L2 cache
        moveIntoL2(L2cache, L1cache, temp, current, temp2, L1setIndex, L2setIndex, j, L2linesPerSet, address);
      }else{
        moveIntoL2(L2cache, L1cache, temp, current, temp2, L1setIndex, L2setIndex, j, L2linesPerSet, address);
        removeAfterThis(before2); //remove current2 in L2 cache
      }

      if (L1policy[0]=='l' && L1linesPerSet!=1) {
        removeAfterThis(before);
        insertBeginning(L1cache[L1setIndex],address);
        return;
      }
      removeAfterThis(L1cache[L1setIndex]);
      if(L1linesPerSet==1){
        struct CacheLine* temp = malloc(sizeof(struct CacheLine));
        temp->L1tag=L1tag;
        temp->L2tag=L2tag;
        temp->address=address;
        temp->next=0;
        L1cache[L1setIndex]->next=temp;
        return;
      }
      insertEnd2(current,tempL1tag,tempL2tag,tempaddress);

      return;

    }
    before2=current2;
    current2=current2->next;
    j++;
    if(j==L2linesPerSet) break;
  }

  L2cachemiss++;
  memread++;

  //not found and L1set not full
  if(i!=L1linesPerSet){
    if(L1policy[0]=='l' && L1linesPerSet!=1) {
      insertBeginning(L1cache[L1setIndex],address);
      return;
    }
    insertEnd(current,address);
    return;
  }

  //printf("not found and L1 full: use L1policy and evict into L2\n");
  if (L1policy[0]=='l' && L1linesPerSet!=1) {
    moveIntoL2(L2cache, L1cache, current2, current, before2, L1setIndex, L2setIndex, j, L2linesPerSet, address);
    removeAfterThis(before);
    insertBeginning(L1cache[L1setIndex],address);
    return;
  }

  //fifo
  //L1cache[L1setIndex]->next->address
  //printf("evict L1cache[L1setIndex]->next->L1tag: %ld\n",L1cache[L1setIndex]->next->L1tag);
  moveIntoL2(L2cache, L1cache, current2, current, before2, L1setIndex, L2setIndex, j, L2linesPerSet, address);
  removeAfterThis(L1cache[L1setIndex]);
  if(L1linesPerSet==1){
    struct CacheLine* temp = malloc(sizeof(struct CacheLine));
    temp->L1tag=L1tag;
    temp->L2tag=L2tag;
    temp->address=address;
    temp->next=0;
    L1cache[L1setIndex]->next=temp;
    return;
  }
  insertEnd(current,address);
  return;

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
  unsigned long int L1linesPerSet=0, L2linesPerSet=0;
  sizes(L1assoc, &L1setSize, &L1linesPerSet,L1n, &L1cacheSize, &L1blockSize);
  sizes(L2assoc, &L2setSize, &L2linesPerSet,L2n, &L2cacheSize, &L2blockSize);

  L1offsetBits=log2(L1blockSize), L2offsetBits=log2(L2blockSize);
  L1setBits=log2(L1setSize), L2setBits=log2(L2setSize);
  L1tagBits = 48 - L1setBits - L1offsetBits, L2tagBits = 48 - L2setBits - L2offsetBits;
  L2linesPerSet=3; //careful with this
/*
  printf("\nL1tagBits: %ld\n",L1tagBits);
  printf("L1offsetBits: %ld\n",L1offsetBits);
  printf("L1setSize: %ld\n",L1setSize);
  printf("L1setBits: %ld\n",L1setBits);
  printf("L1linesPerSet: %ld\n",L1linesPerSet);

  printf("\nL2tagBits: %ld\n",L2tagBits);
  printf("L2offsetBits: %ld\n",L2offsetBits);
  printf("L2setSize: %ld\n",L2setSize);
  printf("L2setBits: %ld\n",L2setBits);
  printf("L2linesPerSet: %ld\n\n",L2linesPerSet);
*/
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
    //printf("\naddress: 0x%lx access:%s\n", address, access);


    if(access[0]=='R'){
      read(L1cache,L1linesPerSet,L2cache,L2linesPerSet,address);
      /*
      if(checked==0){
        //printf("\naddress: 0x%lx access:%s\n", address, access);
        checkDuplicates(L2cache,L2setSize);
      }*/
      continue;
    }

    if(access[0]=='W'){
      write(L1cache,L1linesPerSet,L2cache,L2linesPerSet,address);
      continue;
    }

  }

  //checkDuplicates(L1cache,L1setSize);
  /*
  printf("\n");
  L1printList(L1cache,L1setSize);
  printf("\nL2 CACHE\n\n");
  //checkDuplicates(L2cache,L2setSize);
  L1printList(L2cache,L2setSize);
  printf("\n");
  */
  printf("memread:%ld\nmemwrite:%ld\nl1cachehit:%ld\nl1cachemiss:%ld\nl2cachehit:%ld\nl2cachemiss:%ld\n", memread,memwrite,L1cachehit,L1cachemiss,L2cachehit,L2cachemiss);
  freeEverything(L1cache,L1setSize,L1linesPerSet,L1blockSize);
  freeEverything(L2cache,L2setSize,L2linesPerSet,L2blockSize);
}
