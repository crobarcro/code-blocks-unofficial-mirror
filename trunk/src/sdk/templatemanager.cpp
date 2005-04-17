/*
* This file is part of Code::Blocks Studio, an open-source cross-platform IDE
* Copyright (C) 2003  Yiannis An. Mandravellos
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*
* Contact e-mail: Yiannis An. Mandravellos <mandrav@codeblocks.org>
* Program URL   : http://www.codeblocks.org
*
* $Id$
* $Date$
*/

#include <wx/mdi.h>
#include <wx/intl.h>
#include <wx/dir.h>
#include <wx/menu.h>
#include <wx/log.h>
#include <wx/filedlg.h>
#include <wx/filename.h>
#include <wx/msgdlg.h>
#include <wx/textdlg.h>

#include "templatemanager.h"
#include "manager.h"
#include "configmanager.h"
#include "messagemanager.h"
#include "projectmanager.h"
#include "cbproject.h"
#include "globals.h"
#include "managerproxy.h"
#include "compilerfactory.h"

int idMenuNewFromTemplate = wxNewId();

TemplateManager* TemplateManager::Get()
{
	if (!TemplateManagerProxy::Get())
	 {
		TemplateManagerProxy::Set( new TemplateManager );
		Manager::Get()->GetMessageManager()->Log(_("TemplateManager initialized"));
	 }
	 return TemplateManagerProxy::Get();
}

void TemplateManager::Free()
{
	if (TemplateManagerProxy::Get())
	{
		delete TemplateManagerProxy::Get();
		TemplateManagerProxy::Set( 0L );
	}
}

BEGIN_EVENT_TABLE(TemplateManager, wxEvtHandler)
	EVT_MENU(idMenuNewFromTemplate, TemplateManager::OnNew)
END_EVENT_TABLE()

TemplateManager::TemplateManager()
{
	//ctor
	Manager::Get()->GetAppWindow()->PushEventHandler(this);
}

TemplateManager::~TemplateManager()
{
	//dtor
    // this is a core manager, so it is removed when the app is shutting down.
    // in this case, the app has already un-hooked us, so no need to do it ourselves...
//	Manager::Get()->GetAppWindow()->RemoveEventHandler(this);

	WX_CLEAR_ARRAY(m_Templates);
}

void TemplateManager::CreateMenu(wxMenuBar* menuBar)
{
}

void TemplateManager::ReleaseMenu(wxMenuBar* menuBar)
{
}

void TemplateManager::BuildToolsMenu(wxMenu* menu)
{
	if (menu)
		menu->Append(idMenuNewFromTemplate, _("&From template..."));
}

void TemplateManager::LoadTemplates()
{
    wxLogNull zero; // disable error logging

    wxString baseDir = ConfigManager::Get()->Read("/data_path");
	baseDir << "/templates";

    wxDir dir(baseDir);

    if (!dir.IsOpened())
        return;

	WX_CLEAR_ARRAY(m_Templates);
    wxString filename;
    bool ok = dir.GetFirst(&filename, "*.template", wxDIR_FILES);
    while (ok)
    {
        ProjectTemplateLoader* pt = new ProjectTemplateLoader();
		if (pt->Open(baseDir + "/" + filename))
			m_Templates.Add(pt);
		else
			delete pt;

        ok = dir.GetNext(&filename);
    }
	Manager::Get()->GetMessageManager()->DebugLog(_("%d templates loaded"), m_Templates.GetCount());
}

void TemplateManager::LoadUserTemplates()
{
    wxLogNull zero; // disable error logging

    m_UserTemplates.Clear();
    wxString baseDir = wxGetHomeDir();
	baseDir << "/.CodeBlocks";

    wxDir dir(baseDir);

    if (!dir.IsOpened())
        return;

    wxString filename;
    bool ok = dir.GetFirst(&filename, "*", wxDIR_DIRS);
    while (ok)
    {
        m_UserTemplates.Add(filename);
        ok = dir.GetNext(&filename);
    }
	
	Manager::Get()->GetMessageManager()->DebugLog(_("%d user templates loaded"), m_UserTemplates.GetCount());
}

