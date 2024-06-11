/*
数据结构头文件，定义了chuck结构体，
以及文件夹操作、chunk加载与写disk等函数
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <sys/stat.h>
#include <algorithm>
#include <dirent.h>
#include <vector>
#include <time.h>
#include <sys/time.h>
#include <thread>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <aio.h>
#include <errno.h>

// #define chunkSize 4194304      //4MB
// #define chunkSize 1048576      //1MB
// #define chunkSize 262144       //256KB
// #define chunkSize 131072       //128KB
#define chunkSize 65536 // 64KB
// #define chunkSize 10
#define FolderName "disk" // 文件夹名
using namespace std;

/*
chunk结构体，异或操作以chunk为单位进行；
*/
struct mChunk
{
    int size = chunkSize; // 默认值
    char *data = NULL;
    int offset = -1;
    // 构造函数
    mChunk() {}
    mChunk(int _size, char *_data) : size(_size), data(_data) {}
    // 析构函数
    ~mChunk() { free(data); } // 析构函数
    // 重载异或操作
    mChunk operator^(const mChunk &c) const
    {
        int rsize = std::min(size, c.size);
        char *result = (char *)malloc(rsize * sizeof(char));
        for (int i = 0; i < rsize; i++)
        {
            result[i] = data[i] ^ c.data[i];
        }
        return mChunk(rsize, result);
    }
    void operator^=(const mChunk &c)
    {
        int rsize = std::min(size, c.size);
        for (int i = 0; i < rsize; i++)
        {
            data[i] ^= c.data[i];
        }
    }
};

void printBuf(char *buf, int len)
{
    for (int i = 0; i < len; i++)
        printf("%d ", buf[i]);
    printf("\n");
}

void printBlock(mChunk **mBlock, int p)
{
    for (int i = 0; i < p - 1; i++)
    {
        for (int j = 0; j < p + 2; j++)
        {
            printBuf(mBlock[i][j].data, mBlock[i][j].offset);
        }
    }
}

long getFileSize(const char *fileName)
{ // 文件总字节大小
    if (fileName == NULL)
        return 0;
    struct stat statbuf;
    stat(fileName, &statbuf);
    size_t filesize = statbuf.st_size;
    return (long)filesize;
}

char *getPathname(int ith, int isfile, const char *fileName, long remainSize, int p = 0)
{                                                    // isfile：1返回文件夹 0返回文件路径
    char *path = (char *)malloc(150 * sizeof(char)); // 文件路径
    if (isfile == 1)
        sprintf(path, "%s_%d/%s_%d_%ld", FolderName, ith, fileName, p, remainSize); // 文件名称
    else if (isfile == 0)
        sprintf(path, "%s_%d", FolderName, ith); // 文件夹名称
    return path;
}

/*
前缀比较，用于模糊搜索打开文件
*/
bool preCmp(char *shortname, char *longname)
{ // shortname是否是longname前缀
    if (strlen(shortname) > strlen(longname))
        return 0;
    for (int i = 0; i < strlen(shortname); i++)
        if (shortname[i] != longname[i])
            return 0;
    if (longname[strlen(shortname)] == '_')
        return 1; // 检查前缀相同
    else
        return 0;
}

/*
用于模糊搜索打开文件
*/
char *getFilenameByPre(char *folder, char *filename)
{
    char *path = (char *)malloc(150 * sizeof(char));
    DIR *d = opendir(folder);
    if (d == NULL)
        return NULL;
    struct dirent *entry;
    while ((entry = readdir(d)) != NULL)
    {
        char *tmpname = entry->d_name;
        // printf("%s\n",tmpname);
        if (preCmp(filename, tmpname))
        {
            sprintf(path, "%s/%s", folder, tmpname);
            return path;
        }
    }
    closedir(d);
    return NULL;
}

/*
GetAllFile
*/
void getAllfile(char *folder, std::vector<string> &namelist)
{
    DIR *d = opendir(folder);
    if (d == NULL)
        return;
    struct dirent *entry;
    while ((entry = readdir(d)) != NULL)
    {
        string tmpname = entry->d_name;
        if (tmpname == "." || tmpname == "..")
            continue;
        namelist.push_back(tmpname);
    }
    closedir(d);
}

