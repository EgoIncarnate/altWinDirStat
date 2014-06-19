// item.cpp	- Implementation of CItem
//
// WinDirStat - Directory Statistics
// Copyright (C) 2003-2005 Bernhard Seifert
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
#include "dirstatdoc.h"	// GetItemColor()
#include "mainframe.h"
#include "item.h"
#include "globalhelpers.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#endif

namespace
{
	CString GetFreeSpaceItemName() 	{ return LoadString(IDS_FREESPACE_ITEM); }
	CString GetUnknownItemName() { return LoadString(IDS_UNKNOWN_ITEM); }

	const SIZE sizeDeflatePacman = { 1, 2 };

	// File attribute packing
	const unsigned char INVALID_m_attributes = 0x80;
}


CItem::CItem( ITEMTYPE type, LPCTSTR name, bool dontFollow ) : m_type( std::move( type ) ), m_name( std::move( name ) ), m_size( 0 ), m_files( 0 ), m_subdirs( 0 ), m_done( false ), m_ticksWorked( 0 ), m_readJobs( 0 ), m_attributes( 0 ), m_pathWithoutBackslash( "" ), m_freeDiskSpace( -1 ), m_totalDiskSpace( -1 )
{
	auto thisItem_type = GetType( );

	if ( thisItem_type == IT_FILE || dontFollow || thisItem_type == IT_FREESPACE || thisItem_type == IT_UNKNOWN || thisItem_type == IT_MYCOMPUTER ) {
		SetReadJobDone( );
		m_readJobs = 0;
		}
	else if ( thisItem_type == IT_DIRECTORY || thisItem_type == IT_DRIVE || thisItem_type == IT_FILESFOLDER ) {
		SetReadJobDone( false );
		}

	if ( thisItem_type == IT_DRIVE ) {
		m_name = FormatVolumeNameOfRootPath( m_name );
		}

	m_lastChange.dwHighDateTime = 0;
	m_lastChange.dwLowDateTime = 0;
}

CItem::~CItem( ) {
	//AfxCheckMemory( );
	auto Document = GetDocument( );
	CItem* currentZoomItem = NULL;
	CItem* currentRootItem = NULL;
	CItem* currentlySelectedItem = NULL;
	if ( Document != NULL ) {
		currentZoomItem = Document->GetZoomItem( );
		currentRootItem = Document->GetRootItem( );
		currentlySelectedItem = Document->GetSelection( );
		}
	else {
		AfxCheckMemory( );
		ASSERT( false );
		return;
		}
	//AfxCheckMemory( );
	auto childrenSize = m_children.GetSize( );
	for ( INT i = 0; i < childrenSize; i++ ) {
		if ( ( m_children[ i ] ) != NULL ) {
			delete m_children[ i ];
			if ( m_children[ i ] == currentZoomItem ) {
				Document->clearZoomItem( );
				ASSERT( Document->GetZoomItem( ) != m_children[ i ] );
				}
			if ( m_children[ i ] == currentRootItem ) {
				Document->clearRootItem( );
				ASSERT( Document->GetRootItem( ) != m_children[ i ] );
				}
			if ( m_children[ i ] == currentlySelectedItem ) {
				Document->clearSelection( );
				ASSERT( Document->GetSelection( ) != m_children[ i ] );
				}
			m_children[ i ] = NULL;//sensible?
			}
		else {
			AfxCheckMemory( );
			ASSERT( false );
			}
		}
	}

CRect CItem::TmiGetRectangle( ) const {
	CRect rc;

	rc.left		= m_rect.left;
	rc.top		= m_rect.top;
	rc.right	= m_rect.right;
	rc.bottom	= m_rect.bottom;

	return std::move( rc );
	}

LONG CItem::TmiGetRectLeft( ) const {
	return m_rect.left;
	}

void CItem::TmiSetRectangle( _In_ const CRect& rc ) {
	m_rect.left		= (short)rc.left;
	m_rect.top		= (short)rc.top;
	m_rect.right	= (short)rc.right;
	m_rect.bottom	= (short)rc.bottom;
	}

bool CItem::DrawSubitem( _In_ const INT subitem, _In_ CDC *pdc, _Inout_ CRect& rc, _In_ const UINT state, _Inout_opt_ INT *width, _Inout_ INT *focusLeft ) const {
	ASSERT_VALID( pdc );
	ASSERT( subitem >= 0 );
	ASSERT( &width != NULL );
	ASSERT( &focusLeft != NULL );
	if (subitem == COL_NAME) {
		return CTreeListItem::DrawSubitem( subitem, pdc, rc, state, width, focusLeft );
		}
	if ( subitem != COL_SUBTREEPERCENTAGE ) {
		return false;
		}
	bool showReadJobs = MustShowReadJobs();

	if ( showReadJobs && !GetOptions( )->IsPacmanAnimation( ) ) {
		return false;
		}

	if ( showReadJobs && IsDone( ) ) {
		return false;
		}

	if ( width != NULL ) {
		//*width = GetSubtreePercentageWidth();
		*width = 105;
		return true;
		}

	DrawSelection( GetTreeListControl( ), pdc, rc, state );

	if ( showReadJobs ) {
		rc.DeflateRect( std::move( sizeDeflatePacman ) );
		auto TreeListControl = GetTreeListControl( );
		if ( TreeListControl != NULL ) {
			DrawPacman( pdc, rc, std::move( TreeListControl->GetItemSelectionBackgroundColor( this ) ) );
			}
		else {
			ASSERT( false );
			}
		}
	else {
		rc.DeflateRect( 2, 5 );
		auto indent = GetIndent( );
		for ( INT i = 0; i < indent; i++ ) {
			rc.left += ( rc.right - rc.left ) / 10;
			}

		DrawPercentage( pdc, rc, GetFraction( ), std::move( GetPercentageColor( ) ) );
		}
	return true;
	}

CString CItem::GetText(_In_ const INT subitem) const
{
	CString s;
	ASSERT( subitem >= 0 );
	switch (subitem)
	{
		case COL_NAME:
			s = m_name;
			break;

		case COL_SUBTREEPERCENTAGE:
			if ( IsDone( ) ) {
				ASSERT( m_readJobs == 0 );
				//s = "ok";
				}
			else {
				if ( m_readJobs == 1 ) {
					auto ret = s.LoadString( IDS_ONEREADJOB );//TODO
					if ( ret == 0 ) {
						exit( 666 );
						}
					}
				else {
					s.FormatMessage( IDS_sREADJOBS, FormatCount( m_readJobs ) );
					}
				}
			break;

		case COL_PERCENTAGE:
			if ( GetOptions( )->IsShowTimeSpent( ) && MustShowReadJobs( ) || IsRootItem( ) ) {
			s.Format( _T( "[%s s]" ), FormatMilliseconds( GetTicksWorked( ) ).GetString( ) );
				}
			else {
			s.Format( _T( "%s%%" ), FormatDouble( GetFraction( ) * 100 ).GetString( ) );
				}
			break;

		case COL_SUBTREETOTAL:
			s = FormatBytes( GetSize( ) );
			break;

		case COL_ITEMS:
			{
				auto typeOfItem = GetType( );
				if ( typeOfItem != IT_FILE && typeOfItem != IT_FREESPACE && typeOfItem != IT_UNKNOWN ) {
					s = FormatCount( GetItemsCount( ) );
					}
				break;
			}
		case COL_FILES:
			{
				auto typeOfItem = GetType( );
				if ( typeOfItem != IT_FILE && typeOfItem != IT_FREESPACE && typeOfItem != IT_UNKNOWN ) {
					s = FormatCount( GetFilesCount( ) );
					}
				break;
			}
		case COL_SUBDIRS:
			{
				auto typeOfItem = GetType( );
				if ( typeOfItem != IT_FILE && typeOfItem != IT_FREESPACE && typeOfItem != IT_UNKNOWN ) {
					s = FormatCount( GetSubdirsCount( ) );
					}
				break;
			}
		case COL_LASTCHANGE:
			{
				auto typeOfItem = GetType( );
				if ( typeOfItem != IT_FREESPACE && typeOfItem != IT_UNKNOWN ) {
					s = FormatFileTime( m_lastChange );//FIXME
					}
				break;
			}
		case COL_ATTRIBUTES:
			{
				auto typeOfItem = GetType( );
				if ( typeOfItem != IT_FREESPACE && typeOfItem != IT_FILESFOLDER && typeOfItem != IT_UNKNOWN && typeOfItem != IT_MYCOMPUTER ) {
					s = FormatAttributes( GetAttributes( ) );
					}
				break;
			}
		default:
			ASSERT(false);
			break;
	}
	return s;
}

