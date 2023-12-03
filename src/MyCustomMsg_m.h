//
// Generated file, do not edit! Created by nedtool 5.6 from MyCustomMsg.msg.
//

#ifndef __MYCUSTOMMSG_M_H
#define __MYCUSTOMMSG_M_H

#if defined(__clang__)
#  pragma clang diagnostic ignored "-Wreserved-id-macro"
#endif
#include <omnetpp.h>

// nedtool version check
#define MSGC_VERSION 0x0506
#if (MSGC_VERSION!=OMNETPP_VERSION)
#    error Version mismatch! Probably this file was generated by an earlier version of nedtool: 'make clean' should help.
#endif



/**
 * Class generated from <tt>MyCustomMsg.msg:19</tt> by nedtool.
 * <pre>
 * //
 * // TODO generated message class
 * //
 * packet MyCustomMsg
 * {
 *     \@customize(true);  // see the generated C++ header for more info
 *     int Header;
 *     string Payload;
 *     char Trailer;
 *     int Frame_Type;
 *     int Ack_Nack_Num;
 * }
 * </pre>
 *
 * MyCustomMsg_Base is only useful if it gets subclassed, and MyCustomMsg is derived from it.
 * The minimum code to be written for MyCustomMsg is the following:
 *
 * <pre>
 * class MyCustomMsg : public MyCustomMsg_Base
 * {
 *   private:
 *     void copy(const MyCustomMsg& other) { ... }

 *   public:
 *     MyCustomMsg(const char *name=nullptr, short kind=0) : MyCustomMsg_Base(name,kind) {}
 *     MyCustomMsg(const MyCustomMsg& other) : MyCustomMsg_Base(other) {copy(other);}
 *     MyCustomMsg& operator=(const MyCustomMsg& other) {if (this==&other) return *this; MyCustomMsg_Base::operator=(other); copy(other); return *this;}
 *     virtual MyCustomMsg *dup() const override {return new MyCustomMsg(*this);}
 *     // ADD CODE HERE to redefine and implement pure virtual functions from MyCustomMsg_Base
 * };
 * </pre>
 *
 * The following should go into a .cc (.cpp) file:
 *
 * <pre>
 * Register_Class(MyCustomMsg)
 * </pre>
 */
class MyCustomMsg_Base : public ::omnetpp::cPacket
{
  protected:
    int Header;
    ::omnetpp::opp_string Payload;
    char Trailer;
    int Frame_Type;
    int Ack_Nack_Num;

  private:
    void copy(const MyCustomMsg_Base& other);

  protected:
    // protected and unimplemented operator==(), to prevent accidental usage
    bool operator==(const MyCustomMsg_Base&);
    // make constructors protected to avoid instantiation

    MyCustomMsg_Base(const MyCustomMsg_Base& other);
    // make assignment operator protected to force the user override it
    MyCustomMsg_Base& operator=(const MyCustomMsg_Base& other);

  public:
    MyCustomMsg_Base(const char *name=nullptr, short kind=0);
    virtual ~MyCustomMsg_Base();
    virtual MyCustomMsg_Base *dup() const override {
        return new MyCustomMsg_Base(*this);}
    virtual void parsimPack(omnetpp::cCommBuffer *b) const override;
    virtual void parsimUnpack(omnetpp::cCommBuffer *b) override;

    // field getter/setter methods
    virtual int getHeader() const;
    virtual void setHeader(int Header);
    virtual const char * getPayload() const;
    virtual void setPayload(const char * Payload);
    virtual char getTrailer() const;
    virtual void setTrailer(char Trailer);
    virtual int getFrame_Type() const;
    virtual void setFrame_Type(int Frame_Type);
    virtual int getAck_Nack_Num() const;
    virtual void setAck_Nack_Num(int Ack_Nack_Num);
};


#endif // ifndef __MYCUSTOMMSG_M_H