void getPaSize(string name, int &p, long &size, string &fn)
{
    int first_pos = name.find_first_of("_");
    int last_pos = name.find_last_of("_");
    // printf("%d %d\n",first_pos,last_pos);
    string ff = name.substr(0, first_pos);
    string pp = name.substr(first_pos + 1, last_pos - first_pos - 1);
    string ss = name.substr(last_pos + 1, name.size() - last_pos);
    // printf("%s %s %s\n",ff.data(),pp.data(),ss.data());
    p = atoi(pp.data());
    size = atol(ss.data());
    // printf("%s\n",ff.data());
    fn = ff;
}

/*
写操作：创建文件夹，并打开句柄
*/
void createFolderFile(int p, FILE **pd, char *fileName, long remainSize)
{
    char *pathname, *filename;
    for (int i = 0; i < p + 2; i++)
    { // 创建p+2个文件夹
        pathname = getPathname(i, 0, fileName, remainSize);
        filename = getPathname(i, 1, fileName, remainSize, p);
        mkdir(pathname, 0755);         // 创建文件夹
        pd[i] = fopen(filename, "ab"); // 以追加方式打开/创建文件,不关闭
    }
    free(pathname);
    free(filename);
}

void openFolderFile(int p, FILE **pd, const char *fileName, int *idx, long remainSize)
{ // 打开文件
    char *pathname, *filename;
    for (int i = 0; i < p + 2; i++)
    { // 创建p+2个文件夹
        pathname = getPathname(i, 0, fileName, remainSize);
        filename = getPathname(i, 1, fileName, remainSize, p);
        mkdir(pathname, 0755); // 创建文件夹
        // printf("filename=%s\n",filename);
        if (i != idx[0] && i != idx[1])
            pd[i] = fopen(filename, "rb"); // 以追加方式打开/创建文件,不关闭
        else
            pd[i] = fopen(filename, "ab");
    }
    free(pathname);
    free(filename);
}
void openFolderFileWhileReadRepair(int p, FILE **pd, const char *fileName, int *idx, long remainSize)
{ // 打开文件
    char *pathname, *filename;
    for (int i = 0; i < p + 2; i++)
    { // 创建p+2个文件夹
        pathname = getPathname(i, 0, fileName, remainSize);
        filename = getPathname(i, 1, fileName, remainSize, p);
        // mkdir(pathname, 0755); //创建文件夹
        //  printf("filename=%s\n",filename);
        if (i != idx[0] && i != idx[1])
        {
            pd[i] = fopen(filename, "rb"); // 以追加方式打开/创建文件,不关闭
        }
        // else
        //     pd[i] = fopen(filename, "ab");
    }
    free(pathname);
    free(filename);
}

/*
打开文件，文件指针保存在pd中，同时记录丢失的文件数量
2022.11.25修改，用于测试超过两个文件夹损坏情况
如果损坏的是最后两个校验文件夹，是否可以不用修复直接读出？
*/

void openFolderFile(int p, FILE **pd, char *fileName, long *remainSize, int &times, int *id)
{ // 打开文件
    char *fullfilename;
    id[0] = id[1] = -1;
    int search_flag;
    char *folder;
    for (int i = 0; i < p; i++)
    {
        folder = getPathname(i, 0, fileName, 0);           // 文件夹名称
        fullfilename = getFilenameByPre(folder, fileName); // 模糊搜索
        // printf("%s\n",fullfilename);
        if ((pd[i] = fopen(fullfilename, "rb")) == NULL)
        {
            times++;
            if (times > 2)
            {
                search_flag = i;
                break;
            }
            id[times - 1] = i;
        }
        remainSize[i] = getFileSize(fullfilename);
    }
    if (times)
    {
        for (int i = p; i < p + 2; i++)
        {
            folder = getPathname(i, 0, fileName, 0);           // 文件夹名称
            fullfilename = getFilenameByPre(folder, fileName); // 模糊搜索
            // printf("%s\n",fullfilename);
            if ((pd[i] = fopen(fullfilename, "rb")) == NULL)
            {
                times++;
                if (times > 2)
                {
                    search_flag = p - 1;
                    break;
                }
                id[times - 1] = i;
            }
            else
            {
                fclose(pd[i]);
            }
            remainSize[i] = getFileSize(fullfilename);
        }
    }
    if (times > 2)
    {
        for (int i = 0; i <= search_flag; i++)
        {
            if (pd[i] != NULL)
                fclose(pd[i]);
        }
    }
    free(fullfilename);
    free(folder);
}