COLORREF CItem::GetItemTextColor() const
{
	// Get the file/folder attributes
	DWORD attr = GetAttributes( );

	// This happens e.g. on a Unicode-capable FS when using ANSI APIs to list files with ("real") Unicode names
	if ( attr == INVALID_FILE_ATTRIBUTES ) {
		return std::move( CTreeListItem::GetItemTextColor( ) );
		}
	// Check for compressed flag
	if (attr & FILE_ATTRIBUTE_COMPRESSED) {
		return std::move( GetApp( )->AltColor( ) );
		}
	else if (attr & FILE_ATTRIBUTE_ENCRYPTED) {
		return std::move( GetApp( )->AltEncryptionColor( ) );
		}
	else {
		// The rest is not colored
		return std::move( CTreeListItem::GetItemTextColor( ) );
		}
}

INT CItem::CompareSibling(_In_ const CTreeListItem *tlib, _In_ const INT subitem) const
{ 
	CItem *other = ( CItem * ) tlib;
	ASSERT( subitem >= 0 );
	INT r = 0;
	switch (subitem)
	{
		case COL_NAME:
			if ( GetType( ) == IT_DRIVE ) {
				ASSERT( other->GetType( ) == IT_DRIVE );
				r = signum( GetPath( ).CompareNoCase( other->GetPath( ) ) );
				}
			else {
				r = signum( m_name.CompareNoCase( other->m_name ) );
				}
			break;

		case COL_SUBTREEPERCENTAGE:
			if ( MustShowReadJobs( ) ) {
				r = signum( m_readJobs - other->m_readJobs );
				}
			else {
				r = signum( GetFraction( ) - other->GetFraction( ) );
				}
			break;

		case COL_PERCENTAGE:
			r = signum( GetFraction( ) - other->GetFraction( ) );
			break;

		case COL_SUBTREETOTAL:
			r = signum( GetSize( ) - other->GetSize( ) );
			break;

		case COL_ITEMS:
			r = signum( GetItemsCount( ) - other->GetItemsCount( ) );
			break;

		case COL_FILES:
			r = signum( GetFilesCount( ) - other->GetFilesCount( ) );
			break;

		case COL_SUBDIRS:
			r = signum( GetSubdirsCount( ) - other->GetSubdirsCount( ) );
			break;

		case COL_LASTCHANGE:
			{
				if ( m_lastChange < other->m_lastChange ) {
					return -1;
					}
				else if ( m_lastChange == other->m_lastChange ) {
					return 0;
					}
				else {
					return 1;
					}
			}
			break;

		case COL_ATTRIBUTES:
			r = signum( GetSortAttributes( ) - other->GetSortAttributes( ) );
			break;

		default:
			ASSERT(false);
			break;
	}
	return r;
}

INT CItem::GetChildrenCount() const
{
	return m_children.GetSize();
}

_Must_inspect_result_ CTreeListItem *CItem::GetTreeListChild( _In_ const INT i ) const {
	ASSERT( !( m_children.IsEmpty( ) ) );
	ASSERT( i >= 0 );
	return m_children[ i ];
	}

INT CItem::GetImageToCache( ) const {
	// (Caching is done in CTreeListItem::m_vi.)

	INT image;
	auto type_theItem = GetType( );
	if ( type_theItem == IT_MYCOMPUTER ) {
		image = GetMyImageList( )->GetMyComputerImage( );
		}
	else if ( type_theItem == IT_FILESFOLDER ) {
		image = GetMyImageList( )->GetFilesFolderImage( );
		}
	else if ( type_theItem == IT_FREESPACE ) {
		image = GetMyImageList( )->GetFreeSpaceImage( );
		}
	else if ( type_theItem == IT_UNKNOWN ) {
		image = GetMyImageList( )->GetUnknownImage( );
		}
	else {
		CString path = GetPath();
		auto MyImageList = GetMyImageList( );
		if ( type_theItem == IT_DIRECTORY && GetApp( )->IsMountPoint( path ) ) {
			image = MyImageList->GetMountPointImage( );
			}
		else
		if ( type_theItem == IT_DIRECTORY && GetApp( )->IsJunctionPoint( path ) ) {
			image = MyImageList->GetJunctionImage( );
			}
		else {
			image = MyImageList->GetFileImage( path );
			}
		}
	return image; 
	}

void CItem::DrawAdditionalState(_In_ CDC *pdc, _In_ const CRect& rcLabel) const
{
	ASSERT_VALID( pdc );
	auto thisDocument = GetDocument( );
	if ( !IsRootItem( ) && this == thisDocument->GetZoomItem( ) ) {
		CRect rc = rcLabel;
		rc.InflateRect( 1, 0 );
		rc.bottom++;

		CSelectStockObject sobrush( pdc, NULL_BRUSH );
		CPen pen( PS_SOLID, 2, thisDocument->GetZoomColor( ) );
		CSelectObject sopen( pdc, &pen );

		pdc->Rectangle( rc );
		}
}

_Must_inspect_result_ CItem *CItem::FindCommonAncestor( _In_ const CItem *item1, _In_ const CItem *item2 ) {
	AfxCheckMemory( );
	ASSERT( item1 != NULL);
	ASSERT( item2 != NULL);
	ASSERT( &item1 != NULL);
	ASSERT( &item2 != NULL);

	const CItem *parent = item1;
	while ( !parent->IsAncestorOf( item2 ) ) {
		parent = parent->GetParent( );
		}
	ASSERT( parent != NULL );
	return const_cast< CItem * >( parent );
	}

bool CItem::IsAncestorOf( _In_ const CItem *thisItem ) const {
	AfxCheckMemory( );
	ASSERT( thisItem != NULL );
	ASSERT( &thisItem != NULL );
	const CItem *p = thisItem;
	while ( p != NULL ) {
		if ( p == this ) {
			break;
			}
		p = p->GetParent( );
		}
	return ( p != NULL );
	}

LONGLONG CItem::GetProgressRange() const
{
	switch ( GetType( ) )
	{
		case IT_MYCOMPUTER:
			return GetProgressRangeMyComputer();

		case IT_DRIVE:
			return GetProgressRangeDrive();

		case IT_DIRECTORY:
		case IT_FILESFOLDER:
		case IT_FILE:
			return 0;

		case IT_FREESPACE:
		case IT_UNKNOWN:
		default:
			ASSERT( false );
			return 0;
	}
}

LONGLONG CItem::GetProgressPos() const
{
	switch ( GetType( ) )
	{
		case IT_MYCOMPUTER:
			return GetProgressPosMyComputer( );

		case IT_DRIVE:
			return GetProgressPosDrive( );

		case IT_DIRECTORY:
			return m_files + m_subdirs;

		case IT_FILE:
		case IT_FILESFOLDER:
		case IT_FREESPACE:
		case IT_UNKNOWN:
		default:
			ASSERT( false );
			return 0;
	}
}

_Must_inspect_result_ const CItem *CItem::UpwardGetRoot( ) const {
	AfxCheckMemory( );
	auto myParent = GetParent( );
	if ( myParent == NULL ) {
		return this;
		}
	else {
		return myParent->UpwardGetRoot( );
		}
	}

