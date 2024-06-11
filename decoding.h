#pragma once
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <sys/stat.h>
#include <algorithm>
#include "encoding.h"


//求因子S:解码｜失效两列不为校验列时  Case 4
mChunk computeSDecodeCase4(mChunk **codeArray, int p) {
    mChunk S;
    S.data=(char*)calloc(chunkSize,sizeof(char));
    for(int i = 0; i < p - 1; i++) {
        S ^= codeArray[i][p];
        S ^= codeArray[i][p + 1];
    }
    return S;
}


//求因子S:解码｜失效的两列为 i < m, j = m  Case2
mChunk computeSDecodeCase2(mChunk **codeArray, int p, int *errorCol) {
    int krow = getMod(errorCol[0] - 1, p);
    mChunk S;
    S.data=(char*)calloc(chunkSize,sizeof(char));
    if(krow != p - 1) {
        S ^= codeArray[krow][p + 1];
    }
    for(int i = 0; i < p; i++) {
        krow = getMod(errorCol[0] - i - 1, p);
        if(krow != p - 1) {
            S ^= codeArray[krow][i];
        }
    }
    return S;
}




//解码 case1: i = m, j = m + 1
void decodingCase1(mChunk **codeArray, int p, int *errorCol) {
    //用编码方法可解
    encoding(codeArray, p);//已实现并行
}

void t1_decodingCase2(mChunk **codeArray, int p, int krow, int x, int i, mChunk *S){
    codeArray[i][x] ^= *S;
    int temp;
    if(krow != p - 1) {
        codeArray[i][x] ^= codeArray[krow][p + 1];
    }
    for(int j = 0; j < p; j++) {
        if(j != x) {
            temp = getMod(i + x - j, p);
            if(temp != p - 1) {
                codeArray[i][x] ^= codeArray[temp][j];
            }
        }
    }
}

void t2_decodingCase2(mChunk **codeArray, int p, int x, int i){
    for(int j = 0; j < p; j++) {
            codeArray[i][x] ^= codeArray[i][j];
        }
}

//解码 case2: i < m, j = m
void decodingCase2(mChunk **codeArray, int p, int *errorCol) {
    thread th[p-1];
    mChunk S = computeSDecodeCase2(codeArray, p, errorCol);
    int krow = errorCol[0], temp;
    //解 i 列
    for(int i = 0; i < p - 1; i++) {
        //codeArray[i][errorCol[0]] ^= S;
        krow = getMod(krow, p);

        th[i]=thread(t1_decodingCase2, codeArray, p, krow, errorCol[0], i, &S);
        
        /*
        if(krow != p - 1) {
            codeArray[i][errorCol[0]] ^= codeArray[krow][p + 1];
        }
        for(int j = 0; j < p; j++) {
            if(j != errorCol[0]) {
                temp = getMod(i + errorCol[0] - j, p);
                if(temp != p - 1) {
                    codeArray[i][errorCol[0]] ^= codeArray[temp][j];
                }
            }
        }*/

        krow++;
    }
    for(int i = 0; i < p - 1; i++){
        th[i].join();
    }

    //第 j 列 ： 行校验可解
    for(int i = 0; i < p - 1; i++) {
        th[i]=thread(t2_decodingCase2, codeArray, p, errorCol[1], i);
        /*
        for(int j = 0; j < p; j++) {
            codeArray[i][errorCol[1]] ^= codeArray[i][j];
        }
        */
    }
    for(int i = 0; i < p - 1; i++){
        th[i].join();
    }
}

void t1_decodingCase3(mChunk **codeArray, int p, int x, int i){
    for(int j = 0; j <= p; j++) {
        if(j != x) {
            codeArray[i][x] ^= codeArray[i][j];
        }
    }
}

void t2_decodingCase3(mChunk **codeArray, int p, int x, int i, mChunk *S){
    int krow;
    codeArray[i][x] ^= *S;
    for(int j = 0; j < p; j++) {
        krow = getMod(i - j, p);
        if(krow != p - 1) {
        codeArray[i][x] ^= codeArray[krow][j];
        }
    }
}

//解码 case3: i < m, j = m + 1
void decodingCase3(mChunk **codeArray, int p, int *errorCol) {
    thread th[p-1];
    //用行校验解 i 列
    for(int i = 0; i < p - 1; i++) {
        th[i]=thread(t1_decodingCase3, codeArray, p, errorCol[0], i);
        /*
        for(int j = 0; j <= p; j++) {
            if(j != errorCol[0]) {
                codeArray[i][errorCol[0]] ^= codeArray[i][j];
            }
        }
        */
    }
    for(int i = 0; i < p - 1; i++){
        th[i].join();
    }
    //用编码方法解 j 列
    mChunk S = computeS(codeArray, p);
    //int krow;
    for(int i = 0; i < p - 1; i++) {
        //codeArray[i][errorCol[1]] ^= S;
        th[i]=thread(t2_decodingCase3, codeArray, p, errorCol[1], i, &S);
        /*
        for(int j = 0; j < p; j++) {
            krow = getMod(i - j, p);
            if(krow != p - 1) {
            codeArray[i][errorCol[1]] ^= codeArray[krow][j];
            }
        }
        */
    }
    for(int i = 0; i < p - 1; i++){
        th[i].join();
    }
}

