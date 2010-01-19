/*
 * This file is part of the Code::Blocks IDE and licensed under the GNU General Public License, version 3
 * http://www.gnu.org/licenses/gpl-3.0.html
 *
 * $Revision$
 * $Id$
 * $HeadURL$
 */
#include "sdk_precomp.h"
#ifndef CB_PRECOMP
    #include <wx/filedlg.h>
    #include <wx/listctrl.h>
    #include <wx/menu.h>
    #include <wx/sizer.h>
    #include <wx/txtstrm.h>
    #include <wx/wfstream.h>
    #include <wx/dataobj.h>
    #include "cbplugin.h"
    #include "configmanager.h"
#endif

#include <wx/clipbrd.h>
#include "filefilters.h"

#include "backtracedlg.h"

namespace
{
    const int idList = wxNewId();
    const int idSwitch = wxNewId();
    const int idSave = wxNewId();
    const int idJump = wxNewId();
    const int idCopyToClipboard = wxNewId();
    const int idSettingJumpDefault = wxNewId();
    const int idSettingSwitchDefault = wxNewId();
}

BEGIN_EVENT_TABLE(cbBacktraceDlg, wxPanel)
    EVT_LIST_ITEM_RIGHT_CLICK(idList, cbBacktraceDlg::OnListRightClick)
    EVT_LIST_ITEM_ACTIVATED(idList, cbBacktraceDlg::OnDoubleClick)
    EVT_MENU(idSwitch, cbBacktraceDlg::OnSwitchFrame)
    EVT_MENU(idSave, cbBacktraceDlg::OnSave)
    EVT_MENU(idJump, cbBacktraceDlg::OnJump)
    EVT_MENU(idCopyToClipboard, cbBacktraceDlg::OnCopyToClipboard)

    EVT_MENU(idSettingJumpDefault, cbBacktraceDlg::OnSettingJumpDefault)
    EVT_MENU(idSettingSwitchDefault, cbBacktraceDlg::OnSettingSwitchDefault)
END_EVENT_TABLE()

cbBacktraceDlg::cbBacktraceDlg(wxWindow* parent) :
    wxPanel(parent)
{
    m_list = new wxListCtrl(this, idList, wxDefaultPosition, wxDefaultSize,
                            wxLC_REPORT | wxLC_SINGLE_SEL | wxLC_HRULES | wxLC_VRULES);
    wxBoxSizer* bs = new wxBoxSizer(wxVERTICAL);
    bs->Add(m_list, 1, wxEXPAND | wxALL);
    SetAutoLayout(true);
    SetSizer(bs);

    m_list->InsertColumn(0, _("Nr"), wxLIST_FORMAT_RIGHT);
    m_list->InsertColumn(1, _("Address"), wxLIST_FORMAT_LEFT);
    m_list->InsertColumn(2, _("Function"), wxLIST_FORMAT_LEFT);
    m_list->InsertColumn(3, _("File"), wxLIST_FORMAT_LEFT, 128);
    m_list->InsertColumn(4, _("Line"), wxLIST_FORMAT_RIGHT, 64);
}

void cbBacktraceDlg::Reload()
{
    cbDebuggerPlugin *plugin = Manager::Get()->GetDebuggerManager()->GetActiveDebugger();
    if (!plugin)
        return;

    m_list->Freeze();
    m_list->DeleteAllItems();

    int active_frame = plugin->GetActiveStackFrame();
    for (int ii = 0; ii < plugin->GetStackFrameCount(); ++ii)
    {
        const cbStackFrame& frame = plugin->GetStackFrame(ii);
        wxString addr, num;

        if(frame.GetAddress())
            addr.Printf(wxT("%p"), reinterpret_cast<void*>(frame.GetAddress()));
        else
            addr = wxT("");
        num.Printf(wxT("%d"), frame.GetNumber());
        int idx = m_list->InsertItem(m_list->GetItemCount(), num);
        m_list->SetItem(idx, 1, addr);
        m_list->SetItem(idx, 2, frame.GetSymbol());
        m_list->SetItem(idx, 3, frame.GetFilename());
        m_list->SetItem(idx, 4, frame.GetLine());

        if (active_frame == frame.GetNumber())
            m_list->SetItemBackgroundColour(ii, wxColor(255, 0, 0));
    }
    m_list->Thaw();
    m_list->SetColumnWidth(0, 32);
    for (int i = 1; i < 3; ++i)
    {
        m_list->SetColumnWidth(i, wxLIST_AUTOSIZE);
    }
}


void cbBacktraceDlg::OnListRightClick(wxListEvent& event)
{
    wxMenu m;
    m.Append(idJump, _("Jump to this file/line"));
    m.Append(idSwitch, _("Switch to this frame"));
    m.AppendSeparator();
    m.Append(idSave, _("Save to file..."));
    m.Append(idCopyToClipboard, _("Copy to clipboard"));
    m.AppendSeparator();
    m.AppendRadioItem(idSettingJumpDefault, _("Jump on double-click"));
    m.AppendRadioItem(idSettingSwitchDefault, _("Switch on double-click"));

    bool jump_on_double_click;
    jump_on_double_click = Manager::Get()->GetConfigManager(_T("debugger"))->ReadBool(_T("jump_on_double_click"), true);

    m.Check(idSettingJumpDefault, jump_on_double_click);
    m.Check(idSettingSwitchDefault, !jump_on_double_click);

    m_list->PopupMenu(&m);
}

