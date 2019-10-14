/******************************************************************************
/ padreRmeTotalmix.cpp
/
/ Copyright (c) 2009-2010 Tim Payne (SWS), Jeffos (S&M), P. Bourdon
/ https://code.google.com/p/sws-extension
/
/ Permission is hereby granted, free of charge, to any person obtaining a copy
/ of this software and associated documentation files (the "Software"), to deal
/ in the Software without restriction, including without limitation the rights to
/ use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
/ of the Software, and to permit persons to whom the Software is furnished to
/ do so, subject to the following conditions:
/ 
/ The above copyright notice and this permission notice shall be included in all
/ copies or substantial portions of the Software.
/ 
/ THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
/ EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
/ OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
/ NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
/ HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
/ WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
/ FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
/ OTHER DEALINGS IN THE SOFTWARE.
/
******************************************************************************/

#include "stdafx.h"
#include "padreRmeTotalmix.h"

//JFB not localized: related actions are not hooked (personnal stuff?)

void RmeTotalmix(RmeTotalmixCmd cmd)
{
	HWND hFirefaceWnd = FindWindow("Fireface Mixer Class 1", NULL);

	if(!hFirefaceWnd)
	{
		HWND hMainWnd = GetMainHwnd();
		MessageBox(hMainWnd, "RME Totalmix not found!", "Error", MB_OK);
		return;
	}

	BYTE vkKey;
	BYTE vkModKey = 0;
	switch(cmd)
	{
		case eTOTALMIX_LOADUSER1 : case eTOTALMIX_LOADUSER2 : case eTOTALMIX_LOADUSER3 : case eTOTALMIX_LOADUSER4 :
		case eTOTALMIX_LOADUSER5 : case eTOTALMIX_LOADUSER6 : case eTOTALMIX_LOADUSER7 : case eTOTALMIX_LOADUSER8 :
			vkModKey = VK_MENU;
			vkKey = (BYTE)VkKeyScan('1' + (cmd-eTOTALMIX_LOADUSER1));
		break;

		//! \bug Doesn't work: VK_CONTROL, VK_LCONTROL, VK_RCONTROL emulate 'alt' (VK_MENU) not 'ctrl'
		case eTOTALMIX_LOADFACT1 : case eTOTALMIX_LOADFACT2 : case eTOTALMIX_LOADFACT3 : case eTOTALMIX_LOADFACT4 :
		case eTOTALMIX_LOADFACT5 : case eTOTALMIX_LOADFACT6 : case eTOTALMIX_LOADFACT7 : case eTOTALMIX_LOADFACT8 :
			vkModKey = VK_CONTROL;
			vkKey = (BYTE)VkKeyScan('1' + (cmd-eTOTALMIX_LOADFACT1));
		break;

		case eTOTALMIX_MASTERMUTE :
			vkKey = (BYTE)VkKeyScan('M');
		break;
		
		default :
			return;
		break;
	}

	HWND hForegroundWnd = GetForegroundWindow();
	SetForegroundWindow(hFirefaceWnd);

	if(vkModKey != 0)
		keybd_event(vkModKey, 0, 0, 0);
	keybd_event(vkKey, 0, 0, 0);
	keybd_event(vkKey, 0, KEYEVENTF_KEYUP, 0);
	if(vkModKey != 0)
		keybd_event(vkModKey, 0, KEYEVENTF_KEYUP, 0);

	SetForegroundWindow(hForegroundWnd);
}
