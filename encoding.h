/*
编码头文件，引用数据结构头文件
*/
#pragma once
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <sys/stat.h>
#include <algorithm>
#include "structure.h"
/*
基本函数
*/

char* intTochar(int num){
    char* s=(char*)calloc(4,sizeof(char));
     for(int i=0;i<4;i++){
        s[i]=num;
        num>>=8;
    }
    return s;
}

int getMod(int m, int n) {//取模运算
    if(m >= 0) {
        return m % n;
    }else {
        return (m + n) % n;
    }
}

mChunk computeS(mChunk **codeArray, int p) {//求S因子：编码
    mChunk S;
    S.data=(char*)calloc(chunkSize,sizeof(char));
    for(int i = 1; i < p; i++) {
        S ^= codeArray[p - 1 - i][i];
    }
    return S;
}

void t1_encode(mChunk **codeArray, int p, int i){
    for(int j = 0; j < p; j++) {
        codeArray[i][p] ^= codeArray[i][j];
    }
}
void t2_encode(mChunk **codeArray, int p, int i, mChunk *S){
    int krow;
    //printf("encode2 start\n");
    //printf("%d\n",S.size);
    //codeArray[i][p + 1] ^= S;//先异或斜校验
    //printf("encode2 ^\n");
    codeArray[i][p + 1] ^= *S;
    for(int j = 0; j < p; j++) {//p-1个斜对角线元素
        krow = getMod(i - j, p);
        //printf("%d \n",j);
        if(krow != p - 1) {
            codeArray[i][p + 1] ^= codeArray[krow][j];
        }
    }
    //printf("encode2 end\n");
}

void encoding(mChunk **codeArray, int p) {//编码
    thread th[p-1];
    
    for(int i = 0; i < p - 1; i++) {//行校验
        th[i]=thread(t1_encode, codeArray, p, i);
    }
    for(int i = 0; i < p - 1; i++) {//行校验
        th[i].join();
    }
    /*
    for(int i = 0; i < p - 1; i++) {//行校验
        for(int j = 0; j < p; j++) {
            codeArray[i][p] ^= codeArray[i][j];
        }
    }*/
    //斜校验
    //printf("@@@@@@@@@@  p1\n");
    mChunk S = computeS(codeArray, p);//因子S
    
    for(int i = 0; i < p - 1; i++) {//行校验
        //codeArray[i][p + 1] ^= S;
        th[i]=thread(t2_encode, codeArray, p, i, &S);
    }
    for(int i = 0; i < p - 1; i++) {//行校验
        th[i].join();
    }
    /*
    int krow;
    for(int i = 0; i < p - 1; i++) {//每一行
        codeArray[i][p + 1] ^= S;//先异或斜校验
        for(int j = 0; j < p; j++) {//p-1个斜对角线元素
            krow = getMod(i - j, p);
            if(krow != p - 1) {
                codeArray[i][p + 1] ^= codeArray[krow][j];
            }
        }
    }
    */
}
