// globalhelpers.cpp - Implementation of global helper functions
//
// WinDirStat - Directory Statistics
// Copyright (C) 2003-2004 Bernhard Seifert
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// Author: bseifert@users.sourceforge.net, bseifert@daccord.net
//
// Last modified: $Date$

#include "stdafx.h"
#include "windirstat.h"
#include "globalhelpers.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define BASE 1024
#define HALF_BASE BASE/2
namespace
{
	CString FormatLongLongNormal(_In_ LONGLONG n)
	{
		// Returns formatted number like "123.456.789".

		ASSERT( n >= 0 );
		
		CString all;
		
		do
		{
			INT rest = ( INT ) ( n % 1000 );
			n /= 1000;

			CString s;
			if ( n > 0 ) {
				s.Format( _T( ",%03d") , rest );
				}
			else {
				s.Format( _T( "%d" ), rest );
				}
			all = s + all;
			}
		while ( n > 0 );

		return all;
	}

	void CacheString(_Inout_ CString& s, _In_ UINT resId, _In_ LPCTSTR defaultVal)
	{
		ASSERT( lstrlen( defaultVal ) > 0 );
		if ( s.IsEmpty( ) ) {
			s = LoadString( resId );		
			if ( s.IsEmpty( ) ) {
				s = defaultVal;
				}
			}
	}

}

CString GetLocaleString(_In_ const LCTYPE lctype, _In_ const LANGID langid)
{
	LCID lcid = MAKELCID( langid, SORT_DEFAULT );
	INT len = GetLocaleInfo( lcid, lctype, NULL, 0 );
	CString s;
	GetLocaleInfo(lcid, lctype, s.GetBuffer(len), len);
	s.ReleaseBuffer( );
	return s;
}

CString GetLocaleLanguage(_In_ const LANGID langid)
{
	CString s = GetLocaleString( LOCALE_SNATIVELANGNAME, langid );

	// In the French case, the system returns "francais", but we want "Francais".

	if ( s.GetLength( ) > 0 ) {
		s.SetAt( 0, ( TCHAR ) _totupper( s[ 0 ] ) );
		}

	return s + _T(" - ") + GetLocaleString(LOCALE_SNATIVECTRYNAME, langid);
}


CString FormatBytes( _In_ const LONGLONG n )
{
	if ( GetOptions( )->IsHumanFormat( ) ) {
		return FormatLongLongHuman( n );
		}
	else {
		return FormatLongLongNormal( n );
		}
}

CString FormatLongLongHuman(_In_ LONGLONG n)
{
	// Returns formatted number like "12,4 GB".
	ASSERT(n >= 0);
	CString s;

	DOUBLE B = ( INT ) ( n % BASE );
	n /= BASE;

	DOUBLE KB = ( INT ) ( n % BASE );
	n /= BASE;

	DOUBLE MB = ( INT ) ( n % BASE );
	n /= BASE;

	DOUBLE GB = ( INT ) ( n % BASE );
	n /= BASE;

	DOUBLE TB = ( INT ) ( n );

	if ( TB != 0 || GB == BASE - 1 && MB >= HALF_BASE ) {
		s.Format( _T( "%s TB" ), FormatDouble( TB + GB / BASE ).GetString( ) );
		}
	else if ( GB != 0 || MB == BASE - 1 && KB >= HALF_BASE ) {
		s.Format( _T( "%s GB" ), FormatDouble( GB + MB / BASE ).GetString( ) );
		}
	else if ( MB != 0 || KB == BASE - 1 && B >= HALF_BASE ) {
		s.Format( _T( "%s MB" ), FormatDouble( MB + KB / BASE ).GetString( ) );
		}
	else if ( KB != 0 ) {
		s.Format( _T( "%s KB" ), FormatDouble( KB + B / BASE ).GetString( ) );
		}
	else if ( B != 0 ) {
		s.Format( _T( "%i Bytes" ), ( INT ) B );
		}
	else {
		s = _T( "0" );
		}
	return s;
}


CString FormatCount(_In_ const LONGLONG n )
{
	return FormatLongLongNormal( n );
}

CString FormatDouble(_In_ DOUBLE d) // "98,4" or "98.4"
{

	CString s;
	s.Format( _T( "%.1f" ), d );
	return s;
}

CString PadWidthBlanks( _In_ CString n, _In_ const INT width )
{
	ASSERT( width >= 0 );
	INT blankCount = width - n.GetLength( );
	if ( blankCount > 0 ) {
		CString b;
		LPTSTR psz = b.GetBuffer( blankCount + 1 );
		for ( INT i = 0; i < blankCount; i++ ) {
			psz[ i ] = _T( ' ' );
			psz[ i ] = 0;
			}
		b.ReleaseBuffer( );
		n = b + n;
		}
	return n;
}