/*
读操作：从Filecursor中加载数据到mBlock中，remainSize为文件剩余的数据量
*/
void loadmBlockFromfile(mChunk **mBlock, FILE *Filecursor, long &remainSize, int dBlockSize, int p)
{
    // printf("%ld %d\n",remainSize,dBlockSize);
    if (remainSize >= dBlockSize)
    { // 剩余文件可以读满一个block//多线程优化
        for (int i = 0; i < p - 1; i++)
        {
            for (int j = 0; j < p; j++)
            {
                mBlock[i][j].offset = chunkSize;
                fread(mBlock[i][j].data, sizeof(char), chunkSize, Filecursor);
            }
        }
        remainSize -= dBlockSize;
    }
    else if (remainSize < dBlockSize)
    { // 剩余文件读不满一个block
        bool over = 0;
        for (int i = 0; i < p - 1; i++)
        {
            for (int j = 0; j < p; j++)
            {
                if (remainSize >= chunkSize)
                { // 可以读满一个chunk
                    fread(mBlock[i][j].data, sizeof(char), chunkSize, Filecursor);
                    mBlock[i][j].offset = chunkSize;
                    remainSize -= chunkSize;
                }
                else if (remainSize > 0)
                {
                    fread(mBlock[i][j].data, sizeof(char), remainSize, Filecursor);
                    mBlock[i][j].offset = remainSize; // 记录偏移量
                    remainSize = 0;
                }
                else if (remainSize == 0)
                {
                    mBlock[i][j].offset = -1;
                    memset(mBlock[i][j].data, 0, chunkSize);
                }
            }
        }
    }
}

/*
读操作：从Filecursor中加载数据到mBlock中，remainSize为文件剩余的数据量
2022.11.30多线程版本
*/
void thread_load(mChunk **mBlock, char *filename, int i, int p, int round)
{
    // FILE *tt = fopen(filename,"rb");
    // fseek(tt,i*p*chunkSize + round*(p-1)*p*chunkSize ,0);
    int tt = open(filename, O_RDONLY);
    lseek(tt, i * p * chunkSize + round * (p - 1) * p * chunkSize, 0);
    struct aiocb *listio[p];
    struct aiocb rd[p];
    int count = 0;
    for (int j = 0; j < p; j++)
    {
        mBlock[i][j].offset = chunkSize;
        // fread(mBlock[i][j].data, sizeof(char), chunkSize, tt);

        bzero(&rd[count], sizeof(rd[count]));
        rd[count].aio_buf = mBlock[i][j].data;
        rd[count].aio_fildes = tt;
        rd[count].aio_nbytes = chunkSize;
        rd[count].aio_offset = lseek(tt, 0, 1);
        rd[count].aio_lio_opcode = LIO_READ;
        listio[count] = &rd[count];
        count++;
        lseek(tt, chunkSize, 1);
    }
    lio_listio(LIO_WAIT, listio, count, NULL);
}