void CItem::UpdateLastChange( ) {
	m_lastChange.dwHighDateTime = NULL;
	m_lastChange.dwLowDateTime = NULL;
	auto typeOf_thisItem = GetType( );

	if ( typeOf_thisItem == IT_DIRECTORY || typeOf_thisItem == IT_FILE ) {
		CString path = GetPath( );

		INT i = path.ReverseFind( _T( '\\' ) );
		CString basename = path.Mid( i + 1 );
		CString pattern;
		pattern.Format( _T( "%s\\..\\%s" ), path.GetString( ), basename.GetString( ) );
		CFileFindWDS finder;
		BOOL b = finder.FindFile( pattern );
		if ( !b ) {
			return; // no chance
			}
		finder.FindNextFile( );
		finder.GetLastWriteTime( &m_lastChange );
		SetAttributes( finder.GetAttributes( ) );
		}
	}

_Success_(return != NULL) _Must_inspect_result_ CItem *CItem::GetChild(_In_ const INT i) const {
	/*
	  Returns CItem* to child if passed a valid index. Returns NULL if `i` is NOT a valid index. 
	*/
	ASSERT( !( m_children.IsEmpty( ) ) );
	ASSERT( i >= 0 && i <= ( m_children.GetSize( ) - 1 ) );
	if ( i >= 0 && i <= ( m_children.GetSize( ) -1 ) ) {
		return m_children[ i ];
		}
	else {
		return NULL;
		}
	}

_Must_inspect_result_ CItem *CItem::GetParent( ) const {
	return (CItem *)CTreeListItem::GetParent(); 
	}

INT CItem::FindChildIndex( _In_ const CItem *child ) const {
	ASSERT( child != NULL );
	ASSERT( &child != NULL );//wtf
	auto childCount = GetChildrenCount( );	
	for ( INT i = 0; i < childCount; i++ ) {
		if ( child == m_children[ i ] ) {
			return i;
			}
		}
	ASSERT(false);
	return 0;
	}

void CItem::AddChild( _In_ CItem *child ) {
	ASSERT( child != NULL );
	ASSERT( &child != NULL );//wtf

	ASSERT( !IsDone( ) ); // SetDone() computed m_childrenBySize

	// This sequence is essential: First add numbers, then CTreeListControl::OnChildAdded(), because the treelist will display it immediately. If we did it the other way round, CItem::GetFraction() could ASSERT.
	UpwardAddSize( child->GetSize( ) );
	UpwardAddReadJobs( child->GetReadJobs( ) );
	UpwardUpdateLastChange( child->GetLastChange( ) );

	m_children.Add( child );
	ASSERT( this != NULL );
	child->SetParent( this );
	ASSERT( child->GetParent( ) == this );
	ASSERT( !( child->IsRootItem( ) ) );
	auto TreeListControl = GetTreeListControl( );
	if ( TreeListControl != NULL ) {
		TreeListControl->OnChildAdded( this, child );
		}
	else { 
		ASSERT( false );//What does this even mean?
		}
	}

void CItem::RemoveChild(_In_ const INT i) {
	ASSERT( !( m_children.IsEmpty( ) ) );
	ASSERT( i >= 0 && i <= ( m_children.GetSize( ) - 1 ) );
	if ( i >= 0 && ( i <= ( m_children.GetSize( ) - 1 ) ) ) {
		auto child = GetChild( i );
		auto TreeListControl = GetTreeListControl( );
		if ( TreeListControl != NULL ) {
			if ( ( child != NULL ) ) {
				m_children.RemoveAt( i );
				TreeListControl->OnChildRemoved( this, child );
				delete child;
				child = NULL;
				AfxCheckMemory( );
				}
			else {
				ASSERT( false );
				}
			}
		}
	}

void CItem::RemoveAllChildren() {
	auto TreeListControl = GetTreeListControl( );
	if ( TreeListControl != NULL ) {
		TreeListControl->OnRemovingAllChildren( this );
		}
	auto childCount = GetChildrenCount( );
	for ( auto i = 0; i < childCount; i++ ) {
		ASSERT( ( i >= 0 ) && ( i <= GetChildrenCount( ) - 1 ));
		if ( m_children[ i ] != NULL ) {
			delete m_children[ i ];
			m_children[ i ] = NULL;
			AfxCheckMemory( );
			}
		}
	m_children.SetSize( 0 );
	ASSERT( m_children.IsEmpty( ) );
	AfxCheckMemory( );
	}

void CItem::UpwardAddSubdirs( _In_ const LONGLONG dirCount ) {
	m_subdirs += dirCount;
	auto myParent = GetParent( );
	if ( myParent != NULL ) {
		//auto fut = std::async( std::launch::async, &CItem::UpwardAddSubdirs, myParent, dirCount );//async made it SLOWER!!!
		myParent->UpwardAddSubdirs( dirCount );
		}
	else {
		//Valid condition? `this` may be the root item.
		}
	}

void CItem::UpwardAddFiles( _In_ const LONGLONG fileCount ) {
	m_files += fileCount;
	auto theParent = GetParent( );
	if ( theParent != NULL ) {
		theParent->UpwardAddFiles( fileCount );
		}
	else {
		//Valid condition? `this` may be the root item.
		}
	}

void CItem::UpwardAddSize( _In_ const LONGLONG bytes )
{
	ASSERT( bytes >= 0 || bytes == -GetSize( ) || bytes >= ( -1 * ( GetTotalDiskSpace( this->UpwardGetPathWithoutBackslash( ) ) ) ) );
	m_size += bytes;
	auto myParent = GetParent( );
	if ( myParent != NULL ) {
		myParent->UpwardAddSize( bytes );
		}
	else {
		//Valid condition? `this` may be the root item.
		}
}

void CItem::UpwardAddReadJobs( _In_ const /* signed */LONGLONG count )
{
	m_readJobs += count;
	auto myParent = GetParent( );
	if ( myParent != NULL ) {
		myParent->UpwardAddReadJobs( count );
		}
	else {
		//Valid condition? `this` may be the root item.
		}
}

void CItem::UpwardUpdateLastChange(_In_ const FILETIME& t) {
	/*
	  This method increases the last change
	*/
	if ( m_lastChange < t ) {
		m_lastChange = t;
		auto myParent = GetParent( );
		if ( myParent != NULL ) {
			myParent->UpwardUpdateLastChange( t );
			}
		else {
			//Valid condition? `this` may be the root item.
			}
		}
	}


void CItem::UpwardRecalcLastChange() {
	/*
	  This method may also decrease the last change
	*/
	UpdateLastChange( );
	//TRACE( _T( "GetChildrenCount: %i\r\n" ), GetChildrenCount( ) );
	auto childCount = GetChildrenCount( );
	for ( INT i = 0; i < childCount; i++ ) {
		auto child = GetChild( i );
		if ( child != NULL ) {
			auto receivedLastChange = child->GetLastChange( );
			if ( m_lastChange < receivedLastChange ) {
				m_lastChange = receivedLastChange;
				}
			}
		}
	auto myParent = GetParent( );
	if ( myParent != NULL ) {
		myParent ->UpwardRecalcLastChange( );
		}
	else {
		//Valid condition? `this` may be the root item.
		}
	}

LONGLONG CItem::GetSize() const {
	return m_size;
	}

void CItem::SetSize( _In_ const LONGLONG ownSize ) {

#ifdef _DEBUG
	auto typeOf_thisItem = GetType( );
	bool leafness = IsLeaf(typeOf_thisItem);
	if ( !leafness ) {
		TRACE( _T("%s is NOT a leaf!\r\n"), m_name);
		//ITEMTYPE typeItem = GetType( );
		ASSERT( IsLeaf( typeOf_thisItem ) );
		}
	//TRACE( _T( "%s IS a   leaf!\r\n" ), m_name );
	ASSERT( ownSize >= 0 );
#endif
	m_size = ownSize;
	}

LONGLONG CItem::GetReadJobs() const {
	return m_readJobs;
	}

