#include <stdio.h>
#include <string.h>
#include <string>
#include "encoding.h" //写操作
#include "decoding.h" //读操作

using namespace std;

void usage()
{
    printf("./evenodd write <file_name> <p>\n");
    printf("./evenodd read <file_name> <save_as>\n");
    printf("./evenodd repair <number_erasures> <idx0> ...\n");
}

void t1_write(mChunk **mBlock, int p){
    encoding(mBlock, p);
}

void t2_write(mChunk **mBlock, int round, int p, int Filecursor, long *remainSize, int dBlockSize, char* fn){
    t_loadmBlockFromfile(round, mBlock, Filecursor, *remainSize, dBlockSize, p, fn);
}
void t3_write(mChunk **mBlock, int round, int p, int Filecursor, long *remainSize, int dBlockSize, FILE** pd, char* fn){
    blockTofiles(mBlock, pd, p);
    for (int i = 0; i < p - 1; i++){
        memset(mBlock[i][p].data, 0, chunkSize);
        memset(mBlock[i][p + 1].data, 0, chunkSize);
    } 
    if(remainSize>0)
        t_loadmBlockFromfile(round, mBlock, Filecursor, *remainSize, dBlockSize, p, fn);
}
void t4_write(mChunk **mBlock, int p, FILE** pd){
    blockTofiles(mBlock, pd, p);
}

void round1_write(mChunk ***mBlock, int round, int p, int Filecursor, long *remainSize, int dBlockSize, char* fn){
    thread th[2];
    th[0]=thread(t1_write,mBlock[round%2],p);
    th[1]=thread(t2_write, mBlock[(round+1)%2], round, p, Filecursor, remainSize, dBlockSize, fn);
    th[0].join();
    th[1].join();
}

void round_other_write(mChunk ***mBlock, int round, int p, int Filecursor, long *remainSize, int dBlockSize, FILE** pd, char* fn){
    thread th[2];
    th[0]=thread(t1_write,mBlock[round%2],p);
    th[1]=thread(t3_write, mBlock[(round+1)%2], round, p, Filecursor, remainSize, dBlockSize, pd, fn);
    th[0].join();
    th[1].join();
}

void round_last_write(mChunk ***mBlock, int round, int p, FILE** pd){
    thread th[2];
    th[0]=thread(t1_write,mBlock[round%2],p);
    th[1]=thread(t4_write,mBlock[(round+1)%2], p, pd);
    th[0].join();
    th[1].join();
}

