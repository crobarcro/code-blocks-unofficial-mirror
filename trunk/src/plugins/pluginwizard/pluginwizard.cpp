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

#include "pluginwizard.h"
#include <wx/intl.h>
#include <wx/filename.h>
#include <wx/xrc/xmlres.h>
#include <wx/fs_zip.h>
#include <wx/mdi.h>
#include <wx/msgdlg.h>
#include <manager.h>
#include <configmanager.h>
#include <projectmanager.h>
#include <cbproject.h>
#include "pluginwizarddlg.h"

cbPlugin* GetPlugin()
{
    return new PluginWizard;
}


PluginWizard::PluginWizard()
{
    wxFileSystem::AddHandler(new wxZipFSHandler);
    wxXmlResource::Get()->InitAllHandlers();
    wxString resPath = ConfigManager::Get()->Read(_T("data_path"), wxEmptyString);
    wxXmlResource::Get()->Load(resPath + _T("/plugin_wizard.zip#zip:*.xrc"));

    m_PluginInfo.name = _T("PluginWizard");
    m_PluginInfo.title = _("Code::Blocks Plugin wizard");
    m_PluginInfo.version = _T("0.1");
    m_PluginInfo.description = _("This is the Code::Blocks plugin wizard...");
    m_PluginInfo.author = _T("Yiannis An. Mandravellos");
    m_PluginInfo.authorEmail = _T("info@codeblocks.org");
    m_PluginInfo.authorWebsite = _T("www.codeblocks.org");
    m_PluginInfo.thanksTo = _T("");
	m_PluginInfo.hasConfigure = false;
}

PluginWizard::~PluginWizard()
{
}

void PluginWizard::OnAttach()
{
}

void PluginWizard::OnRelease(bool appShutDown)
{
}

int PluginWizard::Execute()
{
    cbProject* project = Manager::Get()->GetProjectManager()->NewProject();
    if (!project)
        return -1;
    project->AddCompilerOption(_T("-D__GNUWIN32__"));
    project->AddCompilerOption(_T("-DWXUSINGDLL"));
    project->AddCompilerOption(_T("-DBUILDING_PLUGIN"));
    project->AddLinkLib(_T("codeblocks"));

// NOTE (rickg22#1#): How not to hardwire wxmsw242 into the plugin?
    project->AddLinkLib(_T("wxmsw242"));

    wxSetWorkingDirectory(project->GetBasePath());

	PluginWizardDlg dlg;
	if (dlg.ShowModal() == wxID_OK)
	{
        wxString name = !dlg.GetInfo().name.IsEmpty() ? dlg.GetInfo().name : _T("CustomPlugin");
        wxString title = !dlg.GetInfo().title.IsEmpty() ? dlg.GetInfo().title : _T("Custom Plugin");
        project->SetTitle(title);
        project->AddFile(0, dlg.GetHeaderFilename());
        project->AddFile(0, dlg.GetImplementationFilename());

        ProjectBuildTarget* target = project->GetBuildTarget(0);
        target->SetTargetType(ttDynamicLib);
        target->SetCreateDefFile(false);
        target->SetCreateStaticLib(false);
        target->SetOutputFilename(name + _T(".") + DYNAMICLIB_EXT);

        Manager::Get()->GetProjectManager()->RebuildTree();

		wxMessageDialog msg(Manager::Get()->GetAppWindow(),
						_("The new plugin project has been created.\n"
						"Don't forget to add the SDK include and library dirs\n"
						"in the respective project build options..."),
						_("Information"),
						wxOK | wxICON_INFORMATION);
		msg.ShowModal();
		return 0;
	}
		
	return -1;
}
