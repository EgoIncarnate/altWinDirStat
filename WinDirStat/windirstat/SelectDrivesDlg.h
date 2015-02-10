// SelectDrivesDlg.h	- Declaration of CDriveItem, CDrivesList and CSelectDrivesDlg
//
// see `file_header_text.txt` for licensing & contact info.

#pragma once

#include "stdafx.h"

#ifndef WDS_SELECTDRIVESDLG_H
#define WDS_SELECTDRIVESDLG_H


#include "ownerdrawnlistcontrol.h"
#include "layout.h"

class CDrivesList;
class CDriveItem;

// CDriveItem. An item in the CDrivesList Control. All methods are called by the gui thread.
class CDriveItem final : public COwnerDrawnListItem {
public:
	
	//default constructor DOES NOT initialize.
	CDriveItem( ) { }
	CDriveItem                ( _In_z_ PCWSTR const name, const std::uint16_t length ) : m_path( name ), m_totalBytes( 0 ), m_freeBytes( 0 ), m_used( -1 ), COwnerDrawnListItem( name, length ) { }
	CDriveItem( const CDriveItem& in ) = delete;
	CDriveItem& operator=( const CDriveItem& in ) = delete;

	virtual ~CDriveItem( ) final = default;
	
	virtual COLORREF ItemTextColor( ) const override final {
		return default_item_text_color( );
		}

private:
	//concrete_compare is called as a single line INSIDE a single line function. Let's ask for inlining.
	inline  INT concrete_compare( _In_ const CDriveItem* const other, RANGE_ENUM_COL const column::ENUM_COL subitem ) const {
		switch ( subitem )
		{
			case column::COL_NAME:
				WDS_ASSERT_NEVER_REACHED( );
				return default_compare( other );

			case column::COL_TOTAL:
				ASSERT( static_cast< std::uint64_t >( INT64_MAX ) > m_totalBytes );
				ASSERT( static_cast< std::uint64_t >( INT64_MAX ) > other->m_totalBytes );
				return signum( static_cast<std::int64_t>( m_totalBytes ) - static_cast<std::int64_t>( other->m_totalBytes ) );

			case column::COL_FREE:
				ASSERT( static_cast< std::uint64_t >( INT64_MAX ) > m_freeBytes );
				ASSERT( static_cast< std::uint64_t >( INT64_MAX ) > other->m_freeBytes );
				return signum( static_cast<std::int64_t>( m_freeBytes ) - static_cast<std::int64_t>( other->m_freeBytes ) );

			case column::COL_ATTRIBUTES:
			case column::COL_BYTES:
			case column::COL_BYTESPERCENT:
			case column::COL_FILES_TYPEVIEW:
			default:
				WDS_ASSERT_NEVER_REACHED( );
				return 0;
		}
		}
	
	virtual INT Compare( _In_ const COwnerDrawnListItem* const baseOther, RANGE_ENUM_COL const column::ENUM_COL subitem ) const override final {
		if ( subitem == column::COL_NAME ) {
			return default_compare( baseOther );
			}
		const auto other = static_cast<const CDriveItem*>( baseOther );
		return concrete_compare( other, subitem );
		}
	
	
	//CDriveItem NEVER draws self.
	virtual bool DrawSubitem( RANGE_ENUM_COL const column::ENUM_COL subitem, _In_ CDC& pdc, _In_ RECT rc, _In_ const UINT state, _Out_opt_ _Deref_out_range_( 0, 100 ) INT* const width, _Inout_ INT* const focusLeft, _In_ const COwnerDrawnListCtrl* const list ) const override final {
		UNREFERENCED_PARAMETER( pdc );
		UNREFERENCED_PARAMETER( state );
		UNREFERENCED_PARAMETER( rc );
		UNREFERENCED_PARAMETER( focusLeft );
		UNREFERENCED_PARAMETER( subitem );
		UNREFERENCED_PARAMETER( list );
		if ( width != NULL ) {
			*width = 100;
			}
		return false;

		}