CString FormatFileTime(_In_ const FILETIME& t)
{
	ASSERT( &t != NULL );
	SYSTEMTIME st;
	if ( !FileTimeToSystemTime( &t, &st ) ) {
		return MdGetWinerrorText( GetLastError( ) );
		}
	LCID lcid = MAKELCID( GetUserDefaultLangID( ), SORT_DEFAULT );
	CString date;
	VERIFY( 0 < GetDateFormat( lcid, DATE_SHORTDATE, &st, NULL, date.GetBuffer( 256 ), 256 ) );
	date.ReleaseBuffer( );

	CString time;
	VERIFY( 0 < GetTimeFormat( lcid, 0, &st, NULL, time.GetBuffer( 256 ), 256 ) );
	time.ReleaseBuffer( );

	return date + _T("  ") + time;
}

CString FormatAttributes( _In_ const DWORD attr )
{
	if ( attr == INVALID_FILE_ATTRIBUTES ) {
		return _T( "?????" );
		}

	CString attributes;
	attributes.Append( ( attr & FILE_ATTRIBUTE_READONLY )   ? _T( "R" ) : _T( "" ) );
	attributes.Append( ( attr & FILE_ATTRIBUTE_HIDDEN )     ? _T( "H" ) : _T( "" ) );
	attributes.Append( ( attr & FILE_ATTRIBUTE_SYSTEM )     ? _T( "S" ) : _T( "" ) );
	attributes.Append( ( attr & FILE_ATTRIBUTE_ARCHIVE )    ? _T( "A" ) : _T( "" ) );
	attributes.Append( ( attr & FILE_ATTRIBUTE_COMPRESSED ) ? _T( "C" ) : _T( "" ) );
	attributes.Append( ( attr & FILE_ATTRIBUTE_ENCRYPTED )  ? _T( "E" ) : _T( "" ) );

	return attributes;
}

CString FormatMilliseconds( _In_ const unsigned long long ms )
{
	CString ret;
	unsigned long long  sec = ( ms + 500 ) / 1000;
	unsigned long long  s = sec % 60;
	unsigned long long  min = sec / 60;
	unsigned long long  m = min % 60;
	unsigned long long  h = min / 60;

	if ( h > 0 ) {
		ret.Format( _T( "%llu:%02llu:%02llu" ), h, m, s );
		}
	else {
		ret.Format( _T( "%llu:%02llu" ), m, s );
		}
	return ret;
}

bool GetVolumeName( _In_ const LPCTSTR rootPath, _Inout_ CString& volumeName )
{
	CString ret;
	DWORD dummy;

	UINT old = SetErrorMode( SEM_FAILCRITICALERRORS );
	
	//GetVolumeInformation returns 0 on failure
	BOOL b = GetVolumeInformation( rootPath, volumeName.GetBuffer( 256 ), 256, &dummy, &dummy, &dummy, NULL, 0 );
	volumeName.ReleaseBuffer( );

	if ( b == 0) {
		TRACE( _T( "GetVolumeInformation(%s) failed: %u\n" ), rootPath, GetLastError( ) );
		}
	SetErrorMode(old);
	
	return ( b != 0 );
}

CString FormatVolumeNameOfRootPath( _In_ const CString rootPath )
{
	/*
	  Given a root path like "C:\", this function obtains the volume name and returns a complete display string like "BOOT (C:)".
	*/
	ASSERT( rootPath != _T( "" ) );
	CString ret;
	CString volumeName;
	bool b = GetVolumeName( rootPath, volumeName );
	if (b) {
		ret = FormatVolumeName( rootPath, volumeName );
		}
	else {
		ret = rootPath;
		}
	return ret;
}

CString FormatVolumeName( _In_ const CString rootPath, _In_ const CString volumeName )
{
	ASSERT( rootPath != _T( "" ) );
	CString ret;
	ret.Format( _T( "%s (%s)" ), volumeName.GetString( ), rootPath.Left( 2 ).GetString( ) );
	return ret;
}

CString PathFromVolumeName( _In_ const CString name )
{
	/*
	  The inverse of FormatVolumeNameOfRootPath().
	  Given a name like "BOOT (C:)", it returns "C:" (without trailing backslash).
	  Or, if name like "C:\", it returns "C:".
	*/
	ASSERT( name != _T( "" ) );
	INT i = name.ReverseFind( _T( ')' ) );
	if ( i == -1 ) {
		ASSERT( name.GetLength( ) == 3 );
		return name.Left( 2 );
		}

	ASSERT( i != -1 );
	INT k = name.ReverseFind( _T( '(' ) );
	ASSERT( k != -1 );
	ASSERT( k < i );
	CString path = name.Mid( k + 1, i - k - 1 );
	ASSERT( path.GetLength( ) == 2 );
	ASSERT( path[ 1 ] == _T( ':' ) );

	return path;
}