int main(int argc, char **argv)
{
    //long start_time = getTimeUsec();
    if (argc < 2)
    {
        usage();
        return -1;
    }
    char *op = argv[1];

    if (strcmp(op, "write") == 0)
    {
        char *FileName = argv[2]; //(char*)"file465";
        int p = atoi(argv[3]);
        int dBlockSize = p * (p - 1) * chunkSize;
        FILE *pd[p + 2] = {NULL};
        //FILE *Filecursor = NULL;
        int Filecursor;
        //Filecursor = fopen(FileName, "rb"); //打开输入文件
        Filecursor = open(FileName, O_RDONLY);
        if(Filecursor==-1){
            printf("文件打开失败\n");
            return 0;
        }

        long remainSize = getFileSize(FileName);

        //string s=FileName;
        char* fn=(char*)malloc(100*sizeof(char));
        sprintf(fn, "%s", FileName);
        //memcpy(fn, FileName, strlen(FileName));
        
        /* 
        可能是绝对路径，所以需要转化为文件名 /a/b/input -a-b-input
        */
        for(int i=0;i<strlen(FileName);i++){
            if(FileName[i]=='/')FileName[i]='-';
        }   

        //创建p+2个文件夹和对应文件,对应文件已打开
        createFolderFile(p, pd, FileName, remainSize);   
                        
        //创建block并开辟空间
        mChunk ***mBlock = (mChunk ***)malloc(2 * sizeof(mChunk **));
        mBlock[0] = (mChunk **)malloc((p - 1) * sizeof(mChunk *)); 
        mBlock[1] = (mChunk **)malloc((p - 1) * sizeof(mChunk *)); 
        for (int i = 0; i < p - 1; i++){
            mBlock[0][i] = (mChunk *)malloc((p + 2) * sizeof(mChunk));
            mBlock[1][i] = (mChunk *)malloc((p + 2) * sizeof(mChunk));
            for (int j = 0; j < p + 2; j++){
                mBlock[0][i][j] = mChunk();
                mBlock[0][i][j].data = (char *)calloc(chunkSize, sizeof(char));
                mBlock[1][i][j] = mChunk();
                mBlock[1][i][j].data = (char *)calloc(chunkSize, sizeof(char));
            }
        }

        //double t1=0,t2=0,t3=0;
        int round=0;
        //double time1=0,time2=0,time3=0;
        //double nowtime,nowtime1;
        t_loadmBlockFromfile(round, mBlock[1], Filecursor, remainSize, dBlockSize, p,fn);
        while (remainSize > 0)
        {                                                                      //每次编码一个大的block
        
            round++;
            if(round==1){
                round1_write(mBlock, round, p, Filecursor, &remainSize, dBlockSize, fn);
            }
            else{
                if(remainSize>0)
                    round_other_write(mBlock, round, p, Filecursor, &remainSize, dBlockSize, pd, fn);
            }
        /*
        //    nowtime=getTimeUsec();
            //loadmBlockFromfile(mBlock, Filecursor, remainSize, dBlockSize, p); //加载一个数据块
            t_loadmBlockFromfile(round, mBlock, Filecursor, remainSize, dBlockSize, p,fn); //加载一个数据块
        //    nowtime1=getTimeUsec();
        //    time1+=((double)(nowtime1-nowtime)/1000000);
            //printf("load time is: %f s\n", (double)(getTimeUsec() - start_time) / 1000000);
            //printf("@@@@@@@@\n");
            encoding(mBlock, p);
        //    nowtime=getTimeUsec();
        //    time2+=((double)(nowtime-nowtime1)/1000000);
            //printf("encode time is: %f s\n", (double)(getTimeUsec() - start_time) / 1000000);
            blockTofiles(mBlock, pd, p);
            //t_blockTofiles(mBlock, pd, p);
        //    nowtime1=getTimeUsec();
        //    time3+=((double)(nowtime1-nowtime)/1000000);
            //printf("write time is: %f s\n", (double)(getTimeUsec() - start_time) / 1000000);

            for (int i = 0; i < p - 1; i++){
                memset(mBlock[i][p].data, 0, chunkSize);
                memset(mBlock[i][p + 1].data, 0, chunkSize);
            }
            round++;
            //printf("%d\n",round);
        */
        }
        //printf("%lf %lf %lf\n",time1,time2,time3);
        round++;
        round_last_write(mBlock, round, p, pd);
        blockTofiles(mBlock[(round)%2], pd, p);

        cleanUpmemory1(mBlock, pd, p);
        //fclose(Filecursor);
        close(Filecursor);
        free(fn);
    }
    else if (strcmp(op, "read") == 0)
    {
        char *FileName = argv[2]; //所读文件

        char fn[100];
        strcpy(fn,FileName);
        for(int i=0;i<strlen(fn);i++){
            if(fn[i]=='/')fn[i]='-';
        }
        bool r=rsearchFile(fn);
        if(r==0){
            printf("File does not exist!\n");
            return 0;
        }

        /*
        可能是绝对路径，所以需要转化为文件名 /a/b/input -a-b-input
        */
        for(int i=0;i<strlen(FileName);i++){
            if(FileName[i]=='/')FileName[i]='-';
        }
        int p = getPbyFilename(FileName);
        // printf("p=%d\n",p);
        // printf("%s\n",FileName);
        int dBlockSize = p * (p - 1) * chunkSize;

        char *Save_as = argv[3];  //输出的文件
        int PerTimeReadSize = (p - 1) * chunkSize;
        FILE *pd[p + 2] = {NULL};
        long remainSize[p + 2];
        int times = 0;                                          //需要维修的盘的数量
        int id[2];                                              //需要维修的盘的id
        openFolderFile(p, pd, FileName, remainSize, times, id); //打开对应输入文件并统计每个文件大小
        //printf("%d %d\n",id[0],id[1]);
        // for(int i=0;i<p+2;i++)
        //     printf("%ld\n",remainSize[i]);
        //printf("times=%d\n",times);
        if (times > 2)
        {
            printf("File corrupted!\n");
        }
        else{
            //FILE *Filecursor = fopen(Save_as, "ab"); //打开输出文件
            int Filecursor=open(Save_as,O_WRONLY|O_APPEND|O_CREAT,0777);
            if(times==0 || ( id[0]==p && id[1]==p+1 ) ){
                mChunk **mBlock = (mChunk **)malloc((p - 1) * sizeof(mChunk *));
                for (int i = 0; i < p - 1; i++){
                    mBlock[i] = (mChunk *)malloc((p) * sizeof(mChunk));
                    for (int j = 0; j < p; j++){
                        mBlock[i][j] = mChunk();
                        mBlock[i][j].data = (char *)calloc(chunkSize, sizeof(char));
                    }
                }
                bool flag = 1;
                while (flag == 1){
                    loadmBlockFromfiles(mBlock, pd, remainSize, p, flag, Filecursor); //读操作：从多个disk文件中加载一个mblock
                }
                //cleanUpmemory(mBlock, pd, p);
                //fclose(Filecursor);
                close(Filecursor);
            }
            else{
                //------------------------------------------------------------
                //此处需要接入维修部分代码，times为需要维修盘的数量，id[2]为需要维修的盘
                //------------------------------------------------------------
                repair_read(times, id, Filecursor, FileName);
                //openFolderFileAfterRepair(p, pd, FileName, remainSize, times, id);
            }
        }
    }
    else if (strcmp(op, "repair") == 0)
    {
        int num_era = atoi(argv[2]);
        if (num_era > 2)
        {
            printf("Too many corruptions!\n");
            exit(0);
        }
        int errDisk[2] = {-1, -1};
        if (num_era == 1)
        {
            errDisk[0] = atoi(argv[3]);
            errDisk[1] = -1;
        }
        else if (num_era == 2)
        {
            errDisk[0] = atoi(argv[3]);
            errDisk[1] = atoi(argv[4]);
            if (errDisk[0] > errDisk[1])
            {
                int temp = errDisk[0];
                errDisk[0] = errDisk[1];
                errDisk[1] = temp;
            }
        }
        repair(num_era,errDisk);
    }
    else
    {
        printf("Non-supported operations!\n");
    }
    //printf("time is: %f s\n", (double)(getTimeUsec() - start_time) / 1000000);
    return 0;
}