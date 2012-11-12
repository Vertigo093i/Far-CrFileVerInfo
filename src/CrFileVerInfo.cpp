/******************************************************************************
*
*  Creware File Version Info v1.2 plugin for FAR Manager 1.70+
*
*  Copyright (c) 2005 Creware (http://www.creware.com, support@creware.com)
*  Copyright (c) 2006 Alexander Ogorodnikov (anodyne@mail.ru)
*
*  Displays file version information stored in the VERSIONINFO resource
*
******************************************************************************/

#define WIN32_LEAN_AND_MEAN

#pragma pack(2)
#include <windows.h>
#pragma pack()
#include "plugin.hpp"

#pragma comment(lib, "Version.lib")

#define TITLE "Version Info"

static struct PluginStartupInfo Info;

typedef struct TAG_DIALOG
{
  char*   Title;
  int     nButtons;
  char**  Buttons;
  int     nLines;
  char**  Lines;
} DIALOG, * PDIALOG;

PDIALOG DialogCreate(char* Title)
{
  PDIALOG pDlg = (PDIALOG)GlobalAlloc(GPTR, sizeof(DIALOG) + lstrlenA(Title)+1);

  pDlg->Title = (char*)((DWORD)pDlg + sizeof(DIALOG));

  lstrcpyA(pDlg->Title, Title);

  return pDlg;
}

BOOL DialogAddLine(PDIALOG pDlg, char* psz)
{
  void* p;

  if (pDlg->nLines)
  {
    p = GlobalReAlloc((HGLOBAL)pDlg->Lines, sizeof(char*) * (pDlg->nLines+1), GMEM_MOVEABLE);
  }
  else
  {
    p = GlobalAlloc(GPTR, sizeof(char*));
  }

  if (p == NULL) return FALSE;

  pDlg->Lines = (char**)p;
  
  pDlg->Lines[pDlg->nLines] = (char*)GlobalAlloc(GPTR, lstrlenA(psz)+1);
  
  lstrcpyA(pDlg->Lines[pDlg->nLines], psz);
  
  pDlg->nLines++;

  return TRUE;
}

char* DialogAddLinePlaceholder(PDIALOG pDlg, size_t nBytes)
{
  void* p;

  if (pDlg->nLines)
  {
    p = GlobalReAlloc((HGLOBAL)pDlg->Lines, sizeof(char*) * (pDlg->nLines+1), GMEM_MOVEABLE);
  }
  else
  {
    p = GlobalAlloc(GPTR, sizeof(char*));
  }

  if (p == NULL) return NULL;

  pDlg->Lines = (char**)p;
  
  pDlg->Lines[pDlg->nLines] = (char*)GlobalAlloc(GPTR, nBytes);
  
  return pDlg->Lines[pDlg->nLines++];
}

BOOL DialogAddButton(PDIALOG pDlg, char* psz)
{
  void* p;

  if (pDlg->nButtons)
  {
    p = GlobalReAlloc((HGLOBAL)pDlg->Buttons, sizeof(char*) * (pDlg->nButtons+1), GMEM_MOVEABLE);
  }
  else
  {
    p = GlobalAlloc(GPTR, sizeof(char*));
  }

  if (p == NULL) return FALSE;

  pDlg->Buttons = (char**)p;
  
  pDlg->Buttons[pDlg->nButtons] = (char*)GlobalAlloc(GPTR, lstrlenA(psz)+1);
  
  lstrcpyA(pDlg->Buttons[pDlg->nButtons], psz);
  
  pDlg->nButtons++;

  return TRUE;
}

int DialogShow(PDIALOG pDlg, BOOL bError = FALSE)
{
  if ((pDlg->nButtons == 0) || (pDlg->nLines == 0)) return -1;

  char** Msg = (char**)GlobalAlloc(GPTR, sizeof(char*) * (pDlg->nButtons + pDlg->nLines + 2));
  int    ndx = 0;
  int    i;

  if (Msg == NULL) return -2;

  Msg[ndx++] = pDlg->Title;

  for (i = 0; i < pDlg->nLines; i++)
  {
    Msg[ndx++] = pDlg->Lines[i];
  }

  Msg[ndx++] = "\x01";

  for (i = 0; i < pDlg->nButtons; i++)
  {
    Msg[ndx++] = pDlg->Buttons[i];
  }

  return Info.Message(Info.ModuleNumber, FMSG_LEFTALIGN, "Contents", Msg, ndx, pDlg->nButtons);
}