FILETIME CItem::GetLastChange() const {
	return m_lastChange;
	}

void CItem::SetLastChange( _In_ const FILETIME& t ) {
	m_lastChange = t;
	}

// Encode the attributes to fit 1 byte
void CItem::SetAttributes( const DWORD attr ) {
	/*
	Bitmask of m_attributes:

	7 6 5 4 3 2 1 0
	^ ^ ^ ^ ^ ^ ^ ^
	| | | | | | | |__ 1 == R					(0x01)
	| | | | | | |____ 1 == H					(0x02)
	| | | | | |______ 1 == S					(0x04)
	| | | | |________ 1 == A					(0x08)
	| | | |__________ 1 == Reparse point		(0x10)
	| | |____________ 1 == C					(0x20)
	| |______________ 1 == E					(0x40)
	|________________ 1 == invalid attributes	(0x80)
	*/
	
	DWORD ret = attr;

	if ( ret == INVALID_FILE_ATTRIBUTES ) {
		m_attributes = ( unsigned char ) INVALID_m_attributes;
		return;
		}

	ret &=  FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM;// Mask out lower 3 bits
	
	// Prepend the archive attribute
	ret |= ( attr & FILE_ATTRIBUTE_ARCHIVE ) >> 2;
	
	// --> At this point the lower nibble is fully used
	
	// Now shift the reparse point and compressed attribute into the lower 2 bits of the high nibble.
	ret |= ( attr & ( FILE_ATTRIBUTE_REPARSE_POINT | FILE_ATTRIBUTE_COMPRESSED ) ) >> 6;
	
	// Shift the encrypted bit by 8 places
	ret |= ( attr & FILE_ATTRIBUTE_ENCRYPTED ) >> 8;

	m_attributes = ( unsigned char ) ret;
	}

// Decode the attributes encoded by SetAttributes()
DWORD CItem::GetAttributes( ) const {
	DWORD ret = m_attributes;

	if ( ret & INVALID_m_attributes ) {
		return INVALID_FILE_ATTRIBUTES;
		}

	ret &= FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM;// Mask out lower 3 bits
	
	// FILE_ATTRIBUTE_ARCHIVE
	ret |= (m_attributes & 0x08) << 2;
	
	// FILE_ATTRIBUTE_REPARSE_POINT | FILE_ATTRIBUTE_COMPRESSED
	ret |= (m_attributes & 0x30) << 6;
	
	// FILE_ATTRIBUTE_ENCRYPTED
	ret |= (m_attributes & 0x40) << 8;
	
	return ret;
	}

// Returns a value which resembles sorting of RHSACE considering gaps
INT CItem::GetSortAttributes( ) const {
	DWORD ret = 0;

	// We want to enforce the order RHSACE with R being the highest priority attribute and E being the lowest priority attribute.
	ret += (m_attributes & 0x01) ? 1000000 : 0; // R
	ret += (m_attributes & 0x02) ? 100000  : 0; // H
	ret += (m_attributes & 0x04) ? 10000   : 0; // S
	ret += (m_attributes & 0x08) ? 1000    : 0; // A
	ret += (m_attributes & 0x20) ? 100     : 0; // C
	ret += (m_attributes & 0x40) ? 10      : 0; // E

	return ( m_attributes & INVALID_m_attributes ) ? 0 : ret;
	}

DOUBLE CItem::GetFraction() const
{
	auto myParent = GetParent( );
	if ( myParent == NULL ) {
		return 1.0;
		}
	auto parentSize = myParent->GetSize( );
	if ( parentSize == 0){
		return 1.0;
		}
	return (DOUBLE) GetSize() / parentSize;
}

ITEMTYPE CItem::GetType( ) const {
	return ( ITEMTYPE ) ( m_type & ~ITF_FLAGS );
	}

bool CItem::IsRootItem( ) const {
	return ( ( m_type & ITF_ROOTITEM ) != 0 );
	}

CString CItem::GetPath( )  const {
	if ( this == NULL ) {
		TRACE(_T("'this' hasn't been initialized yet!\r\n") );
		CString temp;
		return temp;
		}
	CString path = UpwardGetPathWithoutBackslash( );
	auto typeOfThisItem = GetType( );
	auto Parent = GetParent( );
	if ( Parent != NULL ) {
		if ( ( typeOfThisItem == IT_DRIVE ) || ( typeOfThisItem == IT_FILESFOLDER ) && ( Parent->GetType( ) == IT_DRIVE ) ) {
			path += _T( "\\" );
			}
		}
	else {
		if ( ( typeOfThisItem == IT_DRIVE ) || ( typeOfThisItem == IT_FILESFOLDER ) ) {//ehh TODO: does this make sense?
			path += _T( "\\" );
			}
		}
	return path;
	}

bool CItem::HasUncPath( ) const {
	CString path = GetPath();
	return ( path.GetLength( ) >= 2 && path.Left( 2 ) == _T( "\\\\" ) );
	}

CString CItem::GetFindPattern( ) const {
	CString pattern = GetPath();
	if ( pattern.Right( 1 ) != _T( '\\' ) ) {
		pattern += _T( "\\*.*" );
		}
	else {
		pattern += _T( "*.*" );//Yeah, if you're wondering, `*.*` works for files WITHOUT extensions.
		}
	return pattern;
	}

CString CItem::GetFolderPath( ) const {
	/*
	  Returns the path for "Explorer here" or "Command Prompt here"
	*/
	CString path;
	auto typeOfThisItem = GetType( );
	if ( typeOfThisItem == IT_MYCOMPUTER ) {
		path = GetParseNameOfMyComputer( );
		}
	else {
		path = GetPath( );
		if ( typeOfThisItem == IT_FILE ) {
			INT i = path.ReverseFind( _T( '\\' ) );
			ASSERT( i != -1 );
			path = path.Left( i + 1 );
			}
		}
	return path;
	}

// returns the path for the mail-report
CString CItem::GetReportPath() const
{
	CString path = UpwardGetPathWithoutBackslash();

	if ( GetType( ) == IT_DRIVE || GetType( ) == IT_FILESFOLDER ) {
		path += _T( "\\" );
		}
	
	if ( GetType( ) == IT_FILESFOLDER || GetType( ) == IT_FREESPACE || GetType( ) == IT_UNKNOWN ) {
		path += GetName( );
		}

	return path;
}

CString CItem::GetName() const
{
	return m_name;
}

CString CItem::GetExtension( ) const {
	CString ext;

	switch ( GetType( ) )
	{
		case IT_FILE:
			{
				auto thePath = LPCTSTR (GetName( ) );
				auto ptstrPath = ( PTSTR( thePath ) );
				auto resultPtrStr = PathFindExtension( ptstrPath );
				if ( resultPtrStr != '\0' ) {
					//TRACE( _T( "%s\r\n" ), resultPtrStr );
					ext = resultPtrStr;
					//ext.MakeLower( );
					return ext;
					}
				INT i = GetName( ).ReverseFind( _T( '.' ) );
				if ( i == -1 ) {
					ext = _T( "." );
					}
				else {
					ext = GetName( ).Mid( i );
					}
				ext.MakeLower( );
				break;
			}
		case IT_FREESPACE:
		case IT_UNKNOWN:
			ext = GetName( );
			break;

		default:
			ASSERT(false);
	}
	return ext;
	}

LONGLONG CItem::GetFilesCount() const
{
	return m_files;
}

LONGLONG CItem::GetSubdirsCount() const
{
	return m_subdirs;
}

LONGLONG CItem::GetItemsCount() const
{
	return m_files + m_subdirs;
}

bool CItem::IsReadJobDone() const
{ 
	return m_readJobDone;
}