//求S0 解码case4
mChunk getS0(mChunk **codeArray, int p, int row, int *errorCol) {
    mChunk S0;
    S0.data=(char*)calloc(chunkSize,sizeof(char));
    if(row != p - 1) {
       for(int j = 0; j <= p; j++) {
            if(j != errorCol[0] && j != errorCol[1]) {
                S0 ^= codeArray[row][j];
            }
        }
    }
    return S0;
}

//求S1 解码case4
mChunk getS1(mChunk **codeArray, int p, int row, int *errorCol) {
    //mChunk S = computeSDecodeCase4(codeArray, p);

    mChunk S;
    S.data=(char*)calloc(chunkSize,sizeof(char));
    for(int i = 0; i < p - 1; i++) {
        S ^= codeArray[i][p];
        S ^= codeArray[i][p + 1];
    }

    mChunk S1;
    S1.data=(char*)calloc(chunkSize,sizeof(char));
    S1 ^= S;
    int krow;
    if(row != p - 1) {
        S1 ^= codeArray[row][p + 1];
    }
    
    for(int i = 0; i < p; i++) {
        if(i != errorCol[0] && i != errorCol[1]) {
            krow = getMod(row - i, p);
            if(krow != p - 1) {
                S1 ^= codeArray[krow][i];
            }
        }
    }
    return S1;
}

void t1_decodingCase4(mChunk **codeArray, int p, mChunk *S){
    for(int i = 0; i < p - 1; i++) {
            *S ^= codeArray[i][p];
            *S ^= codeArray[i][p + 1];
        }
}
void t2_decodingCase4(mChunk **codeArray, int p, mChunk *S1 ,int x, int y, int krow){
    int trow;
    for(int i = 0; i < p; i++) {
            if(i != x && i != y) {
                trow = getMod(krow - i, p);
                if(trow != p - 1) {
                    *S1 ^= codeArray[trow][i];
                }
            }
        }
}
void t3_decodingCase4(mChunk **codeArray, int startj, int endj, mChunk *S0 ,int x, int y, int k){
    for(int j = startj; j <= endj; j++) {
        if(j != x && j != y) {
            *S0 ^= codeArray[k][j];
        }
    }
}
//解码 case4: i < m, j < m
void decodingCase4(mChunk **codeArray, int p, int *errorCol) {
    thread th[2];
    //printf("===");
    int k = getMod(-(errorCol[1] - errorCol[0]) -1, p);
    int krow;
    while(k != p - 1) {
        mChunk S0, S1, S ,S2;
        S0.data=(char*)calloc(chunkSize,sizeof(char));
        S2.data=(char*)calloc(chunkSize,sizeof(char));
        S1.data=(char*)calloc(chunkSize,sizeof(char));
        S.data=(char*)calloc(chunkSize,sizeof(char));
        krow = getMod(errorCol[1] + k, p);
        /*
        for(int i = 0; i < p - 1; i++) {
            S ^= codeArray[i][p];
            S ^= codeArray[i][p + 1];
        }*/

        th[0]=thread(t1_decodingCase4, codeArray, p, &S);

        if(krow != p - 1) {
            S1 ^= codeArray[krow][p + 1];
        }
        th[1]=thread(t2_decodingCase4, codeArray, p, &S1, errorCol[0], errorCol[1], krow);

        th[0].join();
        th[1].join();

        S1 ^= S;

        /*
        int trow;
        if(krow != p - 1) {
            S1 ^= codeArray[krow][p + 1];
        }
    
        for(int i = 0; i < p; i++) {
            if(i != errorCol[0] && i != errorCol[1]) {
                trow = getMod(krow - i, p);
                if(trow != p - 1) {
                    S1 ^= codeArray[trow][i];
                }
            }
        }
        */


        codeArray[k][errorCol[1]] ^= S1;
        krow = getMod(k + (errorCol[1] - errorCol[0]), p);
        if(krow != p - 1) {
            codeArray[k][errorCol[1]] ^= codeArray[krow][errorCol[0]];
        }
        if(k != p - 1) {
            //printf("yes!!!");
            
            th[0]=thread(t3_decodingCase4, codeArray, 0, p/2, &S0, errorCol[0], errorCol[1], k);
            th[1]=thread(t3_decodingCase4, codeArray, p/2+1, p, &S2, errorCol[0], errorCol[1], k);
            
            /*
            for(int j = 0; j <= p; j++) {
                if(j != errorCol[0] && j != errorCol[1]) {
                    S0 ^= codeArray[k][j];
                }
            }
            */
            
            th[0].join();
            th[1].join();
            S0^=S2;
            
        }
        codeArray[k][errorCol[0]] ^= S0;
        codeArray[k][errorCol[0]] ^= codeArray[k][errorCol[1]];
        k = getMod(k - (errorCol[1] - errorCol[0]), p);
    }
}

