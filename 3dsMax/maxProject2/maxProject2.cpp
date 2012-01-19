//**************************************************************************/
// Copyright (c) 1998-2007 Autodesk, Inc.
// All rights reserved.
// 
// These coded instructions, statements, and computer programs contain
// unpublished proprietary information written by Autodesk, Inc., and are
// protected by Federal copyright law. They may not be disclosed to third
// parties or copied or duplicated in any form, in whole or in part, without
// the prior written consent of Autodesk, Inc.
//**************************************************************************/
// DESCRIPTION: Appwizard generated plugin
// AUTHOR: 
//***************************************************************************/

#include "maxProject2.h"

#define maxProject2_CLASS_ID	Class_ID(0x9d3af788, 0x7654aef5)


class maxProject2 : public UtilityObj 
{
public:
		
	//Constructor/Destructor
	maxProject2();
	virtual ~maxProject2();

	virtual void DeleteThis() { }		
	
	virtual void BeginEditParams(Interface *ip,IUtil *iu);
	virtual void EndEditParams(Interface *ip,IUtil *iu);

	virtual void Init(HWND hWnd);
	virtual void Destroy(HWND hWnd);
	
	// Singleton access
	static maxProject2* GetInstance() { 
		static maxProject2 themaxProject2;
		return &themaxProject2; 
	}

private:

	static INT_PTR CALLBACK DlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	HWND			hPanel;
	IUtil			*iu;
	Interface		*ip;
};


class maxProject2ClassDesc : public ClassDesc2 
{
public:
	virtual int IsPublic() 							{ return TRUE; }
	virtual void* Create(BOOL /*loading = FALSE*/) 	{ return maxProject2::GetInstance(); }
	virtual const TCHAR *	ClassName() 			{ return GetString(IDS_CLASS_NAME); }
	virtual SClass_ID SuperClassID() 				{ return UTILITY_CLASS_ID; }
	virtual Class_ID ClassID() 						{ return maxProject2_CLASS_ID; }
	virtual const TCHAR* Category() 				{ return GetString(IDS_CATEGORY); }

	virtual const TCHAR* InternalName() 			{ return _T("maxProject2"); }	// returns fixed parsable name (scripter-visible name)
	virtual HINSTANCE HInstance() 					{ return hInstance; }					// returns owning module handle
	

};


ClassDesc2* GetmaxProject2Desc() { 
	static maxProject2ClassDesc maxProject2Desc;
	return &maxProject2Desc; 
}




//--- maxProject2 -------------------------------------------------------
maxProject2::maxProject2()
{
	iu = NULL;
	ip = NULL;	
	hPanel = NULL;
}

maxProject2::~maxProject2()
{

}

void maxProject2::BeginEditParams(Interface* ip,IUtil* iu) 
{
	this->iu = iu;
	this->ip = ip;
	hPanel = ip->AddRollupPage(
		hInstance,
		MAKEINTRESOURCE(IDD_PANEL),
		DlgProc,
		GetString(IDS_PARAMS),
		0);
}
	
void maxProject2::EndEditParams(Interface* ip,IUtil* iu) 
{
	this->iu = NULL;
	this->ip = NULL;
	ip->DeleteRollupPage(hPanel);
	hPanel = NULL;
}

void maxProject2::Init(HWND hWnd)
{

}

void maxProject2::Destroy(HWND hWnd)
{

}

INT_PTR CALLBACK maxProject2::DlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) 
	{
		case WM_INITDIALOG:
			maxProject2::GetInstance()->Init(hWnd);
			break;

		case WM_DESTROY:
			maxProject2::GetInstance()->Destroy(hWnd);
			break;

		case WM_COMMAND:
			#pragma message(TODO("React to the user interface commands.  A utility plug-in is controlled by the user from here."))
			break;

		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_MOUSEMOVE:
			maxProject2::GetInstance()->ip->RollupMouseMessage(hWnd,msg,wParam,lParam); 
			break;

		default:
			return 0;
	}
	return 1;
}
