#ifndef _VERSION_H_
#define _VERSION_H_
/*
 *
 * File: version.h
 * Date: 21.09.2001
 * Project: Foxbot
 *
 * Description: This file provides version information for Foxbot
 *
 * Copyright (c) 2001, Tom Simpson
 *
 *
 * $Id: version.h,v 1.2 2002/04/21 10:52:50 fb_cvs Exp $
 *
 */

/*
 * Build type, either debugging or optimized.
 * This is set by the Makefile. Provide a default
 * in case is isn't defined by the Makefile
 *
 */
#ifndef OPT_TYPE
# if defined(_MSC_VER) && defined(_DEBUG)
#  define OPT_TYPE	  "debugging"
# elif defined(_MSC_VER) && defined(_NDEBUG)
#  define OPT_TYPE	  "optimized"
# else
#  define OPT_TYPE	  "default"
# endif /* _MSC_VER */
#endif


/*
 * Version number.
 * This is also defined by the Makefile.
 * If not, we provide it here.
 */
#ifndef VERSION
#  define VERSION 0.78-b3
#endif

/*
 * Version type.
 * This is also defined by the Makefile.
 * If not, we provide it here.
 */
#ifndef MOD_VERSION
#  ifdef USE_METAMOD
#      define MOD_VERSION VERSION " (MM)"
#  else
#      define MOD_VERSION VERSION
#  endif
#endif


/*
 * We keep the compile time and date in a static string.
 * This info gets updated on every link, indicating the
 * latest time and date the dll was compiled and linked.
 */
extern char *COMPILE_DTTM;
extern char *COMPILE_DATE;


/*
 * We can also provide the timezone. It gets set in the
 * Makefile. If not, we can provide it here.
 */
#ifndef TZONE
#  define TZONE ""
#endif

/*
 * This info is used as Plugin info by Metamod
 */
#define VDATE		COMPILE_DATE
#define VNAME		"FoxBot"
#define VAUTHOR		"Tom Simpson, RoboCop <robocop@lycos.co.uk>"
#define VURL		"http://www.apg-clan.org"


#endif /* _VERSION_H_ */
