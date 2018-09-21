#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>

#define MAX_THREADS 25

typedef struct fileparam{
int threadnum;
int start_offset;
int end_offset;
char *filename;
char *search_string;
int totalthreads;
}fileparam_t;

pthread_barrier_t barrier;
pthread_cond_t cv[MAX_THREADS];
pthread_mutex_t pm[MAX_THREADS];


void threadRead(fileparam_t *fp){	
	FILE *fileptr;
	char *buf=(char *)calloc(fp->end_offset-fp->start_offset,sizeof(char));
	char *line=NULL;
	//char *str="";
	int counter=fp->start_offset,len=0; 
	int nread=0,i=0;
	char c;
	//printf("here\n");
	//strcpy(buf,"\0");
	//printf("inside thread read %d,offset %d, size %d, \n",fp->threadnum,fp->start_offset,fp->end_offset);
	fileptr=fopen(fp->filename, "r");
	if(fp==NULL){
		printf("cannot open the file \n");
		return -1;
	}
	fseek(fileptr,fp->start_offset,SEEK_SET);
	//printf("here\n");
	do{
		nread=0;
		nread=getline(&line,&len,fileptr);//!=-1)
		counter+=strlen(line);
		if(strstr(line,fp->search_string)!=NULL){
			strcat(buf,line);			
			//printf("%s",line);
		}
			//printf("%d : counter %d ,size %d, nread %d, %s\n",fp->threadnum,counter,fp->end_offset,nread,line);
	}while(counter<(fp->end_offset));

	pthread_barrier_wait(&barrier);
	//printf("here for thread number %d\n",fp->threadnum);
	if(fp->threadnum >0){
		pthread_mutex_lock(&pm[fp->threadnum]);
		pthread_cond_wait(&cv[fp->threadnum],&pm[fp->threadnum]);
		pthread_mutex_unlock(&pm[fp->threadnum]);
	}
	//printf("thread number %d\n",fp->threadnum);
	printf("%s",buf);
	
	if(fp->threadnum<fp->totalthreads){
		pthread_cond_signal(&cv[fp->threadnum+1]);
	}
	pthread_exit(NULL);
	
}

void usuage(){
	printf(" Please provide the input in proper format \n");
	printf("./pargrep [-t] word [FILE] \n");
	printf("cat ./filename | ./par word\n");
	exit(0);
}

int main(int argc,char **argv) {
	pthread_t pt[MAX_THREADS];
	int noofthreads=2,noofthreadsrunning=0,offset=0;
	int i;
	FILE *filep;
	char ch;
	fileparam_t **fp;
	char *haystack,*needle;
	struct stat st;
	int sizetoseek=0,count=0;
	char *line=NULL;
	size_t len=0;
	ssize_t nread;
	
	//printf("%s\n",argv[argc-1]);
	if(strcmp(argv[argc-1],"-t")==NULL ||argc<2){
		//printf("here\n");
		usuage();
		//return -1;
	}
		
	if (isatty(fileno(stdin))){
    		printf( "stdin is a terminal %d\n",argc );
		for(i=1;i<argc-1;i++) {
			if(strcmp(argv[i],"-t")==0){
				if(argc>4){
					if(i==1){
						noofthreads=atoi(argv[2]);
						needle=argv[3];
						haystack=argv[4];
					}
					else if( i==2) {
						noofthreads=atoi(argv[i+1]);
						needle=argv[1];
						haystack=argv[4];
					}
					else {
						noofthreads=atoi(argv[i+1]);
						needle=argv[1];
						haystack=argv[2];
					}
					break;
				}
				else{
					usuage();
				}
				//printf("-t %d,%s, %s\n",noofthreads,needle,haystack);
			}
			else {
				needle=argv[1];
				haystack=argv[2];
				//printf("threads %d,%s, %s\n",noofthreads,needle,haystack);
			}	
		  }
		
		
	}
	else{
		for(i=1;i<=argc-1;i++) {
			if(strcmp(argv[i],"-t")==0){
			
				if(argc>=2){
					if(i==1){
						noofthreads=atoi(argv[2]);
						needle=argv[3];
						haystack="/dev/stdin";
					}
					else  {
						noofthreads=atoi(argv[i+1]);
						needle=argv[1];
						haystack="/dev/stdin";
					}
					break;
				}
				else{
					usuage();
				}
				//printf("-t %d,%s, %s\n",noofthreads,needle,haystack);
			}
			else {
				needle=argv[1];
				haystack="/dev/stdin";
				//printf("threads %d,%s, %s\n",noofthreads,needle,haystack);
			}	
		  }
		//printf("%s",haystack);
		if((filep=fopen(haystack,"r"))==NULL){
			printf("could not open the file \n");
			return -1;
		}
		while ((nread = getline(&line, &len, filep)) != -1) {
			if(strstr(line,needle)!=NULL){
				printf("%s",line);
			}
			count+=strlen(line);
		   }
		return 0;
	}
	
	stat(haystack,&st);
	sizetoseek=st.st_size/noofthreads;

	if(sizetoseek<strlen(needle))
		sizetoseek=strlen(needle);

	if((filep=fopen(haystack,"r"))==NULL){
		printf("open 1 could not open the file \n");
		return -1;
	}
	
	fp=malloc(sizeof(fileparam_t *)*noofthreads);
	for(i=0;i<noofthreads;i++) {
		fp[i]=malloc(sizeof(fileparam_t));
		fp[i]->threadnum=i;
		fp[i]->start_offset=ftell(filep);
		fseek(filep,sizetoseek,SEEK_CUR);
	
		do{
			ch=fgetc(filep);		
		}while(ch !='\n' && ftell(filep)< st.st_size);
		
		fp[i]->end_offset=ftell(filep);
			
		fp[i]->filename=haystack;
		fp[i]->search_string=needle;
		pthread_cond_init(&cv[i],NULL);
		pthread_mutex_init(&pm[i],NULL);
		//printf("threadnum %d,offset %d,size %d\n",fp[i]->threadnum,fp[i]->start_offset,fp[i]->end_offset);
		if(ftell(filep)>=st.st_size){
			fp[i]->end_offset=st.st_size;
			noofthreadsrunning=i+1;
			break;
		}
	}

	pthread_barrier_init(&barrier, NULL, noofthreadsrunning);

	for(i=0;i<noofthreadsrunning;i++){
		fp[i]->totalthreads=noofthreadsrunning;
		pthread_create(&pt[i],NULL,threadRead,fp[i]);
	
		
	}
	fclose(filep);
	for(i=0;i<noofthreadsrunning;i++){
		pthread_join(pt[i],NULL);
	}

	for(i=0;i<noofthreadsrunning;i++){
		free(fp[i]);
	}
	free(fp);
}