CString GetParseNameOfMyComputer() throw (CException *)
{
	/*
	  Retrieve the "fully qualified parse name" of "My Computer"
	*/
	CComPtr<IShellFolder> sf;
	HRESULT hr = SHGetDesktopFolder( &sf );
	MdThrowFailed( hr, _T( "SHGetDesktopFolder" ) );

	CCoTaskMem<LPITEMIDLIST> pidl;

	hr = SHGetSpecialFolderLocation( NULL, CSIDL_DRIVES, &pidl );
	MdThrowFailed( hr, _T( "SHGetSpecialFolderLocation(CSIDL_DRIVES)" ) );

	STRRET name;
	name.pOleStr = NULL;
	name.uOffset = NULL;
	name.uType = STRRET_CSTR;
	hr = sf->GetDisplayNameOf( pidl, SHGDN_FORPARSING, &name );
	MdThrowFailed( hr, _T( "GetDisplayNameOf(My Computer)" ) );

	return MyStrRetToString( pidl, &name );
}

void GetPidlOfMyComputer( _Inout_ LPITEMIDLIST *ppidl ) throw ( CException * ) {
	CComPtr<IShellFolder> sf;
	HRESULT hr = SHGetDesktopFolder( &sf );
	if ( hr == S_OK ) {
		MdThrowFailed( hr, _T( "SHGetDesktopFolder" ) );

		hr = SHGetSpecialFolderLocation( NULL, CSIDL_DRIVES, ppidl ); //TODO: DEPRECIATED! 
		if ( hr != S_OK ) {
			ASSERT( false );
			TRACE( _T( "Failed SHGetSpecialFolderLocation!!\r\n" ) );
			}
		MdThrowFailed( hr, _T( "SHGetSpecialFolderLocation( CSIDL_DRIVES )" ) );
		}
	else {
		ASSERT( false );
		TRACE( _T( "Failed to get PidlOfMyComputer!\r\n" ) );
		displayWindowsMsgBoxWithError( );
		}
	}

void ShellExecuteWithAssocDialog( _In_ const HWND hwnd, _In_ const LPCTSTR filename ) throw ( CException * )
{
	CWaitCursor wc;
	//ASSERT( filename != _T( "" ) );
	UINT u = ( UINT ) ShellExecute( hwnd, NULL, filename, NULL, NULL, SW_SHOWNORMAL );
	if ( u == SE_ERR_NOASSOC ) {
		// Q192352
		CString sysDir;
		//-- Get the system directory so that we know where Rundll32.exe resides.
		GetSystemDirectory( sysDir.GetBuffer( _MAX_PATH ), _MAX_PATH );
		sysDir.ReleaseBuffer( );
		
		CString parameters = _T( "shell32.dll,OpenAs_RunDLL " );
		u = ( UINT ) ShellExecute( hwnd, _T( "open" ), _T( "RUNDLL32.EXE" ), parameters + filename, sysDir, SW_SHOWNORMAL );
		}
		
	if ( u <= 32 ) {
		MdThrowStringExceptionF( _T( "ShellExecute failed: %1!s!" ), GetShellExecuteError( u ) );
		}
}

void MyGetDiskFreeSpace( _In_ const LPCTSTR pszRootPath, _Inout_ LONGLONG& total, _Inout_ LONGLONG& unused )
{
	//ASSERT( pszRootPath != _T( "" ) );
	ULARGE_INTEGER uavailable = { { 0 } };
	ULARGE_INTEGER utotal = { { 0 } };
	ULARGE_INTEGER ufree = { { 0 } };
	uavailable.QuadPart = 0;
	utotal.QuadPart = 0;
	ufree.QuadPart = 0;

	// On NT 4.0, the 2nd Parameter to this function must NOT be NULL.
	BOOL b = GetDiskFreeSpaceEx( pszRootPath, &uavailable, &utotal, &ufree );
	if ( !b ) {
		TRACE( _T( "\tGetDiskFreeSpaceEx(%s) failed.\r\n" ), pszRootPath );
		}
	else {
		TRACE( _T("\tGetDiskFreeSpaceEx(%s) successfully returned uavailable: %llu, utotal: %llu, ufree: %llu\r\n"), pszRootPath, uavailable, utotal, ufree);
		ASSERT( uavailable.QuadPart <= utotal.QuadPart);
		ASSERT( ufree.QuadPart <= utotal.QuadPart );
		ASSERT( uavailable.QuadPart != utotal.QuadPart );
		ASSERT( ufree.QuadPart != utotal.QuadPart );
		}
	total = ( LONGLONG ) utotal.QuadPart; // will fail, when more than 2^63 Bytes free ....
	unused = ( LONGLONG ) ufree.QuadPart;
	ASSERT(unused <= total);
}


