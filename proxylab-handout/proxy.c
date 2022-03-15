#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400
#define CACHE_OBJS_COUNT 10

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char *accept_hdr = "accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n";
static const char *accept_encoding_hdr = "accept-encoding: gzip, deflate\r\n";
static const char *language_hdr = "accept-language: zh,en-us;q=0.7,en;q=0.3\r\n";
static const char *connection_hdr = "connection: close\r\n";
static const char *pxy_connection_hdr = "proxy-connection: close\r\n";
static const char *secure_hdr = "upgrade-insecure-requests: 1\r\n";

void doit(int fd);
int parse_uri(char *uri, char *filename, char *path, char *port);
// void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);
// void read_requesthdrs(rio_t *rp);
void *thread(void *vargp);
void cache_init();
int cache_find(const char *uri);
void cache_r_pre(int index);
void cache_r_aft(int index);
void cache_LRU(int index);
void cache_uri(char *uri, char *cachebuf);



typedef struct cacheline{
    char cache_obj[MAX_OBJECT_SIZE];
    char cache_uri[MAXLINE];
    int timer;
    int vivid;

    int readCnt;
    sem_t mutex;
    sem_t wcntmutex;

}cache_block;

typedef struct{
    cache_block cacheobjs[CACHE_OBJS_COUNT];
    int cache_num;
}Cache;

Cache cache;



int main(int argc, char **argv)
{
    int listenfd, connfd;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clienaddr;
    pthread_t tid;

    /*Check command-line args*/
    if(argc != 2){
        fprintf(stderr, "usage: %s <port>\n",argv[0]);
    }

    cache_init();

    listenfd = Open_listenfd(argv[1]);
    while (1)
    {
        clientlen = sizeof(clienaddr);
        connfd = Accept(listenfd, (SA *)&clienaddr, &clientlen);
        Getnameinfo((SA *)&clienaddr, clientlen, hostname, MAXLINE,
                    port, MAXLINE, 0);
        printf("Accepted connection from (%s, %s)\n",hostname, port);
        Pthread_create(&tid, NULL, thread, &connfd);
    }
    
    return 0;
}


void *thread(void *vargp)
{
    int fd = *((int*)vargp);
    Pthread_detach(pthread_self());
    doit(fd);

    Close(fd);
    return NULL;
}


void doit(int fd)
{
    int is_legal;
    int sendfd;
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE],sbuf[MAXLINE];
    char filename[MAXLINE],path[MAXLINE];
    char cachebuf[MAX_OBJECT_SIZE];
    char port[10];
    rio_t rio,sio;
    size_t n,m = 0;
    int cacheindex;

    rio_readinitb(&rio, fd);
    Rio_readlineb(&rio, buf, MAXLINE);
    sscanf(buf, "%s %s %s",method, uri, version);
    if(strcasecmp(method, "GET")){
        // clienterror(fd, method, "501", "Not implemented",
        //             "Proxy does not implement this method");
        return;
    }
    // read_requesthdrs(&rio);

    if((cacheindex = cache_find(uri)) != -1){
        cache_r_pre(cacheindex);
        Rio_writen(fd,cache.cacheobjs[cacheindex].cache_obj,strlen(cache.cacheobjs[cacheindex].cache_obj));
        cache_r_aft(cacheindex);
        cache_LRU(cacheindex);
        return;
    }

    /*parse URI from GET request */
    is_legal = parse_uri(uri, filename, path, port);

    if(!is_legal){
        // clienterror(fd, uri, "404", "Not found",
        //         "Proxy couldn't read this file");
        return;
    }

    sendfd = open_clientfd(path, port);
    Rio_readinitb(&sio,sendfd);

    sprintf(sbuf, "GET %s HTTP/1.0\r\nHost: %s\r\n%s%s%s%s%s%s%s\r\n",
            filename,
            path,
            user_agent_hdr,
            accept_hdr,
            language_hdr,
            accept_encoding_hdr,
            connection_hdr,
            secure_hdr,
            pxy_connection_hdr);

    Rio_writen(sendfd, sbuf, strlen(sbuf));

    while ((n = Rio_readlineb(&sio, sbuf, MAXLINE))!= 0)
    {
        m += n;
        if(m < MAX_OBJECT_SIZE)
            strcat(cachebuf, sbuf);
        Rio_writen(fd, sbuf, n);
    }

    Close(sendfd);
    if(m < MAX_OBJECT_SIZE)
        cache_uri(uri,cachebuf);

}


// void read_requesthdrs(rio_t *rp)
// {
//     char buf[MAXLINE];

//     Rio_readlineb(rp, buf, MAXLINE);
//     while (strcmp(buf, "\r\n"))
//     {
//         Rio_readlineb(rp, buf, MAXLINE);
//         printf("%s", buf);
//     }
//     return;
// }