void CItem::SetReadJobDone( const bool done )
{ 
	//if ( !IsReadJobDone( ) && done ) {
	if ( !m_readJobDone && done ) {
		//TRACE( _T( "IsReadJobDone: %i, m_readJobDone: %i\r\n" ), IsReadJobDone( ), m_readJobDone );
		UpwardAddReadJobs( -1 );
		}
	else {
		UpwardAddReadJobs( 1 - m_readJobs );
		}
	m_readJobDone = done;
}

bool CItem::IsDone() const
{ 
	return m_done; 
}

void CItem::SetDone() 
{ 
	if ( m_done ) {
		return;
		}
	if ( GetType( ) == IT_DRIVE ) {
		//UpdateFreeSpaceItem();
		if ( GetDocument( )->OptionShowUnknown( ) ) {
			CItem *unknown = FindUnknownItem( );
			if ( unknown != NULL ) {
				if ( unknown->GetType( ) == IT_DIRECTORY ) {
					}
				else {
					LONGLONG total;
					LONGLONG free;
					total = 0;
					free = 0;
					auto thisPath = GetPath( );
					TRACE( _T( "MyGetDiskFreeSpace, path: %s\r\n" ), thisPath );
					MyGetDiskFreeSpace( thisPath, total, free );//redundant?

					LONGLONG unknownspace = total - GetSize( );

					if ( !GetDocument( )->OptionShowFreeSpace( ) ) {
						unknownspace -= free;
						}

					// For CDs, the GetDiskFreeSpaceEx()-function is not correct.
					if ( ( unknownspace < 0 ) || ( free < 0 ) || ( total < 0 ) ) {
						TRACE( _T( "GetDiskFreeSpace(%s), (unknownspace: %lld), (free: %lld), (total: %lld) incorrect.\r\n" ), thisPath, unknownspace, free, total );
						unknownspace = 0;
						}
					unknown->SetSize( unknownspace );
					UpwardAddSize( unknownspace );
					}
				}
			}
		}

	qsort( m_children.GetData( ), m_children.GetSize( ), sizeof( CItem * ), &_compareBySize );

	m_rect.bottom = NULL;
	m_rect.left   = NULL;
	m_rect.right  = NULL;
	m_rect.top    = NULL;
	m_done = true;
}

unsigned long long CItem::GetTicksWorked() const
{ 
	return m_ticksWorked; 
}

void CItem::AddTicksWorked(_In_ const unsigned long long more) 
{ 
	ASSERT( more >= 0 );
	m_ticksWorked += more; 
}

void CItem::DoSomeWork(_In_ const unsigned long long ticks)
{
	ASSERT( ticks >= 0 );
	if ( m_done ) {
		return;
		}
	StartPacman( true );
	auto start = GetTickCount64( );
	auto typeOfThisItem = GetType( );
	if ( typeOfThisItem == IT_DRIVE || typeOfThisItem == IT_DIRECTORY ) {
		if ( !m_readJobDone ) {
			LONGLONG dirCount = 0;
			LONGLONG fileCount = 0;
			CList<FILEINFO, FILEINFO> files;
			CFileFindWDS finder;
			BOOL b = finder.FindFile( GetFindPattern( ) );
			while ( b ) {
				b = finder.FindNextFile();
				if ( finder.IsDots( ) ) {
					continue;
					}
				if ( finder.IsDirectory( ) ) {
					dirCount++;
					AddDirectory( std::move( finder ) );
					}
				else {
					fileCount++;
					FILEINFO fi;
					fi.name = finder.GetFileName( );
					fi.attributes = finder.GetAttributes( );
					if ( fi.attributes & FILE_ATTRIBUTE_COMPRESSED ) {
						fi.length = finder.GetCompressedLength( ); // Retrieve file size //MAYBE GetFileInformationByHandleEx would be faster?
						}
					else {
						fi.length = finder.GetLength( );//temp
						}
					finder.GetLastWriteTime( &fi.lastWriteTime );
					// (We don't use GetLastWriteTime(CTime&) here, because, if the file has an invalid timestamp, that function would ASSERT and throw an Exception.)
					files.AddTail( std::move( fi ) );
					}
				if ( ( GetTickCount64( ) - start ) > ticks && ( GetTickCount64( ) % 1000 ) == 0 ) {
					DriveVisualUpdateDuringWork( );
					TRACE( _T( "Exceeding number of ticks! (%llu > %llu)\r\n" ), (GetTickCount64() - start), ticks );
					TRACE( _T( "pumping messages - this is a dirty hack to ensure responsiveness while single-threaded.\r\n" ) );
					}
				}
			CItem* filesFolder = NULL;
			if ( dirCount > 0 && fileCount > 1 ) {
				filesFolder = new CItem( IT_FILESFOLDER, _T( "<Files>" ) );
				filesFolder->SetReadJobDone( );
				AddChild( std::move( filesFolder ) );
				}
			else if (fileCount > 0) {
				filesFolder = this;
				}
			for ( POSITION pos = files.GetHeadPosition( ); pos != NULL; files.GetNext( pos ) ) {
				const FILEINFO& fi = files.GetAt( pos );
				filesFolder->AddFile( std::move( fi ) );
				}
			if ( filesFolder != NULL ) {
				filesFolder->UpwardAddFiles( fileCount );
				if ( dirCount > 0 && fileCount > 1 ) {
					filesFolder->SetDone( );
					}
				}
			//auto UpwardAddSubdirectories = std::async(std::launch::async, &CItem::UpwardAddSubdirs, this, dirCount);//async made it SLOWER!
			UpwardAddSubdirs( dirCount );
			SetReadJobDone( );
			AddTicksWorked( GetTickCount64( ) - start );
			}
		if ( GetTickCount64( ) - start > ticks ) {
			if ( typeOfThisItem == IT_DRIVE && IsReadJobDone( ) ) {
				UpdateFreeSpaceItem( );
				}
			StartPacman( false );
			return;
			}
		}
	//auto thisType_rightHere = GetType( );
	if ( typeOfThisItem == IT_DRIVE || typeOfThisItem == IT_DIRECTORY || typeOfThisItem == IT_MYCOMPUTER ) {
		ASSERT( IsReadJobDone( ) );
		//if ( IsDone( ) ) {
		//	StartPacman( false );
		//	return;
		//	}
		if ( GetChildrenCount( ) == 0 ) {
			SetDone( );
			StartPacman( false );
			return;
			}
		auto startChildren = GetTickCount64( );
		while ( GetTickCount64( ) - start < ticks ) {
			unsigned long long minticks = UINT_MAX;
			CItem *minchild = NULL;
			auto countOfChildren = GetChildrenCount( );
			for ( INT i = 0; i < countOfChildren; i++ ) {
				auto child = GetChild( i );
				if ( child != NULL ) {
					if ( child->IsDone( ) ) {
						continue;
						}
					if ( child->GetTicksWorked( ) < minticks ) {
						minticks = child->GetTicksWorked( );
						minchild = child;
						}
					}
				}
			if ( minchild == NULL ) {
				SetDone( );
				break;
				}
			auto tickssofar = GetTickCount64( ) - start;
			if ( ticks > tickssofar ) {
				minchild->DoSomeWork( ticks - tickssofar );
				}
			}
		AddTicksWorked( GetTickCount64( ) - startChildren );
		}
	else {
		SetDone( );
		}
	StartPacman( false );
}