	_Must_inspect_result_ _Success_( SUCCEEDED( return ) )
	virtual HRESULT      Text_WriteToStackBuffer( RANGE_ENUM_COL const column::ENUM_COL subitem, WDS_WRITES_TO_STACK( strSize, chars_written ) PWSTR psz_text, _In_ const rsize_t strSize, _On_failure_( _Post_valid_ ) rsize_t& sizeBuffNeed, _Out_ rsize_t& chars_written ) const override final {
		switch ( subitem )
		{
				case column::COL_TOTAL:
				case column::COL_FREE:
					//return Text_WriteToStackBuffer_COL_TOTAL( subitem, psz_text, strSize, sizeBuffNeed, chars_written );
					return wds_fmt::FormatBytes( ( ( subitem == column::COL_TOTAL ) ? m_totalBytes : m_freeBytes ), psz_text, strSize, chars_written, sizeBuffNeed );
				case column::COL_NAME:
				case column::COL_ITEMS:
				case column::COL_BYTESPERCENT:
				case column::COL_FILES_TYPEVIEW:
				case column::COL_ATTRIBUTES:
				default:
					return WriteToStackBuffer_default( subitem, psz_text, strSize, sizeBuffNeed, chars_written, L"CDriveItem::" );
		}
		}

public:
									   const std::wstring      m_path; // e.g. "C:\"

	//18446744073709551615 is the maximum theoretical size of an NTFS file              according to http://blogs.msdn.com/b/oldnewthing/archive/2007/12/04/6648243.aspx
	_Field_range_( 0, 18446744073709551615 ) std::uint64_t     m_totalBytes; // Capacity
	_Field_range_( 0, 18446744073709551615 ) std::uint64_t     m_freeBytes;  // Free space
	_Field_range_( 0, 1 )                    DOUBLE            m_used;       // used space / total space
	};


// CDriveInformationThread. Does the GetVolumeInformation() call, which may hang for ca. 30 sec, it a network drive is not accessible.
class CDriveInformationThread final : public CWinThread {

public:
	CDriveInformationThread& operator=( const CDriveInformationThread& in ) = delete;
	CDriveInformationThread( const CDriveInformationThread& in ) = delete;
	virtual ~CDriveInformationThread( ) final = default;

	virtual BOOL InitInstance          ( ) override final;
	CDriveInformationThread( _In_  std::wstring path, LPARAM driveItem, HWND dialog, UINT serial, rsize_t thread_num, _In_ CRITICAL_SECTION* const cs_in, _In_ std::vector<CDriveInformationThread*>* const dlg_in );

public:
						const std::wstring              m_path;         // Path like "C:\"
						const LPARAM                    m_driveItem;    // The list item, we belong to
						std::atomic<HWND>               m_dialog;
						//C4820: 'CDriveInformationThread' : '4' bytes padding added after data member 'CDriveInformationThread::m_serial'
						const UINT                      m_serial;       // serial number of m_dialog
						const rsize_t                   m_threadNum;
	// "[out]"-parameters
						std::atomic<PWSTR>              m_name;         // Result: name like "BOOT (C:)", valid if m_success
						std::atomic<std::uint64_t>      m_totalBytes;   // Result: capacity of the drive, valid if m_success
						std::atomic<std::uint64_t>      m_freeBytes;    // Result: free space on the drive, valid if m_success
						//C4820: 'CDriveInformationThread' : '7' bytes padding added after data member 'CDriveInformationThread::m_success'
						std::atomic<bool>               m_success;      // Result: false, iff drive is unaccessible.
						CRITICAL_SECTION*               dialog_CRITICAL_SECTION_running_threads;
						_Guarded_by_( dialog_CRITICAL_SECTION_running_threads ) std::vector<CDriveInformationThread*>* dialog_running_threads;
	};

class CDrivesList final : public COwnerDrawnListCtrl {
	DECLARE_DYNAMIC(CDrivesList)
	size_t m_drives_count;
	_Field_size_( m_drives_count ) std::unique_ptr<CDriveItem[ ]> m_drives;
public:
	CDrivesList( ) : COwnerDrawnListCtrl( global_strings::drives_str, 20 ), m_drives_count( 0 ) { }
	CDrivesList& operator=( const CDrivesList& in ) = delete;
	CDrivesList( const CDrivesList& in ) = delete;

