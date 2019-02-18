/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   UserPort_4Player.h
 * Author: Test
 *
 * Created on 18. Februar 2019, 22:22
 */

#ifndef USERPORT_4PLAYER_H
#define USERPORT_4PLAYER_H
#include "UserPortInterface.h"

class UserPort_4Player : public UserPortInterface {
public:
    UserPort_4Player();
    
    unsigned char ReadPB();
    unsigned char ReadPA();
    void WritePB(unsigned char byte);
    void WritePA(unsigned char byte);
    
    void setJoy3(unsigned char j);
    void setJoy4(unsigned char j);
    
    UserPortInterface::USER_PORT_INTERFACE_TYPE GetType();
    
    virtual ~UserPort_4Player();
private:
    unsigned char joystick3;
    unsigned char joystick4;
    unsigned char* activeJoystick;
    
};

#endif /* USERPORT_4PLAYER_H */

