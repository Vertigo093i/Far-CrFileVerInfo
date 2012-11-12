/******************************************************************************
*
*  Creware File Version Info v1.3 plugin for FAR Manager 1.70+
*
*  Copyright (c) 2005 Creware (http://www.creware.com, support@creware.com)
*  Copyright (c) 2006 Alexander Ogorodnikov (anodyne@mail.ru)
*  Copyright (c) 2010 Andrew Nefedkin (andrew.nefedkin@gmail.com)
*
*  Displays file version information stored in the VERSIONINFO resource
*
******************************************************************************/

#define WIN32_LEAN_AND_MEAN

#include <plugin.hpp>

#if FARMANAGERVERSION_MAJOR >= 3
#include "guid.h"
#include "version.h"
#endif

#define PLUGIN_TITLE TEXT("Version Info")

#ifndef EXP_NAME
#if UNICODE
#if FARMANAGERVERSION_MAJOR >= 3
#define EXP_NAME(p) p##W
#else
#define EXP_NAME(p) _export p##W
#endif
#else
#define EXP_NAME(p) _export p
#endif
#endif

static struct PluginStartupInfo Info;
static struct FarStandardFunctions FSF;

typedef struct TAG_DIALOG
{
	TCHAR *Title;
	int nLines;
	TCHAR **Lines;
} DIALOG, *PDIALOG;

PDIALOG DialogCreate(const TCHAR* Title)
{
	PDIALOG pDlg = (PDIALOG) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(DIALOG) + (lstrlen(Title) + 1) * sizeof(TCHAR));

	pDlg->Title = (TCHAR *) ((DWORD) pDlg + sizeof(DIALOG));

	lstrcpy(pDlg->Title, Title);

	return pDlg;
}

BOOL DialogAddLine(PDIALOG pDlg, TCHAR* psz)
{
	HANDLE hHeap = GetProcessHeap();
	void* p;

	if (pDlg->nLines)
	{
		p = HeapReAlloc(hHeap, HEAP_ZERO_MEMORY, pDlg->Lines, (pDlg->nLines + 1) * sizeof(TCHAR *));
	}
	else
	{
		p = HeapAlloc(hHeap, HEAP_ZERO_MEMORY, sizeof(TCHAR *));
	}

	if (p == NULL) return FALSE;

	pDlg->Lines = (TCHAR **) p;

	pDlg->Lines[pDlg->nLines] = (TCHAR*) HeapAlloc(hHeap, HEAP_ZERO_MEMORY, (lstrlen(psz) + 1) * sizeof(TCHAR));

	lstrcpy(pDlg->Lines[pDlg->nLines], psz);

	pDlg->nLines++;

	return TRUE;
}

TCHAR* DialogAddLinePlaceholder(PDIALOG pDlg, SIZE_T nChars)
{
	HANDLE hHeap = GetProcessHeap();
	void* p;

	if (pDlg->nLines)
	{
		p = HeapReAlloc(hHeap, HEAP_ZERO_MEMORY, pDlg->Lines, (pDlg->nLines + 1) * sizeof(TCHAR *));
	}
	else
	{
		p = HeapAlloc(hHeap, HEAP_ZERO_MEMORY, sizeof(TCHAR *));
	}

	if (p == NULL) return NULL;

	pDlg->Lines = (TCHAR **) p;

	pDlg->Lines[pDlg->nLines] = (TCHAR *) HeapAlloc(hHeap, HEAP_ZERO_MEMORY, nChars * sizeof(TCHAR));

	return pDlg->Lines[pDlg->nLines++];
}

int DialogShow(PDIALOG pDlg)
{
	if ( pDlg->nLines == 0 ) return -1;

	TCHAR **Msg = (TCHAR **) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (pDlg->nLines + 2) * sizeof(TCHAR *));
	int    ndx = 0;

	if (Msg == NULL) return -2;

	Msg[ndx++] = pDlg->Title;

	for ( int i = 0; i < pDlg->nLines; i++ )
	{
		Msg[ndx++] = pDlg->Lines[i];
	}

	Msg[ndx++] = TEXT("\x01");

