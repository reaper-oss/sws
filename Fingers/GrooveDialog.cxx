#include "stdafx.h"

#include "GrooveDialog.hxx"
#include "GrooveTemplates.hxx"
#include "CommandHandler.h"
#include "FNG_Settings.h"
#include "RprException.hxx"
#include "../../WDL/dirscan.h"

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

static const int FNG_PASSTHROUGH = 1;

void ShowGrooveDialog(int flags, void *data);

GrooveDialog::GrooveDialog()
:SWS_DockWnd(IDD_GROOVEDIALOG, "Groove", "FNGGroove", 30010, CReaperCommandHandler::GetID(ShowGrooveDialog))
{
#ifdef _WIN32
	CoInitializeEx(NULL, 0);
#endif

	/* set up commands and key bindings*/
	addCommand(mKbdCommands, FNG_PASSTHROUGH, "Pass through to the main window");

	mKbdSection = new KbdSectionInfo;
	std::memset(mKbdSection, 0, sizeof(KbdSectionInfo));

	mKbdSection->name = "FNG: groove tool";
	mKbdSection->action_list = &mKbdCommands[0];
	mKbdSection->action_list_cnt = mKbdCommands.size();
	mKbdSection->def_keys = NULL;
	mKbdSection->def_keys_cnt = 0;
	mKbdSection->uniqueID = 0x10000000 | 0x0666;
	CReaperCommandHandler::registerKbdSection(mKbdSection, this);
	mPassToMain = false;
	mIgnoreVelocity = false;

	// Must call SWS_DockWnd::Init() to restore parameters and open the window if necessary
	Init();
}

int GrooveDialog::OnKey(MSG *msg, int iKeyState)
{
	if(msg->hwnd == m_hwnd || IsChild(m_hwnd, msg->hwnd) == TRUE) {
		if(msg->wParam == VK_ESCAPE) {
			DestroyWindow(m_hwnd);
			return 1;
		}
		if(kbd_translateAccelerator(m_hwnd, msg, mKbdSection) == 1) {
			if(mPassToMain) {
				mPassToMain = false;
				return -666;
			}
			return -1;
		}
	}
	return 0;
}

GrooveDialog::~GrooveDialog()
{
	if(mAccel)
		delete mAccel;
	if(mKbdSection)
		delete mKbdSection;
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
	case IDM_OPEN_FOLDER:
		OnGrooveFolderButton(HIWORD(wParam), lParam);
		break;
	case IDC_GROOVELIST:
		OnGrooveList(HIWORD(wParam), lParam);
		break;
	case IDM_REFRESH:
		RefreshGrooveList();
		break;
	case IDM_IGNORE_VEL:
	{
		HMENU sysMenu = GetMenu(m_hwnd);
		MENUITEMINFO mi= {sizeof(MENUITEMINFO),};
		mi.fMask = MIIM_STATE;
		GetMenuItemInfo(sysMenu, IDM_IGNORE_VEL, false, &mi);
		if(mi.fState & MF_CHECKED) {
			mIgnoreVelocity = false;
			CheckMenuItem(sysMenu, IDM_IGNORE_VEL, MF_UNCHECKED | MF_BYCOMMAND);
			setReaperProperty("grooveWnd_ignorevel", 0);
		}
		else {
			mIgnoreVelocity = true;
			CheckMenuItem(sysMenu, IDM_IGNORE_VEL, MF_CHECKED | MF_BYCOMMAND);
			setReaperProperty("grooveWnd_ignorevel", 1);
		}
	}
	case IDC_STRENGTH:
		OnStrengthChange(HIWORD(wParam), lParam);
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
	case IDM_SAVE_GROOVE:
		Main_OnCommandEx(NamedCommandLookup("_FNG_SAVE_GROOVE"), 0, 0);
		RefreshGrooveList();
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
	case IDM_EXIT:
		DestroyWindow(m_hwnd);
		break;
	case IDM_SHOW_ACTION_LIST:
		ShowActionList(mKbdSection, m_hwnd);
		break;
	case FNG_PASSTHROUGH:
		mPassToMain = true;
		break;
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

void GrooveDialog::OnStrengthChange(WORD wParam, LPARAM lParam)
{
	HWND strengthControl = GetDlgItem(m_hwnd, IDC_STRENGTH);
	char percentage[16];
	GetWindowText(strengthControl, percentage, 16);
	int nPerc = atoi(percentage);
	GrooveTemplateHandler *me = GrooveTemplateHandler::Instance();
	me->SetGrooveStrength(nPerc);
	if(nPerc > 100) {
		nPerc = 100;
		SetWindowText(strengthControl, "100");
	}
	if(nPerc < 0) {
		nPerc = 0;
		SetWindowText(strengthControl, "100");
	}
}

void GrooveDialog::OnGrooveFolderButton(WORD wParam, LPARAM lParam)
{
	if (wParam != BN_CLICKED)
		return;
	char cDir[256];
	GrooveTemplateHandler *me = GrooveTemplateHandler::Instance();
	if (BrowseForDirectory("Select folder containing grooves", me->GetGrooveDir().c_str(), cDir, 256))
	{
		std::string sDir = cDir;
		me->SetGrooveDir(sDir);
		RefreshGrooveList();
	}
}

void GrooveDialog::RefreshGrooveList()
{
	SendDlgItemMessage(m_hwnd, IDC_GROOVELIST, LB_RESETCONTENT, 0, 0);
	SendDlgItemMessage(m_hwnd, IDC_GROOVELIST, LB_ADDSTRING, 0, (LPARAM)"** User Groove **");
	WDL_DirScan dirScan;
	WDL_String searchStr;
	searchStr = currentDir.c_str();
	searchStr.Append("\\*.rgt", MAX_PATH);
	int iFind = dirScan.First(searchStr.Get()
#ifdef _WIN32
		, true);
#else
	);
#endif
	if (iFind == 0)
		do
			SendDlgItemMessage(m_hwnd, IDC_GROOVELIST, LB_ADDSTRING, 0, (LPARAM)dirScan.GetCurrentFN());
		while(!dirScan.Next());
}