void MyGetDiskFreeSpace( _In_ const LPCTSTR pszRootPath, _Inout_ LONGLONG& total, _Inout_ LONGLONG& unused, _Inout_ LONGLONG& available )
{
	//ASSERT( pszRootPath != _T( "" ) );
	ULARGE_INTEGER uavailable = { { 0 } };
	ULARGE_INTEGER utotal = { { 0 } };
	ULARGE_INTEGER ufree = { { 0 } };
	uavailable.QuadPart = 0;
	utotal.QuadPart = 0;
	ufree.QuadPart = 0;

	// On NT 4.0, the 2nd Parameter to this function must NOT be NULL.
	BOOL b = GetDiskFreeSpaceEx( pszRootPath, &uavailable, &utotal, &ufree );
	if ( !b ) {
		TRACE( _T( "\tGetDiskFreeSpaceEx(%s) failed.\r\n" ), pszRootPath );
		}
	else {
		TRACE( _T("\tGetDiskFreeSpaceEx(%s) successfully returned uavailable: %llu, utotal: %llu, ufree: %llu\r\n"), pszRootPath, uavailable, utotal, ufree);
		ASSERT( uavailable.QuadPart <= utotal.QuadPart);
		ASSERT( ufree.QuadPart <= utotal.QuadPart );
		ASSERT( uavailable.QuadPart != utotal.QuadPart );
		ASSERT( ufree.QuadPart != utotal.QuadPart );
		}
	total = ( LONGLONG ) utotal.QuadPart; // will fail, when more than 2^63 Bytes free ....
	unused = ( LONGLONG ) ufree.QuadPart;
	available = ( LONGLONG ) uavailable.QuadPart;
	ASSERT(unused <= total);
}

LONGLONG GetTotalDiskSpace( _In_ const CString path ) {
	auto lpcstr_path = ( LPCTSTR ) path;
	ULARGE_INTEGER uavailable = { { 0 } };
	ULARGE_INTEGER utotal = { { 0 } };
	ULARGE_INTEGER ufree = { { 0 } };
	uavailable.QuadPart = 0;
	utotal.QuadPart = 0;
	ufree.QuadPart = 0;
	BOOL res = GetDiskFreeSpaceEx( lpcstr_path, &uavailable, &utotal, &ufree );
	if ( res ) {
		return ( LONGLONG ) utotal.QuadPart;
		}
	else {
		return MAXULONGLONG;
		}
	}


CString GetFolderNameFromPath( _In_ const LPCTSTR path )
{
	//ASSERT( path != _T( "" ) );
	CString s = path;
	INT i = s.ReverseFind( _T( '\\' ) );
	if ( i < 0 ) {
		return s;
		}
	return s.Left( i );
}

CString GetCOMSPEC()
{
	CString cmd;

	DWORD dw = GetEnvironmentVariable( _T( "COMSPEC" ), cmd.GetBuffer( _MAX_PATH ), _MAX_PATH );
	cmd.ReleaseBuffer( );

	if (dw == 0) {
		TRACE(_T("COMSPEC not set.\n"));
		cmd = _T( "cmd.exe" );
		}
	return cmd;
}

void WaitForHandleWithRepainting( _In_ const HANDLE h )
{ 
	/*
	  Code derived from MSDN sample "Waiting in a Message Loop".
	*/
	ASSERT( h != NULL );
	while ( true ) {
		// Read all of the messages in this next loop, removing each message as we read it.
		MSG msg; 
		while ( PeekMessage( &msg, NULL, WM_PAINT, WM_PAINT, PM_REMOVE ) ) {
			DispatchMessage( &msg );
			}

		// Wait for WM_PAINT message sent or posted to this queue or for one of the passed handles be set to signaled.
		DWORD r = MsgWaitForMultipleObjects( 1, &h, FALSE, INFINITE, QS_PAINT );

		// The result tells us the type of event we have.
		if ( r == WAIT_OBJECT_0 + 1 ) {
			// New messages have arrived. 
			// Continue to the top of the always while loop to dispatch them and resume waiting.
			continue;
			}
		else {
			// The handle became signaled. 
			break;
			}
		}
}

