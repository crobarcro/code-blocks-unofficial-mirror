/***************************************************************
 * Name:      CodeSnippetsApp.h
 * Purpose:   Defines Application Class
 * Author:    pecan ()
 * Created:   2007-03-18
 * Copyright: pecan ()
 * License:
 **************************************************************/
/*
	This file is part of Code Snippets, a plugin for Code::Blocks
	Copyright (C) 2007 Pecan Heber

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version 2
	of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
// RCS-ID: $Id: codesnippetsapp.h 63 2007-04-25 16:37:27Z Pecan $

#ifndef CODESNIPPETSAPP_H
#define CODESNIPPETSAPP_H

#include <wx/wxprec.h>

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

// ----------------------------------------------------------------------------
class CodeSnippetsApp : public wxApp
// ----------------------------------------------------------------------------
{
	public:
		virtual bool OnInit();
		//-void OnActivateApp(wxActivateEvent& event);

		DECLARE_EVENT_TABLE()
};

#endif // CODESNIPPETSAPP_H
/***************************************************************
 * Name:      CodeSnippetsAppMain.h
 * Purpose:   Defines Application Frame
 * Author:    pecan ()
 * Created:   2007-03-18
 * Copyright: pecan ()
 * License:
 **************************************************************/

#ifndef CODESNIPPETSAPPMAIN_H
#define CODESNIPPETSAPPMAIN_H

#include <wx/snglinst.h>    //single instance checker

#include "codesnippetsapp.h"
#include "snippetsconfig.h"
#include "codesnippetswindow.h"

// ----------------------------------------------------------------------------
class CodeSnippetsAppFrame: public wxFrame
// ----------------------------------------------------------------------------
{
	public:
		CodeSnippetsAppFrame(wxFrame *frame, const wxString& title);
		~CodeSnippetsAppFrame();

		bool GetFileChanged(){ return GetConfig()->pSnippetsWindow->GetFileChanged(); }
        wxString FindAppPath(const wxString& argv0, const wxString& cwd, const wxString& appVariableName);


	private:

		CodeSnippetsWindow* GetSnippetsWindow(){return GetConfig()->GetSnippetsWindow();}

        void OnFileLoad(wxCommandEvent& event);
        void OnFileSave(wxCommandEvent& event);
        void OnFileSaveAs(wxCommandEvent& event);
		void OnClose(wxCloseEvent& event);
		void OnQuit(wxCommandEvent& event);
        void OnSettings(wxCommandEvent& event);
		void OnAbout(wxCommandEvent& event);
		void OnActivate(wxActivateEvent& event);
		void OnFileBackup(wxCommandEvent& event);

		wxString            buildInfo;
        wxSingleInstanceChecker*  m_checker ;
        int                  m_bOnActivateBusy;


		DECLARE_EVENT_TABLE()
};


#endif // CODESNIPPETSAPPMAIN_H
