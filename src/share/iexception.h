/*****************************************************************************/
/*M*
//                        THINKIT INTERNATIONAL PROPRIETARY INFORMATION
//        This software is supplied under the terms of the license agreement
//		or nondisclosure agreement with Thinkit International and may not be copied
//		or disclosed except in accordance with the terms of that agreement.
//            Copyright (c) 2002 ~ 2008 Thinkit International. All Rights Reserved.
//     VSS:
//     $Workfile:: iexception.h                       $
//     $Author:: Jjwang                               $
//     $Revision:: 1                                  $
//     $Modtime:: 11/09/13 20:39                      $
//     $NoKeywords: $
//
//
//
M*/

#ifndef _ISDT_IEXCEPTIONS_
#define _ISDT_IEXCEPTIONS_

namespace ISDT
{
//----------------------------------------------------------------------------
//               For ISDT internal use only
//----------------------------------------------------------------------------
class UnexpectedException: public ISDTException
{
public:
    UnexpectedException(const char *what) throw(): ISDTException(what) {}
};

//class UncaughtException: public ISDTException
//{ public:
//   UncaughtException(const char *what) throw(): ISDTException(what) {}
//};


typedef void (*pVoidFunction)();
void UnexpectedExceptionHandler() throw (UnexpectedException);
//void UncaughtExceptionHandler() throw (UncaughtException);


class UnexpectedExceptionTrap
{
public:

    UnexpectedExceptionTrap()
    {
        old = set_unexpected(&UnexpectedExceptionHandler);
    }
    UnexpectedExceptionTrap(pVoidFunction f)
    {
        old = set_unexpected(f);
    }
    ~UnexpectedExceptionTrap()
    {
        set_unexpected(old);
    }

private:
    pVoidFunction old;

};

/*
class UncaughtExceptionTrap
{ public:

  UncaughtExceptionTrap() {old = set_terminate(&UncaughtExceptionHandler);}
  UncaughtExceptionTrap(pVoidFunction f) {old = set_terminate(f);}
  ~UncaughtExceptionTrap() {set_terminate(old);}

  private:
   pVoidFunction old;

};
*/
}; // namespace ISDT
using namespace ISDT;

#endif //_ISDT_IEXCEPTIONS_