bool FolderExists( _In_ const LPCTSTR path )
{
	CFileFind finder;
	//ASSERT( path != _T( "" ) );
	BOOL b = finder.FindFile( path );
	if ( b ) {
		finder.FindNextFile( );

		//IsDirectory returns 0 on failure
		return ( finder.IsDirectory( ) != 0 );
		}
	else {
		// Here we land, if path is an UNC drive. In this case we try another FindFile:
		b = finder.FindFile( CString( path ) + _T( "\\*.*" ) );
		if ( b ) {
			return true;
			}
		return false;
		}
}

bool DriveExists( _In_ const CString& path)
{
	//ASSERT( path != _T( "" ) );
	if ( path.GetLength( ) != 3 || path[ 1 ] != _T( ':' ) || path[ 2 ] != _T( '\\' ) ) {
		return false;
		}
	CString letter = path.Left( 1 );
	letter.MakeLower( );
	INT d = letter[ 0 ] - _T( 'a' );
	
	DWORD mask = 0x1 << d;

	if ( ( mask & GetLogicalDrives( ) ) == 0 ) {
		return false;
		}

	CString dummy;
	if ( !GetVolumeName( path, dummy ) ) {
		return false;
		}

	return true;
}

CString lGetUserName( ) {
	CString s;
	DWORD size = UNLEN + 1;
	auto ret = GetUserName( s.GetBuffer( size ), &size );//TODO: BUGBUG FIXME
	s.ReleaseBuffer( );
	if ( ret == 0 ) {
		TRACE( _T( "GetUserName Failed!!!!\r\n" ) );

		}
	return s;
	}

bool IsHexDigit( _In_ const INT c )
{
	if ( _istdigit( ( short ) c ) ) {
		return true;
		}

	if ( _T( 'a' ) <= c && c <= _T( 'f' ) ) {
		return true;
		}

	if ( _T( 'A' ) <= c && c <= _T( 'F' ) ) {
		return true;
		}

	return false;
}

CString MyQueryDosDevice( _In_ const LPCTSTR drive )
{
	/*
	  drive is a drive spec like C: or C:\ or C:\path (path is ignored).
	  This function returns "", if QueryDosDevice is unsupported or drive doesn't begin with a drive letter, 'Information about MS-DOS device names' otherwise: Something like

	  \Device\Harddisk\Volume1                               for a local drive
	  \Device\LanmanRedirector\;T:0000000011e98\spock\temp   for a network drive 
	  \??\C:\programme                                       for a SUBSTed local path
	  \??\T:\Neuer Ordner                                    for a SUBSTed SUBSTed path
	  \??\UNC\spock\temp                                     for a SUBSTed UNC path

	  As always, I had to experimentally determine these strings, Microsoft didn't think it necessary to document them. (Sometimes I think, they even don't document such things internally...)

	  I hope that a drive is SUBSTed iff this string starts with \??\.

	  assarbad:
		It cannot be safely determined weather a path is or is not SUBSTed on NT via this API. You would have to lookup the volume mount points because SUBST only works per session by definition whereas volume mount points work across sessions (after restarts).
	*/
	CString d = drive;

	if ( d.GetLength( ) < 2 || d[ 1 ] != _T( ':' ) ) {
		return _T( "" );
		}

	d = d.Left( 2 );

	CQueryDosDeviceApi api;
	
	if ( !api.IsSupported( ) ) {
		return _T( "" );
		}

	CString info;
	DWORD dw = api.QueryDosDevice( d, info.GetBuffer( 512 ), 512 );
	info.ReleaseBuffer( );

	if (dw == 0) {
		TRACE( _T( "QueryDosDevice(%s) failed: %s\r\n" ), d, MdGetWinerrorText( GetLastError( ) ) );
		return _T("");
		}

	return info;
}

bool IsSUBSTedDrive( _In_ const LPCTSTR drive )
{
	/*
	  drive is a drive spec like C: or C:\ or C:\path (path is ignored).
	  This function returns true, if QueryDosDevice() is supported and drive is a SUBSTed drive.
	*/
	CString info = MyQueryDosDevice( drive );
	return ( info.GetLength( ) >= 4 && info.Left( 4 ) == "\\??\\" );
}

SHELLEXECUTEINFO zeroInitSEI( ) {
	SHELLEXECUTEINFO sei;
	sei.cbSize = NULL;
	sei.dwHotKey = NULL;
	sei.fMask = NULL;
	sei.hIcon = NULL;
	sei.hInstApp = NULL;
	sei.hkeyClass = NULL;
	sei.hMonitor = NULL;
	sei.hProcess = NULL;
	sei.hwnd = NULL;
	sei.lpClass = NULL;
	sei.lpDirectory = NULL;
	sei.lpFile = NULL;
	sei.lpIDList = NULL;
	sei.lpParameters = NULL;
	sei.lpVerb = NULL;
	sei.nShow = NULL;
	return std::move( sei );
	}