void decodingCase4(mChunk **codeArray, int p, int *errorCol,int a) {
    int k = getMod(-(errorCol[1] - errorCol[0]) -1, p);
    int krow;
    mChunk S0, S1;
    S0.data=(char*)calloc(chunkSize,sizeof(char));
    S1.data=(char*)calloc(chunkSize,sizeof(char));
    while(k != p - 1) {
        krow = getMod(errorCol[1] + k, p);
        S1 = getS1(codeArray, p, krow, errorCol);
        codeArray[k][errorCol[1]] ^= S1;
        krow = getMod(k + (errorCol[1] - errorCol[0]), p);
        if(krow != p - 1) {
            codeArray[k][errorCol[1]] ^= codeArray[krow][errorCol[0]];
        }
        S0 = getS0(codeArray, p, k, errorCol);
        codeArray[k][errorCol[0]] ^= S0;
        codeArray[k][errorCol[0]] ^= codeArray[k][errorCol[1]];
        
        k = getMod(k - (errorCol[1] - errorCol[0]), p);
    }
}
//解码
void decoding2err(mChunk **codeArray, int p, int *errorCol) {
    if((errorCol[0] == p && errorCol[1] == p + 1)) {
        decodingCase1(codeArray, p, errorCol);
    }else if((errorCol[0] < p && errorCol[1] == p)) {
        decodingCase2(codeArray, p, errorCol);
    }else if((errorCol[0] < p && errorCol[1] == p + 1)) {
        decodingCase3(codeArray, p, errorCol);
    }else if (errorCol[0] < p && errorCol[1] < p) {
        decodingCase4(codeArray, p, errorCol);
    }
}

void t1_decoding1err(mChunk **codeArray, int p, int i){
    int krow;
    for(int j = 0; j < p; j++) {//p-1个斜对角线元素
        krow = getMod(i - j, p);
        if(krow != p - 1) {
            codeArray[i][p + 1] ^= codeArray[krow][j];
        }
    }
}

void t2_decoding1err(mChunk **codeArray, int p, int errorCol, int i){
    for(int j = 0; j <= p; j++) {
        if(j != errorCol) {
            codeArray[i][errorCol] ^= codeArray[i][j];
        }
    }
}

void decoding1err(mChunk **codeArray, int p, int errorCol){
    thread th[p-1];
    if(errorCol==p+1)
    {
        mChunk S = computeS(codeArray, p);//因子S
        int krow;
        for(int i = 0; i < p - 1; i++) {//每一行
            codeArray[i][p + 1] ^= S;//先异或斜校验
            /*for(int j = 0; j < p; j++) {//p-1个斜对角线元素
                krow = getMod(i - j, p);
                if(krow != p - 1) {
                    codeArray[i][p + 1] ^= codeArray[krow][j];
                }
            }*/
        }
        for(int i = 0; i < p - 1; i++) {
            th[i]=thread(t1_decoding1err, codeArray, p, i);//p-1个斜对角线元素
        }
        for(int i = 0; i < p - 1; i++) {
            th[i].join();
        }
    }
    else
    {
        //printf("yes this\n");
        for(int i = 0; i < p - 1; i++) {
            //th[i]=thread(t2_decoding1err, codeArray, p, errorCol, i);
            
            for(int j = 0; j <= p; j++) {
                if(j != errorCol) {
                    codeArray[i][errorCol] ^= codeArray[i][j];
                }
            }
            
        }
        /*
        for(int i = 0; i < p - 1; i++) {
            th[i].join();
        }
        */
    }
}

/*
Repair
*/

void t1_num1_repair(mChunk **mBlock, int p, int x){
    decoding1err(mBlock, p, x);
}
void t2_num1_repair(mChunk **mBlock, FILE** pd, int p, int *idx, long* remainSize, int dBlockSize){
    //printf("%ld\n",*remainSize);
    if(*remainSize>0)
        fileToblock(mBlock, pd, p, idx, *remainSize, dBlockSize);
    //printf("%ld\n",*remainSize);
}
void t3_num1_repair(mChunk **mBlock, FILE** pd, int p, int *idx, long *remainSize, int dBlockSize){
    blockTofiles(mBlock, pd, p, idx);
    for (int i = 0; i < p - 1; i++)
    {
        memset(mBlock[i][p].data, 0, chunkSize);
        memset(mBlock[i][p + 1].data, 0, chunkSize);
    }
    //printf("%ld\n",*remainSize);
    if(*remainSize>0)
        fileToblock(mBlock, pd, p, idx, *remainSize, dBlockSize);
    //printf("%ld\n",*remainSize);
}
void t4_num1_repair(mChunk **mBlock, FILE** pd, int p, int *idx, long *remainSize, int dBlockSize){
    blockTofiles(mBlock, pd, p, idx);
    for (int i = 0; i < p - 1; i++)
    {
        memset(mBlock[i][p].data, 0, chunkSize);
        memset(mBlock[i][p + 1].data, 0, chunkSize);
    }
    //printf("%ld\n",*remainSize);
}

