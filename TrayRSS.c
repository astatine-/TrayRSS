/*
	TrayRSS 
	Waits arounnd in the tray and periodically checks specified RSS feeds. 	
	Shows baloon prompts when it finds new content
	On a click, will open the related URL in the default browser
	Inspired by USBVirusScan by Didier Stevens (https://DidierStevens.com)
	Uses librss (author not known)
	Source code under CC-BY-SA 3.0 license, Arun Tanksali
	Use at your own risk
*/

#include "trayrss.h"
#include "TrayRSSrc.h"
#include "rss.h"


DWORD nid_cbsize;


// Function prototype
LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void TrayIconAdd(HWND, HINSTANCE);
void TrayIconBalloon(HWND, RSS_char *, RSS_char *, UINT, DWORD);
void TrayIconDelete(HWND);
void ShowContextMenu(HWND);
VOID CALLBACK TimerRoutine(PVOID lpParam, BOOLEAN TimerOrWaitFired);
DWORD GetShellVersion();
void LaunchBrowser(RSS_char *command);
RSS_char *RSS_time2age(double tsince, int szAgeStr, RSS_char *age);
void test_error_handler(RSS_u32 error_level, const RSS_char* error, size_t pos);
void test_feed_walk(const RSS_char* url);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, INT nCmdShow)
{
	MSG	msg;					
	HWND	hwndMain;			// Main window handle
	HANDLE	hTimerQ, hTimer;
	WNDCLASSEX	wcx;			// WINDOW class information	
	DWORD dwVer, dwTarget;
	WSADATA wsaData;
	int iResult;


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
	
	wcx.cbSize = sizeof(wcx);		// Window size. Must always be sizeof(WNDCLASSEX)
	wcx.style = 0 ;				// Class styles
	wcx.lpfnWndProc = (WNDPROC)MainWndProc; // Pointer to the callback procedure
	wcx.cbClsExtra = 0;			// Extra byte to allocate following the wndclassex structure
	wcx.cbWndExtra = 0;			// Extra byte to allocate following an instance of the structure
	wcx.hInstance = hInstance;		// Instance of the application
	wcx.hIcon = NULL;			// Class Icon
	wcx.hCursor = NULL;			// Class Cursor
	wcx.hbrBackground = NULL;		// Background brush
	wcx.lpszMenuName = NULL;		// Menu resource
	wcx.lpszClassName = TRAYRSSCLASS;	// Name of this class
	wcx.hIconSm = NULL;			// Small icon for this class

	// Register this window class with MS-Windows
	if (!RegisterClassEx(&wcx))
		return 0;

	// Create the window
	hwndMain = CreateWindowEx(0,		// Extended window style
				TRAYRSSCLASS,	// Window class name
				RSS_text(""),		// Window title
				WS_POPUP,	// Window style
				0,0,		// (x,y) pos of the window
				0,0,		// Width and height of the window
				NULL,		// HWND of the parent window (can be null also)
				NULL,		// Handle to menu
				hInstance,	// Handle to application instance
				NULL);		// Pointer to window creation data

	// Check if window creation was successful
	if (!hwndMain)
		return 0;

	// Make the window invisible
	ShowWindow(hwndMain, SW_HIDE);

	TrayIconAdd(hwndMain, hInstance);

	hTimerQ = CreateTimerQueue();
	if(hTimerQ){
		if (!CreateTimerQueueTimer(&hTimer,hTimerQ,TimerRoutine,hwndMain,10000,30000,WT_EXECUTEDEFAULT)){
			MessageBox(NULL,RSS_text("Timer creation failed"),strAbout, MB_OK);
		}
	} 
	else {
			MessageBox(NULL,RSS_text("Timer Q creation failed"),strAbout,MB_OK);
	}

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed: %d\n", iResult);
		return 1;
	}


	// Process messages coming to this window
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	WSACleanup();

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
						RSS_text("TrayRSS\n") ,
						strAbout,
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
					break;
				case NIN_BALLOONSHOW:
					break;
				case NIN_BALLOONHIDE:
					break;
				case NIN_BALLOONTIMEOUT:
					break;
				case NIN_BALLOONUSERCLICK:
					{
					LaunchBrowser(RSS_text("http://www.wired.com"));
					break;
					}
				default:
					break;
			}
        
		default:
			// Call the default window handler
			return DefWindowProc(hwnd, msg, wParam, lParam);
	}

	return 0;
}

