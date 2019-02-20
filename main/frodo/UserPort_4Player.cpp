/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   UserPort_4Player.cpp
 * Author: Test
 * 
 * Created on 18. Februar 2019, 22:22
 */

#include "UserPort_4Player.h"
#include <stdio.h>


UserPort_4Player::UserPort_4Player() {
    joystick3 = 0xff;
    joystick4 = 0xff;
}

UserPort_4Player::~UserPort_4Player() {
}

unsigned char UserPort_4Player::ReadPB(){
    return *activeJoystick;
}
unsigned char UserPort_4Player::ReadPA(){
    return 0x4;
}
void UserPort_4Player::WritePB(unsigned char byte){
    if (byte & 0x80){
        // switch to port 3
        activeJoystick = &joystick3;
    } else {
        // switch to port 4
        activeJoystick = &joystick4;
        
    }
   
}
void UserPort_4Player::WritePA(unsigned char byte){
   
}
void UserPort_4Player::setJoy3(unsigned char j){
    joystick3 = j;
}
void UserPort_4Player::setJoy4(unsigned char j){
    // the fire button is on bit 5 (not 4) on this joy.
    if (!(j & 0x10)) j &= 0xDF;
    joystick4 = j;
} 
UserPortInterface::USER_PORT_INTERFACE_TYPE UserPort_4Player::GetType() {
    return TYPE_4PLAYER_PROTOVISION;
}