void t_loadmBlockFromfile(int round, mChunk **mBlock, int Filecursor, long &remainSize, int dBlockSize, int p, char *filename)
{
    // printf("%s\n",filename);
    thread t[p - 1]; // 开p-1个线程
    if (remainSize >= dBlockSize)
    { // 剩余文件可以读满一个block//多线程优化
        for (int i = 0; i < p - 1; i++)
            t[i] = thread(thread_load, mBlock, filename, i, p, round);
        for (int i = 0; i < p - 1; i++)
            t[i].join();
        remainSize -= dBlockSize;
    }
    else if (remainSize < dBlockSize)
    { // 剩余文件读不满一个block
        // fseek(Filecursor,round*dBlockSize,0);
        lseek(Filecursor, round * dBlockSize, 0);
        // bool over = 0;
        struct aiocb *listio[p * (p - 1)];
        struct aiocb rd[p * (p - 1)];
        int count = 0;
        for (int i = 0; i < p - 1; i++)
        {
            for (int j = 0; j < p; j++)
            {
                if (remainSize >= chunkSize)
                { // 可以读满一个chunk
                    // fread(mBlock[i][j].data, sizeof(char), chunkSize, Filecursor);
                    bzero(&rd[count], sizeof(rd[count]));
                    rd[count].aio_buf = mBlock[i][j].data;
                    rd[count].aio_fildes = Filecursor;
                    rd[count].aio_nbytes = chunkSize;
                    rd[count].aio_offset = lseek(Filecursor, 0, SEEK_CUR);
                    rd[count].aio_lio_opcode = LIO_READ;
                    listio[count] = &rd[count];
                    lseek(Filecursor, chunkSize, SEEK_CUR);
                    count++;

                    mBlock[i][j].offset = chunkSize;
                    remainSize -= chunkSize;
                }
                else if (remainSize > 0)
                {
                    // fread(mBlock[i][j].data, sizeof(char), remainSize, Filecursor);
                    bzero(&rd[count], sizeof(rd[count]));
                    rd[count].aio_buf = mBlock[i][j].data;
                    rd[count].aio_fildes = Filecursor;
                    rd[count].aio_nbytes = remainSize;
                    rd[count].aio_offset = lseek(Filecursor, 0, SEEK_CUR);
                    rd[count].aio_lio_opcode = LIO_READ;
                    listio[count] = &rd[count];
                    lseek(Filecursor, remainSize, SEEK_CUR);
                    count++;

                    mBlock[i][j].offset = remainSize; // 记录偏移量
                    remainSize = 0;
                }
                else if (remainSize == 0)
                {
                    mBlock[i][j].offset = -1;
                    memset(mBlock[i][j].data, 0, chunkSize);
                }
            }
        }
        lio_listio(LIO_WAIT, listio, count, NULL);
    }
}

/*
写操作：将mBlock中的数据写入多个disk对应文件中
*/
void blockTofiles(mChunk **mBlock, FILE **pd, int p)
{
    bool flag = 0;
    for (int i = 0; i < p - 1; i++)
    { // 写数据文件
        for (int j = 0; j < p; j++)
        {
            if (mBlock[i][j].offset == chunkSize)
                fwrite(mBlock[i][j].data, sizeof(char), chunkSize, pd[j]);
            else if (mBlock[i][j].offset > 0)
                fwrite(mBlock[i][j].data, sizeof(char), mBlock[i][j].offset, pd[j]);
            else
            {
                flag = 1;
                break;
            }
        }
        if (flag)
            break;
    }
    for (int i = 0; i < p - 1; i++)
    { // 写校验文件
        fwrite(mBlock[i][p].data, sizeof(char), chunkSize, pd[p]);
        fwrite(mBlock[i][p + 1].data, sizeof(char), chunkSize, pd[p + 1]);
    }
}

/*
写操作：将mBlock中的数据写入多个disk对应文件中
*/
void thread_write(mChunk **mBlock, int id, FILE **pd, int p)
{
    if (id >= p)
    { // p p+1
        for (int i = 0; i < p - 1; i++)
        { // 写校验文件
            fwrite(mBlock[i][id].data, sizeof(char), chunkSize, pd[id]);
        }
    }
    else
    {
        for (int i = 0; i < p - 1; i++)
        { // 写数据文件
            if (mBlock[i][id].offset == chunkSize)
                fwrite(mBlock[i][id].data, sizeof(char), chunkSize, pd[id]);
            else if (mBlock[i][id].offset > 0)
                fwrite(mBlock[i][id].data, sizeof(char), mBlock[i][id].offset, pd[id]);
            else
                break;
        }
    }
}

void t_blockTofiles(mChunk **mBlock, FILE **pd, int p)
{ // 多线程优化
    bool flag = 0;
    thread th[p + 2];
    for (int id = 0; id < p + 2; id++)
        th[id] = thread(thread_write, mBlock, id, pd, p);
    for (int id = 0; id < p + 2; id++)
        th[id].join();
}

/*
慧轩 repair
*/
void blockTofiles(mChunk **mBlock, FILE **pd, int p, int *idx)
{
    // printf("blockTofile\n");
    bool flag = 0;
    for (int i = 0; i < p - 1; i++)
    { // 写数据文件
        if (mBlock[i][idx[0]].offset != chunkSize)
        {
            // printf("not a full:i=%d j=%d offset=%d\n",i,idx[0],mBlock[i][idx[0]].offset);
            fwrite(mBlock[i][idx[0]].data, sizeof(char), mBlock[i][idx[0]].offset, pd[idx[0]]);
            break;
        }
        // printf("write a full:i=%d j=%d offset=%d\n",i,idx[0],chunkSize);
        fwrite(mBlock[i][idx[0]].data, sizeof(char), chunkSize, pd[idx[0]]);
    }
    if (idx[1] != -1)
        for (int i = 0; i < p - 1; i++)
        { // 写数据文件
            if (mBlock[i][idx[1]].offset != chunkSize)
            {
                fwrite(mBlock[i][idx[1]].data, sizeof(char), mBlock[i][idx[1]].offset, pd[idx[1]]);
                break;
            }
            fwrite(mBlock[i][idx[1]].data, sizeof(char), chunkSize, pd[idx[1]]);
        }
}