	_Must_inspect_result_ _Success_( return != NULL ) _Ret_maybenull_
	const CDriveItem* GetItem( _In_ _In_range_( 0, INT_MAX ) const int i ) const {
		ASSERT( i < GetItemCount( ) );
		const auto itemCount = GetItemCount( );
		if ( i < itemCount ) {
			return reinterpret_cast< CDriveItem* >( GetItemData( static_cast<int>( i ) ) );
			}
		return NULL;
		}

	void SysColorChanged( ) {
		InitializeColors( );
		}

	DECLARE_MESSAGE_MAP()
	afx_msg void OnLButtonDown( const UINT /*nFlags*/, const CPoint /*point*/ ) {
		if ( ( CWnd::GetFocus( ) == this ) || ( GetSelectedCount( ) == 0 ) ) { // We simulate Ctrl-Key-Down here, so that the dialog can be driven with one hand (mouse) only.
			const auto msg = CWnd::GetCurrentMessage( );
			CWnd::DefWindowProcW( msg->message, ( msg->wParam bitor MK_CONTROL ), msg->lParam );
			return;
			}
		CWnd::SetFocus( );
		// Send a LVN_ITEMCHANGED to the parent, so that it can update the radio button.
		auto lv = zero_init_struct<NMLISTVIEW>( );
		lv.hdr.hwndFrom = m_hWnd;
		lv.hdr.idFrom   = static_cast<UINT_PTR>( GetDlgCtrlID( ) );
		lv.hdr.code     = LVN_ITEMCHANGED;
		TRACE( _T( "Sending LVN_ITEMCHANGED ( via WM_NOTIFY ) to parent!\r\n" ) );
		CWnd::GetParent( )->SendMessageW( WM_NOTIFY, static_cast<WPARAM>( GetDlgCtrlID( ) ), reinterpret_cast<LPARAM>( &lv ) );
		}
	afx_msg void OnLvnDeleteitem( NMHDR* pNMHDR, LRESULT* pResult ) {
		auto pNMLV = reinterpret_cast< LPNMLISTVIEW >( pNMHDR );
		//const auto drive_list_item = GetItem( pNMLV->iItem );
		//TRACE( _T( "Deleting CDriveItem: %p\r\n" ), drive_list_item );
		pNMLV->iItem = -1;
		pNMLV->iSubItem = 0;
		//delete drive_list_item;
		*pResult = 0;
		}
	afx_msg void MeasureItem( PMEASUREITEMSTRUCT pMeasureItemStruct ) {
		pMeasureItemStruct->itemHeight = m_rowHeight;
		}
	afx_msg void OnNMDblclk( NMHDR* /*pNMHDR*/, LRESULT* pResult ) {
		*pResult = 0;

		auto point = GetCurrentMessage( )->pt;
		ASSERT( ::IsWindow( m_hWnd ) );
		//"Return value: If the function succeeds, the return value is nonzero. If the function fails, the return value is zero."
		VERIFY( ::ScreenToClient( m_hWnd, &point ) );
		//ScreenToClient( &point );

		const auto i = HitTest( point );
		if ( i == -1 ) {
			return;
			}
		const auto item_count = GetItemCount( );
		//Not vectorized: 1200, loop contains data dependencies
		for ( int k = 0; k < item_count; k++ ) {
			VERIFY( SetItemState( k, ( k == i ? LVIS_SELECTED : static_cast<UINT>( 0u ) ), LVIS_SELECTED ) );
			}
		TRACE( _T( "User double-clicked! Sending WMU_OK!\r\n" ) );
		GetParent( )->SendMessageW( WMU_OK );
		}
	};

// CSelectDrivesDlg. The initial dialog, where the user can select one or more drives or a folder for scanning.
class CSelectDrivesDlg final : public CDialog {
	DECLARE_DYNAMIC( CSelectDrivesDlg )
	enum {
		IDD = IDD_SELECTDRIVES
		};

public:
	CSelectDrivesDlg( CWnd* pParent = NULL );
	virtual ~CSelectDrivesDlg( ) final {
		DeleteCriticalSection_wrapper( m_running_threads_CRITICAL_SECTION );
		}

	CSelectDrivesDlg& operator=( const CSelectDrivesDlg& in ) = delete;
	CSelectDrivesDlg( const CSelectDrivesDlg& in ) = delete;

private:
	_Pre_satisfies_( m_radio == RADIO_AFOLDER )
	void handle_RADIO_AFOLDER( );

