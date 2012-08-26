/*
	TrayRSS v0.1
	Waits arounnd in the tray and periodically checks specified RSS feeds. 	
	Shows baloon prompts when it finds new content
	On a click, will open the related URL in the default browser
	Based on USBVirusScan by Didier Stevens (https://DidierStevens.com)
	Source code put in public domain by Arun Tanksali, Creative Commons License
	Use at your own risk

	History:
	15/07/2012: Start development
	17/07/2012: Removed references to USB stuff
	17/07/2012: Generalized Shell Notify, with version of Shell32 being detected dynamically
*/

#define _WIN32_IE 0x0500

#undef UNICODE

#include <windows.h>
#include <dbt.h>
#include <direct.h>
#include <stdio.h>
#include <shlwapi.h>

#include "TrayRSSrc.h"
#include "RSSFetch.h"

#define TRAYRSS "TrayRSS"
#define TRAYRSSCLASS "TrayRSSClass"
#define URL "http://tanksali.com"
#define VERSION "0.1"

#define SWM_TRAYMSG	WM_APP
#define SWM_ABOUT		WM_APP + 1//	about
#define SWM_EXIT		WM_APP + 2//	exit the program

#define TRAY_ICON_ID	2110;
#define PACKVERSION(major,minor) MAKELONG(minor,major)


char szScanCommand[1024];
char szAbout[1024];
int iFlagHideConsole;
DWORD dwVer, dwTarget;
DWORD nid_cbsize;





// Function prototype
LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void TrayIconAdd(HWND, HINSTANCE);
void TrayIconBalloon(HWND, char *, char *, UINT, DWORD);
void TrayIconDelete(HWND);
void ShowContextMenu(HWND);
VOID CALLBACK TimerRoutine(PVOID lpParam, BOOLEAN TimerOrWaitFired);
DWORD GetShellVersion();


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, INT nCmdShow)
{
	MSG					msg;					// MSG structure to store messages
	HWND				hwndMain;			// Main window handle
	HANDLE	hTimerQ, hTimer;
	WNDCLASSEX	wcx;					// WINDOW class information	
	char *pchIter;
	int iParseState;
	DLLVERSIONINFO dv;

	// GUID generated on 28/09/2006 by Didier Stevens for this application: 2121fde7-4050-4ecf-9090-d9c357b1caf7
	// Generate your own GUID when you repurpose this program
	GUID FilterGUID = {0x2121fde7, 0x4050, 0x4ecf, {0x90, 0x90, 0xd9, 0xc3, 0x57, 0xb1, 0xca, 0xf7}};    		
	
	sprintf(szAbout, "%s v%s %s", TRAYRSS, VERSION, URL);

		
	
	// Get command line
	if (lpCmdLine[0] == '\0') {
		MessageBox (NULL,
							  "TrayRSS requires the URL to check as a command line parameter\n"
							  "Usage: TrayRSS RSS-URL\n" ,
							  szAbout,
							  MB_OK);
		return 0;
	}
	else
	{
		pchIter = lpCmdLine;
		iParseState = 0;
		iFlagHideConsole = 0;

		do {
			if (*pchIter == ' ' && iParseState == 0)
			{
			}
			else if (*pchIter == ' ' && iParseState == 2)
			{
				iParseState = 0;
			}
			else if (*pchIter == '-' && iParseState == 0)
			{
				iParseState = 1;
			}
			else if (*pchIter == 'i' && (iParseState == 1 || iParseState == 2))
			{
				iParseState = 2;
			}
			else if (*pchIter == 'c' && (iParseState == 1 || iParseState == 2))
			{
				iParseState = 2;
				iFlagHideConsole = 1;
			} else
				break;
		} while (*pchIter++ != '\0');
		if (*pchIter != '\0')
			strncpy (szScanCommand, pchIter, 1024);
	}

	dv.cbSize = sizeof(DLLVERSIONINFO);


	dwVer = GetShellVersion();
	dwTarget = PACKVERSION(6,0);

	if(dwVer > dwTarget)
	{
		nid_cbsize = sizeof(NOTIFYICONDATA);
	}
	else
	{
		nid_cbsize = NOTIFYICONDATA_V3_SIZE; 
	}

	// Initialize the struct to zero
	ZeroMemory(&wcx, sizeof(wcx));
	
	wcx.cbSize = sizeof(wcx);								// Window size. Must always be sizeof(WNDCLASSEX)
	wcx.style = 0 ;													// Class styles
	wcx.lpfnWndProc = (WNDPROC)MainWndProc; // Pointer to the callback procedure
	wcx.cbClsExtra = 0;											// Extra byte to allocate following the wndclassex structure
	wcx.cbWndExtra = 0;											// Extra byte to allocate following an instance of the structure
	wcx.hInstance = hInstance;							// Instance of the application
	wcx.hIcon = NULL;												// Class Icon
	wcx.hCursor = NULL;											// Class Cursor
	wcx.hbrBackground = NULL;								// Background brush
	wcx.lpszMenuName = NULL;								// Menu resource
	wcx.lpszClassName = TRAYRSSCLASS;	// Name of this class
	wcx.hIconSm = NULL;											// Small icon for this class

	// Register this window class with MS-Windows
	if (!RegisterClassEx(&wcx))
		return 0;

	// Create the window
	hwndMain = CreateWindowEx(0,									// Extended window style
								TRAYRSSCLASS,	// Window class name
								"",									// Window title
								WS_POPUP,						// Window style
								0,0,								// (x,y) pos of the window
								0,0,								// Width and height of the window
								NULL,								// HWND of the parent window (can be null also)
								NULL,								// Handle to menu
								hInstance,					// Handle to application instance
								NULL);							// Pointer to window creation data

	// Check if window creation was successful
	if (!hwndMain)
		return 0;

	// Make the window invisible
	ShowWindow(hwndMain, SW_HIDE);

	TrayIconAdd(hwndMain, hInstance);

	hTimerQ = CreateTimerQueue();
	if(hTimerQ){
		if (!CreateTimerQueueTimer(&hTimer,hTimerQ,TimerRoutine,hwndMain,10000,30000,WT_EXECUTEDEFAULT)){
			MessageBox(NULL,"Timer creation failed",szAbout,MB_OK);
		}
	} 
	else {
			MessageBox(NULL,"Timer Q creation failed",szAbout,MB_OK);
	}
	
	  // Process messages coming to this window
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	// return value to the system
	DeleteTimerQueue(hTimerQ);

	return msg.wParam;
 }

LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
			case SWM_EXIT:
				DestroyWindow(hwnd);
				break;
			case SWM_ABOUT:
				MessageBox (NULL,
									"TrayRSS v0.1\n" ,
							  		szAbout,
							  		MB_OK);
				break;
		}
		return 1;

    case WM_DESTROY:
    	  TrayIconDelete(hwnd);
		   PostQuitMessage(0);
      break;

		case SWM_TRAYMSG:
			switch(lParam)
			{
				case WM_LBUTTONDBLCLK:
					break;
				case WM_RBUTTONDOWN:
				case WM_CONTEXTMENU:
					ShowContextMenu(hwnd);
			}
        
		default:
			// Call the default window handler
			return DefWindowProc(hwnd, msg, wParam, lParam);
	}

	return 0;
}

void TrayIconAdd(HWND hwnd, HINSTANCE hInstance)
{
	NOTIFYICONDATA nid;
	char szTip[1024];


  ZeroMemory(&nid, sizeof(NOTIFYICONDATA));

  nid.cbSize = nid_cbsize;
  nid.uID = TRAY_ICON_ID;
  nid.uFlags = NIF_ICON|NIF_MESSAGE|NIF_TIP;
  nid.hIcon = (HICON)LoadImage(hInstance, MAKEINTRESOURCE(IDI_MY_ICON), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
  nid.hWnd = hwnd;
  nid.uCallbackMessage = SWM_TRAYMSG;
  sprintf(szTip, "%s v%s", TRAYRSS, VERSION);
  strcpy(nid.szTip, szTip);
  Shell_NotifyIcon(NIM_ADD, &nid);
	if (nid.hIcon && DestroyIcon(nid.hIcon))
		nid.hIcon = NULL;
}

void TrayIconBalloon(HWND hwnd, char *szMessage, char *szTitle, UINT uTimeout, DWORD dwInfoFlags)
{
	NOTIFYICONDATA nid;

  ZeroMemory(&nid, sizeof(NOTIFYICONDATA));
  nid.cbSize = nid_cbsize;
  nid.uID = TRAY_ICON_ID;
	nid.uFlags = NIF_INFO;
  nid.hWnd = hwnd;
	nid.uTimeout = uTimeout;
	nid.dwInfoFlags = dwInfoFlags;
	strcpy(nid.szInfo, (szMessage));
	strcpy(nid.szInfoTitle, (szTitle));
	Shell_NotifyIcon(NIM_MODIFY, &nid);
}

void TrayIconDelete(HWND hwnd)
{
	NOTIFYICONDATA nid;

  ZeroMemory(&nid, sizeof(NOTIFYICONDATA));
  nid.cbSize = nid_cbsize;
  nid.uID = TRAY_ICON_ID;
  nid.hWnd = hwnd;
  Shell_NotifyIcon(NIM_DELETE, &nid);
}

void ShowContextMenu(HWND hWnd)
{
	POINT pt;
	HMENU hMenu;
	
	GetCursorPos(&pt);
	hMenu = CreatePopupMenu();
	if (hMenu)
	{
		InsertMenu(hMenu, -1, MF_BYPOSITION, SWM_ABOUT, ("About"));
		InsertMenu(hMenu, -1, MF_BYPOSITION, SWM_EXIT, ("Exit"));
		// note:	must set window to the foreground or the
		//			menu won't disappear when it should
		SetForegroundWindow(hWnd);
		TrackPopupMenu(hMenu, TPM_BOTTOMALIGN, pt.x, pt.y, 0, hWnd, NULL );
		DestroyMenu(hMenu);
	}
}

VOID CALLBACK TimerRoutine(PVOID lpParam, BOOLEAN TimerOrWaitFired)
{
	char szBalloon[128];

    if (lpParam == NULL)
    {
        //printf("TimerRoutine lpParam is NULL\n");
    }
    else
    {
        // lpParam points to the argument; in this case it is an int

        //printf("Timer routine called. Parameter is %d.\n", 
        //        *(int*)lpParam);
		sprintf(szBalloon, "Timer event signaled");
		
		TrayIconBalloon((HWND)lpParam, szBalloon, TRAYRSS, 5, NIIF_WARNING);

		 wg(L"www.google.com");

        if(TimerOrWaitFired)
        {
            //printf("The wait timed out.\n");
        }
        else
        {
            //printf("The wait event was signaled.\n");
        }
    }

}

DWORD GetShellVersion()
{
    HINSTANCE hinstDll;
    DWORD dwVersion = 0;
	LPCTSTR lpszDllName = "C:\\Windows\\System32\\Shell32.dll";

    /* For security purposes, LoadLibrary should be provided with a fully qualified 
       path to the DLL. The lpszDllName variable should be tested to ensure that it 
       is a fully qualified path before it is used. */
    hinstDll = LoadLibrary(lpszDllName);
	
    if(hinstDll)
    {
        DLLGETVERSIONPROC pDllGetVersion;
        pDllGetVersion = (DLLGETVERSIONPROC)GetProcAddress(hinstDll, "DllGetVersion");

        /* Because some DLLs might not implement this function, you must test for 
           it explicitly. Depending on the particular DLL, the lack of a DllGetVersion 
           function can be a useful indicator of the version. */

        if(pDllGetVersion)
        {
            DLLVERSIONINFO dvi;
            HRESULT hr;

            ZeroMemory(&dvi, sizeof(dvi));
            dvi.cbSize = sizeof(dvi);

            hr = (*pDllGetVersion)(&dvi);

            if(SUCCEEDED(hr))
            {
               dwVersion = PACKVERSION(dvi.dwMajorVersion, dvi.dwMinorVersion);
            }
        }
        FreeLibrary(hinstDll);
    }
    return dwVersion;
}