void blockTofilesAfterRepair(mChunk **mBlock, int f, int p)
{
    bool flag = 0;
    struct aiocb *listio[p * (p - 1)];
    struct aiocb wr[p * (p - 1)];
    int count = 0;
    for (int i = 0; i < p - 1; i++)
    { // 写数据文件
        for (int j = 0; j < p; j++)
        {
            if (mBlock[i][j].offset == chunkSize)
            {
                // fwrite(mBlock[i][j].data, sizeof(char), chunkSize, f);
                bzero(&wr[count], sizeof(wr[count]));
                wr[count].aio_buf = mBlock[i][j].data;
                wr[count].aio_fildes = f;
                wr[count].aio_nbytes = chunkSize;
                wr[count].aio_lio_opcode = LIO_WRITE;
                listio[count] = &wr[count];
                count++;
            }
            else if (mBlock[i][j].offset > 0)
            {
                //fwrite(mBlock[i][j].data, sizeof(char), mBlock[i][j].offset, f);
                bzero(&wr[count], sizeof(wr[count]));
                wr[count].aio_buf = mBlock[i][j].data;
                wr[count].aio_fildes = f;
                wr[count].aio_nbytes = mBlock[i][j].offset;
                wr[count].aio_lio_opcode = LIO_WRITE;
                listio[count] = &wr[count];
                count++;
            }
            else
            {
                flag = 1;
                break;
            }
        }
        if (flag)
            break;
    }
    lio_listio(LIO_WAIT,listio,count,NULL);
}

/*
2022.11.9
读操作：从多个disk文件中加载一个mblock
*/
void loadmBlockFromfiles(mChunk **mBlock, FILE **pd, long *remainSize, int p, bool &flag, int Filecursor)
{

    bool over = 0;
    struct aiocb *listio[p * (p - 1)];
    struct aiocb wr[p * (p - 1)];
    int count = 0;
    for (int i = 0; i < p - 1; i++)
    {
        for (int j = 0; j < p; j++)
        {
            // printf("i: %d j: %d\n",i,j);
            // printf("%ld %d\n",remainSize[j],chunkSize);
            if (remainSize[j] >= chunkSize)
            {
                // printf("start read\n");
                fread(mBlock[i][j].data, sizeof(char), chunkSize, pd[j]);
                // printf("end read\n");
                // printf("start write\n");
                //  fwrite(mBlock[i][j].data, sizeof(char), chunkSize, Filecursor);
                bzero(&wr[count], sizeof(wr[count]));
                wr[count].aio_buf = mBlock[i][j].data;
                wr[count].aio_fildes = Filecursor;
                wr[count].aio_nbytes = chunkSize;
                wr[count].aio_lio_opcode = LIO_WRITE;
                listio[count] = &wr[count];
                count++;

                // printf("end write\n");
                remainSize[j] -= chunkSize;
            }
            else if (remainSize[j] > 0)
            { // 读文件应该结束
                fread(mBlock[i][j].data, sizeof(char), remainSize[j], pd[j]);

                bzero(&wr[count], sizeof(wr[count]));
                wr[count].aio_buf = mBlock[i][j].data;
                wr[count].aio_fildes = Filecursor;
                wr[count].aio_nbytes = remainSize[j];
                wr[count].aio_lio_opcode = LIO_WRITE;
                listio[count] = &wr[count];
                count++;
                // fwrite(mBlock[i][j].data, sizeof(char), remainSize[j], Filecursor);
                remainSize[j] = 0;
            }
            else if (remainSize[j] == 0)
            {
                // printf("end!");
                flag = 0;
                over = 1;
                break;
            }
        }
        if (over)
            break;
    }
    lio_listio(LIO_WAIT, listio, count, NULL);
}
void loadmBlockTofilesAfterRepair(mChunk **mBlock, FILE **pd, long *remainSize, int p, bool &flag, FILE *Filecursor)
{

    bool over = 0;
    for (int i = 0; i < p - 1; i++)
    {
        for (int j = 0; j < p; j++)
        {
            if (remainSize[j] >= chunkSize)
            {
                // fread(mBlock[i][j].data, sizeof(char), chunkSize, pd[j]);
                fwrite(mBlock[i][j].data, sizeof(char), chunkSize, Filecursor);
                remainSize[j] -= chunkSize;
            }
            else if (remainSize[j] > 0)
            { // 读文件应该结束
                // fread(mBlock[i][j].data, sizeof(char), remainSize[j], pd[j]);
                fwrite(mBlock[i][j].data, sizeof(char), remainSize[j], Filecursor);
                remainSize[j] = 0;
            }
            else if (remainSize[j] == 0)
            {
                flag = 0;
                over = 1;
                break;
            }
        }
        if (over)
            break;
    }
}