#if FARMANAGERVERSION_MAJOR >= 3
	return Info.Message(&PLUGIN_GUID, NULL, FMSG_LEFTALIGN | FMSG_MB_OK, NULL, Msg, ndx, 0);
#else
	return Info.Message(Info.ModuleNumber, FMSG_LEFTALIGN | FMSG_MB_OK, NULL, Msg, ndx, 0);
#endif
}

void DialogFree(PDIALOG pDlg)
{
	HANDLE hHeap = GetProcessHeap();

	for ( int i = 0; i < pDlg->nLines; i++ )
	{
		HeapFree(hHeap, 0, pDlg->Lines[i]);
	}

	if (pDlg->Lines) HeapFree(hHeap, 0, pDlg->Lines);

	HeapFree(hHeap, 0, pDlg);
}

void MsgBox(const TCHAR* pszMsg, const TCHAR* pszTitle, BOOL bError = FALSE)
{
	const TCHAR *Msg[] = {
		pszTitle,
		pszMsg,
		TEXT("\x01")
	};

#if FARMANAGERVERSION_MAJOR >= 3
	Info.Message(&PLUGIN_GUID, nullptr, (bError ? FMSG_WARNING : 0) | FMSG_LEFTALIGN | FMSG_MB_OK, NULL, Msg, ARRAYSIZE(Msg), 0);
#else
	Info.Message(Info.ModuleNumber, (bError ? FMSG_WARNING : 0) | FMSG_LEFTALIGN | FMSG_MB_OK, NULL, Msg, ARRAYSIZE(Msg), 0);
#endif
}

int GetLanguageName(LCID Locale, LPTSTR lpLCData, int cchData)
{
	static TCHAR pszLangIndependent[] = TEXT("language independent");

	if(Locale == 0)
	{
		if(cchData != 0) lstrcpy(lpLCData, pszLangIndependent);
		return sizeof(pszLangIndependent);
	}

	return GetLocaleInfo(Locale, LOCALE_SLANGUAGE, lpLCData, cchData);
}