void DialogFree(PDIALOG pDlg)
{
  int i;

  for (i = 0; i < pDlg->nButtons; i++)
  {
    GlobalFree((HGLOBAL)pDlg->Buttons[i]);
  }

  if (pDlg->Buttons) GlobalFree((HGLOBAL)pDlg->Buttons);

  for (i = 0; i < pDlg->nLines; i++)
  {
    GlobalFree((HGLOBAL)pDlg->Lines[i]);
  }

  if (pDlg->Lines) GlobalFree((HGLOBAL)pDlg->Lines);

  GlobalFree((HGLOBAL)pDlg);
}

void MsgBox(char* pszMsg, BOOL bError = FALSE)
{
  const char* Msg[4];

  Msg[0]=TITLE;
  Msg[1]=pszMsg;
  Msg[2]="\x01";
  Msg[3]="Ok";

  Info.Message(Info.ModuleNumber, (bError ? FMSG_WARNING : 0) + FMSG_LEFTALIGN,
    "Contents", Msg, 4, 1);
}

void WINAPI _export SetStartupInfo(const struct PluginStartupInfo *psi)
{
  Info=*psi;
}

void WINAPI _export GetPluginInfo(struct PluginInfo *pi)
{
  static char* PluginMenuStrings[1];

  pi->StructSize = sizeof(struct PluginInfo);
  //pi->Flags      = PF_EDITOR;

  PluginMenuStrings[0] = "Version Info";

  pi->PluginMenuStrings       = PluginMenuStrings;
  pi->PluginMenuStringsNumber = sizeof(PluginMenuStrings)/sizeof(PluginMenuStrings[0]);
}

int GetLanguageName(LCID Locale, LPTSTR lpLCData, int cchData)
{
    static char pszLangIndependent[] = "language independent";

    if(Locale == 0)
    {
        if(cchData != 0) lstrcpy(lpLCData, pszLangIndependent);
        return sizeof(pszLangIndependent);
    }

    return GetLocaleInfo(Locale, LOCALE_SLANGUAGE, lpLCData, cchData);
}