WINDOWPLACEMENT zeroInitWINDOWPLACEMENT( ) {
	WINDOWPLACEMENT wp;
	wp.flags = NULL;
	wp.ptMaxPosition.x = NULL;
	wp.ptMaxPosition.y = NULL;
	wp.ptMinPosition.x = NULL;
	wp.ptMinPosition.y = NULL;
	wp.rcNormalPosition.bottom = NULL;
	wp.rcNormalPosition.left = NULL;
	wp.rcNormalPosition.right = NULL;
	wp.rcNormalPosition.top  = NULL;
	wp.showCmd = NULL;
	wp.length = sizeof( wp );

	return std::move( wp );
	}

USN_JOURNAL_DATA zeroInitUSN_JOURNAL_DATA( ) {
	USN_JOURNAL_DATA UpdateSequenceNumber_JournalData;
	UpdateSequenceNumber_JournalData.AllocationDelta = NULL;
	UpdateSequenceNumber_JournalData.FirstUsn = NULL;
	UpdateSequenceNumber_JournalData.LowestValidUsn = NULL;
	UpdateSequenceNumber_JournalData.MaximumSize = NULL;
	UpdateSequenceNumber_JournalData.MaxUsn = NULL;
	UpdateSequenceNumber_JournalData.NextUsn = NULL;
	UpdateSequenceNumber_JournalData.UsnJournalID = NULL;
	return std::move( UpdateSequenceNumber_JournalData );
	}

LVHITTESTINFO zeroInitLVHITTESTINFO( ) {
	LVHITTESTINFO hti;
	hti.flags = NULL;
	hti.iGroup = NULL;
	hti.iItem = NULL;
	hti.iSubItem = NULL;
	hti.pt.x = NULL;
	hti.pt.y = NULL;
	return std::move( hti );
	}

HDITEM zeroInitHDITEM( ) {
	HDITEM hditem;

	hditem.cchTextMax = NULL;
	hditem.cxy = NULL;
	hditem.fmt = NULL;
	hditem.hbm = NULL;
	hditem.iImage = NULL;
	hditem.iOrder = NULL;
	hditem.lParam = NULL;
	hditem.mask = NULL;
	hditem.pszText = NULL;
	hditem.pvFilter = NULL;
	hditem.state = NULL;
	hditem.type = NULL;
	return std::move( hditem );
	}

LVFINDINFO zeroInitLVFINDINFO( ) {
	LVFINDINFO fi;
	fi.flags = NULL;
	fi.lParam = NULL;
	fi.psz = NULL;
	fi.pt.x = NULL;
	fi.pt.y = NULL;
	fi.vkDirection = NULL;
	return std::move( fi );
	}

LVITEM zeroInitLVITEM( ) {
	LVITEM lvitem;
	lvitem.cchTextMax = NULL;
	lvitem.cColumns = NULL;
	lvitem.iGroup = NULL;
	lvitem.iGroupId = NULL;
	lvitem.iImage = NULL;
	lvitem.iIndent = NULL;
	lvitem.iItem = NULL;
	lvitem.iSubItem = NULL;
	lvitem.lParam = NULL;
	lvitem.mask = NULL;
	lvitem.piColFmt = NULL;
	lvitem.pszText = NULL;
	lvitem.puColumns = NULL;
	lvitem.state = NULL;
	lvitem.stateMask = NULL;
	return std::move( lvitem );
	}

MFT_ENUM_DATA_V0 zeroInitMFT_ENUM_DATA_V0( ) {
	MFT_ENUM_DATA_V0 data;
	data.HighUsn = NULL;
	data.LowUsn = NULL;
	data.StartFileReferenceNumber = NULL;
	return std::move( data );
	}

MFT_ENUM_DATA_V1 zeroInitMFT_ENUM_DATA_V1( ) {
	ASSERT( false );
	//Supported only on servers with ReFS
	throw 666;
	MFT_ENUM_DATA_V1 data;
	data.HighUsn = NULL;
	data.LowUsn = NULL;
	data.MaxMajorVersion = NULL;
	data.MinMajorVersion = NULL;
	data.StartFileReferenceNumber = NULL;
	return std::move( data );
	}

STORAGE_DEVICE_NUMBER zeroInitSTORAGE_DEVICE_NUMBER( ) {
	STORAGE_DEVICE_NUMBER driveNumber;
	driveNumber.DeviceNumber = NULL;
	driveNumber.DeviceType = NULL;
	driveNumber.PartitionNumber = NULL;
	return std::move( driveNumber );
	}

