#ifndef _STDDEF_H
#define _STDDEF_H 1

  #define NULL ((void*)0)

  #ifndef _NTYPEDEF

    typedef signed int ptrdiff_t;
    typedef unsigned short int size_t;
    typedef int wchar_t;

  #else

    //#define ptrdiff_t signed int
    //#define size_t unsigned short int
    //#define wchar_t int

  #endif /*_NTYPEDEF*/

  // define offsetof macro here

#endif /* _STDDEF_H */
