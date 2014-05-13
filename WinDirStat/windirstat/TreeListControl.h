// TreeListControl.h - Declaration of CTreeListItem and CTreeListControl
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

#pragma once

#include "ownerdrawnlistcontrol.h"
#include "pacman.h"

class CTreeListItem;
class CTreeListControl;


//
// CTreeListItem. An item in the CTreeListControl. (CItem is derived from CTreeListItem.)
// In order to save memory, once the item is actually inserted in the List, we allocate the VISIBLEINFO structure (m_vi).
// m_vi is freed as soon as the item is removed from the List.
class CTreeListItem: public COwnerDrawnListItem
{
	// Data needed to display the item.
	struct VISIBLEINFO
	{
		int    indent;			// 0 for the root item, 1 for its children, and so on.
		int    image;			// -1 as long as not needed, >= 0: valid index in MyImageList.
		CRect  rcPlusMinus;	    // Coordinates of the little +/- rectangle, relative to the upper left corner of the item.
		CRect  rcTitle;		    // Coordinates of the label, relative to the upper left corner of the item.
		bool   isExpanded;	    // Whether item is expanded.

		// sortedChildren: This member contains our children (the same set of children as in CItem::m_children) and is initialized as soon as we are expanded.
		// In contrast to CItem::m_children, this array is always sorted depending on the current user-defined sort column and -order.
		CArray<CTreeListItem *, CTreeListItem *> sortedChildren;

		CPacman pacman;
	};

	public:
		CTreeListItem( );
		virtual ~CTreeListItem( );

		virtual int            Compare          ( const CSortingListItem *other, const int subitem                                                        ) const;
		virtual int            CompareSibling   ( const CTreeListItem *tlib,     int subitem                                                              ) const = 0;
		virtual bool           DrawSubitem      ( const int subitem,             CDC *pdc,         CRect rc, const UINT state, int *width, int *focusLeft ) const;
		virtual CString        GetText          ( const int subitem                                                                                       ) const;
		virtual int            GetImage         (                                                                                                         ) const;
		virtual CTreeListItem *GetTreeListChild ( int i                                                                                                   ) const = 0;
		virtual int            GetChildrenCount (                                                                                                         ) const = 0;
		virtual int            GetImageToCache  (                                                                                                         ) const = 0;

		int  GetIndent                          (                                                                                     ) const;
		int  FindSortedChild                    ( const CTreeListItem *child                                                          );
	
		void DrawPacman                         ( CDC *pdc,                  const CRect& rc, const COLORREF bgColor                  ) const;
		void SetExpanded                        ( const bool expanded = true                                                          );
		void SetParent                          ( CTreeListItem *parent                                                               );
		void SetPlusMinusRect                   ( const CRect& rc                                                                     ) const;
		void SetTitleRect                       ( const CRect& rc                                                                     ) const;
		void SetVisible                         ( const bool visible = true                                                           );
		void SortChildren                       (                                                                                     );
		void UncacheImage                       (                                                                                     );
	
		CTreeListItem *GetSortedChild           ( const int i                                                                         );
		CTreeListItem *GetParent                (                                                                                     ) const;
	
		bool  HasSiblings                       (                                                                                     ) const;
		bool  HasChildren                       (                                                                                     ) const;
		bool  IsExpanded                        (                                                                                     ) const;
		bool  IsVisible                         (                                                                                     ) const;
	
		CRect GetPlusMinusRect                  (                                                                                     ) const;
		CRect GetTitleRect                      (                                                                                     ) const;
	
	
	protected:
		static int __cdecl _compareProc             ( const void *p1, const void *p2 );
		static CTreeListControl *GetTreeListControl (                                );

		bool DrivePacman                            ( const LONGLONG readJobs        );

		void SetScrollPosition                      ( const int top                  );
		void StartPacman                            ( const bool start               );
		
		int  GetScrollPosition                      (                                );

	private:
		CTreeListItem       *m_parent;
		mutable VISIBLEINFO *m_vi;
	};


