#include <stdio.h>          //Temel C Komutlarini Kullanabilmek Icin Ekledigimiz Header Dosyasi
#include <stdlib.h>         //exit Fonksiyonu Icin Header Dosyasi
#include <unistd.h>         //Fork ve pid_t Tanimlarini Kullanabilmek Icin Ekledigimiz Header Dosyasi
#include <string.h>         //String(Metin) Islemleri Icin Kullandigimiz Header Dosyasi
#include <sys/wait.h>       //wait() Fonksiyonu
#include <pthread.h>        //Thread Tanimlayabilmek Ve Kullanabilmemiz Icin Header
#include <sys/shm.h>        //Shared Memory Kullanimi Icin

//Pipe icin global degiskenler
#define BUFFER_SIZE 400
#define READ_END 0
#define WRITE_END 1
int offset = 11;

//Thread Kullanimini Kolaylastirmak Icin Struct YApisi
typedef struct thread_data {
    int arraySize;
    int indis;
    int choise;
    char* text;
}thread_data;

//Bolmek Icin Kullanacagimiz Dort Parca
thread_data part[4];

//Dinamik dizi icin bellekten alan tahsisi.
void createArray(thread_data* d, int sizeVal, int choise)
{
	d->text = (char*)malloc(sizeVal * sizeof(char));
	d->arraySize = sizeVal;
	d->indis = 0;
    d->choise = choise;
}

//Dinamik dizi degerinin ilk tanimdan buyuk olmasi durumunda genisletme ve bellekten ek alan alma.
void expandArray(thread_data* d)
{
	if (d->indis == d->arraySize) //Yine de maksimum indis degeri ile tanimli dizi genisligini karsilastiyoruz.
	{
		char* cntrl;
		d->arraySize++;

		//Bellekten alan alinamamasi durumunda uyari almasi icin duzenliyoruz.
		cntrl = (char*)realloc(d->text, sizeof(char) * d->arraySize);
		if (cntrl == NULL)
		{
			printf("Memory Fault!");
			exit(1);
		}
		d->text = cntrl;
	}
}

//Dinamik diziye bellek ekleme ve indis degerinin arttirilamasi.
void addArray(thread_data* d, char v)
{
	expandArray(d);
	d->text[d->indis++] = v;
    d->choise = 1;
}

//Dinamik dizi alanini tamimiyla silinerek bellege iadesi.
void freeArray(thread_data* d)
{
	free(d->text);
	d->arraySize = 0;
	d->indis = 0;
    d->choise = 0;
}

//Kullanicidan Sifrelenecek Olan Metinin Alinmasi
void getText(char temp[])
{
    printf("\n--------------Hi! Welcome To The Encryption Program--------------");
    printf("\n---Please Remember That You Can Enter a Maximum Of 400 Letters!(Englisch Alphabet Only Use)---");
    printf("\nPlease Enter The Text To Be Encrypted:");
    fgets(temp,400,stdin);

    if (strlen(temp) > 400 || strlen(temp)<5)
    {
        printf("You Have Exceeded The Maximum Chracter(400) Limit! Try Again.");
        exit(0);
    }
}

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
}

//Pipe Yazma Ve Okuma Islemleri
void PipeWrite(int fd[],char * msg)
{
    close(fd[READ_END]);
    write(fd[WRITE_END], msg ,strlen(msg)+1);
    close(fd[WRITE_END]);
    printf("\nData Send-Write Pipe: %s",msg);
    wait(NULL);
}
char* PipeRead(int fd[], char*msg)
{
    close(fd[WRITE_END]);
    read(fd[READ_END], msg ,BUFFER_SIZE);
    close(fd[READ_END]);
    printf("\nData Read Pipe-Close Pipe: %s",msg);
    return msg;
}

//Metini dort parcaya threadler icin ayiriyoruz.
void textPartition(char* tempText)
{
    int len = strlen(tempText);
        for(int i=0;i<4;i++)
        {
            createArray(&part[i],len,1);
        }
        
        int a=0, counter=0;
        while(a<len)
        {
            if(a<len/4)
                addArray(&part[0],tempText[a]);
            else if(a>=len/4 && a<(len/4)*2)
                addArray(&part[1],tempText[a]);
            else if(a>=(len/4)*2 && a<(len/4)*3)
                addArray(&part[2],tempText[a]);
            else
                addArray(&part[3],tempText[a]);
            a++;
        }

        printf("\n\n%s\n",part[0].text);
        printf("%s\n",part[1].text);
        printf("%s\n",part[2].text);
        printf("%s",part[3].text);
}

//Shared Memory Yazma Ve Okuma
void sharedMemorySender(void* shared_memory, char* buff, int shmid)
{
    shmid = shmget((key_t)1122,1024,0666|IPC_CREAT);
    shared_memory = shmat(shmid,NULL,0);
    strcpy(shared_memory,buff);
    printf("\nData Write Shared Memory Area: %s",(char *)shared_memory);
}
char* sharedMemoryReceiver(void *shared_memory,int shmid)
{
    shmid = shmget((key_t)1122,1024,0666);
    shared_memory = shmat(shmid,NULL,0);
    //printf("\nRead Data Shared Memory Area: %s", (char *)shared_memory);
    return (char *)shared_memory;
}

int main()
{
    char write_msg[BUFFER_SIZE], read_msg[BUFFER_SIZE], tempText[BUFFER_SIZE];
    int pass=0;

    //Pipe ve Thread Islemi Icin Tanimlar
    int fd[2];
    pid_t pid; 

    //Shared Memory Kullanimi Icin Gerekli Tanimlar
    void *shared_memory;
    char buff[BUFFER_SIZE];
    int shmid=0;

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

        //Shared Memory'den Alip Sifrelemeyi Cozuyoruz.
        part[0].text = sharedMemoryReceiver(shared_memory,shmid);
        part[0].choise = 0;
        printf("\n\nCrypted Text: %s",part[0].text);

        CrypteOperations(&part[0]);
        printf("\nDecrypted Text: %s",part[0].text);
        
        pthread_exit(NULL);
    }
    //Child Process (pid=0)
    else
    {
        //Thread Olusturuyoruz
        pthread_t tid[4];

        //Pipe uzerinden veriyi okuduk ve tempText icerisine attik.
        strcpy(tempText, PipeRead(fd,read_msg));
        textPartition(tempText);

        for(int i=0;i<4;i++)
        {
            pthread_create(&(tid[i]), NULL, CrypteOperations, (void *)&part[i]);
        }
        for(int a=0;a<4;a++)
        {
            pthread_join(tid[a], NULL);
        }
        for(int i=1;i<4;i++)
        {
            for(int a=0;a<part[i].indis;a++)
            {
                addArray(&part[0],part[i].text[a]);
            }
        }
        
        sharedMemorySender(shared_memory, part[0].text ,shmid);
        pthread_exit(NULL);
    }
    return 0;
}