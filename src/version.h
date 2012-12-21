#include "farversion.hpp"

#define PLUGIN_MAJOR_VER 1
#define PLUGIN_MINOR_VER 3
#if FARMANAGERVERSION_MAJOR >= 3
#define PLUGIN_BUILD 1
#else
#define PLUGIN_BUILD 0
#endif
#define PLUGIN_DESC TEXT("Creware FileVersionInfo Plugin")
#define PLUGIN_NAME TEXT("CrFileVerInfo")
#define PLUGIN_FILENAME TEXT("CrFileVerInfo.dll")
#define PLUGIN_COPYRIGHT TEXT("© 2005 Creware, © 2006 Alexander Ogorodnikov, © 2010-2012 Andrew Nefedkin")
#define PLUGIN_AUTHOR TEXT("Creware, Alexander Ogorodnikov, Andrew Nefedkin")
#define PLUGIN_VERSION MAKEFARVERSION(PLUGIN_MAJOR_VER, PLUGIN_MINOR_VER, PLUGIN_BUILD, FARMANAGERVERSION_BUILD, VS_RELEASE)
