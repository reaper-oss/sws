#include "stdafx.h"

#include "GrooveDialog.h"
#include "GrooveTemplates.h"
#include "CommandHandler.h"
#include "FNG_Settings.h"
#include "RprException.h"

#include <WDL/localize/localize.h>
#include <WDL/dirscan.h>

#ifndef PATH_SEP
#ifdef _WIN32
#define PATH_SEP "\\"
#else
#define PATH_SEP "/"
#endif
#endif


static void addDefaultBinding(std::vector<KbdKeyBindingInfo> &bindings, int cmd, int key, int flags = 0)
{
    KbdKeyBindingInfo binding;
    binding.cmd = cmd;
    binding.flags = flags;
    binding.key = key;
    bindings.push_back(binding);
}

static void addCommand(std::vector<KbdCmd> &cmds, int cmd, const char *name)
{
    KbdCmd keycmd;
    keycmd.cmd = cmd;
    keycmd.text = name;
    cmds.push_back(keycmd);
}

static const int FNG_OPEN_FOLDER = 0xFF00;
static const int FNG_REFRESH     = 0xFF01;

void ShowGrooveDialog(int flags, void *data);

GrooveDialog::GrooveDialog()
:SWS_DockWnd(IDD_GROOVEDIALOG, __LOCALIZE("Groove","sws_DLG_157"), "FNGGroove")
{
#ifdef _WIN32
    CoInitializeEx(NULL, 0);
#endif

    mHorizontalView = false; // Unused now; maybe later?

    // Must call SWS_DockWnd::Init() to restore parameters and open the window if necessary
    Init();
}

HMENU GrooveDialog::OnContextMenu(int x, int y, bool* wantDefaultItems)
{
    HMENU contextMenu = CreatePopupMenu();
    AddToMenu(contextMenu, __LOCALIZE("Select groove folder...","sws_DLG_157"), FNG_OPEN_FOLDER);
    AddToMenu(contextMenu, __LOCALIZE("Save groove...","sws_DLG_157"), NamedCommandLookup("_FNG_SAVE_GROOVE"));
    AddToMenu(contextMenu, __LOCALIZE("Refresh","sws_DLG_157"), FNG_REFRESH);
    return contextMenu;
}

int GrooveDialog::OnKey(MSG* msg, int iKeyState)
{
    if (GetDlgItem(m_hwnd, IDC_GROOVELIST) == msg->hwnd && msg->message == WM_KEYDOWN && !iKeyState)
    {
        // Pass arrows on to the list
        if (msg->wParam == VK_UP || msg->wParam == VK_DOWN)
            return -1;
        else if (msg->wParam == VK_RETURN)
        {
            ApplySelectedGroove();
            return 1;
        }
    }
    return 0;
}

GrooveDialog::~GrooveDialog()
{
#ifdef _WIN32
    CoUninitialize();
#endif
}
static void setGrooveTolerance(int tol)
{
    GrooveTemplateHandler *me = GrooveTemplateHandler::Instance();
    me->SetGrooveTolerance(tol);
}

#define TARGET_ITEMS 0
#define TARGET_NOTES 1

static void setGrooveTarget(int target)
{
    GrooveTemplateHandler *me = GrooveTemplateHandler::Instance();
    me->SetGrooveTarget(target);
}

static void setSensitivity(HWND mainHwnd, int sensitivity)
{
    int nId = IDC_SENS_16TH;
    if(sensitivity == 4)
        nId = IDC_SENS_4TH;
    if(sensitivity == 8)
        nId = IDC_SENS_8TH;
    if(sensitivity == 32)
        nId = IDC_SENS_32ND;
    for (int iD = IDC_SENS_4TH; iD <= IDC_SENS_32ND; iD++)
        CheckDlgButton(mainHwnd, iD, 0);
    CheckDlgButton(mainHwnd, nId, BST_CHECKED);
}