void round1_num1_repair(int round, mChunk ***mBlock, FILE** pd, int p ,int * idx ,long* remainSize, int dBlockSize){
    thread th[2];
    th[0]=thread(t1_num1_repair, mBlock[(round+1)%2], p, idx[0]);
    //printf("round:%d\n",round);
    th[1]=thread(t2_num1_repair, mBlock[round%2], pd, p, idx, remainSize, dBlockSize);
    th[0].join();
    th[1].join();
}

void round_other_num1_repair(int round, mChunk ***mBlock, FILE** pd, int p ,int * idx ,long *remainSize, int dBlockSize){
    thread th[2];
    th[0]=thread(t1_num1_repair, mBlock[(round+1)%2], p, idx[0]);
        //printf("%ld\n",remainSize);
    //printf("round:%d\n",round);
    th[1]=thread(t3_num1_repair, mBlock[round%2], pd, p, idx, remainSize, dBlockSize);
    th[0].join();
    th[1].join();
}

void round_last_num1_repair(int round, mChunk ***mBlock, FILE** pd, int p ,int * idx ,long *remainSize, int dBlockSize){
    thread th[2];
    th[0]=thread(t1_num1_repair, mBlock[(round+1)%2], p, idx[0]);
        //printf("%ld\n",remainSize);
    //printf("round:%d\n",round);
    th[1]=thread(t4_num1_repair, mBlock[round%2], pd, p, idx, remainSize, dBlockSize);
    th[0].join();
    th[1].join();
}

void t1_num2_repair(mChunk** mBlock, int p ,int* idx){
    decoding2err(mBlock, p, idx);
}

void t2_num2_repair(mChunk** mBlock, FILE** pd, int p, int* idx ,long *remainSize, int dBlockSize){
    if(*remainSize>0)
        fileToblock(mBlock, pd, p, idx, *remainSize, dBlockSize);
}

void t3_num2_repair(mChunk** mBlock, FILE** pd, int p, int* idx ,long *remainSize, int dBlockSize){
    blockTofiles(mBlock, pd, p, idx);
    for (int i = 0; i < p - 1; i++)
    {
        memset(mBlock[i][p].data, 0, chunkSize);
        memset(mBlock[i][p + 1].data, 0, chunkSize);
    }
    if(*remainSize>0)
        fileToblock(mBlock, pd, p, idx, *remainSize, dBlockSize);
}

void t4_num2_repair(mChunk** mBlock, FILE** pd, int p, int* idx ,long *remainSize, int dBlockSize){
    blockTofiles(mBlock, pd, p, idx);
}

void round1_num2_repair(int round, mChunk ***mBlock, FILE** pd, int p ,int * idx ,long *remainSize, int dBlockSize){
    thread th[2];
    th[0]=thread(t1_num2_repair, mBlock[(round+1)%2], p, idx);
    th[1]=thread(t2_num2_repair, mBlock[round%2], pd , p, idx, remainSize, dBlockSize);
    th[0].join();
    th[1].join();
}

void round_other_num2_repair(int round, mChunk ***mBlock, FILE** pd, int p ,int * idx ,long *remainSize, int dBlockSize){
    thread th[2];
    th[0]=thread(t1_num2_repair, mBlock[(round+1)%2], p, idx);
    th[1]=thread(t3_num2_repair, mBlock[round%2], pd , p, idx, remainSize, dBlockSize);
    th[0].join();
    th[1].join();
}

void round_last_num2_repair(int round, mChunk ***mBlock, FILE** pd, int p ,int * idx ,long *remainSize, int dBlockSize){
    thread th[2];
    th[0]=thread(t1_num2_repair, mBlock[(round+1)%2], p, idx);
    th[1]=thread(t4_num2_repair, mBlock[round%2], pd , p, idx, remainSize, dBlockSize);
    th[0].join();
    th[1].join();
}




