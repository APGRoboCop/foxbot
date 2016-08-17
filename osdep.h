/*
 *
 * This file keeps OS dependencies
 *
 */

#ifndef FB_OSDEP_H
#define FB_OSDEP_H

#ifdef __linux__

//#define _strinc(a) do{if (*(a)!='\0') a++;}while(0)
//#define _strinc(a) (a)++  // this version of _strinc was making g++ complain
#define _strinc(a) ((a) + 1) // this version of _strinc was copied from Dev-cpps _wcsinc
#define _snprintf snprintf

#endif /* __linux__ */

#endif /* FB_OSDEP_H */
