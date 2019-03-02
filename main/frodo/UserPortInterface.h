
/* 
 * File:   UserPortInterface.h - Interface to the User port
 * Author: Jan P. Schümann
 *
 * Notes:
 * ------
 * Only read / wrtie for PB bit 0-7 and PA bit 2 yet
 */

#ifndef USERPORTINTERFACE_H
#define USERPORTINTERFACE_H


class UserPortInterface {
public:
    enum USER_PORT_INTERFACE_TYPE {
        TYPE_NONE = 0,
        TYPE_4PLAYER_PROTOVISION
    };
    virtual unsigned char ReadPB() = 0;
    virtual unsigned char ReadPA() = 0;
    virtual void WritePB(unsigned char byte) = 0;
    virtual void WritePA(unsigned char byte) = 0;
    virtual USER_PORT_INTERFACE_TYPE GetType() = 0;

    
};


#endif /* USERPORTINTERFACE_H */