bool CItem::StartRefresh( ) {
	/*
	  Returns false if deleted
	*/

	AfxCheckMemory( );
	m_ticksWorked = 0;

	auto typeOf_thisItem = GetType( );

	ASSERT( typeOf_thisItem != IT_FREESPACE );
	ASSERT( typeOf_thisItem != IT_UNKNOWN );

	// Special case IT_MYCOMPUTER
	if ( typeOf_thisItem == IT_MYCOMPUTER ) {
		m_lastChange.dwHighDateTime = 0;
		m_lastChange.dwLowDateTime = 0;
		auto childCount = GetChildrenCount( );
		for ( INT i = 0; i < childCount; i++ ) {
			auto Child = GetChild( i );
			if ( Child != NULL ) {
				Child->StartRefresh( );
				}
			}

		return true;
		}
	ASSERT(typeOf_thisItem == IT_FILE || typeOf_thisItem == IT_DRIVE || typeOf_thisItem == IT_DIRECTORY || typeOf_thisItem == IT_FILESFOLDER);

	bool wasExpanded = IsVisible( ) && IsExpanded( );
	auto oldScrollPosition = 0;
	if ( IsVisible( ) ) {
		oldScrollPosition = GetScrollPosition( );
		ASSERT( oldScrollPosition >= 0 );
		}

	UncacheImage( );

	// Upward clear data
	UpdateLastChange( );

	UpwardSetUndone( );

	UpwardAddReadJobs( -GetReadJobs( ) );
	ASSERT( GetReadJobs( ) == 0 );

	if ( typeOf_thisItem == IT_FILE ) {
		auto Parent = GetParent( );
		if ( Parent != NULL ) {
			Parent->UpwardAddFiles( -1 );
			}
		}
	else {
		UpwardAddFiles( -GetFilesCount( ) );
		}
	ASSERT( GetFilesCount( ) == 0 );

	if ( typeOf_thisItem == IT_DIRECTORY || typeOf_thisItem == IT_DRIVE ) {
		UpwardAddSubdirs( -GetSubdirsCount( ) );
		}
	ASSERT( GetSubdirsCount( ) == 0 );

	UpwardAddSize( -GetSize( ) );
	ASSERT( GetSize( ) == 0 );

	RemoveAllChildren( );
	UpwardRecalcLastChange( );

	// Special case IT_FILESFOLDER
	if ( typeOf_thisItem == IT_FILESFOLDER ) {
		CFileFindWDS finder;
		BOOL b = finder.FindFile( GetFindPattern( ) );
		while (b) {
			b = finder.FindNextFile();
			if ( finder.IsDirectory( ) ) {
				continue;
				}

			FILEINFO fi;
			fi.name = finder.GetFileName();
			fi.attributes = finder.GetAttributes();
			// Retrieve file size
			fi.length = finder.GetCompressedLength( );
			finder.GetLastWriteTime( &fi.lastWriteTime );

			AddFile( std::move( fi ) );
			UpwardAddFiles(1);
			}
		SetDone();
		if ( wasExpanded ) {
			auto TreeListControl = GetTreeListControl( );
			if ( TreeListControl != NULL ) {
				TreeListControl->ExpandItem( this );
				}
			}
		return true;
		}
	ASSERT( typeOf_thisItem == IT_FILE || typeOf_thisItem == IT_DRIVE || typeOf_thisItem == IT_DIRECTORY );

	// The item may have been deleted.
	bool deleted = false;
	if ( typeOf_thisItem == IT_DRIVE ) {
		deleted = !DriveExists( GetPath( ) );
		}
	else if ( typeOf_thisItem == IT_FILE ) {
		deleted = !FileExists( GetPath( ) );
		}
	else if ( typeOf_thisItem == IT_DIRECTORY ) {
		deleted = !FolderExists( GetPath( ) );
		}
	if ( deleted ) {
		auto myParent_here = GetParent( );
		if (myParent_here == NULL) {
			GetDocument( )->UnlinkRoot( );
		}
		else {
			myParent_here->UpwardRecalcLastChange( );
			auto myParent_IndexOfME = myParent_here->FindChildIndex( this );
			myParent_here->RemoveChild( myParent_IndexOfME );// --> delete this
			}
		return false;
		}
	// Case IT_FILE
	if ( typeOf_thisItem == IT_FILE ) {
		CFileFindWDS finder;
		BOOL b = finder.FindFile( GetPath( ) );
		if ( b ) {
			finder.FindNextFile( );
			if (!finder.IsDirectory()) {
				FILEINFO fi;
				fi.name = finder.GetFileName( );
				fi.attributes = finder.GetAttributes( );
				// Retrieve file size
				fi.length = finder.GetCompressedLength( );
				finder.GetLastWriteTime( &fi.lastWriteTime );

				SetLastChange( fi.lastWriteTime );

				UpwardAddSize( fi.length );
				UpwardUpdateLastChange( GetLastChange( ) );
				auto Parent = GetParent( );
				if ( Parent != NULL ) {
					Parent->UpwardAddFiles( 1 );
					}
				}
			}
		SetDone( );
		return true;
		}
	ASSERT( typeOf_thisItem == IT_DRIVE || typeOf_thisItem == IT_DIRECTORY );
	auto Options = GetOptions( );
	auto App = GetApp( );
	if ( Options != NULL ) {
		if ( typeOf_thisItem == IT_DIRECTORY && !IsRootItem( ) && App->IsMountPoint( GetPath( ) ) && !Options->IsFollowMountPoints( ) ) {
			return true;
			}
		if ( typeOf_thisItem == IT_DIRECTORY && !IsRootItem( ) && App->IsJunctionPoint( GetPath( ) ) && !Options->IsFollowJunctionPoints( ) ) {
			return true;
			}
		}
	else { 
		//Fall back to values that I like :)
		if ( typeOf_thisItem == IT_DIRECTORY && !IsRootItem( ) && App->IsMountPoint( GetPath( ) ) ) {
			return true;
			}
		if ( typeOf_thisItem == IT_DIRECTORY && !IsRootItem( ) && App->IsJunctionPoint( GetPath( ) ) ) {
			return true;
			}
		}
	// Initiate re-read
	SetReadJobDone( false );
	// Re-create <free space> and <unknown>
	if ( typeOf_thisItem == IT_DRIVE ) {
		auto Document = GetDocument( );
		if ( Document != NULL ) {
			if ( Document->OptionShowFreeSpace( ) ) {
				CreateFreeSpaceItem( );
				}
			if ( Document->OptionShowUnknown( ) ) {
				CreateUnknownItem( );
				}
			}
		else {
			//Fall back to values that I like :)
			CreateFreeSpaceItem( );
			CreateUnknownItem( );
			}
		}
	DoSomeWork( 999 );
	if ( wasExpanded ) {
		auto TreeListControl = GetTreeListControl( );
		if ( TreeListControl != NULL ) {
			TreeListControl->ExpandItem( this );
			}
		else {
			ASSERT( false );//What the fuck would this even mean??
			}
		}
	if ( IsVisible( ) ) {
		SetScrollPosition( oldScrollPosition );
		}
	return true;
}

void CItem::UpwardSetUndone( ) {
	auto thisItemType = GetType( );
	if ( thisItemType == IT_DIRECTORY ) {
		}
	else {
		auto Document = GetDocument( );
		if ( Document != NULL ) {
			if ( thisItemType == IT_DRIVE && IsDone( ) && Document->OptionShowUnknown( ) ) {
				auto childCount = GetChildrenCount( );
				for ( INT i = 0; i < childCount; i++ ) {
					auto thisChild = GetChild( i );
					if ( thisChild != NULL ) {
						auto childType = thisChild->GetType( );
						if ( ( childType == IT_UNKNOWN ) || ( childType == IT_DIRECTORY ) ) {
							break;
							}
						auto unknown = thisChild;
						UpwardAddSize( -unknown->GetSize( ) );
						unknown->SetSize( 0 );
						}
					else {
						ASSERT( false );
						}
					}
				}
			}
		else {
			ASSERT( false );
			}
		}
		m_done = false;
		auto Parent = GetParent( );
		if ( Parent != NULL ) {
			Parent->UpwardSetUndone( );
			}
	}

void CItem::CreateFreeSpaceItem( ) {
	ASSERT( GetType( ) == IT_DRIVE );
	UpwardSetUndone( );
	LONGLONG total = 0;
	LONGLONG free = 0;

	TRACE( _T( "MyGetDiskFreeSpace\r\n" ) );
	MyGetDiskFreeSpace( GetPath( ), total, free );
	auto freespace = new CItem( IT_FREESPACE, GetFreeSpaceItemName( ) );//std::make_shared<CItem>
	freespace->SetSize( free );
	freespace->SetDone( );
	AddChild( freespace );
	}