void fileToblock(mChunk **mBlock, FILE **pd, int p, int *idx, long &remainSize, int dBlockSize)
{
    // int dif=0;
    if (remainSize >= dBlockSize)
    { // 剩余文件可以读满一个block
        // printf("1满block\n");
        for (int i = 0; i < p - 1; i++)
        {
            for (int j = 0; j < p + 2; j++)
            {
                mBlock[i][j].offset = chunkSize;
                if (j != idx[0] && j != idx[1])
                {
                    // printf("%d ",j);
                    fread(mBlock[i][j].data, sizeof(char), chunkSize, pd[j]);
                    // if(j==dif)
                    // fwrite(mBlock[i][j].data,sizeof(char),chunkSize,abf);
                }
                else
                    memset(mBlock[i][j].data, 0, chunkSize);
            }
            // printf("\n");
        }
        remainSize -= dBlockSize;
        // printf("满block\n");
    }
    else if (remainSize < dBlockSize)
    { // 剩余文件读不满一个block
        bool over = 0;
        for (int i = 0; i < p - 1; i++)
        {
            for (int j = 0; j < p; j++)
            {
                // printf("i=%d j=%d remainSize=%ld\n",i,j,remainSize);
                if (remainSize >= chunkSize)
                { // 可以读满一个chunk
                    if (j != idx[0] && j != idx[1])
                    {
                        fread(mBlock[i][j].data, sizeof(char), chunkSize, pd[j]);
                        // if(j==dif)
                        // fwrite(mBlock[i][j].data,sizeof(char),chunkSize,abf);
                    }
                    else
                        memset(mBlock[i][j].data, 0, chunkSize);
                    mBlock[i][j].offset = chunkSize;
                    remainSize -= chunkSize;
                }
                else if (remainSize > 0)
                {
                    // printf("i=%d j=%d\n",i,j);
                    if (j != idx[0] && j != idx[1])
                    {
                        memset(mBlock[i][j].data, 0, chunkSize);
                        fread(mBlock[i][j].data, sizeof(char), remainSize, pd[j]);
                        // if(j==dif)
                        // fwrite(mBlock[i][j].data,sizeof(char),remainSize,abf);
                    }
                    else
                        memset(mBlock[i][j].data, 0, chunkSize);
                    mBlock[i][j].offset = remainSize; // 记录偏移量
                    printf("%d %d %ld\n", i, j, remainSize);
                    for (int k = 0; k < 10; k++)
                        printf("%d ", mBlock[i][j].data[k]);
                    printf("\n\n");
                    remainSize = 0;
                }
                else if (remainSize == 0)
                {
                    mBlock[i][j].offset = 0;
                    memset(mBlock[i][j].data, 0, chunkSize);
                }
            }
        }
        for (int i = 0; i < p - 1; i++)
        {
            for (int j = p; j < p + 2; j++)
            {
                mBlock[i][j].offset = chunkSize;
                if (j != idx[0] && j != idx[1])
                {
                    // printf("%d ",j);
                    fread(mBlock[i][j].data, sizeof(char), chunkSize, pd[j]);
                    // if(j==dif)
                    // fwrite(mBlock[i][j].data,sizeof(char),chunkSize,abf);
                }
                else
                    memset(mBlock[i][j].data, 0, chunkSize);
            }
        }
    }
    // printf("fileToblock\n");
}
/*
垃圾回收
*/
void cleanUpmemory(mChunk **mBlock, FILE **pd, int p)
{
    for (int j = 0; j < p; j++)
        fclose(pd[j]);
    for (int i = 0; i < p - 1; i++)
    {
        for (int j = 0; j < p; j++)
            free(mBlock[i][j].data);
        free(mBlock[i]);
    }
    free(mBlock);
}
void cleanUpmemory1(mChunk ***mBlock, FILE **pd, int p)
{
    for (int j = 0; j < p; j++)
        fclose(pd[j]);
    for (int i = 0; i < p - 1; i++)
    {
        for (int j = 0; j < p; j++)
        {
            free(mBlock[0][i][j].data);
            free(mBlock[1][i][j].data);
        }
        free(mBlock[0][i]);
        free(mBlock[1][i]);
    }
    free(mBlock[0]);
    free(mBlock[1]);
    free(mBlock);
}

