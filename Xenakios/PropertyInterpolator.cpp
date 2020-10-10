/******************************************************************************
/ PropertyInterpolator.cpp
/
/ Copyright (c) 2010 Tim Payne (SWS), original code by Xenakios
/
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

#include "../Breeder/BR_Util.h"
#include "../SnM/SnM_Dlg.h"

#include <WDL/localize/localize.h>

using namespace std;

LICE_SysBitmap *g_framebuffer = NULL;

double g_MinItemTime=0;
double g_MaxItemTime=0;

void PerformPropertyChanges();

#ifdef _WIN32
WNDPROC g_OldGraphAreaWndProc;
#endif
vector<MediaItem_Take*> g_IItakes;

HWND g_hIIdlg=0;

typedef struct t_interpolator_envelope_node
{
	double Time;
	double Value;
} t_interpolator_envelope_node;

typedef vector<t_interpolator_envelope_node> t_interpolator_envelope;

int g_activePropertyEnvelope=0;

typedef struct t_interpolator_item_state
{
	double position;
	double length;
	double pitch;
	double playrate;
	bool preservepp;
	double mediaoffset;
	double itemvol;
	double takepan;
	double takevol;
	double fipmYpos;

	MediaItem_Take *pTake;
} t_interpolator_item_state;

vector<t_interpolator_item_state> g_ii_storeditemstates;

typedef struct t_interpolator_item_property
{
	bool enabled;
	const char *Name;
	bool IsTakeProperty;
	const char *APIAccessID;
	double MinValue;
	double MaxValue;
	double NeutralValue;
	t_interpolator_envelope *Envelope;
	t_interpolator_envelope *RndSpreadEnv;
} t_interpolator_item_property;

t_interpolator_item_property g_IIproperties[] =
{
	{false,"Item length",false,"D_LENGTH",0.01,2.0,1.0,0,0},
	{false,"Item volume",false,"D_VOL",0.0,2.0,1.0,0,0},
	{false,"Item fade in length",false,"D_FADEINLEN",0.0,0.25,0.0,0,0},
	{false,"Item fade out length",false,"D_FADEOUTLEN",0.0,0.25,0.0,0,0},

	{false,"Item FIPM Y-pos",false,"F_FREEMODE_Y",0.0,1.0,0.0,0,0},
	{false,"Take pan",true,"D_PAN",-1.0,1.0,0.0,0,0},

	{false,"Take pitch",true,"D_PITCH",-24.0,24.0,0.0,0,0},
	{false,"Take pitch (resampled)",true,"D_PLAYRATE",-24.0,24.0,0.0,0,0},
	{false,"Take media offset",true,"D_STARTOFFS",0.0,1.0,0.0,0,0},

	{false,"Item position",false,"D_POSITION",0.0,2.0,1.0,0,0},
	{false,"Item rnd sel toggle",false,"B_UISEL",0.0,1.0,1.0,0,0},

	{false,NULL,}

};

bool MyNodeSortFunction (t_interpolator_envelope_node a,t_interpolator_envelope_node b)
{
	if (a.Time<b.Time) return true;
	return false;
}

double RealValToNormzdVal(int propIDX,double theValue)
{
	double maxVal=g_IIproperties[propIDX].MaxValue;
	double minVal=g_IIproperties[propIDX].MinValue;
	double valRang=maxVal-minVal;
	double normzFactor=1.0/valRang;
	//double normzVal=(minVal*normzFactor)+(theValue*normzFactor);
	double normzVal=1.0-((maxVal*normzFactor)-(theValue*normzFactor));
	return normzVal;
}

void DrawEnvelope()
{
	RECT r;
	GetClientRect(GetDlgItem(g_hIIdlg,IDC_IIENVAREA), &r);
	int NodeSize=3;
	double itemTimeRange=g_MaxItemTime-g_MinItemTime;
	if (strcmp(g_IIproperties[g_activePropertyEnvelope].Name,"Take pitch")==0||strcmp(g_IIproperties[g_activePropertyEnvelope].Name,"Take pitch (resampled)")==0)
	{
		double pitchGridSpacing=2.0;
		double parRange=g_IIproperties[g_activePropertyEnvelope].MaxValue-g_IIproperties[g_activePropertyEnvelope].MinValue;
		int numVertlines=1+(int)(parRange/pitchGridSpacing);
		double ycorScaler=r.bottom/parRange;
		for (int i = 0; i < numVertlines; i++)
		{
			int ycor = (int)(ycorScaler * i * pitchGridSpacing);
			LICE_Line(g_framebuffer,0,ycor,r.right,ycor,LICE_RGBA(90,90,90,255));
		}
	}
	for (int i = 0; i < (int)g_ii_storeditemstates.size(); i++)
	{
		double itemPos=g_ii_storeditemstates[i].position;
		itemPos-=g_MinItemTime;
		int xcor=(int)((r.right/itemTimeRange)*itemPos);
		LICE_Line(g_framebuffer,xcor,0,xcor,r.bottom,LICE_RGBA(128,128,128,255));
		if (strcmp(g_IIproperties[g_activePropertyEnvelope].Name,"Item position")==0)
		{
			int ycor=r.bottom-(int)((r.bottom/(g_MaxItemTime-g_MinItemTime))*itemPos);
			LICE_Line(g_framebuffer,xcor-3,ycor,xcor+3,ycor,LICE_RGBA(128,128,128,255));

		}
	}
	t_interpolator_envelope *pActiveEnvelope=g_IIproperties[g_activePropertyEnvelope].Envelope;
	LICE_pixel thecolor=LICE_RGBA(0,255,0,255);

	for (int i = 0; i < (int)pActiveEnvelope->size(); i++)
	{
		int Xcor1, Ycor1, Xcor2, Ycor2;
		Xcor1 = (int)(r.right*pActiveEnvelope->at(i).Time); // [i].Time;
		Ycor1 = (int)(r.bottom*(1.0-pActiveEnvelope->at(i).Value)); // [i].Value);
		if (i<(int)pActiveEnvelope->size()-1)
		{
			Xcor2 = (int)(r.right*pActiveEnvelope->at(i+1).Time); // [i+1].Time;
			Ycor2 = (int)(r.bottom*(1.0-pActiveEnvelope->at(i+1).Value)); // [i+1].Value);
			LICE_Line(g_framebuffer, Xcor1, Ycor1, Xcor2, Ycor2, LICE_RGBA(255,255,255,255));
		}
		LICE_Line(g_framebuffer, Xcor1-NodeSize, Ycor1-NodeSize, Xcor1+NodeSize, Ycor1-NodeSize, thecolor);
		LICE_Line(g_framebuffer, Xcor1+NodeSize, Ycor1-NodeSize, Xcor1+NodeSize, Ycor1+NodeSize, thecolor);
		LICE_Line(g_framebuffer, Xcor1+NodeSize, Ycor1+NodeSize, Xcor1-NodeSize, Ycor1+NodeSize, thecolor);
		LICE_Line(g_framebuffer, Xcor1-NodeSize, Ycor1+NodeSize, Xcor1-NodeSize, Ycor1-NodeSize, thecolor);
		if (i==0 && Xcor1>0)
			LICE_Line(g_framebuffer,0,Ycor1,Xcor1,Ycor1,LICE_RGBA(255,255,255,255));
		if (i==pActiveEnvelope->size()-1 && Xcor1 < r.right)
			LICE_Line(g_framebuffer,Xcor1,Ycor1,r.right,Ycor1,LICE_RGBA(255,255,255,255));

	}
}

int GetHotNodeIndex(int xcor,int ycor)
{
	RECT r;
	GetWindowRect(GetDlgItem(g_hIIdlg,IDC_IIENVAREA),&r);
	int detectRange=6;
	int GraphWidth=r.right-r.left;
	int GraphHeight=abs(r.bottom-r.top);
	RECT dlgRect;
	GetWindowRect(g_hIIdlg,&dlgRect);

	RECT compareRECT;
	t_interpolator_envelope *TargetEnvelope=g_IIproperties[g_activePropertyEnvelope].Envelope;

	for (int i = 0; i < (int)TargetEnvelope->size(); i++)
	{
		compareRECT.left   = (LONG)(GraphWidth*TargetEnvelope->at(i).Time)-detectRange+r.left;
		compareRECT.right  = (LONG)(GraphWidth*TargetEnvelope->at(i).Time)+detectRange+r.left;
		compareRECT.top    = (LONG)(GraphHeight*(1.0-TargetEnvelope->at(i).Value))-detectRange+r.top;
		compareRECT.bottom = (LONG)(GraphHeight*(1.0-TargetEnvelope->at(i).Value))+detectRange+r.top;
		POINT pt;
		pt.x = xcor+r.left;
		pt.y = ycor+r.top;
		if (PtInRect(&compareRECT,pt)>0)
			return i;
	}
	return -1;
}

void MoveNodeByCoords(int nodeIndex,int x,int y)
{
	RECT r;
	GetClientRect(GetDlgItem(g_hIIdlg,IDC_IIENVAREA),&r);
	t_interpolator_envelope *TargetEnvelope=g_IIproperties[g_activePropertyEnvelope].Envelope;

	if (nodeIndex>=0 && nodeIndex<(int)TargetEnvelope->size())
	{
		double NewTime=(1.0/r.right)*x;
		double NewValue=1.0-((1.0/r.bottom)*y);
		if (nodeIndex==0 && NewTime<0) NewTime=0.0;
		if (TargetEnvelope->size()>1)
			if (nodeIndex==0 && NewTime>=TargetEnvelope->at(1).Time) NewTime=TargetEnvelope->at(1).Time-0.001;
		if (TargetEnvelope->size()==1)
			if (nodeIndex==0 && NewTime>1.0) NewTime=1.0;
		if (nodeIndex>0 && nodeIndex<(int)TargetEnvelope->size()-1 && NewTime>=TargetEnvelope->at(nodeIndex+1).Time) NewTime=TargetEnvelope->at(nodeIndex+1).Time-0.001;// [nodeIndex+1].Time-0.001;
		if (nodeIndex>0 && NewTime<=TargetEnvelope->at(nodeIndex-1).Time) NewTime=TargetEnvelope->at(nodeIndex-1).Time+0.001;

		if (nodeIndex==TargetEnvelope->size()-1 && x>=r.right) NewTime=1.0;
		if (NewValue<0.0) NewValue=0.0;
		if (NewValue>1.0) NewValue=1.0;
		TargetEnvelope->at(nodeIndex).Time=NewTime;// [nodeIndex].Time=NewTime;
		TargetEnvelope->at(nodeIndex).Value=NewValue; // [nodeIndex].Value=NewValue;
	}
	if (TargetEnvelope->size()>1)
		sort(TargetEnvelope->begin(),TargetEnvelope->end(),MyNodeSortFunction);
	InvalidateRect(g_hIIdlg,NULL,FALSE);
}

int AddNodeFromCoordinates(int x,int y)
{
	RECT r;
	GetClientRect(GetDlgItem(g_hIIdlg,IDC_IIENVAREA),&r);
	double NewTime=((1.0/r.right)*x);
	double NewValue=1.0-((1.0/r.bottom)*y);
	t_interpolator_envelope *TargetEnv=g_IIproperties[g_activePropertyEnvelope].Envelope;
	t_interpolator_envelope_node NewNode;
	NewNode.Time=NewTime;
	NewNode.Value=NewValue;
	TargetEnv->push_back(NewNode);
	sort(TargetEnv->begin(),TargetEnv->end(),MyNodeSortFunction);
	InvalidateRect(g_hIIdlg,NULL,FALSE);
	return -666;
}

void RecallOrigProps()
{
	PreventUIRefresh(1);
	int i;
	t_interpolator_item_state ista;
	for (i=0;i<(int)g_ii_storeditemstates.size();i++)
	{
		ista=g_ii_storeditemstates[i];
		MediaItem_Take *pTk=g_ii_storeditemstates[i].pTake;
		MediaItem *pIt=(MediaItem*)GetSetMediaItemTakeInfo(pTk,"P_ITEM",0);
		GetSetMediaItemInfo(pIt,"D_POSITION",&ista.position);
		GetSetMediaItemInfo(pIt,"D_LENGTH",&ista.length);
		GetSetMediaItemInfo(pIt,"D_VOL",&ista.itemvol);
		GetSetMediaItemTakeInfo(pTk,"D_PAN",&ista.takepan);
		GetSetMediaItemTakeInfo(pTk,"B_PPITCH",&ista.preservepp);
		GetSetMediaItemTakeInfo(pTk,"D_PITCH",&ista.pitch);
		GetSetMediaItemTakeInfo(pTk,"D_PLAYRATE",&ista.playrate);
		bool uisel=true;
		GetSetMediaItemInfo(pIt,"B_UISEL",&uisel);
		GetSetMediaItemTakeInfo(pTk,"D_STARTOFFS",&ista.mediaoffset);
	}
	PreventUIRefresh(-1);
}

LRESULT CALLBACK EnveAreaWndProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	static bool LeftMouseBtnDown = false;
	static int HotNodeIndex = -1;
	static HCURSOR hLinkCursor    = LoadCursor(NULL, IDC_HAND);
	static HCURSOR hDefaultCursor = LoadCursor(NULL, IDC_ARROW);
	switch (Message)
	{
		case WM_MOUSEMOVE:
		{
			int mouseX=GET_X_LPARAM(lParam);
			int mouseY=GET_Y_LPARAM(lParam);
			if (!LeftMouseBtnDown)
				HotNodeIndex = GetHotNodeIndex(mouseX, mouseY);
			else if (LeftMouseBtnDown && HotNodeIndex >= 0)
				MoveNodeByCoords(HotNodeIndex, mouseX, mouseY);

			if (HotNodeIndex >= 0)
				SetCursor(hLinkCursor);
			else
				SetCursor(hDefaultCursor);
			return 0;
		}
		case WM_LBUTTONDOWN:
		{
			SetCapture(hwnd);
			if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
			{	// Remove point if CTRL pressed
				if (HotNodeIndex>=0)
				{
					t_interpolator_envelope *TargetEnv=g_IIproperties[g_activePropertyEnvelope].Envelope;
					if (TargetEnv->size() > 1)
					{
						TargetEnv->erase(TargetEnv->begin() + HotNodeIndex);
						sort(TargetEnv->begin(), TargetEnv->end(), MyNodeSortFunction);
						InvalidateRect(g_hIIdlg, NULL, FALSE);

					}
					else
						MessageBox(g_hIIdlg, __LOCALIZE("Cannot remove only point of envelope!","sws_mbox"), __LOCALIZE("Xenakios - Error","sws_mbox"), MB_OK);

				}
				return 0;
			}
			LeftMouseBtnDown = true;
			if (HotNodeIndex < 0)
			{
				AddNodeFromCoordinates(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
				HotNodeIndex=GetHotNodeIndex(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			}
			return 0;
		}
		case WM_LBUTTONUP:
		{
			RecallOrigProps();
			PerformPropertyChanges();
			ReleaseCapture();
			HotNodeIndex = -1;
			LeftMouseBtnDown = false;
			return 0;
		}
		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC dc = BeginPaint(hwnd, &ps);
			LICE_Clear(g_framebuffer, LICE_RGBA(0,0,0,1));
			DrawEnvelope();
			BitBlt(dc,0,0,g_framebuffer->getWidth(),g_framebuffer->getHeight(),g_framebuffer->getDC(),0,0,SRCCOPY);
			EndPaint(hwnd, &ps);
			return 0;
		}
	}
#ifdef _WIN32
	return CallWindowProc(g_OldGraphAreaWndProc, hwnd, Message, wParam, lParam);
#else
	return DefWindowProc(hwnd, Message, wParam, lParam);
#endif
}



void GetActItemsMinMaxTimes(double *timea,double *timeb)
{
	double maxtime=0;
	int i;
	MediaItem *CurItem;
	for (i=0;i<(int)g_IItakes.size();i++)
	{
		CurItem=(MediaItem*)GetSetMediaItemTakeInfo(g_IItakes[i],"P_ITEM",0);
		if (CurItem)
		{
			double itemPos=*(double*)GetSetMediaItemInfo(CurItem,"D_POSITION",0);
			if (itemPos>=maxtime)
				maxtime=itemPos;
		}
	}
	double mintime=maxtime;
	for (i=0;i<(int)g_IItakes.size();i++)
	{
		CurItem=(MediaItem*)GetSetMediaItemTakeInfo(g_IItakes[i],"P_ITEM",0);
		if (CurItem)
		{
			double itemPos=*(double*)GetSetMediaItemInfo(CurItem,"D_POSITION",0);
			if (itemPos<=mintime)
				mintime=itemPos;
		}
	}
	*timea=mintime;
	*timeb=maxtime;
}

double GetInterpolatedNodeValue(t_interpolator_envelope &TheNodes, double TheTime)
{
	if (TheNodes[0].Time>TheTime)
		return TheNodes[0].Value;
	if (TheTime>TheNodes[TheNodes.size()-1].Time)
		return TheNodes[TheNodes.size()-1].Value;
	if (TheTime>=1.0)
		return TheNodes[TheNodes.size()-1].Value;
	int NodeIndexA=-1;
	int i;
	for (i=0;i<(int)TheNodes.size()-1;i++)
	{
		if (TheTime>=TheNodes[i].Time && TheTime<TheNodes[i+1].Time)
		{
			NodeIndexA=i;
			break;
		}
	}
	if (NodeIndexA>=0)
	{
		double NodeTimeA=TheNodes[NodeIndexA].Time;
		double NodeValueA=TheNodes[NodeIndexA].Value;
		double NodeTimeB=1.0;
		double NodeValueB=0.5;
		if (NodeIndexA<(int)TheNodes.size()-1)
		{
			NodeTimeB=TheNodes[NodeIndexA+1].Time;
			NodeValueB=TheNodes[NodeIndexA+1].Value;
		} else
		{
			//NodeTimeB=g_BenderPitchNodes[g_BenderPitchNodes.size()-1].Time;
			NodeTimeB=1.0;
			NodeValueB=TheNodes[TheNodes.size()-1].Value;
		}
		double interpvalue;
		double timedur=NodeTimeB-NodeTimeA;
		if (timedur<=0)
		{
			timedur=0.01;
		}
		double normalizedtime=TheTime-NodeTimeA;
		interpvalue=NodeValueA+(((NodeValueB-NodeValueA)/timedur)*normalizedtime);
		if (interpvalue<0.0) interpvalue=0.0;
		if (interpvalue>1.0) interpvalue=1.0;
		return interpvalue;

	}
	return 0.5;
}

void PerformPropertyChanges()
{
	int i;
	int j;
	double accumScaler=1.0;

	PreventUIRefresh(1);
	for (i=0;i<(int)g_IItakes.size();i++)
	{
		j=0;
		double itemLen=g_ii_storeditemstates[i].length;
		while (g_IIproperties[j].Name!=NULL)
		{

		if (!g_IIproperties[j].IsTakeProperty)
		{
			double newItemPropValue=0.0;
			//double ItemPos=*(double*)GetSetMediaItemInfo((MediaItem*)GetSetMediaItemTakeInfo(g_IItakes[i],"P_ITEM",0),"D_POSITION",0);
			double ItemPos=g_ii_storeditemstates[i].position;
			ItemPos-=g_MinItemTime;
			double NormItemPos=(1.0/(g_MaxItemTime-g_MinItemTime)*ItemPos);
			double interpValue=GetInterpolatedNodeValue(*g_IIproperties[j].Envelope,NormItemPos);
			double MinValue=g_IIproperties[j].MinValue;
			double MaxValue=g_IIproperties[j].MaxValue;
			if (strcmp(g_IIproperties[j].APIAccessID,"B_UISEL")==0 && g_IIproperties[j].enabled)
			{
				double rndpercent=MinValue+(MaxValue-MinValue)*interpValue;
				double rndval=(1.0/RAND_MAX)*rand();
				bool uisel=false;
				if (rndval<rndpercent)
					uisel=true;
				GetSetMediaItemInfo((MediaItem*)GetSetMediaItemTakeInfo(g_IItakes[i],"P_ITEM",0),"B_UISEL",&uisel);
			}
			if (strcmp(g_IIproperties[j].APIAccessID,"D_FADEINLEN")==0 && g_IIproperties[j].enabled)
			{
				newItemPropValue=MinValue+(MaxValue-MinValue)*interpValue;
				GetSetMediaItemInfo((MediaItem*)GetSetMediaItemTakeInfo(g_IItakes[i],"P_ITEM",0),"D_FADEINLEN",&newItemPropValue);
			}
			if (strcmp(g_IIproperties[j].APIAccessID,"D_FADEOUTLEN")==0 && g_IIproperties[j].enabled)
			{
				newItemPropValue=MinValue+(MaxValue-MinValue)*interpValue;
				GetSetMediaItemInfo((MediaItem*)GetSetMediaItemTakeInfo(g_IItakes[i],"P_ITEM",0),"D_FADEOUTLEN",&newItemPropValue);
			}
			if (strcmp(g_IIproperties[j].APIAccessID,"D_POSITION")==0 && g_IIproperties[j].enabled)
			{
				double ItemPosX=g_ii_storeditemstates[i].position-g_MinItemTime;
				double newItemPropValueF=MinValue+((MaxValue-MinValue)*interpValue);

				ItemPosX=g_MinItemTime+(ItemPosX*newItemPropValueF);
				GetSetMediaItemInfo((MediaItem*)GetSetMediaItemTakeInfo(g_IItakes[i],"P_ITEM",0),"D_POSITION",&ItemPosX);
				accumScaler+=interpValue;
			}
			if (strcmp(g_IIproperties[j].APIAccessID,"F_FREEMODE_Y")==0 && g_IIproperties[j].enabled)
			{
				float newItemPropValueF = (float)(MinValue+((MaxValue-MinValue)*interpValue));
				GetSetMediaItemInfo((MediaItem*)GetSetMediaItemTakeInfo(g_IItakes[i],"P_ITEM",0),"F_FREEMODE_Y",&newItemPropValueF);
			}
			if (strcmp(g_IIproperties[j].APIAccessID,"D_LENGTH")==0 && g_IIproperties[j].enabled)
			{
				double valRange=MaxValue-MinValue;
				newItemPropValue=(MinValue+valRange*interpValue)*itemLen;
				itemLen=newItemPropValue;
				GetSetMediaItemInfo((MediaItem*)GetSetMediaItemTakeInfo(g_IItakes[i],"P_ITEM",0),"D_LENGTH",&newItemPropValue);

			}
			if (strcmp(g_IIproperties[j].APIAccessID,"D_VOL")==0 && g_IIproperties[j].enabled)
			{
				newItemPropValue=MinValue+((MaxValue-MinValue)*interpValue);
				GetSetMediaItemInfo((MediaItem*)GetSetMediaItemTakeInfo(g_IItakes[i],"P_ITEM",0),"D_VOL",&newItemPropValue);
			}
		}
		if (g_IIproperties[j].IsTakeProperty)
		{
			double newItemPropValue=0.0;
			double ItemPos=*(double*)GetSetMediaItemInfo((MediaItem*)GetSetMediaItemTakeInfo(g_IItakes[i],"P_ITEM",0),"D_POSITION",0);
			ItemPos-=g_MinItemTime;
			double NormItemPos=(1.0/(g_MaxItemTime-g_MinItemTime)*ItemPos);
			double interpValue=GetInterpolatedNodeValue(*g_IIproperties[j].Envelope,NormItemPos);
			double MinValue=g_IIproperties[j].MinValue;
			double MaxValue=g_IIproperties[j].MaxValue;
			if (strcmp(g_IIproperties[j].APIAccessID,"D_PITCH")==0 && g_IIproperties[j].enabled)
			{
				newItemPropValue=MinValue+((MaxValue-MinValue)*interpValue);
				//newItemPropValue=pow(2.0,newItemPropValue/12.0);
				GetSetMediaItemTakeInfo(g_IItakes[i],"D_PITCH",&newItemPropValue);
			}
			if (strcmp(g_IIproperties[j].APIAccessID,"D_PAN")==0 && g_IIproperties[j].enabled)
			{
				newItemPropValue=MinValue+((MaxValue-MinValue)*interpValue);
				newItemPropValue=-newItemPropValue;
				//newItemPropValue=pow(2.0,newItemPropValue/12.0);
				GetSetMediaItemTakeInfo(g_IItakes[i],"D_PAN",&newItemPropValue);
			}
			if (strcmp(g_IIproperties[j].APIAccessID,"D_STARTOFFS")==0 && g_IIproperties[j].enabled)
			{
				PCM_source *source=(PCM_source*)GetSetMediaItemTakeInfo(g_IItakes[i],"P_SOURCE",0);
				if (source)
				{
					double takesourceLen=source->GetLength();
					newItemPropValue=0.0+(takesourceLen*interpValue);
					//newItemPropValue=pow(2.0,newItemPropValue/12.0);
					GetSetMediaItemTakeInfo(g_IItakes[i],"D_STARTOFFS",&newItemPropValue);
				}
			}
			if (strcmp(g_IIproperties[j].Name,"Take pitch (resampled)")==0 && g_IIproperties[j].enabled)
			{
				bool Preserve=false;
				GetSetMediaItemTakeInfo(g_IItakes[i],"B_PPITCH",&Preserve);
				newItemPropValue=MinValue+((MaxValue-MinValue)*interpValue);
				newItemPropValue=pow(2.0,newItemPropValue/12.0);
				GetSetMediaItemTakeInfo(g_IItakes[i],"D_PLAYRATE",&newItemPropValue);
				// note : this now presumes item length processing is done before take pitch resampling processing
				newItemPropValue=itemLen*(1.0/newItemPropValue);
				GetSetMediaItemInfo((MediaItem*)GetSetMediaItemTakeInfo(g_IItakes[i],"P_ITEM",0),"D_LENGTH",&newItemPropValue);
			}

		}
		j++;
		}

	}
	PreventUIRefresh(-1);
	UpdateArrange();
}

void StoreOrigProps()
{
	int i;
	g_ii_storeditemstates.clear();
	for (i=0;i<(int)g_IItakes.size();i++)
	{
		t_interpolator_item_state NewStoredState;
		MediaItem *CurItem=(MediaItem*)GetSetMediaItemTakeInfo(g_IItakes[i],"P_ITEM",0);
		if (CurItem)
		{
			NewStoredState.position=*(double*)GetSetMediaItemInfo(CurItem,"D_POSITION",0);
			NewStoredState.itemvol=*(double*)GetSetMediaItemInfo(CurItem,"D_VOL",0);
			NewStoredState.length=*(double*)GetSetMediaItemInfo(CurItem,"D_LENGTH",0);
			NewStoredState.mediaoffset=*(double*)GetSetMediaItemTakeInfo(g_IItakes[i],"D_STARTOFFS",0);
			NewStoredState.playrate=*(double*)GetSetMediaItemTakeInfo(g_IItakes[i],"D_PLAYRATE",0);
			NewStoredState.pitch=*(double*)GetSetMediaItemTakeInfo(g_IItakes[i],"D_PITCH",0);
			NewStoredState.preservepp=*(bool*)GetSetMediaItemTakeInfo(g_IItakes[i],"B_PPITCH",0);
			NewStoredState.takepan=*(double*)GetSetMediaItemTakeInfo(g_IItakes[i],"D_PAN",0);
			NewStoredState.pTake=g_IItakes[i];
			g_ii_storeditemstates.push_back(NewStoredState);
		}
	}
}



void SetListViewSingleSelected(HWND hList,int idx)
{
	HWND hOih=GetDlgItem(g_hIIdlg,IDC_IIACTPARLIST);
	int n=ListView_GetItemCount(hOih);
	for (int i=0;i<n;i++)
		ListView_SetItemState(hOih, i, i == idx ? LVIS_SELECTED : 0,LVIS_SELECTED);


}

WDL_DLGRET ItemInterpDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	static bool FirstRun = true;
	static bool projectPlays=false;
	static HWND hGraph;

	if (ThemeListViewInProc(hwnd, Message, lParam, GetDlgItem(hwnd,IDC_IIACTPARLIST), false))
		return 1;
	if (INT_PTR r = SNM_HookThemeColorsMessage(hwnd, Message, wParam, lParam))
		return r;

	switch (Message)
	{
		case WM_INITDIALOG:
		{
			if (SWS_THEMING) SNM_ThemeListView(GetDlgItem(hwnd,IDC_IIACTPARLIST));

			g_hIIdlg=hwnd;
			g_IItakes.clear();
			XenGetProjectTakes(g_IItakes,true,true);
			StoreOrigProps();
			GetActItemsMinMaxTimes(&g_MinItemTime,&g_MaxItemTime);
			t_interpolator_envelope_node NewNode;
			if (FirstRun)
			{
				for (int i = 0; g_IIproperties[i].Name; i++)
				{
					g_IIproperties[i].Envelope=new t_interpolator_envelope;
					NewNode.Time=0.0;
					NewNode.Value=RealValToNormzdVal(i,g_IIproperties[i].NeutralValue);
					g_IIproperties[i].Envelope->push_back(NewNode);
					g_IIproperties[i].RndSpreadEnv=new t_interpolator_envelope;
					NewNode.Time=0.0;
					NewNode.Value=0.0;
					g_IIproperties[i].RndSpreadEnv->push_back(NewNode);
					NewNode.Time=1.0;
					NewNode.Value=RealValToNormzdVal(i,g_IIproperties[i].NeutralValue);
					g_IIproperties[i].Envelope->push_back(NewNode);
					NewNode.Time=1.0;
					NewNode.Value=0.0;
					g_IIproperties[i].RndSpreadEnv->push_back(NewNode);
				}
				FirstRun=false;
			}

			HWND hList = GetDlgItem(hwnd,IDC_IIACTPARLIST);
			ListView_SetExtendedListViewStyleEx(hList, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);
#ifdef _WIN32
			WDL_UTF8_HookListView(hList);
#endif
			LVCOLUMN col;
			col.mask=LVCF_TEXT|LVCF_WIDTH;
			col.cx=15;
			col.pszText=(char*)"";
			ListView_InsertColumn(GetDlgItem(hwnd,IDC_IIACTPARLIST), 0 ,&col);
			col.cx=140;
			col.pszText=(char*)"Property";
			ListView_InsertColumn(GetDlgItem(hwnd,IDC_IIACTPARLIST), 1 ,&col);
			for (int i = 0; g_IIproperties[i].Name; i++)
			{
				LVITEM item;
				item.mask = LVIF_TEXT;
				item.iItem = i;
				item.iSubItem = 0;
				item.pszText = (char*)(g_IIproperties[i].enabled ? UTF8_BULLET : "");
				ListView_InsertItem(GetDlgItem(hwnd,IDC_IIACTPARLIST),&item);
				item.iSubItem = 1;
				item.pszText=(char*)(g_IIproperties[i].Name);
				ListView_SetItem(GetDlgItem(hwnd,IDC_IIACTPARLIST),&item);
			}

			RECT r;
			GetWindowRect(GetDlgItem(hwnd,IDC_IIENVAREA), &r);
			g_framebuffer = new LICE_SysBitmap(r.right-r.left, abs(r.bottom-r.top));

#ifdef _WIN32
			hGraph = GetDlgItem(hwnd,IDC_IIENVAREA);
			g_OldGraphAreaWndProc = (WNDPROC)SetWindowLongPtr(hGraph, GWLP_WNDPROC, (LONG_PTR)EnveAreaWndProc);
#else
			hGraph = CreateDialog(0, 0, hwnd, EnveAreaWndProc);
			SetOpaque(hGraph, true);
			ScreenToClient(hwnd, (LPPOINT)&r);
			ScreenToClient(hwnd, ((LPPOINT)&r)+1);
			SetWindowPos(hGraph, HWND_TOP, r.left, r.top < r.bottom ? r.top : r.bottom, r.right-r.left, abs(r.bottom-r.top), 0);
			ShowWindow(hGraph, SW_SHOW);
#endif
			InvalidateRect(hwnd,NULL,TRUE);
			SetListViewSingleSelected(GetDlgItem(hwnd,IDC_IIACTPARLIST),g_activePropertyEnvelope);
			break;
		}
		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDC_IIPLAYPROJ:
					if (projectPlays)
					{
						projectPlays=false;
						Main_OnCommand(1016,0); // stop project play
						SetEditCurPos(g_MinItemTime,false,false);
						SetWindowText(GetDlgItem(hwnd,IDC_IIPLAYPROJ),"Play");
					}
					else
					{
						RecallOrigProps();
						PerformPropertyChanges();
						projectPlays=true;
						SetWindowText(GetDlgItem(hwnd,IDC_IIPLAYPROJ),"Stop");
						SetEditCurPos(g_MinItemTime,false,false);
						Main_OnCommand(1007,0);
					}
					break;
				case IDOK:
					PerformPropertyChanges();
					Undo_OnStateChangeEx("Interpolate item properties",4,-1);
					EndDialog(hwnd,0);
					break;
				case IDCANCEL:
					Main_OnCommand(1016,0); // stop project play
					RecallOrigProps();
					UpdateTimeline();
					projectPlays=false;
					SetEditCurPos(g_MinItemTime,false,false);
					EndDialog(hwnd,0);
					break;
				case IDC_IIPRESETALL:
					for (int i = 0; g_IIproperties[i].Name; i++)
					{
						t_interpolator_envelope *TargetEnv=g_IIproperties[i].Envelope;
						TargetEnv->clear();
						t_interpolator_envelope_node NewNode;
						NewNode.Time=0.0;
						NewNode.Value=RealValToNormzdVal(i,g_IIproperties[i].NeutralValue);
						g_IIproperties[i].Envelope->push_back(NewNode);
						NewNode.Time=1.0;
						NewNode.Value=RealValToNormzdVal(i,g_IIproperties[i].NeutralValue);
						g_IIproperties[i].Envelope->push_back(NewNode);
					}
					RecallOrigProps();
					PerformPropertyChanges();
					InvalidateRect(hwnd,NULL,FALSE);
					break;
				case IDC_IIRESETENV:
				{
					t_interpolator_envelope* TargetEnv=g_IIproperties[g_activePropertyEnvelope].Envelope;
					TargetEnv->clear();
					t_interpolator_envelope_node NewNode;
					NewNode.Time=0.0;
					NewNode.Value=RealValToNormzdVal(g_activePropertyEnvelope,g_IIproperties[g_activePropertyEnvelope].NeutralValue);
					g_IIproperties[g_activePropertyEnvelope].Envelope->push_back(NewNode);
					NewNode.Time=1.0;
					NewNode.Value=RealValToNormzdVal(g_activePropertyEnvelope,g_IIproperties[g_activePropertyEnvelope].NeutralValue);
					g_IIproperties[g_activePropertyEnvelope].Envelope->push_back(NewNode);
					RecallOrigProps();
					PerformPropertyChanges();
					InvalidateRect(hwnd,NULL,FALSE);
					break;
				}
			}
		case WM_NOTIFY:
			if (wParam==IDC_IIACTPARLIST)
			{
				NMLISTVIEW *nm = (NMLISTVIEW*)lParam;
				if (nm->hdr.code == LVN_ITEMCHANGED)
				{
					if (ListView_GetSelectedCount(GetDlgItem(hwnd,IDC_IIACTPARLIST))==1)
					{
						for (int i=0;i<ListView_GetItemCount(GetDlgItem(hwnd,IDC_IIACTPARLIST));i++)
						{
							UINT itemState=ListView_GetItemState(GetDlgItem(hwnd,IDC_IIACTPARLIST),i,LVIS_SELECTED);
							if (itemState==LVIS_SELECTED)
							{
								g_activePropertyEnvelope=i;
								InvalidateRect(hwnd,NULL,FALSE);
							}
						}
					}
				}
				else if (nm->hdr.code == NM_CLICK && nm->iItem >=0 && nm->iSubItem == 0)
				{
					g_IIproperties[nm->iItem].enabled = !g_IIproperties[nm->iItem].enabled;
					ListView_SetItemText(GetDlgItem(g_hIIdlg, IDC_IIACTPARLIST), nm->iItem, 0, g_IIproperties[nm->iItem].enabled ? UTF8_BULLET : "");
				}
			}
			break;
		case WM_DESTROY:
			delete g_framebuffer;
			g_hIIdlg = NULL;
			break;
	}
	return 0;
}

void DoShowItemInterpDLG(COMMAND_T*)
{
	if (CountSelectedMediaItems(NULL) < 2)
		MessageBox(g_hwndParent, "You must have 2 or more items selected.", "Item Interpolator Error", MB_OK);
	else
		DialogBox(g_hInst,MAKEINTRESOURCE(IDD_ITEMPROPINTERP),g_hwndParent,(DLGPROC)ItemInterpDlgProc);
}
