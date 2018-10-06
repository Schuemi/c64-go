/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Utils.h
 * Author: Test
 *
 * Created on 23. September 2018, 13:01
 */

#ifndef UTILS_H
#define UTILS_H
#include <stdbool.h>


#ifdef __cplusplus
extern "C" {
#endif

int fileExist(const char* fileName);
int dirExist(const char* dirName);
bool writeNAV96(void);
bool writeDEFAULT(void);
    
#ifdef __cplusplus
}
#endif
#endif /* UTILS_H */

