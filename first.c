#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

int checkSizes(int num1, int num2){
  if((ceil(log2(num1)) != floor(log2(num1)))) return 0;
  if((ceil(log2(num2)) != floor(log2(num2)))) return 0;
  return 1;
}

int main(int argc, char const *argv[argc+1]) {
  // ./first 32 assoc:2 fifo 4 trace2.txt
  if (argc<6){
    printf("error\n");
    return EXIT_SUCCESS;
  }

  //cache: 1st blocksize: 4th
  int cacheSize = atoi(argv[1]);
  int blockSize = atoi(argv[4]);
  char policy[5];
  strcpy(policy,argv[3]);
  int n = 0;
  char assoc[7];

  if(argv[2][0]=='a' && argv[2][5]==':'){
    size_t j = 0;
    strcpy(assoc, "assoc");
    char *temp = malloc((strlen(argv[2])-6)*sizeof(char*));
    for (size_t i = 6; i < strlen(argv[2]); i++) {
      temp[j++]=argv[2][i];
    }
    n = atoi(temp);
    free(temp);
  }else strcpy(assoc,argv[2]);

  printf("n%d\n",n);
  printf("%s\n", policy);


  if(checkSizes(cacheSize, blockSize)==0){
    printf("error\n");
    return EXIT_SUCCESS;
  }

  FILE *f;
  f = fopen(argv[5],"r");
  if (f==0){
    printf("error\n");
    return EXIT_SUCCESS;
  }

  /////////////////////////////////////////
  //NEEDS TO CHECK FOR EACH LINE IN TRACE//
  /////////////////////////////////////////

  char access[2];
  unsigned long int address;
  while(fscanf(f,"%s %lx",access,&address)!=EOF){
    printf("access:%s address:%lx\n", access, address);
  }

  return EXIT_SUCCESS;
}
