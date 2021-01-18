#pragma once

//#define NOCOLOR				(COLOR_BACKGROUND)
//#define NONLS					(MultiByteToWideChar)
//#define NOOPENFILE			(OPENFILENAME)
//#define NOCTLMGR
//#define WIN32_LEAN_AND_MEAN

#define NOGDICAPMASKS
#define NOSYSMETRICS
#define NOMENUS
#define NOICONS
#define NOSYSCOMMANDS
#define NORASTEROPS
#define OEMRESOURCE
#define NOATOM
#define NOCLIPBOARD
#define NODRAWTEXT
#define NOKERNEL
#define NOMEMMGR
#define NOMETAFILE
#define NOSCROLL
#define NOSERVICE
#define NOSOUND
#define NOTEXTMETRIC
#define NOWH
#define NOCOMM
#define NOKANJI
#define NOHELP
#define NOPROFILER
#define NODEFERWINDOWPOS
#define NOMCX
#define NORPC
#define NOPROXYSTUB
#define NOIMAGE
#define NOTAPE
#define NOMINMAX

#ifndef UNICODE
#define UNICODE
#endif

#include <Windows.h>

#undef near
#undef far