void cleanUpmemory1(mChunk ***mBlock, FILE **pd, int p, int *idx, int num)
{
    for (int j = 0; j < p; j++)
        if (j != idx[0] && j != idx[1])
            fclose(pd[j]);
    for (int i = 0; i < p - 1; i++)
    {
        for (int j = 0; j < p; j++)
        {
            free(mBlock[0][i][j].data);
            free(mBlock[1][i][j].data);
        }
        free(mBlock[0][i]);
        free(mBlock[1][i]);
    }
    free(mBlock[0]);
    free(mBlock[1]);
    free(mBlock);
}

void cleanUpmemory(mChunk **mBlock, FILE **pd, int p, int *idx, int num)
{
    for (int j = 0; j < p; j++)
        if (j != idx[0] && j != idx[1])
            fclose(pd[j]);
    for (int i = 0; i < p - 1; i++)
    {
        for (int j = 0; j < p; j++)
            free(mBlock[i][j].data);
        free(mBlock[i]);
    }
    free(mBlock);
}

void cleanUpmemoryWhileTooManyError(FILE **pd, int p)
{
    for (int j = 0; j < p + 2; j++)
        fclose(pd[j]);
}

long getTimeUsec()
{
    struct timeval t;
    gettimeofday(&t, 0);
    return (long)((long)t.tv_sec * 1000 * 1000 + t.tv_usec);
}

/*
read操作 先取出对应文件的p
*/
int getPbyFilename(char *filename)
{
    // 找到第一个可以打开的文件夹
    char *folder;
    DIR *d;
    int folder_index = 0;
    char *path = (char *)malloc(10 * sizeof(char));
    int max_p = 100;
    while (1)
    {
        if (max_p == 0)
            break;
        sprintf(path, "disk_%d", folder_index);
        d = opendir(path);
        if (d != NULL)
        {
            folder = path;
            break;
        }
        folder_index++;
        max_p--;
    }
    closedir(d);
    vector<string> str; //
    getAllfile(folder, str);
    for (int i = 0; i < str.size(); i++)
    {
        int num, p;
        long remainSize;
        const char *filen;
        string fn;
        getPaSize(str[i], p, remainSize, fn);
        filen = fn.data();
        // printf("%s %d %ld %s\n",str[i].data(), p, remainSize, filen);
        // printf("%s\n",filename);
        if (strcmp(filen, filename) == 0)
        {
            return p;
        }
    }
    return 0;
}

/*
read操作 判断文件是否存在
*/
bool rsearchFile(char fileName[])
{
    int flag = 100; // 文件夹数量上限
    int folder_index = 0;
    int p;
    long size;
    string name;
    while (flag--)
    {                                                     // 不断的打开文件
        char *path = (char *)malloc(150 * sizeof(char));  // 文件路径
        sprintf(path, "%s_%d", FolderName, folder_index); // 文件夹名称
        DIR *d = opendir(path);
        if (d != NULL)
        {
            struct dirent *entry;
            while ((entry = readdir(d)) != NULL)
            {
                string tmpname = entry->d_name;
                getPaSize(tmpname, p, size, name);
                if (strcmp(fileName, name.data()) == 0)
                {
                    closedir(d);
                    free(path);
                    return 1;
                }
            }
        }
        closedir(d);
        folder_index += 1;
    }
    return 0;
}