void test_error_handler(RSS_u32 error_level, const RSS_char* error, size_t pos)
{
	if(error_level == RSS_EL_ERROR)
	{
		if(pos != RSS_NO_POS_INFO)
			RSS_printf(RSS_text("[%d] %s\n"), pos, error);
		else
			RSS_printf(RSS_text("%s\n"), error);
	}
}


void TrayIconAdd(HWND hwnd, HINSTANCE hInstance)
{
	NOTIFYICONDATA nid;
	RSS_char szTip[1024];


  	ZeroMemory(&nid, sizeof(NOTIFYICONDATA));

  	nid.cbSize = nid_cbsize;
  	nid.uID = TRAY_ICON_ID;
  	nid.uFlags = NIF_ICON|NIF_MESSAGE|NIF_TIP;
  	nid.hIcon = (HICON)LoadImage(hInstance, MAKEINTRESOURCE(IDI_MY_ICON), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
  	nid.hWnd = hwnd;
  	nid.uCallbackMessage = SWM_TRAYMSG;
  	RSS_sprintf(szTip, 1024/2, RSS_text("%s v%s"), TRAYRSS, VERSION);
  	RSS_strcpy(nid.szTip, szTip);
  	Shell_NotifyIcon(NIM_ADD, &nid);
	if (nid.hIcon && DestroyIcon(nid.hIcon))
		nid.hIcon = NULL;
}

void TrayIconBalloon(HWND hwnd, RSS_char *szMessage, RSS_char *szTitle, UINT uTimeout, DWORD dwInfoFlags)
{
	NOTIFYICONDATA nid;

	ZeroMemory(&nid, sizeof(NOTIFYICONDATA));
	nid.cbSize = nid_cbsize;
	nid.uID = TRAY_ICON_ID;
	nid.uFlags = NIF_INFO;
	nid.hWnd = hwnd;
	nid.uTimeout = uTimeout;
	nid.dwInfoFlags = dwInfoFlags;
	RSS_strcpy(nid.szInfo, (szMessage));
	RSS_strcpy(nid.szInfoTitle, (szTitle));
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
		InsertMenu(hMenu, -1, MF_BYPOSITION, SWM_ABOUT, RSS_text("About"));
		InsertMenu(hMenu, -1, MF_BYPOSITION, SWM_EXIT, RSS_text("Exit"));
		// note:	must set window to the foreground or the
		//			menu won't disappear when it should
		SetForegroundWindow(hWnd);
		TrackPopupMenu(hMenu, TPM_BOTTOMALIGN, pt.x, pt.y, 0, hWnd, NULL );
		DestroyMenu(hMenu);
	}
}

VOID CALLBACK TimerRoutine(PVOID lpParam, BOOLEAN TimerOrWaitFired)
{
	RSS_char szBalloon[128];

    	if (lpParam != NULL) {
		RSS_sprintf(szBalloon, 128/2, RSS_text("Timer event signaled"));
		
		TrayIconBalloon((HWND)lpParam, szBalloon, TRAYRSS, 5, NIIF_WARNING);

		test_feed_walk(RSS_text("feeds.bbci.co.uk/news/rss.xml"));

        	if(TimerOrWaitFired) {
            		//printf("The wait timed out.\n");
        	}
        	else {
            		//printf("The wait event was signaled.\n");
        	}
    	}

}

