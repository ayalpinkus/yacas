/** \file stringio.h
 * definitions of input output classes that read and write from string.
 */

#ifndef YACAS_STRINGIO_H
#define YACAS_STRINGIO_H

#include "yacasbase.h"
#include "lispio.h"
#include "lispstring.h"

class StringInput : public LispInput
{
public:
    StringInput(LispString& aString,InputStatus& aStatus);
    virtual LispChar Next();
    virtual LispChar Peek();
    virtual bool EndOfStream();
    virtual const LispChar* StartPtr();
    virtual LispInt Position();
    virtual void SetPosition(LispInt aPosition);
protected:
    LispString iString;
    LispInt iCurrent;
};

class StringOutput : public LispOutput
{
public:
    StringOutput(LispString& aString);
    virtual ~StringOutput();
    virtual void PutChar(LispChar aChar);
public:
    LispString& iString;
};



#endif

