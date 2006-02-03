/*
* This file is part of Code::Blocks Studio, an open-source cross-platform IDE
* Copyright (C) 2003  Yiannis An. Mandravellos
*
* This program is distributed under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or (at your option) any later version.
*
* $Revision$
* $Id$
* $HeadURL$
*/

#include "sdk_precomp.h"

#ifndef CB_PRECOMP
    #include "compilerfactory.h"
    #include "manager.h"
    #include "messagemanager.h"
    #include "configmanager.h"
#endif

#include "autodetectcompilers.h"

// statics
CompilersArray CompilerFactory::Compilers;
int CompilerFactory::s_DefaultCompilerIdx = 0;

void CompilerFactory::RegisterCompiler(Compiler* compiler)
{
    CompilerFactory::Compilers.Add(compiler);
}

void CompilerFactory::RegisterUserCompilers()
{
    wxArrayString paths = Manager::Get()->GetConfigManager(_T("compiler"))->EnumerateSubPaths(_T("/sets"));
    for (unsigned int i = 0; i < paths.GetCount(); ++i)
    {
		int parent = Manager::Get()->GetConfigManager(_T("compiler"))->ReadInt(_T("/sets/") + paths[i] + _T("/parent"), -1);
        if (CompilerIndexOK(parent - 1))
            CreateCompilerCopy(Compilers[parent - 1]);
	}
}

int CompilerFactory::CreateCompilerCopy(Compiler* compiler)
{
    Compiler* newC = compiler->CreateCopy();
    RegisterCompiler(newC);
    newC->LoadSettings(_T("/sets"));
    Manager::Get()->GetMessageManager()->DebugLog(_("Added compiler \"%s\""), newC->GetName().c_str());
    return Compilers.GetCount() - 1; // return the index for the new compiler
}

void CompilerFactory::RemoveCompiler(Compiler* compiler)
{
    if (!compiler)
        return;
    int listIdx = compiler->m_ID;

    // loop through compilers list and adjust all following compilers m_ID -= 1 and m_ParentID -= 1
    for (unsigned int i = listIdx; i < Compilers.GetCount(); ++i)
    {
        Compiler* tmp = Compilers[i];
        if (tmp->m_ParentID == compiler->m_ID)
        {
            // this compiler has parent the compiler to be deleted
            tmp->m_ParentID = compiler->m_ParentID;
        }
        else if (tmp->m_ParentID >= listIdx)
        {
            tmp->m_ParentID -= 1;
        }
        tmp->m_ID -= 1;
    }
    Compilers.Remove(compiler);
    Manager::Get()->GetMessageManager()->DebugLog(_("Compiler \"%s\" removed"), compiler->GetName().c_str());
    delete compiler;

    SaveSettings();
}

void CompilerFactory::UnregisterCompilers()
{
    WX_CLEAR_ARRAY(CompilerFactory::Compilers);
    CompilerFactory::Compilers.Empty();
}

bool CompilerFactory::CompilerIndexOK(int compilerIdx)
{
    return CompilerFactory::Compilers.GetCount() && compilerIdx >= 0 && compilerIdx < (int)CompilerFactory::Compilers.GetCount();
}

int CompilerFactory::GetDefaultCompilerIndex()
{
    return CompilerIndexOK(s_DefaultCompilerIdx) ? s_DefaultCompilerIdx : 0;
}

void CompilerFactory::SetDefaultCompilerIndex(int compilerIdx)
{
    if (CompilerIndexOK(compilerIdx))
        s_DefaultCompilerIdx = compilerIdx;
}

Compiler* CompilerFactory::GetDefaultCompiler()
{
    if (CompilerIndexOK(s_DefaultCompilerIdx))
        return Compilers[s_DefaultCompilerIdx];
    return 0;
}

void CompilerFactory::SetDefaultCompiler(Compiler* compiler)
{
    for (unsigned int i = 0; i < Compilers.GetCount(); ++i)
    {
        if (compiler == Compilers[i])
        {
            s_DefaultCompilerIdx = i;
            break;
        }
    }
}

void CompilerFactory::SaveSettings()
{
    wxString baseKey = _T("/sets");
//    Manager::Get()->GetConfigManager(_T("compiler"))->UnSet(baseKey);
    for (unsigned int i = 0; i < Compilers.GetCount(); ++i)
    {
        Compilers[i]->SaveSettings(baseKey);
    }
}

void CompilerFactory::LoadSettings()
{
    bool needAutoDetection = false;
    wxString baseKey = _T("/sets");
    for (unsigned int i = 0; i < Compilers.GetCount(); ++i)
    {
        Compilers[i]->LoadSettings(baseKey);
        if (Compilers[i]->GetMasterPath().IsEmpty())
            needAutoDetection = true;
    }

    // auto-detect missing compilers
    if (needAutoDetection)
    {
        AutoDetectCompilers adc(0L);
        adc.ShowModal();
    }
}