void TemplateManager::NewProject()
{
	LoadTemplates();
	LoadUserTemplates();
	NewFromTemplateDlg dlg(m_Templates, m_UserTemplates);
	if (dlg.ShowModal() == wxID_OK)
	{
        if (dlg.SelectedUserTemplate())
            NewProjectFromUserTemplate(dlg);
        else
            NewProjectFromTemplate(dlg);
	}
}

void TemplateManager::NewProjectFromTemplate(NewFromTemplateDlg& dlg)
{
    ProjectTemplateLoader* pt = dlg.GetTemplate();
    if (!pt)
    {
        Manager::Get()->GetMessageManager()->DebugLog(_("Templates dialog returned OK but no template was selected ?!?"));
        return;
    }
    int optidx = dlg.GetOptionIndex();
    int filesetidx = dlg.GetFileSetIndex();
    TemplateOption& option = pt->m_TemplateOptions[optidx];
    FileSet& fileset = pt->m_FileSets[filesetidx];

    wxString baseDir = ConfigManager::Get()->Read("/data_path");
    wxFileDialog fdlg(0L,
                        _("Save project"),
                        wxEmptyString,
                        pt->m_Name,
                        CODEBLOCKS_FILES_FILTER,
                        wxSAVE | wxHIDE_READONLY | wxOVERWRITE_PROMPT);

    if (fdlg.ShowModal() != wxID_OK)
        return;

    wxFileName fname;
    fname.Assign(fdlg.GetPath());
    wxString path = fname.GetPath(wxPATH_GET_VOLUME);
    wxString filename = fname.GetFullPath();
    wxString sep = wxFileName::GetPathSeparator();

    baseDir << sep << "templates";
    wxCopyFile(baseDir + sep + option.file, filename);

    cbProject* prj = Manager::Get()->GetProjectManager()->LoadProject(filename);
    if (prj)
    {
        prj->SetCompilerIndex(CompilerFactory::GetDefaultCompilerIndex());
        for (int i = 0; i < prj->GetBuildTargetsCount(); ++i)
        {
            ProjectBuildTarget* target = prj->GetBuildTarget(i);
            target->SetCompilerIndex(CompilerFactory::GetDefaultCompilerIndex());
        }
        
        for (unsigned int i = 0; i < fileset.files.GetCount(); ++i)
        {
            FileSetFile& fsf = fileset.files[i];
            wxString dst = path + sep + fsf.destination;
            bool skipped = false;
            while (wxFileExists(dst))
            {
                wxString msg;
                msg.Printf(_("File %s already exists.\nDo you really want to overwrite this file?"), dst.c_str());
                if (wxMessageBox(msg, _("Overwrite existing file?"), wxYES_NO | wxICON_WARNING) == wxYES)
                    break;
                wxFileDialog fdlg(0L,
                                    _("Save file as..."),
                                    wxEmptyString,
                                    dst,
                                    SOURCE_FILES_FILTER,
                                    wxSAVE | wxHIDE_READONLY);
                if (fdlg.ShowModal() == wxID_CANCEL)
                {
                    msg.Printf(_("File %s is skipped..."), dst.c_str());
                    wxMessageBox(msg, _("File skipped"), wxICON_ERROR);
                    skipped = true;
                    break;
                }
                dst = fdlg.GetPath();
            }
            if (skipped)
                continue;
            wxCopyFile(baseDir + sep + fsf.source, dst);
            prj->AddFile(0, dst);
        }
    
        for (unsigned int i = 0; i < option.extraCFlags.GetCount(); ++i)
            prj->AddCompilerOption(option.extraCFlags[i]);
        for (unsigned int i = 0; i < option.extraLDFlags.GetCount(); ++i)
            prj->AddLinkerOption(option.extraLDFlags[i]);
        
        prj->CalculateCommonTopLevelPath(); // must-do because we manually added files
        Manager::Get()->GetProjectManager()->RebuildTree();
    }
}