void repair(int num, int* idx)
{
    int num_era = num;
    if (num_era > 2)
    {
        printf("Too many corruptions!");
        exit(0);
    }
    int errDisk[2] = {-1, -1};
    if (num_era == 1)
    {
        errDisk[0] = idx[0];
        errDisk[1] = -1;
    }
    else if (num_era == 2)
    {
        errDisk[0] = idx[0];
        errDisk[1] = idx[1];
        if (errDisk[0] > errDisk[1])
        {
            int temp = errDisk[0];
            errDisk[0] = errDisk[1];
            errDisk[1] = temp;
        }
    }
    char *folder0 = (char *)"disk_0";
    char *folder1 = (char *)"disk_1";
    char *folder2 = (char *)"disk_2";
    DIR *d = opendir(folder0);
    char folder[7] = "disk_0";
    string a;
    if (d == NULL)
    {
        d = opendir(folder1);
        folder[5] = '1';
    }
    if (d == NULL)
    {
        d = opendir(folder2);
        folder[5] = '2';
    }
    closedir(d);
    vector<string> str;
    getAllfile(folder, str);
    for (int i = 0; i < str.size(); i++)
    {

        int num, p;
        long remainSize;
        int idx[2] = {-1, -1};
        const char *filename;
        string fn;
        char temp[300];
        getPaSize(str[i], p, remainSize, fn);
        filename = fn.data();

        if (num_era == 1)
        {
            if (errDisk[0] <= p + 1)
            {
                idx[0] = errDisk[0];
                idx[1] = -1;
                num = 1;
            }
            else
                continue;
        }
        else if (num_era == 2)
        {
            if (errDisk[1] <= p + 1)
            {
                idx[0] = errDisk[0];
                idx[1] = errDisk[1];
                num = 2;
            }
            else if (errDisk[0] > p + 1)
                continue;
            else
            {
                idx[0] = errDisk[0];
                idx[1] = -1;
                num = 1;
            }
        }

        int dBlockSize = p * (p - 1) * chunkSize;
        FILE *pd[p + 2] = {NULL};
        mChunk ***mBlock= (mChunk ***)malloc(2 * sizeof(mChunk **));
        mBlock[0] = (mChunk **)malloc((p - 1) * sizeof(mChunk *)); //创建block并开辟空间
        mBlock[1] = (mChunk **)malloc((p - 1) * sizeof(mChunk *)); //创建block并开辟空间
        for (int i = 0; i < p - 1; i++)
        {
            mBlock[0][i] = (mChunk *)malloc((p + 2) * sizeof(mChunk));
            mBlock[1][i] = (mChunk *)malloc((p + 2) * sizeof(mChunk));
            for (int j = 0; j < p + 2; j++)
            {
                mBlock[0][i][j] = mChunk();
                mBlock[0][i][j].data = (char *)calloc(chunkSize, sizeof(char));
                mBlock[1][i][j] = mChunk();
                mBlock[1][i][j].data = (char *)calloc(chunkSize, sizeof(char));
            }
        }

        if (num == 1)
        {
            openFolderFile(p, pd, filename, idx, remainSize);
            fileToblock(mBlock[0], pd, p, idx, remainSize, dBlockSize);
            int round=0;
            if(remainSize==0){
                decoding1err(mBlock[0], p, idx[0]);
                blockTofiles(mBlock[0], pd, p, idx);
                for (int i = 0; i < p - 1; i++)
                {
                    memset(mBlock[0][i][p].data, 0, chunkSize);
                    memset(mBlock[0][i][p + 1].data, 0, chunkSize);
                }
                break;
            }
            while (remainSize > 0)
            {
                round++;
                if(round==1){
                    round1_num1_repair(round, mBlock, pd, p, idx, &remainSize, dBlockSize);
                }
                else{
                    round_other_num1_repair(round, mBlock, pd, p, idx, &remainSize, dBlockSize);
                }
                /*
                fileToblock(mBlock, pd, p, idx, remainSize, dBlockSize);
                decoding1err(mBlock, p, idx[0]);
                blockTofiles(mBlock, pd, p, idx);
                for (int i = 0; i < p - 1; i++)
                {
                    memset(mBlock[i][p].data, 0, chunkSize);
                    memset(mBlock[i][p + 1].data, 0, chunkSize);
                }
                */
            }
            round++;
            round_last_num1_repair(round, mBlock, pd, p, idx, &remainSize, dBlockSize);
            blockTofiles(mBlock[(round+1)%2], pd, p, idx);
        }
        else if (num == 2)
        {
            if (idx[0] > idx[1])
            {
                int temp = idx[0];
                idx[0] = idx[1];
                idx[1] = temp;
            }
            openFolderFile(p, pd, filename, idx, remainSize);
            fileToblock(mBlock[0], pd, p, idx, remainSize, dBlockSize);
            int round=0;
            if(remainSize==0){
                decoding2err(mBlock[0], p, idx);
                blockTofiles(mBlock[0], pd, p, idx);
                for (int i = 0; i < p - 1; i++)
                {
                    memset(mBlock[0][i][p].data, 0, chunkSize);
                    memset(mBlock[0][i][p + 1].data, 0, chunkSize);
                }
                break;
            }
            while (remainSize > 0)
            {
                round++;
                if(round==1){
                    round1_num2_repair(round, mBlock, pd, p, idx, &remainSize, dBlockSize);
                }
                else{
                    round_other_num2_repair(round, mBlock, pd, p, idx, &remainSize, dBlockSize);
                }
                /*
                fileToblock(mBlock, pd, p, idx, remainSize, dBlockSize);
                decoding2err(mBlock, p, idx);
                blockTofiles(mBlock, pd, p, idx);
                for (int i = 0; i < p - 1; i++)
                {
                    memset(mBlock[i][p].data, 0, chunkSize);
                    memset(mBlock[i][p + 1].data, 0, chunkSize);
                }
                */
            }
            round++;
            round_last_num2_repair(round, mBlock, pd, p, idx, &remainSize, dBlockSize);
            blockTofiles(mBlock[(round+1)%2], pd, p, idx);
        }
        cleanUpmemory1(mBlock, pd, p);
        //printf("File %d has been repaired!\n", i);
    }
}
void t1_num1_repair_read(mChunk **mBlock, int p, int x){
    decoding1err(mBlock, p, x);
}
void t2_num1_repair_read(mChunk **mBlock, FILE** pd, int p, int *idx, long* remainSize, int dBlockSize){
    //printf("%ld\n",*remainSize);
    if(*remainSize>0)
        fileToblock(mBlock, pd, p, idx, *remainSize, dBlockSize);
    //printf("%ld\n",*remainSize);
}
void t3_num1_repair_read(mChunk **mBlock, FILE** pd, int p, int *idx, long *remainSize, int dBlockSize, int Filecursor){
    blockTofilesAfterRepair(mBlock, Filecursor, p);
    for (int i = 0; i < p - 1; i++)
    {
        memset(mBlock[i][p].data, 0, chunkSize);
        memset(mBlock[i][p + 1].data, 0, chunkSize);
    }
    //printf("%ld\n",*remainSize);
    if(*remainSize>0)
        fileToblock(mBlock, pd, p, idx, *remainSize, dBlockSize);
    //printf("%ld\n",*remainSize);
}
void t4_num1_repair_read(mChunk **mBlock, FILE** pd, int p, int *idx, long *remainSize, int dBlockSize, int Filecursor){
    blockTofilesAfterRepair(mBlock, Filecursor, p);
    for (int i = 0; i < p - 1; i++)
    {
        memset(mBlock[i][p].data, 0, chunkSize);
        memset(mBlock[i][p + 1].data, 0, chunkSize);
    }
    //printf("%ld\n",*remainSize);
}

