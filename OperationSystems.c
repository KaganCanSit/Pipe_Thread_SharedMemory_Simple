#include <stdio.h>          //Temel C Komutlarini Kullanabilmek Icin Ekledigimiz Header Dosyasi
#include <stdlib.h>
#include <unistd.h>         //Fork ve pid_t Tanimlarini Kullanabilmek Icin Ekledigimiz Header Dosyasi
#include <string.h>         //String(Metin) Islemleri Icin Kullandigimiz Header Dosyasi
#include <sys/types.h>      //wait() Fonksiyonu
#include <sys/wait.h>
#include <pthread.h>        //Thread Tanimlayabilmek Ve Kullanabilmemiz Icin Header
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>


//Pipe icin global degiskenler
#define BUFFER_SIZE 400
#define READ_END 0
#define WRITE_END 1
int offset = 11;

//Thread Kullanimini Kolaylastirmak Icin Struct YApisi
typedef struct thread_data {
   char text[400];
   int choise;
}thread_data;

//Bolmek Icin Kullanacagimiz Dort Parca
thread_data part[4];

//Kullanicidan Sifrelenecek Olan Metinin Alinmasi
void getText(char temp[])
{
    printf("\n--------------Hi! Welcome To The Encryption Program--------------");
    printf("\n---Please Remember That You Can Enter a Maximum Of 400 Letters!(Englisch Alphabet Only Use)---");
    printf("\nPlease Enter The Text To Be Encrypted:");
    fgets(temp,400,stdin);

    if (strlen(temp) > 400)
    {
        printf("You Have Exceeded The Maximum Chracter(400) Limit! Try Again.");
    }
}

/*
//Kullanicidan Oteleme Degerini Aliyoruz.
void getOffset()
{
    int deger = 0;
    printf("Sifre Degeri Belirleyiniz: ");
    scanf("%d",&deger);
    offset %= 25;
}
*/


//Disaridan aldigi metin verisini, ikinci parametre olarak aldigimiz kaydirma sayisina  ve operasyon turune gore Sezar Sifrelemesi uygular veya cozer.
void* CrypteOperations(void *arg)
{ 
    thread_data *tdata=(thread_data *)arg;
 
    char text[400];
    strcpy(text,tdata->text);
    int operationChoise = tdata->choise;
    
    //Encrypte -> 1 | Decrypt ->0
    if (operationChoise == 0)
    {
        offset = -offset;
    }
    int i = 0;
    while (text[i] != '\0')
    {
        if (text[i] == ' ')
        {
            i++;
        }
        if (text[i] >= 65 && text[i] <= 90)
        {
            text[i] = ((text[i] + offset - 65) % 90) + 65;
        }
        else
        {
            text[i] = ((text[i] + offset - 97) % 122) + 97;
        }
        i++;
    }
    strcpy(tdata->text,text);
    pthread_exit(NULL);
}


void PipeWrite(int fd[],char * msg)
{
    close(fd[READ_END]);
    write(fd[WRITE_END], msg ,strlen(msg)+1);
    //printf("Parent Process Pipe'a veri yazdi.\n");
    close(fd[WRITE_END]);
    wait(NULL);
    //printf("Parent Process Sonlandi!\n")    ;
}

char* PipeRead(int fd[], char*msg)
{
    close(fd[WRITE_END]);
    read(fd[READ_END], msg ,BUFFER_SIZE);
    //printf("Child Process Pipe'dan okudu: %s\n",msg);
    close(fd[READ_END]);
    //printf("Child process sonlandi.\n");
    return msg;
}

void substring(char* sub_string,char text[],int len, int quarter)
{
    int start=((quarter-1)*(len/4));
    int end=start+len/4;

    int j=0, i=0;
    for(i=start; i<end; i++)
    {
        sub_string[j]=text[i];
        j++;
    }
    printf("sub %s\n",sub_string);
}

void sharedMemorySender(void* shared_memory, char* buff, int shmid)
{
    shmid = shmget((key_t)1122,1024,0666|IPC_CREAT);
    printf("KEy of shared memory is: %d\n",shmid);
    shared_memory = shmat(shmid,NULL,0);
    printf("Process attached at %p\n",shared_memory);
    strcpy(shared_memory,buff);
    printf("You wrote: %s\n",(char *)shared_memory);
}

void sharedMemoryReceiver(void *shared_memory,int shmid)
{
    shmid = shmget((key_t)1122,1024,0666);
    printf("KEy of shared memory is%d\n",shmid);
    shared_memory = shmat(shmid,NULL,0);
    printf("Process attached at:%p\n",shared_memory);
    printf("Data read from shared memory is: %s\n",(char *)shared_memory);
}

int main()
{
    char write_msg[BUFFER_SIZE], read_msg[BUFFER_SIZE], tempText[BUFFER_SIZE];
    int pass=0;

    //Pipe ve Thread islemi icin tanim.
    int fd[2];
    pid_t pid; 

    //Shared Memory
    void *shared_memory;
    char buff[BUFFER_SIZE];
    int shmid;


    //Pipe'in Kontrolu
    if(pipe(fd)==-1)
    {
        fprintf(stderr,"Pipe Failed!");
        return 1;
    }
    
    //Parent-Child iliskisi olusturmak icin process'i catalliyoruz(fork).
    pid = fork();
    
    //Fork'un Kontrolu
    if(pid<0)
    {
        fprintf(stderr,"Fork Failed!");
        return 1;
    }
    //Parent Process (pid=1)
    if(pid>0)
    {
        //Sifrelenecek olan metini ve kaydirma degerini kullanicidan aliyoruz.
        getText(write_msg);
        PipeWrite(fd,write_msg);
        sharedMemoryReceiver(shared_memory,shmid);
    }
    //Child Process (pid=0)
    else
    {
        //Thread Olusturuyoruz
        pthread_t tid[4];

        strcpy(tempText, PipeRead(fd,read_msg));

        for(int i=0;i<4;i++)
        {
            strcpy(part[i].text, tempText);
            printf("%s",part[i].text);
            part[i].choise = 1;
        }

        pthread_create(&(tid[0]), NULL, CrypteOperations, (void *)&part[0]);
        pthread_join(tid[0], NULL);
        printf("%s",part[0].text);

        strcpy(buff,part[0].text);
        sharedMemorySender(shared_memory, buff ,shmid);
    }
    return 0;
}