void GrooveDialog::ApplySelectedGroove()
{
	int index = SendDlgItemMessage(m_hwnd, IDC_GROOVELIST, LB_GETCURSEL, 0, 0);
	std::string szGroove = "** User Groove **";
	GrooveTemplateMemento memento = GrooveTemplateHandler::GetMemento();
	
	if(index > 0) {
		std::string itemLocation;
		char *itemText = new char[MAX_PATH];
		SendDlgItemMessage(m_hwnd, IDC_GROOVELIST, LB_GETTEXT, index, (LPARAM)itemText);
		itemLocation = currentDir;
		itemLocation += "\\";
		itemLocation += itemText;
		szGroove = itemText;
		delete[] itemText;

		GrooveTemplateHandler *me = GrooveTemplateHandler::Instance();
		
		std::string errMessage;
		if(!me->LoadGroove(itemLocation, errMessage))
			MessageBox(GetMainHwnd(), errMessage.c_str(), "Error", 0);
	}
	if(index >= 0) {
		
		GrooveTemplateHandler *me = GrooveTemplateHandler::Instance();
		int beatDivider = me->GetGrooveTolerance();

		/* set all amplitudes to < 0.0 so they are not used */
		if(mIgnoreVelocity) {
			me->resetAmplitudes();
		}
		
		bool midiEditorTarget = SendDlgItemMessage(m_hwnd, IDC_TARG_NOTES, BM_GETCHECK, 0, 0) == BST_CHECKED;
		
		HWND editControl = GetDlgItem(m_hwnd, IDC_STRENGTH);
		char percentage[16];
		GetWindowText(editControl, percentage, 16);
		double strength = atoi(percentage) / 100.0f;
		std::string undoMessage = "FNG: load and apply groove - " + szGroove;
		
		try {
			if(midiEditorTarget)
				me->ApplyGrooveToMidiEditor(beatDivider, strength);
			else
				me->ApplyGroove(beatDivider, strength);
			Undo_OnStateChange2(0, undoMessage.c_str());
		} catch(RprLibException &e) {
			if(e.notify()) {
				MessageBox(GetMainHwnd(), e.what(), "Error", 0);
			}
		}
	}
	GrooveTemplateHandler::SetMemento(memento);
}

void GrooveDialog::OnInitDlg()
{
	GrooveTemplateHandler *me = GrooveTemplateHandler::Instance();
	currentDir = me->GetGrooveDir();
	
	SetWindowText(m_hwnd, "Groove tool");
	SetDlgItemInt(m_hwnd, IDC_STRENGTH, me->GetGrooveStrength(), true);
	
#ifdef _WIN32
	HMENU sysMenu = GetMenu(m_hwnd);
#else
	HMENU sysMenu = LoadMenu(g_hInst, MAKEINTRESOURCE(IDR_GROOVEMENU));
	if (sysMenu)
		SetMenu(m_hwnd, sysMenu);
	else
		sysMenu = GetMenu(m_hwnd);
	// TODO - should probably include the "REAPER" generic first menu item that OSX should have
	// code will be something like SWELL_GetDefaultMenu, SWELL_DuplicateMenu, etc
#endif 

	if(getReaperProperty("grooveWnd_ignorevel") == "1") {
		mIgnoreVelocity = true;
		CheckMenuItem(sysMenu, IDM_IGNORE_VEL, MF_CHECKED | MF_BYCOMMAND);
	} else {
		mIgnoreVelocity = false;
	}
		
	setSensitivity(m_hwnd, me->GetGrooveTolerance());
	setTarget(m_hwnd, me->GetGrooveTarget() == TARGET_ITEMS);

	m_resize.init_item(IDC_GROOVELIST, 0.0, 0.0, 0.0, 1.0);
	
	RefreshGrooveList();
}