#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([the64], [1.1], [info@retrogames.biz])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_CPP
AM_PROG_AR

AC_CHECK_TOOL(STRIP, strip, :)

# Checks for libraries.

AC_SUBST([ALSA_CFLAGS])
AC_SUBST([ALSA_LIBS])

PKG_CHECK_MODULES([ALSA],[alsa])
PKG_CHECK_MODULES([EVDEV],[libevdev])
PKG_CHECK_MODULES([HARFBUZZ],[harfbuzz])
PKG_CHECK_MODULES([ZLIB],[zlib])
PKG_CHECK_MODULES([GLESV2],[glesv2])
PKG_CHECK_MODULES([EGL],[egl])
PKG_CHECK_MODULES([PNG],[libpng16])
PKG_CHECK_MODULES([FREETYPE2],[freetype2])
PKG_CHECK_MODULES([JPEG], [libjpeg], [
    AC_DEFINE([HAVE_LIBJPEG], 1, [Define to 1 if you have jpeg.])
], [
    AC_CHECK_LIB([jpeg], [jpeg_read_scanlines],
        AC_DEFINE([HAVE_LIBJPEG], 1, [Define to 1 if you have jpeg.]) AC_SUBST([JPEG_LIBS], [-ljpeg]),
        AC_MSG_ERROR([jpeg support requested but library not found])
    )]
)

#AC_PATH_X

RESID_USE_SSE=0

    AC_SUBST([CONF_CFLAGS], ["-march=armv7-a -mtune=cortex-a7"])

AC_ARG_WITH(flashlight,[  --with-flashlight Compile with flashlight library.],[flprefix=$withval flf="yes"],[flf="no"])
if test $flf = 'yes'; then
    AC_SUBST([FLASHLIGHT_LIB], [${flprefix}])
    AC_SUBST([FLASHLIGHT_INC], [${flprefix}"/include"])
fi
AC_ARG_WITH(maliaw,[  --with-maliaw Compile with maliaw library.],[mawprefix=$withval mawf="yes"],[mawf="no"])
if test $mawf = 'yes'; then
    AC_SUBST([MALIAW_LIB], [${mawprefix}])
    AC_SUBST([MALIAW_INC], [${mawprefix}"/include"])
fi
    
dnl resid-fp stuff
RESID_HAVE_BOOL=1
AC_SUBST(RESID_HAVE_BOOL)

if test x"$host_os" != "xminix" -a x"$host_os" != "xmint"; then
  AC_CHECK_FUNCS(logf expf)
fi

AC_MSG_CHECKING([if the logf prototype is present])
AC_TRY_COMPILE([#include <math.h>
                #include <stdio.h>],
               [printf("%d",logf);],
               [ AC_MSG_RESULT(yes)
                 HAVE_LOGF_PROTOTYPE=1
                ],
               [ AC_MSG_RESULT(no)
                HAVE_LOGF_PROTOTYPE=0
                ])

AC_MSG_CHECKING([if the expf prototype is present])
AC_TRY_COMPILE([#include <math.h>
                #include <stdio.h>],
               [printf("%d",expf);],
               [ AC_MSG_RESULT(yes)
                 HAVE_EXPF_PROTOTYPE=1
                ],
               [ AC_MSG_RESULT(no)
                HAVE_EXPF_PROTOTYPE=0
                ])

RESID_INLINE=inline
AC_SUBST(HAVE_LOGF_PROTOTYPE)
AC_SUBST(HAVE_EXPF_PROTOTYPE)
AC_SUBST(RESID_INLINE)
AC_SUBST(RESID_USE_SSE)
AC_OUTPUT(src/vice2/src/resid-fp/siddefs-fp.h)

dnl General outputs
AC_OUTPUT(Makefile
src/ui/Makefile src/ui/gamelibrary/Makefile
src/input/Makefile src/common/Makefile src/freetype/Makefile 
src/viceport/Makefile
src/vbuild/Makefile.libc64cart
src/vbuild/Makefile.libc64cartsystem
src/vbuild/Makefile.libc64commoncart
src/vbuild/Makefile.libc64
src/vbuild/Makefile.libcore
src/vbuild/Makefile.libdiskimage
src/vbuild/Makefile.libdriveiecc64exp
src/vbuild/Makefile.libdriveiecieee
src/vbuild/Makefile.libdriveiec
src/vbuild/Makefile.libdriveieee
src/vbuild/Makefile.libdrive
src/vbuild/Makefile.libfileio
src/vbuild/Makefile.libfsdevice
src/vbuild/Makefile.libgfxoutputdrv
src/vbuild/Makefile.libiecbus
src/vbuild/Makefile.libimagecontents
src/vbuild/Makefile.libmonitor
src/vbuild/Makefile.libp64
src/vbuild/Makefile.libparallel
src/vbuild/Makefile.libplatform
src/vbuild/Makefile.libprinterdrv
src/vbuild/Makefile.libraster
src/vbuild/Makefile.libresidfp
src/vbuild/Makefile.libresid
src/vbuild/Makefile.librs232drv
src/vbuild/Makefile.librtc
src/vbuild/Makefile.libserial
src/vbuild/Makefile.libsid
src/vbuild/Makefile.libsounddrv
src/vbuild/Makefile.libtape
src/vbuild/Makefile.libuserport
src/vbuild/Makefile.libvdrive
src/vbuild/Makefile.libvicemain
src/vbuild/Makefile.libvicii
src/vbuild/Makefile.libvideo
src/vbuild/Makefile
)