static void setTarget(HWND mainHwnd, bool items)
{
    CheckDlgButton(mainHwnd, IDC_TARG_ITEMS, items ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(mainHwnd, IDC_TARG_NOTES, items ? BST_UNCHECKED : BST_CHECKED);
}

void GrooveDialog::OnCommand(WPARAM wParam, LPARAM lParam)
{
    switch(LOWORD(wParam))
    {
    case FNG_OPEN_FOLDER:
        OnGrooveFolderButton(HIWORD(wParam), lParam);
        break;
    case FNG_REFRESH:
        RefreshGrooveList();
        break;
    case IDC_GROOVELIST:
        OnGrooveList(HIWORD(wParam), lParam);
        break;
    case IDC_STRENGTH:
        OnStrengthChange(HIWORD(wParam), lParam);
        break;
    case IDC_VELSTRENGTH:
        OnVelStrengthChange(HIWORD(wParam), lParam);
        break;
    case IDC_SENS_32ND:
        setSensitivity(m_hwnd, 32);
        setGrooveTolerance(32);
        break;
    case IDC_SENS_16TH:
        setSensitivity(m_hwnd, 16);
        setGrooveTolerance(16);
        break;
    case IDC_SENS_4TH:
        setSensitivity(m_hwnd, 4);
        setGrooveTolerance(4);
        break;
    case IDC_SENS_8TH:
        setSensitivity(m_hwnd, 8);
        setGrooveTolerance(8);
        break;
    case IDC_TARG_ITEMS:
        setTarget(m_hwnd, true);
        setGrooveTarget(TARGET_ITEMS);
        break;
    case IDC_TARG_NOTES:
        setTarget(m_hwnd, false);
        setGrooveTarget(TARGET_NOTES);
        break;
    case IDC_APPLYGROOVE:
        ApplySelectedGroove();
        break;
    case IDC_STORE:
        if(IsDlgButtonChecked(m_hwnd, IDC_TARG_ITEMS) == BST_CHECKED)
            Main_OnCommandEx(NamedCommandLookup("_FNG_GET_GROOVE"), 0, 0);
        else
            Main_OnCommandEx(NamedCommandLookup("_FNG_GET_GROOVE_MIDI"), 0, 0);
        SendDlgItemMessage(m_hwnd, IDC_GROOVELIST, LB_SETCURSEL, 0, 0);
        break;
    default:
        Main_OnCommand((int)wParam, (int)lParam); // Required when you have reaper commands in the context menu
    }
}

void GrooveDialog::OnGrooveList(WORD wParam, LPARAM lParam)
{
    switch(wParam)
    {
    case LBN_DBLCLK:
        ApplySelectedGroove();
        break;
    }
}
static int getStrengthValue(HWND parent, int control)
{
    HWND strengthControl = GetDlgItem(parent, control);
    char percentage[16];
    GetWindowText(strengthControl, percentage, 16);
    int nPerc = atoi(percentage);
    if(nPerc > 100) {
        nPerc = 100;
        SetWindowText(strengthControl, "100");
    }
    if(nPerc < 0) {
        nPerc = 0;
        SetWindowText(strengthControl, "100");
    }
    return nPerc;
}

void GrooveDialog::OnVelStrengthChange(WORD wParam, LPARAM lParam)
{
    GrooveTemplateHandler *me = GrooveTemplateHandler::Instance();
    me->SetGrooveVelStrength(getStrengthValue(m_hwnd, IDC_VELSTRENGTH));
}

void GrooveDialog::OnStrengthChange(WORD wParam, LPARAM lParam)
{
    GrooveTemplateHandler *me = GrooveTemplateHandler::Instance();
    me->SetGrooveStrength(getStrengthValue(m_hwnd, IDC_STRENGTH));
}

void GrooveDialog::OnGrooveFolderButton(WORD wParam, LPARAM lParam)
{
    if (wParam != BN_CLICKED)
        return;
    char cDir[256];
    GrooveTemplateHandler *me = GrooveTemplateHandler::Instance();
    if (BrowseForDirectory(__LOCALIZE("Select folder containing grooves","sws_DLG_157"), me->GetGrooveDir().c_str(), cDir, 256))
    {
        currentDir = cDir;
        me->SetGrooveDir(currentDir);
        RefreshGrooveList();
    }
}

void GrooveDialog::RefreshGrooveList()
{
    SendDlgItemMessage(m_hwnd, IDC_GROOVELIST, LB_RESETCONTENT, 0, 0);
    SendDlgItemMessage(m_hwnd, IDC_GROOVELIST, LB_ADDSTRING, 0, (LPARAM)__LOCALIZE("** User Groove **","sws_DLG_157"));
    WDL_DirScan dirScan;
    WDL_String searchStr(currentDir.c_str());
/* dirScan doesn't support wildcards on OSX do filtering later */
#ifdef _WIN32
    searchStr.Append( PATH_SEP "*.rgt", MAX_PATH);
    int iFind = dirScan.First(searchStr.Get(), true);
#else
    int iFind = dirScan.First(searchStr.Get());
#endif

    if (iFind == 0) {
        do {
            std::string fileName = dirScan.GetCurrentFN();
/* dirScan doesn't support wildcards on OSX so do basic filtering here */
#ifndef _WIN32
            std::string::size_type index = fileName.find_last_of(".");
            if(index == std::string::npos)
                continue;
            if(fileName.substr(index) != ".rgt")
                continue;
#endif
            std::string fileHead = fileName.substr(0, fileName.size() - 4);
            SendDlgItemMessage(m_hwnd, IDC_GROOVELIST, LB_ADDSTRING, 0, (LPARAM)fileHead.c_str());
        } while(!dirScan.Next());
    }
}

void GrooveDialog::ApplySelectedGroove()
{
    int index = (int)SendDlgItemMessage(m_hwnd, IDC_GROOVELIST, LB_GETCURSEL, 0, 0);
    std::string grooveName = __LOCALIZE("** User Groove **","sws_DLG_157");
    GrooveTemplateMemento memento = GrooveTemplateHandler::GetMemento();

    if(index > 0) {
        std::string itemLocation;
        char itemText[MAX_PATH];
        SendDlgItemMessage(m_hwnd, IDC_GROOVELIST, LB_GETTEXT, index, (LPARAM)itemText);
        grooveName = itemText;
        itemLocation = currentDir;
        itemLocation += PATH_SEP;
        itemLocation += grooveName;
        itemLocation += ".rgt";


        GrooveTemplateHandler *me = GrooveTemplateHandler::Instance();

        std::string errMessage;
        if(!me->LoadGroove(itemLocation, errMessage))
            MessageBox(GetMainHwnd(), errMessage.c_str(), __LOCALIZE("FNG - Error","sws_DLG_157"), 0);
    }
    if(index >= 0) {

        GrooveTemplateHandler *me = GrooveTemplateHandler::Instance();
        int beatDivider = me->GetGrooveTolerance();

        bool midiEditorTarget = SendDlgItemMessage(m_hwnd, IDC_TARG_NOTES, BM_GETCHECK, 0, 0) == BST_CHECKED;

        HWND editControl = GetDlgItem(m_hwnd, IDC_STRENGTH);
        char percentage[16];
        GetWindowText(editControl, percentage, 16);
        double posStrength = (double)atoi(percentage) / 100.0;
        editControl = GetDlgItem(m_hwnd, IDC_VELSTRENGTH);
        GetWindowText(editControl, percentage, 16);
        double velStrength = (double)atoi(percentage) / 100.0;
        std::string undoMessage = __LOCALIZE("FNG: load and apply groove - ","sws_DLG_157") + grooveName;

        try {
            if(midiEditorTarget)
                me->ApplyGrooveToMidiEditor(beatDivider, posStrength, velStrength);
            else
                me->ApplyGroove(beatDivider, posStrength, velStrength);
            Undo_OnStateChange2(0, undoMessage.c_str());
        } catch(RprLibException &e) {
            if(e.notify()) {
                MessageBox(GetMainHwnd(), e.what(), __LOCALIZE("FNG - Error","sws_DLG_157"), 0);
            }
        }
    }
    GrooveTemplateHandler::SetMemento(memento);
}

void GrooveDialog::OnInitDlg()
{
    GrooveTemplateHandler *me = GrooveTemplateHandler::Instance();
    currentDir = me->GetGrooveDir();

    SetWindowText(m_hwnd, __LOCALIZE("Groove tool","sws_DLG_157"));
    SetDlgItemInt(m_hwnd, IDC_STRENGTH, me->GetGrooveStrength(), true);
    SetDlgItemInt(m_hwnd, IDC_VELSTRENGTH, me->GetGrooveVelStrength(), true);

    setSensitivity(m_hwnd, me->GetGrooveTolerance());
    setTarget(m_hwnd, me->GetGrooveTarget() == TARGET_ITEMS);

    m_resize.init_item(IDC_GROOVELIST, 0.0, 0.0, 0.0, 1.0);

    SetWindowLongPtr(GetDlgItem(m_hwnd, IDC_STRENGTH), GWLP_USERDATA, 0xdeadf00b);
    SetWindowLongPtr(GetDlgItem(m_hwnd, IDC_VELSTRENGTH), GWLP_USERDATA, 0xdeadf00b);

#ifdef _WIN32
    WDL_UTF8_HookListBox(GetDlgItem(m_hwnd, IDC_GROOVELIST));
#endif

    RefreshGrooveList();
}