PROCESS_MEMORY_COUNTERS zeroInitPROCESS_MEMORY_COUNTERS( ) {
	PROCESS_MEMORY_COUNTERS pmc;
	pmc.cb = NULL;
	pmc.PageFaultCount = NULL;
	pmc.PagefileUsage = NULL;
	pmc.PeakPagefileUsage = NULL;
	pmc.PeakWorkingSetSize = NULL;
	pmc.QuotaNonPagedPoolUsage = NULL;
	pmc.QuotaPagedPoolUsage = NULL;
	pmc.QuotaPeakNonPagedPoolUsage = NULL;
	pmc.QuotaPeakPagedPoolUsage = NULL;
	pmc.WorkingSetSize = NULL;
	return std::move( pmc );
	}
STARTUPINFO zeroInitSTARTUPINFO( ) {
	STARTUPINFO si;
	si.cb = NULL;
	si.cbReserved2 = NULL;
	si.dwFillAttribute = NULL;
	si.dwFlags = NULL;
	si.dwX = NULL;
	si.dwXCountChars = NULL;
	si.dwXSize = NULL;
	si.dwY = NULL;
	si.dwYCountChars = NULL;
	si.dwYSize = NULL;
	si.hStdError = NULL;
	si.hStdInput = NULL;
	si.hStdOutput = NULL;
	si.lpDesktop = NULL;
	si.lpReserved = NULL;
	si.lpReserved2 = NULL;
	si.lpTitle = NULL;
	si.wShowWindow = NULL;
	return std::move( si );
	}

PROCESS_INFORMATION zeroInitPROCESS_INFORMATION( ) {
	PROCESS_INFORMATION pi;
	pi.dwProcessId = NULL;
	pi.dwThreadId = NULL;
	pi.hProcess = NULL;
	pi.hThread = NULL;
	return std::move( pi );
	}

NMLISTVIEW zeroInitNMLISTVIEW( ) {
	NMLISTVIEW listView;
	listView.hdr.code = NULL;
	listView.hdr.hwndFrom = NULL;
	listView.hdr.idFrom = NULL;
	listView.iItem = NULL;
	listView.iSubItem = NULL;
	listView.lParam = NULL;
	listView.ptAction.x = NULL;
	listView.ptAction.y = NULL;
	listView.uChanged = NULL;
	listView.uNewState = NULL;
	listView.uOldState = NULL;
	return std::move( listView );
	}

BROWSEINFO zeroInitBROWSEINFO( ) {
	BROWSEINFO bi;
	bi.hwndOwner = NULL;
	bi.iImage = NULL;
	bi.lParam = NULL;
	bi.lpfn = NULL;
	bi.lpszTitle = NULL;
	bi.pidlRoot = NULL;
	bi.pszDisplayName = NULL;
	bi.ulFlags = NULL;
	return std::move( bi );
	}

void displayWindowsMsgBoxWithError( ) {
	LPVOID lpMsgBuf = NULL;
	DWORD err = GetLastError( );
	TRACE( _T( "Error number: %llu\t\n" ), err );
	MessageBox(NULL, TEXT("Whoa! Err!"), (LPCWSTR)err, MB_OK );
	FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, err, MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ), ( LPTSTR ) &lpMsgBuf, 0, NULL );
	//LPCTSTR msg = ( LPCTSTR ) lpMsgBuf;
	MessageBox( NULL, ( LPCTSTR ) lpMsgBuf, TEXT( "Error" ), MB_OK );
	TRACE( _T( "Error: %s\r\n" ), lpMsgBuf );

	}

CString GetSpec_Bytes()
{
	static CString s;
	CacheString( s, IDS_SPEC_BYTES, _T( "Bytes" ) );
	return s;
}

CString GetSpec_KB()
{
	static CString s;
	CacheString( s, IDS_SPEC_KB, _T( "KB" ) );
	return s;
}

CString GetSpec_MB()
{
	static CString s;
	CacheString( s, IDS_SPEC_MB, _T( "MB" ) );
	return s;
}

CString GetSpec_GB()
{
	static CString s;
	CacheString( s, IDS_SPEC_GB, _T( "GB" ) );
	return s;
}

CString GetSpec_TB()
{
	static CString s;
	CacheString( s, IDS_SPEC_TB, _T( "TB" ) );
	return s;
}

