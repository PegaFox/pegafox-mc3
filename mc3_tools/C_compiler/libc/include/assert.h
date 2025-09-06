#ifndef _ASSERT_H
#define _ASSERT_H 1

  #ifdef assert
    #undef assert
  #endif /* assert */

  #ifdef NDEBUG

    #define assert(ignore) ((void)0)

  #else /* NDEBUG */

    // assert definition goes here

  #endif /* NDEBUG */

  #ifdef static_assert
    #undef static_assert
  #endif /* static_assert */

  #define static_assert _Static_assert

#endif /* _ASSERT_H */