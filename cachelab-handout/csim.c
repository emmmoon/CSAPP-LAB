#include "cachelab.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>

int s,E,b,S,tmark;
char t[100];
FILE *pFile;
typedef struct CacheLine
{
    int vivid;
    int mark;
    int timer;
} CacheLineNode;
CacheLineNode **head;
int hits;
int misses;
int evictions;

void Print_Usage(){
    printf("Usage: ./csim-ref [-hv] -s <num> -E <num> -b <num> -t <file>\n");
    printf("Options:\n");
    printf("  -h         Print this help message.\n");
    printf("  -v         Optional verbose flag.\n");
    printf("  -s <num>   Number of set index bits.\n");
    printf("  -E <num>   Number of lines per set.\n");
    printf("  -b <num>   Number of block offset bits.\n");
    printf("  -t <file>  Trace file.\n");
    printf("\n");
    printf("Examples:\n");
    printf("  linux>  ./csim-ref -s 4 -E 1 -b 4 -t traces/yi.trace\n");
    printf("  linux>  ./csim-ref -v -s 8 -E 2 -b 4 -t traces/yi.trace\n");
}


void Create_Cache(){
    head = (CacheLineNode **)malloc(sizeof(CacheLineNode *) * S);
    if(head == NULL){
        return;
    }
    for(int i = 0; i < S;i++){
        head[i] = (CacheLineNode *)malloc(sizeof(CacheLineNode) * E);
        if(head[i] == NULL){
            return;
        }
    }
    for(int i = 0;i < S;i++)
        for(int j = 0;j < E;j++){
            head[i][j].vivid = 0;
            head[i][j].timer = -1;
            head[i][j].mark = -1;
        }
}


void New_Cache(int address){
    tmark = 64 - s - b;
    int tmask = (1 << tmark) - 1;
    int mark1 = (address >> (s + b)) & tmask;
    int smask = (1 << s) - 1;
    int s1 = (address >> b) & smask;
    int firstempty = -1;
    for(int j = 0;j < E;j++){
        if(head[s1][j].vivid == 1 && head[s1][j].mark == mark1){
            hits++;
            head[s1][j].timer = 0;
            return;
        }
        if(head[s1][j].vivid == 0 && firstempty == -1){
            firstempty = j;
        }
    }
    misses++;
    if(firstempty != -1){
        head[s1][firstempty].timer = 0;
        head[s1][firstempty].vivid = 1;
        head[s1][firstempty].mark = mark1;
        return;
    }
    evictions++;
    int maxtimernum = -1;
    int maxtimerindex = -1;
    for(int j = 0;j < E;j++){
        if(head[s1][j].timer > maxtimernum){
            maxtimernum = head[s1][j].timer;
            maxtimerindex = j;
        }
    }
    head[s1][maxtimerindex].mark = mark1;
    head[s1][maxtimerindex].timer = 0;
    return;
}


void Update_Timer(){
    for(int i = 0;i < S;i++)
        for(int j = 0;j < E;j++){
            if(head[i][j].vivid == 1)
                head[i][j].timer++;
        }
}


void Scanf_Example(){
    char identifier;
    unsigned address;
    int size;
    while (fscanf(pFile, "%c %x,%d", &identifier, &address, &size) > 0)
    {
        switch (identifier)
        {
        case 'M':
            New_Cache(address);
            New_Cache(address);
            break;
        case 'L':
            New_Cache(address);
            break;

        case 'S':
            New_Cache(address);
            break;
        }
        Update_Timer();
    }
    for(int i = 0;i < S;i++)
        free(head[i]);
    free(head);

}

int main(int argc, char **argv)
{
    int opt;
    while(-1 != (opt = getopt(argc,argv, "hvs:E:b:t:"))){
        switch (opt)
        {
        case 'h':
            Print_Usage();
            break;

        case 'v':
            Print_Usage();
            break;
        
        case 's':
            s = atoi(optarg);
            break;

        case 'E':
            E = atoi(optarg);
            break;

        case 'b':
            b = atoi(optarg);
            break;

        case 't':
            strcpy(t, optarg);
            break;
              
        default:
            Print_Usage();
            break;
        }
    }
    if(s <= 0 || E <= 0 || b <= 0 || t == NULL){
        return -1;
    }
    pFile = fopen(t,"r");
    if(pFile == NULL){
        printf("traces/one.trace: No such file or directory\n");
        return 1;
    }
    S = 1 << s;
    Create_Cache();
    if(head == NULL)
        return 1;
    Scanf_Example();
    fclose(pFile);
    printSummary(hits, misses, evictions);
    return 0;
}
