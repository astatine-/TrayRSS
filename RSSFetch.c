
#include <windows.h>

#include <winhttp.h>
#include <stdio.h>


#include "RSSFetch.h"

int wgprintf(const char [] , int );

int wg(TCHAR *URL) {
	DWORD dwSize = 0;
	  DWORD dwDownloaded = 0;
	  LPSTR pszOutBuffer;
	  BOOL  bResults = FALSE;
	  HINTERNET  hSession = NULL, 
				 hConnect = NULL,
				 hRequest = NULL;

	  // Use WinHttpOpen to obtain a session handle.
	  hSession = WinHttpOpen( L"TrayRSS/1.0",  
							  WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
							  WINHTTP_NO_PROXY_NAME, 
							  WINHTTP_NO_PROXY_BYPASS, 0 );

	  // Specify an HTTP server.
	  if( hSession )
		hConnect = WinHttpConnect( hSession, URL,
								   INTERNET_DEFAULT_HTTPS_PORT, 0 );

	  // Create an HTTP request handle.
	  if( hConnect )
		hRequest = WinHttpOpenRequest( hConnect, L"GET", NULL,
									   NULL, WINHTTP_NO_REFERER, 
									   WINHTTP_DEFAULT_ACCEPT_TYPES, 
									   WINHTTP_FLAG_SECURE );

	  // Send a request.
	  if( hRequest )
		bResults = WinHttpSendRequest( hRequest,
									   WINHTTP_NO_ADDITIONAL_HEADERS, 0,
									   WINHTTP_NO_REQUEST_DATA, 0, 
									   0, 0 );


	  // End the request.
	  if( bResults )
		bResults = WinHttpReceiveResponse( hRequest, NULL );

	  // Keep checking for data until there is nothing left.
	  if( bResults )
	  {
		do 
		{
		  // Check for available data.
		  dwSize = 0;
		  if( !WinHttpQueryDataAvailable( hRequest, &dwSize ) )
			wgprintf( "Error %u in WinHttpQueryDataAvailable.\n",
					GetLastError( ) );

		  // Allocate space for the buffer.
		  pszOutBuffer = calloc(dwSize+1,1);
		  if( !pszOutBuffer )
		  {
			wgprintf( "Out of memory\n",0 );
			dwSize=0;
		  }
		  else
		  {
			// Read the data.
			ZeroMemory( pszOutBuffer, dwSize+1 );

			if( !WinHttpReadData( hRequest, (LPVOID)pszOutBuffer, 
								  dwSize, &dwDownloaded ) )
			  wgprintf( "Error %u in WinHttpReadData.\n", GetLastError( ) );
			else
			  wgprintf(pszOutBuffer,0 );

			// Free the memory allocated to the buffer.
			if(pszOutBuffer) free (pszOutBuffer) ;
		  }
		} while( dwSize > 0 );
	  }


	  // Report any errors.
	  if( !bResults )
		wgprintf( "Error %d has occurred.\n", GetLastError( ) );

	  // Close any open handles.
	  if( hRequest ) WinHttpCloseHandle( hRequest );
	  if( hConnect ) WinHttpCloseHandle( hConnect );
	  if( hSession ) WinHttpCloseHandle( hSession );
	  
	  return 0;
}

int wgprintf(const char s[] , int n)
{
	TCHAR buf[128];

	if(strlen(s) > 127)
		return -1;
	_snprintf_s(buf,127,127,s, n);
	buf[127] = 0;
	MessageBox(GetActiveWindow(),buf,L"wg msg",MB_OK);
	return 0;
}