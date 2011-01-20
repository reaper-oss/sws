#include "stdafx.h"

#include "GrooveDialog.hxx"
#include "GrooveTemplates.hxx"
#include "resource.h"
#include "reaper_helper.h"
#include "reaper_plugin_functions.h"
#include "CommandHandler.h"
#include "FNG_Settings.h"
#include "RprException.hxx"
#include "../../WDL/dirscan.h"

extern int (*kbd_translateAccelerator)(HWND hwnd, MSG* msg, KbdSectionInfo* section);
extern bool (*kbd_RunCommandThroughHooks)(KbdSectionInfo* section, int* actionCommandID, int* val, int* valhw, int* relmode, HWND hwnd);
extern void (*ShowActionList)(KbdSectionInfo* caller, HWND callerWnd);

static
int translateAccel(MSG *msg, accelerator_register_t *ctx)
{
	GrooveDialog *dlg = (GrooveDialog *)ctx->user;
	return dlg->OnKeyCommand(msg);
}

static void setWindowPosition(HWND grooveWnd)
{
	std::string sBottom, sTop, sLeft, sRight;
	sLeft = getReaperProperty("grooveWnd_left");
	sTop = getReaperProperty("grooveWnd_top");
	if(sLeft.empty() || sTop.empty())
		return;
	int x = ::atoi(sLeft.c_str());
	int y = ::atoi(sTop.c_str());
	SetWindowPos(grooveWnd, HWND_TOP, x, y, 0, 0, SWP_NOSIZE);
}

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

GrooveDialog::GrooveDialog(HINSTANCE hInstance, HWND parent) : CBaseDialog(hInstance, parent, IDD_GROOVEDIALOG)
{
#ifdef _WIN32
	CoInitializeEx(NULL, 0);
#endif

	mAccel = NULL;
	mAccel = new accelerator_register_t;
	mAccel->translateAccel = &translateAccel;
	mAccel->user = this;
	mAccel->isLocal = TRUE;
	CReaperCommandHandler::registerAccelerator(mAccel);

	/* set up commands and key bindings*/
	addCommand(mKbdCommands, 1, "Pass through to the main window");

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
}

bool GrooveDialog::OnRprCommand(int cmd)
{
	if(cmd == 1) {
		mPassToMain = true;
		return true;
	}
	else
		return false;
}