void ProcessFile(const TCHAR *szFilePath, const TCHAR *szFileName)
{
	LPVOID  pData    = NULL;
	DWORD   dwHandle = 0;
	DWORD   dwLength = GetFileVersionInfoSize(szFilePath, &dwHandle);
	static PDIALOG pDlg     = NULL;

	if (dwLength)
	{
		pDlg = DialogCreate(szFileName);
		HANDLE hHeap = GetProcessHeap();

		if ( (pData = HeapAlloc(hHeap, HEAP_ZERO_MEMORY, dwLength)) != NULL )
		{
			VS_FIXEDFILEINFO* pFFI = NULL;
			UINT              siz  = 0;

			/*BOOL bResult =*/ GetFileVersionInfo(szFilePath, dwHandle, dwLength, pData);

			if (VerQueryValue(pData, TEXT("\\"), (LPVOID*)&pFFI, &siz))
			{
				TCHAR    sz[128];
				DWORD    dwFlags;
				TCHAR    *pszString;

				FSF.sprintf(sz, TEXT("File version      %d.%d.%d.%d"), 
					HIWORD(pFFI->dwFileVersionMS),
					LOWORD(pFFI->dwFileVersionMS),
					HIWORD(pFFI->dwFileVersionLS),
					LOWORD(pFFI->dwFileVersionLS) );
				DialogAddLine(pDlg, sz);

				FSF.sprintf(sz, TEXT("Product version   %d.%d.%d.%d"), 
					HIWORD(pFFI->dwProductVersionMS),
					LOWORD(pFFI->dwProductVersionMS),
					HIWORD(pFFI->dwProductVersionLS),
					LOWORD(pFFI->dwProductVersionLS) );
				DialogAddLine(pDlg, sz);

				switch(pFFI->dwFileType)
				{
				case VFT_APP:        pszString = TEXT("application"); break;
				case VFT_DLL:        pszString = TEXT("dynamic-link library"); break;
				case VFT_DRV:        pszString = TEXT("device driver"); break;
				case VFT_FONT:       pszString = TEXT("font"); break;
				case VFT_VXD:        pszString = TEXT("virtual device"); break;
				case VFT_STATIC_LIB: pszString = TEXT("static-link library"); break;
				default:             pszString = TEXT("unknown");
				}
				FSF.sprintf(sz, TEXT("File type         %s"), pszString);
				DialogAddLine(pDlg, sz);

				if (pFFI->dwFileType == VFT_DRV)
				{
					switch(pFFI->dwFileSubtype)
					{
					case VFT2_DRV_PRINTER:           pszString = TEXT("printer driver");
					case VFT2_DRV_KEYBOARD:          pszString = TEXT("keyboard driver");
					case VFT2_DRV_LANGUAGE:          pszString = TEXT("language driver");
					case VFT2_DRV_DISPLAY:           pszString = TEXT("display driver");
					case VFT2_DRV_MOUSE:             pszString = TEXT("mouse driver");
					case VFT2_DRV_NETWORK:           pszString = TEXT("network driver");
					case VFT2_DRV_SYSTEM:            pszString = TEXT("system driver");
					case VFT2_DRV_INSTALLABLE:       pszString = TEXT("installable driver");
					case VFT2_DRV_SOUND:             pszString = TEXT("sound driver");
					case VFT2_DRV_COMM:              pszString = TEXT("communications driver");
					case VFT2_DRV_INPUTMETHOD:       pszString = TEXT("input method editor");
					case VFT2_DRV_VERSIONED_PRINTER: pszString = TEXT("versioned printer driver");
					default:                         pszString = TEXT("unknown");
					}
					FSF.sprintf(sz, TEXT("File subtype      %s"), pszString);
					DialogAddLine(pDlg, sz);
				}
				else if (pFFI->dwFileType == VFT_FONT)
				{
					switch(pFFI->dwFileSubtype)
					{
					case VFT2_FONT_RASTER:   pszString = TEXT("raster font");
					case VFT2_FONT_VECTOR:   pszString = TEXT("vector font");
					case VFT2_FONT_TRUETYPE: pszString = TEXT("TrueType font");
					default:                 pszString = TEXT("unknown");
					}
					FSF.sprintf(sz, TEXT("File subtype      %s"), pszString);
					DialogAddLine(pDlg, sz);
				}
				else if (pFFI->dwFileType == VFT_VXD)
				{
					FSF.sprintf(sz, TEXT("Device ID         0x%04X"), pFFI->dwFileSubtype);
					DialogAddLine(pDlg, sz);
				}

				dwFlags = pFFI->dwFileFlags & pFFI->dwFileFlagsMask;
				if (dwFlags != 0)
				{
					lstrcpy(sz, TEXT("File flags       "));
					if (dwFlags & VS_FF_DEBUG)        lstrcat(sz, TEXT(" DEBUG"));
					if (dwFlags & VS_FF_INFOINFERRED) lstrcat(sz, TEXT(" INFOINFERRED"));
					if (dwFlags & VS_FF_PATCHED)      lstrcat(sz, TEXT(" PATCHED"));
					if (dwFlags & VS_FF_PRERELEASE)   lstrcat(sz, TEXT(" PRERELEASE"));
					if (dwFlags & VS_FF_PRIVATEBUILD) lstrcat(sz, TEXT(" PRIVATEBUILD"));
					if (dwFlags & VS_FF_SPECIALBUILD) lstrcat(sz, TEXT(" SPECIALBUILD"));
					DialogAddLine(pDlg, sz);
				}

				switch(pFFI->dwFileOS)
				{
				case VOS_DOS:           pszString = TEXT("MS-DOS"); break;
				case VOS_OS216:         pszString = TEXT("OS/2 (16 bit)"); break;
				case VOS_OS232:         pszString = TEXT("OS/2 (32 bit)"); break;
				case VOS_NT:
				case VOS_NT_WINDOWS32:  pszString = TEXT("Windows NT/2000/XP/2003"); break;
				case VOS_WINCE:         pszString = TEXT("Windows CE"); break;
				case VOS__WINDOWS16:    pszString = TEXT("Windows (16 bit)"); break;
				case VOS__PM16:         pszString = TEXT("PM (16 bit)"); break;
				case VOS__PM32:         pszString = TEXT("PM (32 bit)"); break;
				case VOS__WINDOWS32:    pszString = TEXT("Windows (32 bit)"); break;
				case VOS_DOS_WINDOWS16: pszString = TEXT("Windows (16 bit) with MS-DOS"); break;
				case VOS_DOS_WINDOWS32: pszString = TEXT("Windows (32 bit) with MS-DOS"); break;
				case VOS_OS216_PM16:    pszString = TEXT("OS/2 with PM (16 bit)"); break;
				case VOS_OS232_PM32:    pszString = TEXT("OS/2 with PM (32 bit)"); break;
				default:                pszString = TEXT("unknown");
				}
				FSF.sprintf(sz, TEXT("Operating system  %s"), pszString);
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

						FSF.sprintf(sz, TEXT("File date         %02d/%02d/%d  %02d:%02d:%02d.%03d"),
							stCreate.wDay, stCreate.wMonth, stCreate.wYear,
							stCreate.wHour, stCreate.wMinute, stCreate.wSecond, 
							stCreate.wMilliseconds);

						DialogAddLine(pDlg, sz);
					}
					else
					{
						DialogAddLine(pDlg, TEXT("File date         invalid"));
					}
				}

				// the pszStringName buffer must fit at least this string:
				// \StringFileInfo\12345678\OriginalFilename + 22 chars just in case
				TCHAR pszStringName[1 + 14 + 1 + 8 + 1 + 16 + 1 + 22] = TEXT("\\StringFileInfo\\");

				BOOL bStringTableFound = FALSE;

				// variable information block

				struct LANGANDCODEPAGE {
					WORD wLanguage;
					WORD wCodePage;
				} *pLangAndCodePage;

				if(VerQueryValue(pData, TEXT("\\VarFileInfo\\Translation"), (LPVOID*) &pLangAndCodePage, &siz))
				{
					static const TCHAR *pszPrompt = TEXT("Language          ");

					int cchLang = 0, i, cLang = siz / sizeof(LANGANDCODEPAGE);

					if ( cLang > 0 )
					{
						for ( i = 0; i < cLang; i++ )
							cchLang += GetLanguageName(pLangAndCodePage[i].wLanguage, NULL, 0);

						TCHAR *pszLang = DialogAddLinePlaceholder(pDlg, lstrlen(pszPrompt) + cchLang + cLang - 1);
						TCHAR *pszDest = pszLang;

						lstrcpy(pszDest, pszPrompt);
						pszDest += lstrlen(pszPrompt);

						for ( i = 0; i < cLang; i++ )
						{
							if ( i > 0 )
							{
								pszDest[-1] = TEXT(',');
								pszDest = CharNext(pszDest);
								*pszDest = TEXT(' ');
							}

							pszDest += GetLanguageName(pLangAndCodePage[i].wLanguage, pszDest, cchLang);

							if ( !bStringTableFound )
							{
								FSF.sprintf(&pszStringName[16], TEXT("%04x%04x"), pLangAndCodePage[i].wLanguage, pLangAndCodePage[i].wCodePage);

								if ( VerQueryValue(pData, pszStringName, (LPVOID *) &pszString, &siz) )
								{
									bStringTableFound = TRUE;
								}
							}
						}

#ifndef UNICODE
						CharToOem(pszLang, pszLang);
#endif
					}
					else
					{
						lstrcpy(sz, pszPrompt);
						lstrcat(sz, TEXT("unknown"));

						DialogAddLine(pDlg, sz);
					}
				}

				// string information block

				static const struct KEYVALUEPAIR {
					TCHAR *pszKey;
					TCHAR *pszValue;
				} KeyVal[] =
				{
					// .NET assembly version
					TEXT("Assembly Version"), TEXT("Assembly version  "),
					TEXT("FileVersion"),      TEXT("File version      "),
					TEXT("ProductVersion"),   TEXT("Product version   "),
					TEXT("FileDescription"),  TEXT("File description  "),
					TEXT("InternalName"),     TEXT("Internal name     "),
					TEXT("OriginalFilename"), TEXT("Original filename "),
					TEXT("PrivateBuild"),     TEXT("Private build     "),
					TEXT("SpecialBuild"),     TEXT("Special build     "),
					TEXT("ProductName"),      TEXT("Product name      "),
					TEXT("CompanyName"),      TEXT("Company name      "),
					// Found in some Adobe products (i.e. Flash Player installer)
					TEXT("CompanyWebsite"),   TEXT("Company website   "),
					TEXT("LegalCopyright"),   TEXT("Legal copyright   "),
					TEXT("LegalTrademarks"),  TEXT("Legal trademarks  "),
					TEXT("Comments"),         TEXT("Comments          "),
				};

				if ( !bStringTableFound )
				{
					// Try some common translations until first one is found

					TCHAR pszLangCP[][9] = 
					{
						TEXT("040904E4"), // English (US) and multilingual character set
						TEXT("040904B0"), // English (US) and Unicode character set
						TEXT("FFFFFFFF"), // system default language and current ANSI code page
						TEXT("FFFF04B0"), // system default language and Unicode character set
						TEXT("000004E4"), // no language and multilingual character set
						TEXT("000004B0")  // no language and Unicode character set
					};

					FSF.sprintf(pszLangCP[2], TEXT("%04X%04X"), GetSystemDefaultLangID(), GetACP());
					FSF.sprintf(pszLangCP[3], TEXT("%04X04B0"), GetSystemDefaultLangID());

					for ( int i = 0; i < ARRAYSIZE(pszLangCP); i++ )
					{
						lstrcpy(&pszStringName[16], pszLangCP[i]);
						bStringTableFound = VerQueryValue(pData, pszStringName, (LPVOID*) &pszString, &siz);
						if ( bStringTableFound )
							break;
					}
				}

				if ( bStringTableFound )
				{
					DialogAddLine(pDlg, TEXT("\x01"));

					pszStringName[16 + 8] = TEXT('\\');

					for ( int i = 0; i < ARRAYSIZE(KeyVal); i++ )
					{
						lstrcpy(&pszStringName[16 + 9], KeyVal[i].pszKey);

						if ( VerQueryValue(pData, pszStringName, (LPVOID*) &pszString, &siz) )
						{
#ifndef UNICODE
							TCHAR *pszSrc = pszString;
							while ( *pszSrc )
							{
								// '\xA9' - copyright, '\xAE' - registered trademark
								if ( *pszSrc == TEXT('\xA9') || *pszSrc == TEXT('\xAE') )
									siz += 2;

								pszSrc = CharNext(pszSrc);
							}
#endif

							int cchValue = lstrlen(KeyVal[i].pszValue);
							TCHAR* pszLine = DialogAddLinePlaceholder(pDlg, cchValue + siz);
							TCHAR *pszDest = pszLine;

							lstrcpy(pszDest, KeyVal[i].pszValue);

							pszDest += cchValue;

#ifdef UNICODE
							lstrcpy(pszDest, pszString);
#else
							pszSrc = pszString;
							while ( *pszSrc )
							{
								if ( *pszSrc == TEXT('\xA9') )
								{
									*pszDest = TEXT('(');
									pszDest = CharNext(pszDest);
									*pszDest = TEXT('C');
									pszDest = CharNext(pszDest);
									*pszDest = TEXT(')');
								}
								else if ( *pszSrc == TEXT('\xAE') )
								{
									*pszDest = TEXT('(');
									pszDest = CharNext(pszDest);
									*pszDest = TEXT('R');
									pszDest = CharNext(pszDest);
									*pszDest = TEXT(')');
								}
								else
								{
									*pszDest = *pszSrc;
								}

								pszSrc = CharNext(pszSrc);
								pszDest = CharNext(pszDest);
							}
#endif

#ifndef UNICODE
							CharToOem(pszLine, pszLine);
#endif
						}
					}
				}

				DialogShow(pDlg);
			}

			HeapFree(hHeap, 0, pData);
		}

		DialogFree(pDlg);
	}
	else
	{
		MsgBox(TEXT("No version information available"), szFileName);
	}
}