//
// CTreeListControl. A CListCtrl, which additionally behaves an looks like a tree control.
//
class CTreeListControl : public COwnerDrawnListControl {
	DECLARE_DYNAMIC( CTreeListControl )

	// In order to save memory, and as we have only one CTreeListControl in the application,
	// this is global.
	static CTreeListControl *_theTreeListControl;

	public:
		static CTreeListControl *GetTheTreeListControl ( );

		CTreeListControl( int rowHeight = -1 );
		
		virtual ~CTreeListControl( );
		virtual BOOL CreateEx                          ( const DWORD dwExStyle, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, const UINT nID );
		virtual void SysColorChanged                   ( );
		virtual bool HasImages( ) const;


		void MySetImageList                            ( CImageList *il                     );
		void SetItemScrollPosition                     ( CTreeListItem *item, const int top );
		void SetRootItem                               ( CTreeListItem *root                );
		void OnChildAdded                              ( CTreeListItem *parent, CTreeListItem *child     );
		void OnChildRemoved                            ( CTreeListItem *parent, CTreeListItem *childdata );
		void OnRemovingAllChildren                     ( CTreeListItem *parent                           );
		
		CTreeListItem *GetItem                         ( const int i         );

		int  GetItemScrollPosition                     ( CTreeListItem *item );
		
		void DeselectAll                               (                                                                                                              );
		void SelectAndShowItem                         ( const CTreeListItem *item, const bool showWholePath                                                          );
		void DrawNode                                  ( CDC *pdc,                  CRect& rc,              CRect& rcPlusMinus, const CTreeListItem *item, int *width );
		void SelectItem                                ( const CTreeListItem *item                                                                                    );
		void Sort                                      (                                                                                                              );
		void EnsureItemVisible                         ( const CTreeListItem *item                                                                                    );
		void ExpandItem                                ( CTreeListItem *item                                                                                          );
		int  FindTreeItem                              ( const CTreeListItem *item                                                                                    );
		bool SelectedItemCanToggle                     (                                                                                                              );
		void ToggleSelectedItem                        (                                                                                                              );


	protected:
		virtual void OnItemDoubleClick                 ( const int i );
		void         InitializeNodeBitmaps             (             );


		void InsertItem                                ( const int i, CTreeListItem *item      );
		void DeleteItem                                ( const int i                           );
		void CollapseItem                              ( const int i                           );
		void ExpandItem                                ( const int i, const bool scroll = true );
		void ToggleExpansion                           ( const int i                           );
		void SelectItem                                ( const int i                           );
		int GetSelectedItem                            (                                       ) const;

		CBitmap     m_bmNodes0;			            // The bitmaps needed to draw the treecontrol-like branches
		CBitmap     m_bmNodes1;			            // The same bitmaps with stripe-background color
		CImageList *m_imageList;	                // We don't use the system-supplied SetImageList(), but MySetImageList().
		int         m_lButtonDownItem;		        // Set in OnLButtonDown(). -1 if not item hit.
		bool        m_lButtonDownOnPlusMinusRect;	// Set in OnLButtonDown(). True, if plus-minus-rect hit.

		DECLARE_MESSAGE_MAP()

		afx_msg void MeasureItem(LPMEASUREITEMSTRUCT mis);
		afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
		afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
		afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
		afx_msg BOOL OnEraseBkgnd(CDC* pDC);
};



// $Log$
// Revision 1.8  2004/11/13 08:17:07  bseifert
// Remove blanks in Unicode Configuration names.
//
// Revision 1.7  2004/11/12 22:14:16  bseifert
// Eliminated CLR_NONE. Minor corrections.
//
// Revision 1.6  2004/11/12 00:47:42  assarbad
// - Fixed the code for coloring of compressed/encrypted items. Now the coloring spans the full row!
//
// Revision 1.5  2004/11/08 00:46:26  assarbad
// - Added feature to distinguish compressed and encrypted files/folders by color as in the Windows 2000/XP explorer.
//   Same rules apply. (Green = encrypted / Blue = compressed)
//
// Revision 1.4  2004/11/05 16:53:07  assarbad
// Added Date and History tag where appropriate.
//