	_Pre_satisfies_( m_radio != RADIO_AFOLDER )
	void handle_RADIO_other( );
protected:
	_Pre_defensive_ virtual void DoDataExchange ( CDataExchange* pDX ) override final;
	                virtual BOOL OnInitDialog   (                    ) override final;
	_Pre_defensive_ virtual void OnOK           (                    ) override final;


	void buildSelectList( );
	void initWindow( );
	void addControls( ) {
		m_layout.AddControl( IDOK,                  1, 0, 0, 0 );
		m_layout.AddControl( IDCANCEL,              1, 0, 0, 0 );
		m_layout.AddControl( IDC_DRIVES,            0, 0, 1, 1 );
		m_layout.AddControl( IDC_AFOLDER,           0, 1, 0, 0 );
		m_layout.AddControl( IDC_FOLDERNAME,        0, 1, 1, 0 );
		m_layout.AddControl( IDC_BROWSEFOLDER,      1, 1, 0, 0 );
		}

	void insertColumns( ) {
		m_list.InsertColumn( column::COL_NAME,  global_strings::name,  LVCFMT_LEFT , 120, column::COL_NAME  );
		m_list.InsertColumn( column::COL_TOTAL, global_strings::total, LVCFMT_RIGHT,  55, column::COL_TOTAL );
		m_list.InsertColumn( column::COL_FREE,  global_strings::free,  LVCFMT_RIGHT,  55, column::COL_FREE  );
		}

	void setListOptions( );

	_Pre_defensive_ void UpdateButtons          (                    );

public:
	       // Dialog Data
	       //C4820: 'CSelectDrivesDlg' : '4' bytes padding added after data member 'CSelectDrivesDlg::m_radio'
	       int                       m_radio;       // out.
		   std::wstring              m_folder_name_heap;// out. Valid if m_radio = RADIO_AFOLDER
	       std::vector<std::wstring> m_drives;	    // out. Valid if m_radio != RADIO_AFOLDER
		   CRITICAL_SECTION          m_running_threads_CRITICAL_SECTION;
		   _Guarded_by_( m_running_threads_CRITICAL_SECTION ) std::vector<CDriveInformationThread*> m_running_threads;
protected:
	static UINT                      _serial;       // Each Instance of this dialog gets a serial number
	       CDrivesList               m_list;
	       CButton                   m_okButton;
	       std::vector<std::wstring> m_selectedDrives;
	       CLayout                   m_layout;

	DECLARE_MESSAGE_MAP()
	afx_msg void OnBnClickedBrowsefolder();
	afx_msg void OnLbnSelchangeDrives();
	afx_msg void OnMeasureItem( const INT nIDCtl, PMEASUREITEMSTRUCT pMeasureItemStruct ) {
		if ( nIDCtl == IDC_DRIVES ) {
			pMeasureItemStruct->itemHeight = 20;
			return;
			}
		CDialog::OnMeasureItem( nIDCtl, pMeasureItemStruct );
		}
	afx_msg void OnLvnItemchangedDrives( NMHDR* pNMHDR, LRESULT* pResult ) {
		UNREFERENCED_PARAMETER( pNMHDR );
		m_radio = RADIO_SOMEDRIVES;
		VERIFY( UpdateData( false ) );
		UpdateButtons( );
		*pResult = 0;
		}
	afx_msg void OnGetMinMaxInfo( _Out_ MINMAXINFO* lpMMI ) {
		m_layout.OnGetMinMaxInfo( lpMMI );
		CDialog::OnGetMinMaxInfo( lpMMI );
		}
	afx_msg void OnDestroy( );

	afx_msg LRESULT OnWmuOk( const WPARAM, const LPARAM ) {
		OnOK( );
		return 0;
		}

	afx_msg _Function_class_( "GUI_THREAD" ) LRESULT OnWmuThreadFinished( const WPARAM, const LPARAM lparam );
	
	afx_msg void OnSysColorChange( ) {
		CDialog::OnSysColorChange( );
		m_list.SysColorChanged( );
		}

	afx_msg void OnSize( UINT nType, INT cx, INT cy ) {
		CDialog::OnSize( nType, cx, cy );
		m_layout.OnSize( );
		}

	};


#else


#endif