void ProcessPanelItem()
{
	struct PanelInfo PInfo = { sizeof(PanelInfo) };

#ifdef UNICODE
#if FARMANAGERVERSION_MAJOR >= 3
	Info.PanelControl(PANEL_ACTIVE, FCTL_GETPANELINFO, 0, &PInfo);
#else
	Info.Control(PANEL_ACTIVE, FCTL_GETPANELINFO, 0, (LONG_PTR) &PInfo);
#endif
#else
	Info.Control(INVALID_HANDLE_VALUE, FCTL_GETPANELINFO, &PInfo);
#endif // UNICODE

	if ( PInfo.PanelType != PTYPE_FILEPANEL )
	{
		MsgBox(TEXT("Only file panels are supported"), PLUGIN_TITLE, TRUE);

		return;
	}

	if ( !PInfo.ItemsNumber || PInfo.CurrentItem < 0 )
	{
		return;
	}

#ifdef UNICODE
#if FARMANAGERVERSION_MAJOR >= 3
	INT_PTR nPDirSize = Info.PanelControl(PANEL_ACTIVE, FCTL_GETPANELDIRECTORY, 0, NULL);
#else
	int nPDirSize = Info.Control(PANEL_ACTIVE, FCTL_GETPANELDIR, 0, NULL);
#endif
#else
	int nPDirSize = lstrlen(PInfo.CurDir);
#endif // UNICODE

	if ( nPDirSize )
	{
		HANDLE hHeap = GetProcessHeap();

#ifdef UNICODE
#if FARMANAGERVERSION_MAJOR >= 3
		INT_PTR nPPItemSize = Info.PanelControl(PANEL_ACTIVE, FCTL_GETCURRENTPANELITEM, 0, NULL);
#else
		int nPPItemSize = Info.Control(PANEL_ACTIVE, FCTL_GETCURRENTPANELITEM, 0, NULL);
#endif

		struct PluginPanelItem *PPItem = (PluginPanelItem*) HeapAlloc(hHeap, HEAP_ZERO_MEMORY, nPPItemSize);

#if FARMANAGERVERSION_MAJOR >= 3
		FarGetPluginPanelItem gpi = { nPPItemSize, PPItem };

		Info.PanelControl(PANEL_ACTIVE, FCTL_GETCURRENTPANELITEM, 0, &gpi);

		if ( (PPItem->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY )
		{
			struct FarPanelDirectory *PDir = (FarPanelDirectory*) HeapAlloc(hHeap, HEAP_ZERO_MEMORY, nPDirSize);

			PDir->StructSize = sizeof(FarPanelDirectory);

			Info.PanelControl(PANEL_ACTIVE, FCTL_GETPANELDIRECTORY, (int) nPDirSize, PDir);

			const TCHAR *pszFileName = PPItem->FileName;
			TCHAR *pszFilePath = (TCHAR *) HeapAlloc(hHeap, HEAP_ZERO_MEMORY, sizeof(TCHAR) * (lstrlen(PDir->Name) + 1 + lstrlen(pszFileName) + 1));

			lstrcpy(pszFilePath, PDir->Name);

			HeapFree(hHeap, 0, PDir);
#else
		Info.Control(PANEL_ACTIVE, FCTL_GETCURRENTPANELITEM, 0, (LONG_PTR) PPItem);

		if ( (PPItem->FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY )
		{
			const TCHAR *pszFileName = PPItem->FindData.lpwszFileName;
			TCHAR *pszFilePath = (TCHAR *) HeapAlloc(hHeap, HEAP_ZERO_MEMORY, sizeof(TCHAR) * (nPDirSize + 1 + lstrlen(pszFileName) + 1));

			Info.Control(PANEL_ACTIVE, FCTL_GETPANELDIR, nPDirSize, (LONG_PTR) pszFilePath);
#endif
#else
		if ( (PInfo.PanelItems[PInfo.CurrentItem].FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY )
		{
			TCHAR *pszFileName = PInfo.PanelItems[PInfo.CurrentItem].FindData.cFileName;
			TCHAR *pszFilePath = (TCHAR *) HeapAlloc(hHeap, HEAP_ZERO_MEMORY, sizeof(TCHAR) * (nPDirSize + 1 + lstrlen(pszFileName) + 1));

			lstrcpy(pszFilePath, PInfo.CurDir);
#endif // UNICODE

			if ( lstrlen(pszFilePath) > 0 )
			{
				FSF.AddEndSlash(pszFilePath);
			}

			lstrcat(pszFilePath, pszFileName);

			ProcessFile(pszFilePath, pszFileName);

			HeapFree(hHeap, 0, pszFilePath);
		}
#ifdef UNICODE
		HeapFree(hHeap, 0, PPItem);
#endif
	}
}

void WINAPI EXP_NAME(SetStartupInfo)(const struct PluginStartupInfo *Info)
{
	::Info = *Info;
	::FSF = *Info->FSF;
	::Info.FSF = &FSF;
}

#if FARMANAGERVERSION_MAJOR >= 3
void WINAPI GetGlobalInfoW(struct GlobalInfo *Info)
{
	Info->StructSize = sizeof(GlobalInfo);
	Info->MinFarVersion = MAKEFARVERSION(3, 0, 0, 2572, VS_RELEASE);
	Info->Version = PLUGIN_VERSION;
	Info->Guid = PLUGIN_GUID;
	Info->Title = PLUGIN_TITLE;
	Info->Description = PLUGIN_DESC;
	Info->Author = PLUGIN_AUTHOR;
}
#elif FARMANAGERVERSION_MAJOR == 2
int WINAPI GetMinFarVersionW()
{
	return MAKEFARVERSION(2, 0, 789); // 1145
}
#endif

void WINAPI EXP_NAME(GetPluginInfo)(struct PluginInfo *Info)
{
	static TCHAR* PluginMenuStrings[1];
	static TCHAR* CommandPrefix = TEXT("crver");

	Info->StructSize = sizeof(PluginInfo);
	//Info->Flags      = PF_EDITOR;

	PluginMenuStrings[0] = PLUGIN_TITLE;

#if FARMANAGERVERSION_MAJOR >= 3
	Info->PluginMenu.Guids = &PLUGIN_MENU_GUID;
	Info->PluginMenu.Strings = PluginMenuStrings;
	Info->PluginMenu.Count = ARRAYSIZE(PluginMenuStrings);
#else
	Info->PluginMenuStrings = PluginMenuStrings;
	Info->PluginMenuStringsNumber = ARRAYSIZE(PluginMenuStrings);
#endif

	Info->CommandPrefix = CommandPrefix;
}

#if FARMANAGERVERSION_MAJOR >= 3
HANDLE WINAPI OpenW(const struct OpenInfo *Info)
{
	if ( Info->OpenFrom == OPEN_COMMANDLINE )
	{
		TCHAR *cmd = (TCHAR *) Info->Data;
#else
HANDLE WINAPI EXP_NAME(OpenPlugin)(int OpenFrom, INT_PTR Item)
{
	if ( OpenFrom == OPEN_COMMANDLINE )
	{
		TCHAR *cmd = (TCHAR *) Item;
#endif

		cmd = FSF.Trim(cmd);

		if ( lstrlen(cmd) )
		{
			FSF.Unquote(cmd);
			MsgBox(cmd, PLUGIN_TITLE);
		}
		else
		{
			ProcessPanelItem();
		}
	}
	else
	{
		ProcessPanelItem();
	}

#if FARMANAGERVERSION_MAJOR >= 3
	return nullptr;
#else
	return INVALID_HANDLE_VALUE;
#endif
}

#if defined(__GNUC__)
#ifdef __cplusplus
extern "C"{
#endif
	BOOL WINAPI DllMainCRTStartup(HANDLE hDll, DWORD dwReason, LPVOID lpReserved);
#ifdef __cplusplus
};
#endif

BOOL WINAPI DllMainCRTStartup(HANDLE hDll, DWORD dwReason, LPVOID lpReserved)
{
	(void) lpReserved;
	(void) dwReason;
	(void) hDll;
	return TRUE;
}
#endif