BOOL ProcessFile(char* szFilePath, char* szFileName)
{
  BOOL    bShown   = FALSE;
  LPVOID  pData    = NULL;
  DWORD   dwHandle = 0;
  DWORD   dwLength = GetFileVersionInfoSizeA(szFilePath, &dwHandle);
  PDIALOG pDlg     = NULL;

  if (dwLength)
  {
    pDlg = DialogCreate(TITLE);

    DialogAddButton(pDlg, "Ok");

    if (pData = GlobalAlloc(GMEM_FIXED, dwLength))
    {
      VS_FIXEDFILEINFO* pFFI = NULL;
      UINT              siz  = 0;

      BOOL bResult = GetFileVersionInfoA(szFilePath, dwHandle, dwLength, pData);

      if (VerQueryValueA(pData, "\\", (LPVOID*)&pFFI, &siz))
      {
        char       sz[128];
        DWORD      dwFlags;
        char       *pszString;

        wsprintfA(sz, "File version      %d.%d.%d.%d", 
                  HIWORD(pFFI->dwFileVersionMS),
                  LOWORD(pFFI->dwFileVersionMS),
                  HIWORD(pFFI->dwFileVersionLS),
                  LOWORD(pFFI->dwFileVersionLS) );
        DialogAddLine(pDlg, sz);

        wsprintfA(sz, "Product version   %d.%d.%d.%d", 
                  HIWORD(pFFI->dwProductVersionMS),
                  LOWORD(pFFI->dwProductVersionMS),
                  HIWORD(pFFI->dwProductVersionLS),
                  LOWORD(pFFI->dwProductVersionLS) );
        DialogAddLine(pDlg, sz);

        switch(pFFI->dwFileType)
        {
            case VFT_APP:        pszString = "application"; break;
            case VFT_DLL:        pszString = "dynamic-link library"; break;
            case VFT_DRV:        pszString = "device driver"; break;
            case VFT_FONT:       pszString = "font"; break;
            case VFT_VXD:        pszString = "virtual device"; break;
            case VFT_STATIC_LIB: pszString = "static-link library"; break;
            default:             pszString = "unknown";
        }
        wsprintf(sz, "File type         %s", pszString);
        DialogAddLine(pDlg, sz);

        if (pFFI->dwFileType == VFT_DRV)
        {
            switch(pFFI->dwFileSubtype)
            {
                case VFT2_DRV_PRINTER:           pszString = "printer driver";
                case VFT2_DRV_KEYBOARD:          pszString = "keyboard driver";
                case VFT2_DRV_LANGUAGE:          pszString = "language driver";
                case VFT2_DRV_DISPLAY:           pszString = "display driver";
                case VFT2_DRV_MOUSE:             pszString = "mouse driver";
                case VFT2_DRV_NETWORK:           pszString = "network driver";
                case VFT2_DRV_SYSTEM:            pszString = "system driver";
                case VFT2_DRV_INSTALLABLE:       pszString = "installable driver";
                case VFT2_DRV_SOUND:             pszString = "sound driver";
                case VFT2_DRV_COMM:              pszString = "communications driver";
                case VFT2_DRV_INPUTMETHOD:       pszString = "input method editor";
                case VFT2_DRV_VERSIONED_PRINTER: pszString = "versioned printer driver";
                default:                         pszString = "unknown";
            }
            wsprintf(sz, "File subtype      %s", pszString);
            DialogAddLine(pDlg, sz);
        }
        else if (pFFI->dwFileType == VFT_FONT)
        {
            switch(pFFI->dwFileSubtype)
            {
                case VFT2_FONT_RASTER:   pszString = "raster font";
                case VFT2_FONT_VECTOR:   pszString = "vector font";
                case VFT2_FONT_TRUETYPE: pszString = "TrueType font";
                default:                 pszString = "unknown";
            }
            wsprintf(sz, "File subtype      %s", pszString);
            DialogAddLine(pDlg, sz);
        }
        else if (pFFI->dwFileType == VFT_VXD)
        {
            wsprintf(sz, "Device ID         0x%04X", pFFI->dwFileSubtype);
            DialogAddLine(pDlg, sz);
        }

        dwFlags = pFFI->dwFileFlags & pFFI->dwFileFlagsMask;
        if (dwFlags != 0)
        {
            lstrcpy(sz, "File flags       ");
            if (dwFlags & VS_FF_DEBUG)        lstrcat(sz, " DEBUG");
            if (dwFlags & VS_FF_INFOINFERRED) lstrcat(sz, " INFOINFERRED");
            if (dwFlags & VS_FF_PATCHED)      lstrcat(sz, " PATCHED");
            if (dwFlags & VS_FF_PRERELEASE)   lstrcat(sz, " PRERELEASE");
            if (dwFlags & VS_FF_PRIVATEBUILD) lstrcat(sz, " PRIVATEBUILD");
            if (dwFlags & VS_FF_SPECIALBUILD) lstrcat(sz, " SPECIALBUILD");
            DialogAddLine(pDlg, sz);
        }

        switch(pFFI->dwFileOS)
        {
            case VOS_DOS:           pszString = "MS-DOS"; break;
            case VOS_OS216:         pszString = "OS/2 (16 bit)"; break;
            case VOS_OS232:         pszString = "OS/2 (32 bit)"; break;
            case VOS_NT:
            case VOS_NT_WINDOWS32:  pszString = "Windows NT/2000/XP/2003"; break;
            case VOS_WINCE:         pszString = "Windows CE"; break;
            case VOS__WINDOWS16:    pszString = "Windows (16 bit)"; break;
            case VOS__PM16:         pszString = "PM (16 bit)"; break;
            case VOS__PM32:         pszString = "PM (32 bit)"; break;
            case VOS__WINDOWS32:    pszString = "Windows (32 bit)"; break;
            case VOS_DOS_WINDOWS16: pszString = "Windows (16 bit) with MS-DOS"; break;
            case VOS_DOS_WINDOWS32: pszString = "Windows (32 bit) with MS-DOS"; break;
            case VOS_OS216_PM16:    pszString = "OS/2 with PM (16 bit)"; break;
            case VOS_OS232_PM32:    pszString = "OS/2 with PM (32 bit)"; break;
            default:                pszString = "unknown";
        }
        wsprintf(sz, "Operating system  %s", pszString);
        DialogAddLine(pDlg, sz);

        if (pFFI->dwFileDateLS | pFFI->dwFileDateMS)
        {
          FILETIME   ft;
          FILETIME   ftLocal;

          ft.dwHighDateTime = pFFI->dwFileDateMS;
          ft.dwLowDateTime  = pFFI->dwFileDateLS;
          
          if (FileTimeToLocalFileTime(&ft, &ftLocal))
          {
            SYSTEMTIME stCreate;

            FileTimeToSystemTime(&ftLocal, &stCreate);

            wsprintfA(sz, "File date         %02d/%02d/%d  %02d:%02d:%02d.%03d",
              stCreate.wDay, stCreate.wMonth, stCreate.wYear,
              stCreate.wHour, stCreate.wMinute, stCreate.wSecond, 
              stCreate.wMilliseconds);

            DialogAddLine(pDlg, sz);
          }
          else
          {
            DialogAddLine(pDlg, "File date         invalid");
          }
        }

        // string information block

        static struct KEYVALUEPAIR {
            char *pszKey;
            char *pszValue;
        } KeyVal[] =
        {
            "FileVersion",      "File version      ",
            "ProductVersion",   "Product version   ",
            "FileDescription",  "File description  ",
            "InternalName",     "Internal name     ",
            "OriginalFilename", "Original filename ",
            "PrivateBuild",     "Private build     ",
            "SpecialBuild",     "Special build     ",
            "ProductName",      "Product name      ",
            "CompanyName",      "Company name      ",
            "LegalCopyright",   "Legal copyright   ",
            "LegalTrademarks",  "Legal trademarks  ",
            "Comments",         "Comments          ",
        };

        char *pszLangCP[] = 
        {
            "040904E4", // English (US) and multilingual character set
            "040904B0", // English (US) and Unicode character set
            "FFFFFFFF", // system default language and current ANSI code page
            "FFFF04B0", // system default language and Unicode character set
            "000004E4", // no language and multilingual character set
            "000004B0"  // no language and Unicode character set
        };

        wsprintf(pszLangCP[2], "%08X", GetSystemDefaultLangID() << 16 | GetACP());
        *((DWORD *) pszLangCP[3]) = *((DWORD *) pszLangCP[2]);

        // the pszStringName buffer must fit at least this string:
        // \StringFileInfo\12345678\OriginalFilename + 22 chars just in case
        char pszStringName[1+14+1+8+1+16+1+22] = "\\StringFileInfo\\";

        BOOL bStringTableFound;

        for(int i = 0; i < sizeof(pszLangCP) / sizeof(pszLangCP[0]); i++)
        {
            lstrcpy(&pszStringName[16], pszLangCP[i]);
            bStringTableFound = VerQueryValue(pData, pszStringName, (LPVOID *) &pszString, &siz);
            if(bStringTableFound) break;
        }

        if(bStringTableFound)
        {
            DialogAddLine(pDlg, "\x01");

            pszStringName[16 + 8] = '\\';

            for(int i = 0; i < sizeof(KeyVal) / sizeof(KEYVALUEPAIR); i++)
            {
                lstrcpy(&pszStringName[16 + 9], KeyVal[i].pszKey);
                if(VerQueryValue(pData, pszStringName, (LPVOID *) &pszString, &siz))
                {
                    char *pszSrc = pszString - 1;
                    while(*++pszSrc)
                        // '\xA9' - copyright, '\xAE' - registered trademark
                        if(*pszSrc == '\xA9' || *pszSrc == '\xAE') siz += 2;

                    int cchValue = lstrlen(KeyVal[i].pszValue);
                    char* pszLine = DialogAddLinePlaceholder(pDlg, cchValue + siz);
                    char *pszDest = pszLine;

                    lstrcpy(pszDest, KeyVal[i].pszValue);
                    pszDest += cchValue;

                    pszSrc = pszString - 1;
                    while(*++pszSrc)
                        if(*pszSrc == '\xA9')
                        {
                            *pszDest++ = '('; *pszDest++ = 'C'; *pszDest++ = ')';
                        }
                        else if(*pszSrc == '\xAE')
                        {
                            *pszDest++ = '('; *pszDest++ = 'R'; *pszDest++ = ')';
                        }
                        else *pszDest++ = *pszSrc;

                    CharToOemA(pszLine, pszLine);
                }
            }
        }

        // variable information block

        struct LANGANDCODEPAGE {
            WORD wLanguage;
            WORD wCodePage;
        } *pLangAndCodePage;

        if(VerQueryValueA(pData, "\\VarFileInfo\\Translation", (LPVOID *) &pLangAndCodePage, &siz))
        {
            static char pszPrompt[] = "Language          ";

            int cchLang = 0, i, cLang = siz / sizeof(LANGANDCODEPAGE);

            for(i = 0; i < cLang; i++)
                cchLang += GetLanguageName(pLangAndCodePage[i].wLanguage, NULL, 0);

            char *pszLang = DialogAddLinePlaceholder(pDlg, sizeof(pszPrompt) - 1 + cchLang + cLang - 1);
            char *pszDest = pszLang;

            lstrcpy(pszDest, pszPrompt);
            pszDest += sizeof(pszPrompt) - 1;

            for(i = 0;; i++)
            {
                pszDest += GetLanguageName(pLangAndCodePage[i].wLanguage, pszDest, cchLang);
                if(i == cLang - 1) break;
                pszDest[-1] = ','; *pszDest++ = ' ';
            }

            CharToOemA(pszLang, pszLang);
        }

        DialogShow(pDlg);
      }

      GlobalFree((HGLOBAL)pData);
    }

    DialogFree(pDlg);
  }
  else
  {
    MsgBox("No version information available");
  }


  return bShown;
}