void round1_num1_repair_read(int round, mChunk ***mBlock, FILE** pd, int p ,int * idx ,long* remainSize, int dBlockSize){
    thread th[2];
    th[0]=thread(t1_num1_repair_read, mBlock[(round+1)%2], p, idx[0]);
    //printf("round:%d\n",round);
    th[1]=thread(t2_num1_repair_read, mBlock[round%2], pd, p, idx, remainSize, dBlockSize);
    th[0].join();
    th[1].join();
}

void round_other_num1_repair_read(int round, mChunk ***mBlock, FILE** pd, int p ,int * idx ,long *remainSize, int dBlockSize, int Filecursor){
    thread th[2];
    th[0]=thread(t1_num1_repair_read, mBlock[(round+1)%2], p, idx[0]);
        //printf("%ld\n",remainSize);
    //printf("round:%d\n",round);
    th[1]=thread(t3_num1_repair_read, mBlock[round%2], pd, p, idx, remainSize, dBlockSize, Filecursor);
    th[0].join();
    th[1].join();
}

void round_last_num1_repair_read(int round, mChunk ***mBlock, FILE** pd, int p ,int * idx ,long *remainSize, int dBlockSize, int Filecursor){
    thread th[2];
    th[0]=thread(t1_num1_repair_read, mBlock[(round+1)%2], p, idx[0]);
        //printf("%ld\n",remainSize);
    //printf("round:%d\n",round);
    th[1]=thread(t4_num1_repair_read, mBlock[round%2], pd, p, idx, remainSize, dBlockSize, Filecursor);
    th[0].join();
    th[1].join();
}

void t1_num2_repair_read(mChunk** mBlock, int p ,int* idx){
    decoding2err(mBlock, p, idx);
}

void t2_num2_repair_read(mChunk** mBlock, FILE** pd, int p, int* idx ,long *remainSize, int dBlockSize){
    if(*remainSize>0)
        fileToblock(mBlock, pd, p, idx, *remainSize, dBlockSize);
}

void t3_num2_repair_read(mChunk** mBlock, FILE** pd, int p, int* idx ,long *remainSize, int dBlockSize, int Filecursor){
    blockTofilesAfterRepair(mBlock, Filecursor, p);
    for (int i = 0; i < p - 1; i++)
    {
        memset(mBlock[i][p].data, 0, chunkSize);
        memset(mBlock[i][p + 1].data, 0, chunkSize);
    }
    if(*remainSize>0)
        fileToblock(mBlock, pd, p, idx, *remainSize, dBlockSize);
}

void t4_num2_repair_read(mChunk** mBlock, FILE** pd, int p, int* idx ,long *remainSize, int dBlockSize, int Filecursor){
    blockTofilesAfterRepair(mBlock, Filecursor, p);
}

void round1_num2_repair_read(int round, mChunk ***mBlock, FILE** pd, int p ,int * idx ,long *remainSize, int dBlockSize){
    thread th[2];
    th[0]=thread(t1_num2_repair_read, mBlock[(round+1)%2], p, idx);
    th[1]=thread(t2_num2_repair_read, mBlock[round%2], pd , p, idx, remainSize, dBlockSize);
    th[0].join();
    th[1].join();
}