void cbBacktraceDlg::OnJump(wxCommandEvent& event)
{
    if (m_list->GetSelectedItemCount() == 0)
        return;

    // find selected item index
    int index = m_list->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);

    wxListItem info;
    info.m_itemId = index;
    info.m_mask = wxLIST_MASK_TEXT;
    info.m_col = 3;
    wxString file = m_list->GetItem(info) ? info.m_text : _T("");
    info.m_col = 4;
    wxString line = m_list->GetItem(info) ? info.m_text : _T("");
    if (!file.IsEmpty() && !line.IsEmpty())
    {
        long line_number;
        line.ToLong(&line_number, 10);
        Manager::Get()->GetDebuggerManager()->SyncEditor(file, line_number, false);
    }
}

void cbBacktraceDlg::OnSwitchFrame(wxCommandEvent& event)
{
    if (m_list->GetSelectedItemCount() == 0)
        return;

    // find selected item index
    int index = m_list->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    // read the frame number from the first column
    long realFrameNr;
    if (m_list->GetItemText(index).ToLong(&realFrameNr))
    {
        // switch to this frame
        cbDebuggerPlugin *plugin = Manager::Get()->GetDebuggerManager()->GetActiveDebugger();
        if (plugin)
            plugin->SwitchToFrame(realFrameNr);
    }
    else
        cbMessageBox(_("Couldn't find out the frame number!"), _("Error"), wxICON_ERROR);
}

void cbBacktraceDlg::OnDoubleClick(wxListEvent& event)
{
    bool jump = Manager::Get()->GetConfigManager(_T("debugger"))->ReadBool(_T("jump_on_double_click"), true);
    wxCommandEvent evt;
    if (jump)
        OnJump(evt);
    else
        OnSwitchFrame(evt);
}

void cbBacktraceDlg::OnSave(wxCommandEvent& event)
{
    wxFileDialog dlg(this, _("Save as text file"), wxEmptyString, wxEmptyString,
                     FileFilters::GetFilterAll(), wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    PlaceWindow(&dlg);
    if (dlg.ShowModal() != wxID_OK)
        return;

    wxFFileOutputStream output(dlg.GetPath());
    wxTextOutputStream text(output);

    for (int ii = 0; ii < m_list->GetItemCount(); ++ii)
    {
        wxListItem info;
        info.m_itemId = ii;
        info.m_col = 1;
        info.m_mask = wxLIST_MASK_TEXT;
        wxString addr = m_list->GetItem(info) && !info.m_text.IsEmpty() ? info.m_text : _T("??");
        info.m_col = 2;
        wxString func = m_list->GetItem(info) && !info.m_text.IsEmpty() ? info.m_text : _T("??");
        info.m_col = 3;
        wxString file = m_list->GetItem(info) && !info.m_text.IsEmpty() ? info.m_text : _T("??");
        info.m_col = 4;
        wxString line = m_list->GetItem(info) && !info.m_text.IsEmpty() ? info.m_text : _T("??");

        text << _T('#') << m_list->GetItemText(ii) << _T(' ')
            << addr << _T('\t')
            << func << _T(' ')
            << _T('(') << file << _T(':') << line << _T(')')
            << _T('\n');
    }
    cbMessageBox(_("File saved"), _("Result"), wxICON_INFORMATION);
}

void cbBacktraceDlg::OnCopyToClipboard(wxCommandEvent& event)
{
    wxString text;
    for (int ii = 0; ii < m_list->GetItemCount(); ++ii)
    {
        wxListItem info;
        info.m_itemId = ii;
        info.m_col = 1;
        info.m_mask = wxLIST_MASK_TEXT;
        wxString addr = m_list->GetItem(info) && !info.m_text.IsEmpty() ? info.m_text : _T("??");
        info.m_col = 2;
        wxString func = m_list->GetItem(info) && !info.m_text.IsEmpty() ? info.m_text : _T("??");
        info.m_col = 3;
        wxString file = m_list->GetItem(info) && !info.m_text.IsEmpty() ? info.m_text : _T("??");
        info.m_col = 4;
        wxString line = m_list->GetItem(info) && !info.m_text.IsEmpty() ? info.m_text : _T("??");

        text << _T('#') << m_list->GetItemText(ii) << _T(' ')
            << addr << _T('\t')
            << func << _T(' ')
            << _T('(') << file << _T(':') << line << _T(')')
            << _T('\n');
    }
    wxTextDataObject *object = new wxTextDataObject(text);
    if(wxTheClipboard->Open())
    {
        wxTheClipboard->SetData(object);
        wxTheClipboard->Close();
    }
}

void cbBacktraceDlg::OnSettingJumpDefault(wxCommandEvent& event)
{
    bool checked = event.IsChecked();
    Manager::Get()->GetConfigManager(_T("debugger"))->Write(_T("jump_on_double_click"), checked);
}

void cbBacktraceDlg::OnSettingSwitchDefault(wxCommandEvent& event)
{
    bool checked = event.IsChecked();
    Manager::Get()->GetConfigManager(_T("debugger"))->Write(_T("jump_on_double_click"), !checked);
}