int parse_uri(char *uri, char *filename, char *path, char *port)
{
    char *ptr, *tmp;
    char tmpath[10];
    int len;
    ptr = uri;
    tmp = strchr(ptr, ':');
    if(tmp == NULL){
        return 0;
    }
    len = tmp - ptr;
    strncpy(tmpath, ptr, len);
    tmpath[len] = '\0';
    for(int i = 0;i < len;i++)
        tmpath[i] = tolower(tmpath[i]);
    if(strcasecmp(tmpath, "http")){
        return 0;
    }
    ptr = ++tmp;
    for(int i = 0;i < 2;i++,tmp++){
        if(*tmp != '/')
            return 0;
    }
    ptr = tmp;
    tmp = strchr(ptr, ':');
    if(tmp == NULL){
        tmp = ptr;
        while(*tmp != '/' && *tmp != '\0')
            tmp++;
        len = tmp - ptr;
        strncpy(path, ptr, len);
        path[len] = '\0';
        strcpy(port, "80");
    }
    else{
        len = tmp - ptr;
        strncpy(path, ptr, len);
        path[len] = '\0';
        ptr = ++tmp;
        while(*tmp != '/' && *tmp != '\0')
            tmp++;
        len = tmp - ptr;
        strncpy(port, ptr, len);
        port[len] = '\0';
    }
    ptr = tmp;
    if(*ptr == '\0'){
        strcpy(filename, "/");
        return 1;
    }
    else{
        ptr = tmp;
        while (*tmp != '\0')
            tmp++;
        len = tmp - ptr;
        strncpy(filename, ptr, len);
        filename[len] = '\0';
        return 1;
    }

}

// void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg)
// {
//     char buf[MAXLINE],body[MAXLINE];

//     sprintf(body, "<html><title>Proxy Error</title>");
//     sprintf(body, "%s<body bgcolor=""ffffff"">\r\n",body);
//     sprintf(body, "%s%s: %s\r\n",body, errnum, shortmsg);
//     sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
//     sprintf(body, "%s<hr><em>The Proxy</em>\r\n",body);

//     sprintf(buf, "HTTP/1.0 %s %s\r\n",errnum, shortmsg);
//     Rio_writen(fd, buf, strlen(buf));
//     sprintf(buf, "Countent-type: text/html\r\n");
//     Rio_writen(fd, buf, strlen(buf));
//     sprintf(buf, "Content-length: %d\r\n\r\n",(int)strlen(body));
//     Rio_writen(fd,buf,strlen(buf));
//     Rio_writen(fd,body,strlen(body));

// }

void cache_r_pre(int index)
{
    P(&cache.cacheobjs[index].mutex);
    cache.cacheobjs[index].readCnt++;
    if(cache.cacheobjs[index].readCnt == 1)
        P(&cache.cacheobjs[index].wcntmutex);
    V(&cache.cacheobjs[index].mutex);
}


void cache_r_aft(int index)
{
    P(&cache.cacheobjs[index].mutex);
    cache.cacheobjs[index].readCnt--;
    if(cache.cacheobjs[index].readCnt == 0)
        V(&cache.cacheobjs[index].wcntmutex);
    V(&cache.cacheobjs[index].mutex);    
}

void cache_init()
{
    cache.cache_num = 0;
    for(int i = 0;i < CACHE_OBJS_COUNT;i++){
        cache.cacheobjs[i].vivid = 0;
        cache.cacheobjs[i].timer = 0;
        cache.cacheobjs[i].readCnt = 0;
        sem_init(&cache.cacheobjs[i].mutex, 0, 1);
        sem_init(&cache.cacheobjs[i].wcntmutex, 0, 1);

    }
}


int cache_find(const char *uri)
{
    int i,index = -1;
    for(i = 0;i < CACHE_OBJS_COUNT;i++){
        cache_r_pre(i);
        if((cache.cacheobjs[i].vivid) && (!strcmp(cache.cacheobjs[i].cache_uri, uri))){
            index = i;
        }
        cache_r_aft(i);
    }
    return index;
}



void cache_LRU(int index)
{
    int i;
    for(i = 0;i < CACHE_OBJS_COUNT;i++){
        if (i != index){
            P(&cache.cacheobjs[i].wcntmutex);
            if(cache.cacheobjs[i].vivid == 1){
                cache.cacheobjs[i].timer++;
            }
            V(&cache.cacheobjs[i].wcntmutex);
        }
        else{
            P(&cache.cacheobjs[i].wcntmutex);
            cache.cacheobjs[i].timer = 0;    
            V(&cache.cacheobjs[i].wcntmutex);                        
        }    
    }
}


void cache_uri(char *uri, char *cachebuf)
{
    int i,index = -1, maxtimer = 0, maxtimeri;
    for(i = 0;i < CACHE_OBJS_COUNT;i++){
        cache_r_pre(i);
        if(cache.cacheobjs[i].vivid == 0){
            index = i;
        }        
        else if(cache.cacheobjs[i].timer > maxtimer){
            maxtimer = cache.cacheobjs[i].timer;
            maxtimeri = i;
        }
        cache_r_aft(i);
    }


    if(index == -1){
        P(&cache.cacheobjs[maxtimeri].wcntmutex);
        strcpy(cache.cacheobjs[maxtimeri].cache_obj, cachebuf);
        strcpy(cache.cacheobjs[maxtimeri].cache_uri, uri);
        V(&cache.cacheobjs[maxtimeri].wcntmutex);
        cache_LRU(maxtimeri);
    }
    else{

        P(&cache.cacheobjs[index].wcntmutex);
        strcpy(cache.cacheobjs[index].cache_obj, cachebuf);
        strcpy(cache.cacheobjs[index].cache_uri, uri);
        cache.cacheobjs[index].vivid = 1;
        V(&cache.cacheobjs[index].wcntmutex);
        cache_LRU(index);
    }

    return;

}