_Success_(return != NULL) _Must_inspect_result_ CItem *CItem::FindFreeSpaceItem( ) const {
	INT i = FindFreeSpaceItemIndex( );
	if ( i < GetChildrenCount( ) ) {
		return GetChild( i );
		}
	else {
		return NULL;
		}
	}


void CItem::UpdateFreeSpaceItem( ) {
	ASSERT( GetType( ) == IT_DRIVE );
	if ( !GetDocument( )->OptionShowFreeSpace( ) ) {
		return;
		}
	auto freeSpaceItem = FindFreeSpaceItem( );
	if ( freeSpaceItem != NULL ) {
		LONGLONG total = 0;
		LONGLONG free = 0;
		TRACE( _T( "MyGetDiskFreeSpace, path: %s\r\n" ), GetPath( ) );
		MyGetDiskFreeSpace( GetPath( ), total, free );
		LONGLONG before = freeSpaceItem->GetSize( );
		LONGLONG diff = free - before;
		freeSpaceItem->UpwardAddSize( diff );
		ASSERT( freeSpaceItem->GetSize( ) == free );
		}
	}

void CItem::RemoveFreeSpaceItem( ) {
	ASSERT( GetType( ) == IT_DRIVE );
	UpwardSetUndone( );
	auto i = FindFreeSpaceItemIndex( );
	ASSERT( i < GetChildrenCount( ) );
	if ( i < GetChildrenCount( ) ) {
		auto freespace = GetChild( i );
		if ( freespace != NULL ) {
			UpwardAddSize( -freespace->GetSize( ) );
			RemoveChild( i );
			}
		}
	}

void CItem::CreateUnknownItem( ) {
	ASSERT( GetType( ) == IT_DRIVE );
	UpwardSetUndone( );
	auto unknown = new CItem( IT_UNKNOWN, GetUnknownItemName( ) );//std::make_shared<CItem>
	unknown->SetDone( );
	AddChild( unknown );
	}

_Success_(return != NULL) _Must_inspect_result_ CItem *CItem::FindUnknownItem( ) const {
	auto i = FindUnknownItemIndex( );
	ASSERT( i >= 0 );
	if ( i < GetChildrenCount( ) ) {
		return GetChild( i );
		}
	else {
		return NULL;
		}
	}

void CItem::RemoveUnknownItem( ) {
	ASSERT( GetType( ) == IT_DRIVE );

	UpwardSetUndone( );

	auto i = FindUnknownItemIndex( );
	ASSERT( i < GetChildrenCount( ) );
	if ( i < GetChildrenCount( ) ) {
		auto unknown = GetChild( i );
		if ( unknown != NULL ) {
			UpwardAddSize( -unknown->GetSize( ) );
			RemoveChild( i );
			}
		}
	}

_Success_(return != NULL) _Must_inspect_result_ CItem *CItem::FindDirectoryByPath( _In_ const CString& path ) {
	AfxCheckMemory( );
	ASSERT( path != _T( "" ) );
	auto myPath = GetPath( );
	myPath.MakeLower( );

	INT i = 0;
	auto myPath_GetLength = myPath.GetLength( );
	auto path_GetLength = path.GetLength( );
	while ( i < myPath_GetLength && i < path_GetLength && myPath[ i ] == path[ i ] ) {
		i++;
		}

	if ( i < myPath_GetLength ) {
		return NULL;
		}

	if ( i >= path_GetLength ) {
		ASSERT( myPath == path );
		return this;
		}

	auto thisChildCount = GetChildrenCount( );
	for ( i = 0; i < thisChildCount; i++ ) {
		auto Child = GetChild( i );
		if ( Child != NULL ) {
			auto item = Child->FindDirectoryByPath( path );
			if ( item != NULL ) {
				return item;
				}
			}
		}
	return NULL;
	}

void CItem::RecurseCollectExtensionData( _Inout_ CExtensionData *ed ) {
	auto typeOfItem = GetType( );
	if ( IsLeaf( typeOfItem ) ) {

		if ( typeOfItem == IT_FILE ) {

			CString ext = GetExtension( );
			SExtensionRecord r;

			if ( ed->Lookup( ext, r ) ) {
				r.bytes += GetSize( );
				r.files++;
				}

			else {
				r.bytes = GetSize( );
				r.files = 1;
				}
			ed->SetAt( ext, r );
			}
		}
	else {
		auto childrenCount = GetChildrenCount( );
		for ( INT i = 0; i < childrenCount; i++ ) {
			auto Child = GetChild( i );
			if ( Child != NULL ) {
				Child->RecurseCollectExtensionData( ed );
				}
			}
		}
	}

void CItem::stdRecurseCollectExtensionData( _Inout_ std::map<CString, SExtensionRecord>& stdExtensionData ) {
	auto typeOfItem = GetType( );
	if ( IsLeaf( typeOfItem ) ) {
		if ( typeOfItem == IT_FILE ) {
			auto ext = GetExtension( );
			SExtensionRecord r;
			if ( stdExtensionData.count( ext ) > 0 ) {
				r.bytes = GetSize( ) + stdExtensionData[ext].bytes;
				r.files = stdExtensionData[ext].files + 1;

				}
			else {
				r.bytes = GetSize( );
				r.files = 1;
				}
			stdExtensionData[ ext ] = r;
			}
		}
	else {
		auto childCount = GetChildrenCount( );
		for ( INT i = 0; i < childCount; ++i ) {
			auto Child = GetChild( i );
			if ( Child != NULL ) {
				Child->stdRecurseCollectExtensionData( stdExtensionData );
				}
			}
		}
	}


INT __cdecl CItem::_compareBySize( _In_ const void *p1, _In_ const void *p2 ) {
	CItem *item1 = *( CItem ** ) p1;
	CItem *item2 = *( CItem ** ) p2;
	LONGLONG size1 = item1->GetSize( );
	LONGLONG size2 = item2->GetSize( );

	// TODO: Use 2nd sort column (as set in our TreeListView?)
	return signum( size2 - size1 ); // biggest first
	}

LONGLONG CItem::GetProgressRangeMyComputer( ) const {
	ASSERT( GetType( ) == IT_MYCOMPUTER );
	LONGLONG range = 0;
	auto childCountHere = GetChildrenCount( );
	for ( INT i = 0; i < childCountHere; i++ ) {
		auto Child = GetChild( i );
		if ( Child != NULL ) {
			range += Child->GetProgressRangeDrive( );
			}
		}
	return range;
	}

LONGLONG CItem::GetProgressPosMyComputer() const
{
	ASSERT( GetType( ) == IT_MYCOMPUTER );
	LONGLONG pos = 0;
	auto childCountHere = GetChildrenCount( );
	for (INT i = 0; i < childCountHere; i++) {
		auto Child = GetChild( i );
		if ( Child != NULL ) {
			pos += Child->GetProgressPosDrive( );
			}
		}
	return pos;
}

LONGLONG CItem::GetProgressRangeDrive( ) const {
	LONGLONG total = 0;
	LONGLONG free = 0;
	//TRACE( _T( "MyGetDiskFreeSpace\r\n" ) );
	if ( ( m_freeDiskSpace == -1 ) && ( m_totalDiskSpace == -1 ) ) {
		MyGetDiskFreeSpace( GetPath( ), total, free );
		}
	else {
		total = m_totalDiskSpace;
		free = m_freeDiskSpace;
		}
	auto range = total - free;
	ASSERT( range >= 0 );
	return range;
	}

LONGLONG CItem::GetProgressPosDrive() const
{
	auto pos = GetSize( );
	CItem *fs = FindFreeSpaceItem( );
	if ( fs != NULL ) {
		pos -= fs->GetSize( );
		}
	return pos;
}