void TemplateManager::NewProjectFromUserTemplate(NewFromTemplateDlg& dlg)
{
    if (!dlg.SelectedUserTemplate())
    {
        Manager::Get()->GetMessageManager()->DebugLog(_("TemplateManager::NewProjectFromUserTemplate() called when no user template was selected ?!?"));
        return;
    }

    // select directory to copy user template files
    wxString sep = wxFileName::GetPathSeparator();
    wxString path = ChooseDirectory(0, _("Choose a directory to create the new project"));
    if (path.IsEmpty())
        return;

    wxBusyCursor busy;

    wxString templ = wxGetHomeDir();
    templ << sep << ".CodeBlocks" << sep << dlg.GetSelectedUserTemplate();
    if (!wxDirExists(templ))
    {
        Manager::Get()->GetMessageManager()->DebugLog(_("Cannot open user-template source path '%s'!"), templ.c_str());
        return;
    }

    // copy files
    wxString project_filename;
    wxArrayString files;
    wxDir::GetAllFiles(templ, &files);
    int count = 0;
    int total_count = files.GetCount();
    for (size_t i = 0; i < files.GetCount(); ++i)
    {
        wxFileName dstname(files[i]);
        dstname.MakeRelativeTo(templ + sep);
        wxString src = files[i];
        wxString dst = path + sep + dstname.GetFullPath();
//        Manager::Get()->GetMessageManager()->DebugLog("dst=%s, dstname=%s", dst.c_str(), dstname.GetFullPath().c_str());
        if (!CreateDirRecursively(dst))
            Manager::Get()->GetMessageManager()->DebugLog("Failed creating directory for %s", dst.c_str());
        if (wxCopyFile(src, dst, true))
        {
            if (FileTypeOf(dst) == ftCodeBlocksProject)
                project_filename = dst;
            ++count;
        }
        else
            Manager::Get()->GetMessageManager()->DebugLog("Failed copying %s to %s", src.c_str(), dst.c_str());
    }
    if (count != total_count)
        wxMessageBox(_("Some files could not be loaded with the template..."), _("Error"), wxICON_ERROR);
    else
    {
        // open new project
        if (project_filename.IsEmpty())
            wxMessageBox(_("User-template saved succesfuly but no project file exists in it!"));
        else
            Manager::Get()->GetProjectManager()->LoadProject(project_filename);
    }
}

void TemplateManager::SaveUserTemplate(cbProject* prj)
{
    if (!prj)
        return;

    // save project & all files
    if (!prj->SaveAllFiles() ||
        !prj->Save())
    {
        wxMessageBox(_("Could not save project and/or all its files. Aborting..."), _("Error"), wxICON_ERROR);
        return;
    }

    // create destination dir
    wxString templ = wxGetHomeDir();
    templ << "/.CodeBlocks";
    if (!wxDirExists(templ))
        wxMkdir(templ, 0755);

    // check if it exists and ask a different title
    wxString title = prj->GetTitle();
    while (true)
    {
        // ask for template title (unique)
        wxTextEntryDialog dlg(0, _("Enter a title for this template"), _("Enter title"), title);
        if (dlg.ShowModal() != wxID_OK)
            return;

        title = dlg.GetValue();
        if (!wxDirExists(templ + "/" + title))
        {
            templ << "/" << title;
            wxMkdir(templ, 0755);
            break;
        }
        else
            wxMessageBox(_("You have another template with the same title.\nPlease choose another title..."));
    }

    wxBusyCursor busy;

    // copy project and all files to destination dir
    int count = 0;
    int total_count = prj->GetFilesCount();
    templ << "/";
    wxFileName fname;
    for (int i = 0; i < total_count; ++i)
    {
        wxString src = prj->GetFile(i)->file.GetFullPath();
        wxString dst = templ + prj->GetFile(i)->relativeFilename;
//        Manager::Get()->GetMessageManager()->DebugLog("Copying %s to %s", src.c_str(), dst.c_str());
        if (!CreateDirRecursively(dst))
            Manager::Get()->GetMessageManager()->DebugLog("Failed creating directory for %s", dst.c_str());
        if (wxCopyFile(src, dst, true))
            ++count;
        else
            Manager::Get()->GetMessageManager()->DebugLog("Failed copying %s to %s", src.c_str(), dst.c_str());
    }
    fname.Assign(prj->GetFilename());
    if (!wxCopyFile(prj->GetFilename(), templ + fname.GetFullName()))
        wxMessageBox(_("Failed to copy the project file!"), _("Error"), wxICON_ERROR);

    if (count == total_count)
        wxMessageBox(_("User-template saved succesfuly"));
    else
        wxMessageBox(_("Some files could not be saved with the template..."), _("Error"), wxICON_ERROR);
}

// events

void TemplateManager::OnNew(wxCommandEvent& event)
{
	NewProject();
}