DWORD GetShellVersion()
{
	HINSTANCE hinstDll;
	DWORD dwVersion = 0;
	RSS_char lpszDllName[100] = RSS_text("C:\\Windows\\System32\\Shell32.dll");

    	hinstDll = LoadLibrary(lpszDllName);
	
    	if(hinstDll) {
        	DLLGETVERSIONPROC pDllGetVersion;
        	pDllGetVersion = (DLLGETVERSIONPROC)GetProcAddress(hinstDll, "DllGetVersion");

        	if(pDllGetVersion) {
            		DLLVERSIONINFO dvi;
            		HRESULT hr;

            		ZeroMemory(&dvi, sizeof(dvi));
            		dvi.cbSize = sizeof(dvi);

            		hr = (*pDllGetVersion)(&dvi);

            		if(SUCCEEDED(hr)) {
               			dwVersion = PACKVERSION(dvi.dwMajorVersion, dvi.dwMinorVersion);
            		}
        	}
        	FreeLibrary(hinstDll);
    	}
    	return dwVersion;
}

void LaunchBrowser(RSS_char *command)
{

	// let the system figure out how to handle the URL and launch the default browser
	ShellExecute(NULL,RSS_text("open"),command,NULL,NULL,SW_SHOWNORMAL);
}

void test_feed_walk(const RSS_char* url)
{
	RSS_Feed* feed;
	
	feed = RSS_create_feed(url, test_error_handler);
	if(feed) {
		RSS_Item *fi;
		double tsince;
		time_t tnow;
		int RSS_item_count = 0;
		RSS_char age[101];

		fi = feed->items;
		while (fi) {
			RSS_item_count++;
			tsince = difftime(time(&tnow),fi->pubDate);
			RSS_time2age(tsince,101,age);
			RSS_printf(RSS_text("[%d]Title:\t%s [%s]\n[%d]Link:\t%s\n"),RSS_item_count, fi->title, age, RSS_item_count, fi->link);
			fi = fi->next;
		}
		RSS_printf(RSS_text("Items in feed: %d\n\n"),RSS_item_count);
	}
	else
		RSS_printf(RSS_text("test_feed [OK] - wrong format: %s\n"), url);

	RSS_free_feed(feed);
}
/* convert time in seconds to an "English" description - e.g. "2 days ago", "47 minutes ago" etc
	Maximum space taken to describe age is 100 characters */
RSS_char *RSS_time2age(double tsince, int szAgeStr, RSS_char *age)
{
	if(tsince < 60) {  // less than 60 seconds
		RSS_sprintf(age, szAgeStr, RSS_text("Less than a minute ago"));
		return age;
	}

	if(tsince < 60*2) {  // less than 120 seconds
		RSS_sprintf(age, szAgeStr, RSS_text("About a minute ago"));
		return age;
	}
	
	if(tsince < 60*60){ // less than 3660 seconds = 1 hour
		RSS_sprintf(age, szAgeStr, RSS_text("About %d minutes ago"), (int)tsince/60);
		return age;
	}

	if(tsince < 2*60*60){ // less than 7260 seconds = 2 hour
		RSS_sprintf(age, szAgeStr, RSS_text("About an hour ago"), (int)tsince/60);
		return age;
	}

	if(tsince < 24*60*60){ // less than 24 hours ago
		RSS_sprintf(age,szAgeStr, RSS_text("%d hours ago"), (int)tsince/(60*60));
		return age;
	}

	if(tsince < 48*60*60){ // less than 48 hours ago
		RSS_sprintf(age,szAgeStr, RSS_text("Yesterday"));
		return age;
	}
	
	if(tsince < 7*24*60*60){ // less than 7 days ago
		RSS_sprintf(age, szAgeStr, RSS_text("This week"));
		return age;
	}

	if(tsince < 2*7*24*60*60){ // less than 14 days 
		RSS_sprintf(age, szAgeStr, RSS_text("Last week"));
		return age;
	}

	if(tsince < 30*24*60*60){ // less than 30 days 
		RSS_sprintf(age, szAgeStr, RSS_text("This month"));
		return age;
	}

	if(tsince < 2*30*24*60*60){ // less than 60 days
		RSS_sprintf(age, szAgeStr, RSS_text("Last month"));
		return age;
	}
	if(tsince < 365*24*60*60){ // less than 365 days 
		RSS_sprintf(age, szAgeStr, RSS_text("This year"));
		return age;
	}

	RSS_sprintf(age, szAgeStr, RSS_text("More than an year ago"));

	return age;
}