BOOL DoIt()
{
  struct PanelInfo PInfo;
  
  Info.Control(INVALID_HANDLE_VALUE, FCTL_GETPANELINFO, &PInfo);

  if (PInfo.PanelType != PTYPE_FILEPANEL) return TRUE;

  if (PInfo.ItemsNumber && (PInfo.CurrentItem >= 0))
  {
    int nLen = lstrlenA(PInfo.CurDir);

    if (nLen)
    {
      char  szFilePath[MAX_PATH];
      char* pszFileName;

      lstrcpyA(szFilePath, PInfo.CurDir);
      if (szFilePath[nLen-1] != '\\')
      {
        lstrcatA(szFilePath, "\\");
      }

      pszFileName = PInfo.PanelItems[PInfo.CurrentItem].FindData.cFileName;

      lstrcatA(szFilePath, pszFileName);

      return ProcessFile(szFilePath, pszFileName);
    }
  }

  return TRUE;
}

HANDLE WINAPI _export OpenPlugin(int OpenFrom,int item)
{
  try
  {
    //HANDLE hScreen=Info.SaveScreen(0,0,-1,-1);

    DoIt();

    //Info.RestoreScreen(hScreen);
    //Info.Control(INVALID_HANDLE_VALUE,FCTL_UPDATEPANEL,NULL);
    //Info.Control(INVALID_HANDLE_VALUE,FCTL_REDRAWPANEL,NULL);
  }
  catch (...)
  {
    MsgBox("Exception catched", TRUE);
  }

  return  INVALID_HANDLE_VALUE;
}

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved)
{
/*
  switch (ul_reason_for_call)
  {
  case DLL_PROCESS_DETACH:
    break;
  }
*/
  return TRUE;
}

