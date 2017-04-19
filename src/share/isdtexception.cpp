/*****************************************************************************/
/*M*
//                        THINKIT INTERNATIONAL PROPRIETARY INFORMATION
//        This software is supplied under the terms of the license agreement
//		or nondisclosure agreement with Thinkit International and may not be copied
//		or disclosed except in accordance with the terms of that agreement.
//            Copyright (c) 2002 ~ 2008 Thinkit International. All Rights Reserved.
//     VSS:
//     $Workfile:: isdtexception.cpp                     $
//     $Author:: Jjwang                               $
//     $Revision:: 1                                  $
//     $Modtime:: 11/09/13 20:39                      $
//     $NoKeywords: $
//
//
//
M*/

#include <string.h>
//#include "comm.h"
#include "isdtexception.h"
#include "iexception.h"

ISDT::ISDTException::ISDTException() throw()
    : exception(), whatIsHappend(NULL)
{
    whatIsHappend = new char[40];
    if (whatIsHappend) strcpy(whatIsHappend, "Internal ISDT exception ...");
}

ISDT::ISDTException::ISDTException(const char *what) throw()
    : exception(), whatIsHappend(NULL)
{
    size_t len = strlen(what)+1;
    if (len > MAX_EMSG_LENGTH || len < 1) len = MAX_EMSG_LENGTH;
    whatIsHappend = new char[len];
    if (whatIsHappend && what)
    {
        strncpy(whatIsHappend, what, len);
        whatIsHappend[len-1] = '\0';
    }
}

ISDT::ISDTException::ISDTException(const string &s) throw()
    : exception(), whatIsHappend(NULL)
{
    const char *what = s.c_str();
    size_t len = strlen(what)+1;
    if (len > MAX_EMSG_LENGTH || len < 1) len = MAX_EMSG_LENGTH;
    whatIsHappend = new char[len];
    if (whatIsHappend && what)
    {
        strncpy(whatIsHappend, what, len);
        whatIsHappend[len-1] = '\0';
    }
}

ISDT::ISDTException::~ISDTException() throw()
{
    if (whatIsHappend)
        delete whatIsHappend;
}
const char *ISDT::ISDTException::what() const throw()
{
    return whatIsHappend;
}

/*
ISDTException& ISDTException::operator= (const ISDTException &eSrc) throw()
{
  if (this == &eSrc) return *this;
  //*((exception*)this) = exception(eSrc); // temp obj???
  //exception::operator=(eSrc);            //???
  static_cast<exception&>(*this) = eSrc; //???

  strncpy(whatIsHappend, eSrc.whatIsHappend, MAX_EMSG_LENGTH);
  whatIsHappend[MAX_EMSG_LENGTH-1] = '\0'; //if overflowed
  return *this;
}

ISDTException::ISDTException(const ISDTException &eSrc) throw()
 : exception(eSrc)
{ this->operator=(eSrc);
}

void ISDT::UncaughtExceptionHandler() throw (UncaughtException)
{throw UncaughtException("Uncaught exception");}

*/

void ISDT::UnexpectedExceptionHandler() throw (UnexpectedException)
{
    throw UnexpectedException("Unexpected exception");
}

ISDT::BadAlloc::BadAlloc()                     throw() :ISDTException()     {}
ISDT::BadAlloc::BadAlloc(const char *what)     throw() :ISDTException(what) {}
ISDT::BadAlloc::BadAlloc(const string &s)      throw() :ISDTException(s)    {}


ISDT::BadCreate::BadCreate()                   throw() :ISDTException()     {}
ISDT::BadCreate::BadCreate(const char *what)   throw() :ISDTException(what) {}
ISDT::BadCreate::BadCreate(const string &s)    throw() :ISDTException(s)    {}


ISDT::RangeError::RangeError()                 throw() :ISDTException()     {}
ISDT::RangeError::RangeError(const char *what) throw() :ISDTException(what) {}
ISDT::RangeError::RangeError(const string &s)  throw() :ISDTException(s)    {}

//-------------------------   F i l e   O p e r a t i o n s  -------------------

ISDT::BadFileOperation::BadFileOperation()                 throw() :ISDTException()     {}
ISDT::BadFileOperation::BadFileOperation(const char *what) throw() :ISDTException(what) {}
ISDT::BadFileOperation::BadFileOperation(const string &s)  throw() :ISDTException(s)    {}


ISDT::BadFileOpen::BadFileOpen()                   throw() :BadFileOperation()     {}
ISDT::BadFileOpen::BadFileOpen(const char *what)   throw() :BadFileOperation(what) {}
ISDT::BadFileOpen::BadFileOpen(const string &s)    throw() :BadFileOperation(s)    {}


ISDT::BadFileClose::BadFileClose()                 throw() :BadFileOperation()     {}
ISDT::BadFileClose::BadFileClose(const char *what) throw() :BadFileOperation(what) {}
ISDT::BadFileClose::BadFileClose(const string &s)  throw() :BadFileOperation(s)    {}


ISDT::BadFileRead::BadFileRead()                   throw() :BadFileOperation()     {}
ISDT::BadFileRead::BadFileRead(const char *what)   throw() :BadFileOperation(what) {}
ISDT::BadFileRead::BadFileRead(const string &s)    throw() :BadFileOperation(s)    {}


ISDT::BadFileWrite::BadFileWrite()                 throw() :BadFileOperation()     {}
ISDT::BadFileWrite::BadFileWrite(const char *what) throw() :BadFileOperation(what) {}
ISDT::BadFileWrite::BadFileWrite(const string &s)  throw() :BadFileOperation(s)    {}

//------------------------- L o g i c  E r r o r s ---------------------------

ISDT::LogicError::LogicError()                     throw() :ISDTException()     {}
ISDT::LogicError::LogicError(const char *what)     throw() :ISDTException(what) {}
ISDT::LogicError::LogicError(const string &s)      throw() :ISDTException(s)    {}