void round_other_num2_repair_read(int round, mChunk ***mBlock, FILE** pd, int p ,int * idx ,long *remainSize, int dBlockSize, int Filecursor){
    thread th[2];
    th[0]=thread(t1_num2_repair_read, mBlock[(round+1)%2], p, idx);
    th[1]=thread(t3_num2_repair_read, mBlock[round%2], pd , p, idx, remainSize, dBlockSize, Filecursor);
    th[0].join();
    th[1].join();
}

void round_last_num2_repair_read(int round, mChunk ***mBlock, FILE** pd, int p ,int * idx ,long *remainSize, int dBlockSize, int Filecursor){
    thread th[2];
    th[0]=thread(t1_num2_repair_read, mBlock[(round+1)%2], p, idx);
    th[1]=thread(t4_num2_repair_read, mBlock[round%2], pd , p, idx, remainSize, dBlockSize, Filecursor);
    th[0].join();
    th[1].join();
}

void repair_read(int num, int* idx, int Filecursor,char* FileName111)
{
    int num_era = num;
    if (num_era > 2)
    {
        printf("Too many corruptions!");
        exit(0);
    }
    int errDisk[2] = {-1, -1};
    if (num_era == 1)
    {
        errDisk[0] = idx[0];
        errDisk[1] = -1;
    }
    else if (num_era == 2)
    {
        errDisk[0] = idx[0];
        errDisk[1] = idx[1];
        if (errDisk[0] > errDisk[1])
        {
            int temp = errDisk[0];
            errDisk[0] = errDisk[1];
            errDisk[1] = temp;
        }
    }
    char *folder0 = (char *)"disk_0";
    char *folder1 = (char *)"disk_1";
    char *folder2 = (char *)"disk_2";
    DIR *d = opendir(folder0);
    char folder[7] = "disk_0";
    string a;
    if (d == NULL)
    {
        d = opendir(folder1);
        folder[5] = '1';
    }
    if (d == NULL)
    {
        d = opendir(folder2);
        folder[5] = '2';
    }
    closedir(d);
    vector<string> str;
    getAllfile(folder, str);
    for (int i = 0; i < str.size(); i++)
    {
        int num, p;
        long remainSize;
        int idx[2] = {-1, -1};
        const char *filename;
        string fn;
        char temp[300];
        string x;
        x=FileName111;
        getPaSize(str[i], p, remainSize, fn);
        if(fn!=x)continue;
        filename=fn.data();
        if (num_era == 1)
        {
            if (errDisk[0] <= p + 1)
            {
                idx[0] = errDisk[0];
                idx[1] = -1;
                num = 1;
            }
            else
                continue;
        }
        else if (num_era == 2)
        {
            if (errDisk[1] <= p + 1)
            {
                idx[0] = errDisk[0];
                idx[1] = errDisk[1];
                num = 2;
            }
            else if (errDisk[0] > p + 1)
                continue;
            else
            {
                idx[0] = errDisk[0];
                idx[1] = -1;
                num = 1;
            }
        }

        int dBlockSize = p * (p - 1) * chunkSize;
        FILE *pd[p + 2] = {NULL};
        mChunk ***mBlock = (mChunk***)malloc(2 * sizeof(mChunk **));
        mBlock[0]=(mChunk **)malloc((p - 1) * sizeof(mChunk *));
        mBlock[1]=(mChunk **)malloc((p - 1) * sizeof(mChunk *));
        for (int i = 0; i < p - 1; i++)
        {
            mBlock[0][i] = (mChunk *)malloc((p + 2) * sizeof(mChunk));
            mBlock[1][i] = (mChunk *)malloc((p + 2) * sizeof(mChunk));
            for (int j = 0; j < p + 2; j++)
            {
                mBlock[0][i][j] = mChunk();
                mBlock[0][i][j].data = (char *)calloc(chunkSize, sizeof(char));
                mBlock[1][i][j] = mChunk();
                mBlock[1][i][j].data = (char *)calloc(chunkSize, sizeof(char));
            }
        }
        /*
        mChunk **mBlock = (mChunk **)malloc((p - 1) * sizeof(mChunk *)); //创建block并开辟空间
        for (int i = 0; i < p - 1; i++)
        {
            mBlock[i] = (mChunk *)malloc((p + 2) * sizeof(mChunk));
            for (int j = 0; j < p + 2; j++)
            {
                mBlock[i][j] = mChunk();
                mBlock[i][j].data = (char *)calloc(chunkSize, sizeof(char));
            }
        }
        */

        if (num == 1)
        {
            thread th[2];
            openFolderFileWhileReadRepair(p, pd, filename, idx, remainSize);
            int round=0;
            int couldread=1;
            fileToblock(mBlock[0], pd, p, idx, remainSize, dBlockSize);
            //printf("round:0\n%ld\n",remainSize);
            if(remainSize==0)
            {
                decoding1err(mBlock[round%2], p, idx[0]);
                blockTofilesAfterRepair(mBlock[round%2], Filecursor, p);
                break;
            }
            while (remainSize > 0)
            {
                //printf("####%ld\n",remainSize);
                round++;
                if(round==1){//只运行read和decoding
                    /*
                        decoding1err(mBlock[(round+1)%2], p, idx[0]);
                        fileToblock(mBlock[round%2], pd, p, idx, remainSize, dBlockSize);
                    */
                    round1_num1_repair_read(round, mBlock, pd, p, idx, &remainSize, dBlockSize);
                }
                else{//运行decoding 和 write read
                    /*
                        decoding1err(mBlock[(round+1)%2], p, idx[0]);

                        blockTofilesAfterRepair(mBlock[round%2], Filecursor, p);
                        for (int i = 0; i < p - 1; i++)
                        {
                            memset(mBlock[i][p].data, 0, chunkSize);
                            memset(mBlock[i][p + 1].data, 0, chunkSize);
                        }
                        if(remainSize>0)
                            fileToblock(mBlock[round%2], pd, p, idx, remainSize, dBlockSize);
                    */
                    round_other_num1_repair_read(round, mBlock, pd, p, idx, &remainSize, dBlockSize, Filecursor);
                }
                /*
                fileToblock(mBlock, pd, p, idx, remainSize, dBlockSize);
                decoding1err(mBlock, p, idx[0]);
                //blockTofiles(mBlock, pd, p, idx);
                blockTofilesAfterRepair(mBlock, Filecursor, p);
                for (int i = 0; i < p - 1; i++)
                {
                    memset(mBlock[i][p].data, 0, chunkSize);
                    memset(mBlock[i][p + 1].data, 0, chunkSize);
                }
                */
            }
            blockTofilesAfterRepair(mBlock[(round+1)%2], Filecursor, p);
            decoding1err(mBlock[(round)%2], p, idx[0]);
            /*for (int i=0;i<10;i++)
            printf("%d ",mBlock[round%2][1][0].data[i]);
            printf("\n");
            for (int i=0;i<10;i++)
            printf("%d ",mBlock[round%2][1][1].data[i]);
            printf("\n");
            for (int i=0;i<10;i++)
            printf("%d ",mBlock[round%2][1][3].data[i]);
            printf("\n");
            for (int i=0;i<10;i++)
            printf("%d ",mBlock[round%2][1][4].data[i]);
            printf("\n");
            for (int i=0;i<10;i++)
            printf("%d ",mBlock[round%2][1][5].data[i]);
            printf("\n");
            for (int i=0;i<10;i++)
            printf("%d ",mBlock[round%2][1][6].data[i]);
            printf("\n");
*/
            blockTofilesAfterRepair(mBlock[(round)%2], Filecursor, p);
            for (int i = 0; i < p - 1; i++)
            {
                memset(mBlock[0][i][p].data, 0, chunkSize);
                memset(mBlock[0][i][p + 1].data, 0, chunkSize);
                memset(mBlock[1][i][p].data, 0, chunkSize);
                memset(mBlock[1][i][p + 1].data, 0, chunkSize);
            }
        }
        else if (num == 2)
        {
            if (idx[0] > idx[1])
            {
                int temp = idx[0];
                idx[0] = idx[1];
                idx[1] = temp;
            }
            openFolderFileWhileReadRepair(p, pd, filename, idx, remainSize);
            int round=0;
            fileToblock(mBlock[0], pd, p, idx, remainSize, dBlockSize);
            if(remainSize==0){
                decoding2err(mBlock[0], p, idx);
                blockTofilesAfterRepair(mBlock[0], Filecursor, p);
                break;
            }
            while (remainSize > 0)
            {
                round++;
                if(round==1)
                {   //只运行 read 和 decoding
                    round1_num2_repair_read(round, mBlock, pd, p, idx, &remainSize, dBlockSize);
                }
                else
                {
                    round_other_num2_repair_read(round, mBlock, pd, p, idx, &remainSize, dBlockSize, Filecursor);
                }
                /*
                fileToblock(mBlock, pd, p, idx, remainSize, dBlockSize);
                decoding2err(mBlock, p, idx);
                //blockTofiles(mBlock, pd, p, idx);
                blockTofilesAfterRepair(mBlock, Filecursor, p);
                for (int i = 0; i < p - 1; i++)
                {
                    memset(mBlock[i][p].data, 0, chunkSize);
                    memset(mBlock[i][p + 1].data, 0, chunkSize);
                }
                */
            }
            round++;
            round_last_num2_repair_read(round, mBlock, pd, p, idx, &remainSize, dBlockSize, Filecursor);
            //blockTofilesAfterRepair(mBlock[(round+1)%2], Filecursor, p);
            //decoding2err(mBlock[round%2], p, idx);
            blockTofilesAfterRepair(mBlock[(round+1)%2], Filecursor, p);
        }
        cleanUpmemory1(mBlock, pd, p, idx, num);
        //printf("File %d has been repaired!\n", i);
    }
}