int GrooveDialog::OnKeyCommand(MSG *msg)
{
	if(msg->hwnd == getHwnd() || IsChild(getHwnd(), msg->hwnd) == TRUE) {
		if(msg->wParam == VK_ESCAPE) {
			Hide();
			return 1;
		}
		if(kbd_translateAccelerator(getHwnd(), msg, mKbdSection) == 1) {
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
	case IDM__IGNORE_VEL:
	{
		HMENU sysMenu = GetMenu(getHwnd());
		MENUITEMINFO mi= {sizeof(MENUITEMINFO),};
		mi.fMask = MIIM_STATE;
		GetMenuItemInfo(sysMenu, IDM__IGNORE_VEL, false, &mi);
		if(mi.fState & MF_CHECKED) {
			mIgnoreVelocity = false;
			CheckMenuItem(sysMenu, IDM__IGNORE_VEL, MF_UNCHECKED | MF_BYCOMMAND);
			setReaperProperty("grooveWnd_ignorevel", 0);
		}
		else {
			mIgnoreVelocity = true;
			CheckMenuItem(sysMenu, IDM__IGNORE_VEL, MF_CHECKED | MF_BYCOMMAND);
			setReaperProperty("grooveWnd_ignorevel", 1);
		}
	}
	case IDC_STRENGTH:
		OnStrengthChange(HIWORD(wParam), lParam);
		break;
	case IDC_SENS_32ND:
		setSensitivity(getHwnd(), 32);
		setGrooveTolerance(32);
		break;
	case IDC_SENS_16TH:
		setSensitivity(getHwnd(), 16);
		setGrooveTolerance(16);
		break;
	case IDC_SENS_4TH:
		setSensitivity(getHwnd(), 4);
		setGrooveTolerance(4);
		break;
	case IDC_SENS_8TH:
		setSensitivity(getHwnd(), 8);
		setGrooveTolerance(8);
		break;
	case IDC_TARG_ITEMS:
		setTarget(getHwnd(), true);
		setGrooveTarget(TARGET_ITEMS);
		break;
	case IDC_TARG_NOTES:
		setTarget(getHwnd(), false);
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
		if(IsDlgButtonChecked(getHwnd(), IDC_TARG_ITEMS) == BST_CHECKED)
			Main_OnCommandEx(NamedCommandLookup("_FNG_GET_GROOVE"), 0, 0);
		else
			Main_OnCommandEx(NamedCommandLookup("_FNG_GET_GROOVE_MIDI"), 0, 0);
		SendDlgItemMessage(getHwnd(), IDC_GROOVELIST, LB_SETCURSEL, 0, 0);
		break;
	case IDM_STAY_ON_TOP:
	{
		HMENU sysMenu = GetMenu(getHwnd());
		MENUITEMINFO mi= {sizeof(MENUITEMINFO),};
		mi.fMask = MIIM_STATE;
		GetMenuItemInfo(sysMenu, IDM_STAY_ON_TOP, false, &mi);
		
		if(mi.fState & MF_CHECKED) {
			mStayOnTop = false;
			CheckMenuItem(sysMenu, IDM_STAY_ON_TOP, MF_UNCHECKED | MF_BYCOMMAND);
			SetWindowPos(getHwnd(), HWND_NOTOPMOST, 0,0,0,0, SWP_NOMOVE | SWP_NOSIZE);
			setReaperProperty("grooveWnd_topmost", 0);
		}
		else {
			mStayOnTop = true;
			CheckMenuItem(sysMenu, IDM_STAY_ON_TOP, MF_CHECKED | MF_BYCOMMAND);
			SetWindowPos(getHwnd(), HWND_TOPMOST, 0,0,0,0, SWP_NOMOVE | SWP_NOSIZE);
			setReaperProperty("grooveWnd_topmost", 1);
		}
	}
	break;
	case IDM_EXIT:
		Hide();
		break;
	case IDM_SHOW_ACTION_LIST:
		ShowActionList(mKbdSection, getHwnd());
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
	HWND strengthControl = GetDlgItem(getHwnd(), IDC_STRENGTH);
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
	SendDlgItemMessage(getHwnd(), IDC_GROOVELIST, LB_RESETCONTENT, 0, 0);
	SendDlgItemMessage(getHwnd(), IDC_GROOVELIST, LB_ADDSTRING, 0, (LPARAM)"** User Groove **");
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
			SendDlgItemMessage(getHwnd(), IDC_GROOVELIST, LB_ADDSTRING, 0, (LPARAM)dirScan.GetCurrentFN());
		while(!dirScan.Next());
}

void GrooveDialog::ApplySelectedGroove()
{
	int index = SendDlgItemMessage(getHwnd(), IDC_GROOVELIST, LB_GETCURSEL, 0, 0);
	std::string szGroove = "** User Groove **";
	GrooveTemplateMemento memento = GrooveTemplateHandler::GetMemento();
	
	if(index > 0) {
		std::string itemLocation;
		char *itemText = new char[MAX_PATH];
		SendDlgItemMessage(getHwnd(), IDC_GROOVELIST, LB_GETTEXT, index, (LPARAM)itemText);
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
		
		bool midiEditorTarget = SendDlgItemMessage(getHwnd(), IDC_TARG_NOTES, BM_GETCHECK, 0, 0) == BST_CHECKED;
		
		HWND editControl = GetDlgItem(getHwnd(), IDC_STRENGTH);
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

void GrooveDialog::Setup()
{
	GrooveTemplateHandler *me = GrooveTemplateHandler::Instance();
	currentDir = me->GetGrooveDir();
	
	SetWindowText(getHwnd(), "Groove tool");
	//SendDlgItemMessage(getHwnd(), IDC_STRENGTH_SPIN,UDM_SETRANGE, 0, 100);
	//SendDlgItemMessage(getHwnd(), IDC_STRENGTH_SPIN,UDM_SETPOS, 0, me->GetGrooveStrength());
	
	setWindowPosition(getHwnd());

	HMENU sysMenu = GetMenu(getHwnd());
	if(getReaperProperty("grooveWnd_topmost") == "1") {
		mStayOnTop = true;
		CheckMenuItem(sysMenu, IDM_STAY_ON_TOP, MF_CHECKED | MF_BYCOMMAND);
		SetWindowPos(getHwnd(), HWND_TOPMOST, 0,0,0,0, SWP_NOMOVE | SWP_NOSIZE);
	} else {
		mStayOnTop = false;
	}

	if(getReaperProperty("grooveWnd_ignorevel") == "1") {
		mIgnoreVelocity = true;
		CheckMenuItem(sysMenu, IDM__IGNORE_VEL, MF_CHECKED | MF_BYCOMMAND);
	} else {
		mIgnoreVelocity = false;
	}
		
	setSensitivity(getHwnd(), me->GetGrooveTolerance());
	setTarget(getHwnd(), me->GetGrooveTarget() == TARGET_ITEMS);
	
	RefreshGrooveList();
}

void GrooveDialog::onHide()
{
	RECT rect;
	GetWindowRect(getHwnd(), &rect);
	setReaperProperty("grooveWnd_left", rect.left);
	setReaperProperty("grooveWnd_top", rect.top);
}

void GrooveDialog::OnCreate(HWND hWnd)
{
	
}

void GrooveDialog::OnActivate(bool activated)
{
	if(activated && mStayOnTop) {
		SetWindowPos(getHwnd(), HWND_TOPMOST, 0,0,0,0, SWP_NOMOVE | SWP_NOSIZE);
	}
}