void check8Dot3NameCreationAndNotifyUser( ) {
	HKEY keyHandle = NULL;

	auto res = RegOpenKeyEx( HKEY_LOCAL_MACHINE, _T( "SYSTEM\\CurrentControlSet\\Control\\FileSystem" ), NULL, KEY_READ, &keyHandle );

	if ( res != ERROR_SUCCESS ) {
		TRACE( _T( "key not found!\r\n" ) );
		return;
		}

	else {
		}

	DWORD valueType = 0;
	//std::unique_ptr<char[ ]> databuffer = std::make_unique<char[]>(4);//I wish...
	BYTE data[4];
	static_assert( sizeof( data ) == sizeof( REG_DWORD ), "bad size!" );
			
	DWORD bufferSize = sizeof( data );
			
	res = RegQueryValueEx( keyHandle, _T( "NtfsDisable8dot3NameCreation" ), NULL, &valueType, &data[0], &bufferSize );

	if ( res != ERROR_SUCCESS ) {
		if ( res == ERROR_MORE_DATA ) {
			return;
			}
		else if ( res == ERROR_FILE_NOT_FOUND) {
			return;
			}
		else {
			return;
			}
		}
	DWORD value = data[ 0 ];
	/*
		0 = NTFS creates short file names. This setting enables applications that cannot process long file names and computers that use differentcode pages to find the files.
		1 = NTFS does not create short file names. Although this setting increases file performance, applications that cannot process long file names, and computers that use different code pages, might not be able to find the files.
		2 = NTFS sets the 8.3 naming convention creation on a per volume basis.
		3 = NTFS disables 8dot3 name creation on all volumes except the system volume.
	*/
	if ( value == 0 ) {
		MessageBox( NULL, _T( "Your computer is set to create short (8.3 style) names for files on all NTFS volumes. This can TREMENDOUSLY slow directory operations - As a result, the amount of time required to perform a directory listing increases with the square of the number of files in the directory! For more, see Microsoft KnowledgeBase article ID: 130694" ), _T( "Performance warning!"), MB_ICONWARNING );
		}

	if ( value == 2 ) {
		MessageBox( NULL, _T( "Your computer is set to create short (8.3 style) names for files on NTFS volumes, on a per-volume-setting basis. Shore file name creation can TREMENDOUSLY slow directory operations - As a result, the amount of time required to perform a directory listing increases with the square of the number of files in the directory! For more, see Microsoft KnowledgeBase article ID: 130694" ), _T( "Performance warning!"), MB_ICONWARNING );
		}

	if ( value == 3 ) {
		MessageBox( NULL, _T( "Your computer is set to create short (8.3 style) names for files on the system volume. If you're running WinDirStat against any other volume you can safely ignore this warning. Short file name creation can TREMENDOUSLY slow directory operations - As a result, the amount of time required to perform a directory listing increases with the square of the number of files in the directory! For more, see Microsoft KnowledgeBase article ID: 130694" ), _T( "Performance warning!"), MB_ICONWARNING );
		}
				

	}

// $Log$
// Revision 1.20  2004/11/28 14:40:06  assarbad
// - Extended CFileFindWDS to replace a global function
// - Now packing/unpacking the file attributes. This even spares a call to find encrypted/compressed files.
//
// Revision 1.19  2004/11/25 21:13:38  assarbad
// - Implemented "attributes" column in the treelist
// - Adopted width in German dialog
// - Provided German, Russian and English version of IDS_TREECOL_ATTRIBUTES
//
// Revision 1.18  2004/11/25 11:58:52  assarbad
// - Minor fixes (odd behavior of coloring in ANSI version, caching of the GetCompressedFileSize API)
//   for details see the changelog.txt
//
// Revision 1.17  2004/11/14 21:50:44  assarbad
// - Pre-select the last used folder
//
// Revision 1.16  2004/11/14 08:49:06  bseifert
// Date/Time/Number formatting now uses User-Locale. New option to force old behavior.
//
// Revision 1.15  2004/11/12 22:14:16  bseifert
// Eliminated CLR_NONE. Minor corrections.
//
// Revision 1.14  2004/11/12 13:19:44  assarbad
// - Minor changes and additions (in preparation for the solution of the "Browse for Folder" problem)
//
// Revision 1.13  2004/11/12 00:47:42  assarbad
// - Fixed the code for coloring of compressed/encrypted items. Now the coloring spans the full row!
//
// Revision 1.12  2004/11/07 21:10:25  assarbad
// - Corrected IF statement
// - Changed solution file to build consistent "Unicode Debug" configuration for "Unicode Debug" of main project
//
// Revision 1.11  2004/11/07 20:14:30  assarbad
// - Added wrapper for GetCompressedFileSize() so that by default the compressed file size will be shown.
//
// Revision 1.10  2004/11/05 16:53:07  assarbad
// Added Date and History tag where appropriate.
//