COLORREF CItem::GetGraphColor( ) const {
	COLORREF color;

	switch ( GetType() )
	{
		case IT_UNKNOWN:
			color = RGB( 255, 255, 0 ) | CTreemap::COLORFLAG_LIGHTER;
			break;

		case IT_FREESPACE:
			color = RGB( 100, 100, 100 ) | CTreemap::COLORFLAG_DARKER;
			break;

		case IT_FILE:
			color = GetDocument( )->GetCushionColor( GetExtension( ) );
			break;

		default:
			color = RGB( 0, 0, 0 );
			break;
	}
	return std::move( color );
	}

bool CItem::MustShowReadJobs() const
{
	auto myParent = GetParent( );
	if ( myParent != NULL ) {
		return !myParent->IsDone( );
		}
	else {
		return !IsDone( );
		}
}

COLORREF CItem::GetPercentageColor( ) const {
	auto Options = GetOptions( );
	if ( Options != NULL ) {
		auto i = GetIndent( ) % Options->GetTreelistColorCount( );
		return std::move( Options->GetTreelistColor( i ) );
		}
	else {
		ASSERT( false );//should never ever happen, but just in case, we'll generate a random color.
		DWORD fakeColor = 0;
		fakeColor = ( DWORD ) rand( );
		return std::move( fakeColor );
		}
	}

INT CItem::FindFreeSpaceItemIndex() const
{
	auto childCount = GetChildrenCount( );
	for ( INT i = 0; i < childCount; i++ ) {
		if ( GetChild( i )->GetType( ) == IT_FREESPACE ) {
			//break;
			return i; // maybe == GetChildrenCount() (=> not found)
			}
		}
	return childCount;
}

INT CItem::FindUnknownItemIndex() const
{
	auto childCount = GetChildrenCount( );
	for ( INT i = 0; i < childCount; i++ ) {
		if ( GetChild( i )->GetType( ) == IT_UNKNOWN ) {
			//break;
			return i; // maybe == GetChildrenCount() (=> not found)
			}	
		}
	return childCount;
}

CString CItem::UpwardGetPathWithoutBackslash() const
{
	if ( m_pathWithoutBackslash != "" ) {
		//return m_pathWithoutBackslash;
		}
	CString path;
	auto myParent = GetParent( );
	if ( myParent != NULL ) {
		path = myParent->UpwardGetPathWithoutBackslash( );
		}
	switch (GetType())
	{
		case IT_MYCOMPUTER:
			// empty
			break;

		case IT_DRIVE:
			// (we don't use our parent's path here.)
			path = PathFromVolumeName(m_name);
			break;

		case IT_DIRECTORY:
			if ( !path.IsEmpty( ) ) {
				path += _T( "\\" );
				}
			path += m_name;
			break;

		case IT_FILE:
			path += _T("\\") + m_name;
			break;

		case IT_FILESFOLDER:
			break;

		case IT_FREESPACE:
		case IT_UNKNOWN:
			break;

		default:
			ASSERT(false);
	}
	//m_pathWithoutBackslash = path;
	return path; 
}

void CItem::AddDirectory( _In_ const CFileFindWDS& finder ) {
	ASSERT( &finder != NULL );
	auto thisApp = GetApp( );
	auto thisFilePath = finder.GetFilePath( );
	auto thisOptions = GetOptions( );
	bool dontFollow = thisApp->IsMountPoint( thisFilePath ) && !thisOptions->IsFollowMountPoints( );
	
	//dontFollow |= thisApp->IsJunctionPoint( thisFilePath ) && !thisOptions->IsFollowJunctionPoints( );
	dontFollow |= thisApp->IsJunctionPoint( thisFilePath, finder.GetAttributes( ) ) && !thisOptions->IsFollowJunctionPoints( );

	CItem *child = new CItem( IT_DIRECTORY, finder.GetFileName( ), dontFollow );
	FILETIME t;
	finder.GetLastWriteTime( &t );
	child->SetLastChange( t );
	child->SetAttributes( finder.GetAttributes( ) );
	AddChild( child );
	}

void CItem::AddFile(_In_ const FILEINFO& fi)
{
	ASSERT( &fi != NULL );
	CItem *child = new CItem( IT_FILE, fi.name );
	child->SetSize( fi.length );
	child->SetLastChange( fi.lastWriteTime );
	child->SetAttributes( fi.attributes );
	child->SetDone( );
	AddChild( child );
}

void CItem::DriveVisualUpdateDuringWork( ) {
	AfxCheckMemory( );
	MSG msg;
	while ( PeekMessage( &msg, NULL, WM_PAINT, WM_PAINT, PM_REMOVE ) ) {
		DispatchMessage( &msg );
		}
	//GetMainFrame( )->DrivePacman( );
	//UpwardDrivePacman( );
	}

void CItem::UpwardDrivePacman()
{
	if ( !GetOptions( )->IsPacmanAnimation( ) ) {
		return;
		}
	DrivePacman( );
	auto myParent = GetParent( );
	if ( myParent != NULL  && myParent->IsVisible( ) ) {
		myParent->UpwardDrivePacman( );
		}
}

void CItem::DrivePacman()
{
	if ( !IsVisible( ) ) {
		return;
		}

	if ( !CTreeListItem::DrivePacman( GetReadJobs( ) ) ) {
		return;
		}

	auto thisTreeListControl = GetTreeListControl( );
	auto i = thisTreeListControl->FindTreeItem(this);
	CClientDC dc( thisTreeListControl );
	CRect rc = thisTreeListControl->GetWholeSubitemRect( i, COL_SUBTREEPERCENTAGE );
	rc.DeflateRect( sizeDeflatePacman );
	DrawPacman( &dc, rc, thisTreeListControl->GetItemSelectionBackgroundColor( i ) );
}


// $Log$
// Revision 1.27  2005/04/10 16:49:30  assarbad
// - Some smaller fixes including moving the resource string version into the rc2 files
//
// Revision 1.26  2004/12/31 16:01:42  bseifert
// Bugfixes. See changelog 2004-12-31.
//
// Revision 1.25  2004/12/12 08:34:59  bseifert
// Aboutbox: added Authors-Tab. Removed license.txt from resource dlls (saves 16 KB per dll).
//
// Revision 1.24  2004/11/29 07:07:47  bseifert
// Introduced SRECT. Saves 8 Bytes in sizeof(CItem). Formatting changes.
//
// Revision 1.23  2004/11/28 19:20:46  assarbad
// - Fixing strange behavior of logical operators by rearranging code in
//   CItem::SetAttributes() and CItem::GetAttributes()
//
// Revision 1.22  2004/11/28 15:38:42  assarbad
// - Possible sorting implementation (using bit-order in m_attributes)
//
// Revision 1.21  2004/11/28 14:40:06  assarbad
// - Extended CFileFindWDS to replace a global function
// - Now packing/unpacking the file attributes. This even spares a call to find encrypted/compressed files.
//
// Revision 1.20  2004/11/25 23:07:23  assarbad
// - Derived CFileFindWDS from CFileFind to correct a problem of the ANSI version
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
// Revision 1.17  2004/11/12 22:14:16  bseifert
// Eliminated CLR_NONE. Minor corrections.
//
// Revision 1.16  2004/11/12 00:47:42  assarbad
// - Fixed the code for coloring of compressed/encrypted items. Now the coloring spans the full row!
//
// Revision 1.15  2004/11/10 01:03:00  assarbad
// - Style cleaning of the alternative coloring code for compressed/encrypted items
//
// Revision 1.14  2004/11/08 00:46:26  assarbad
// - Added feature to distinguish compressed and encrypted files/folders by color as in the Windows 2000/XP explorer.
//   Same rules apply. (Green = encrypted / Blue = compressed)
//
// Revision 1.13  2004/11/07 20:14:30  assarbad
// - Added wrapper for GetCompressedFileSize() so that by default the compressed file size will be shown.
//
// Revision 1.12  2004/11/05 16:53:07  assarbad
// Added Date and History tag where appropriate.
//
