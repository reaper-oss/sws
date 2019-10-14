/******************************************************************************
/ Envelope_actions.cpp
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
#include "../Breeder/BR_EnvelopeUtil.h"

using namespace std;

typedef struct
{
	double time;
	double value;
	int shape;
	int selected;
	int mystery;
} t_reap_envnode;

class CEnvelopeStateHandler
{
public:
	CEnvelopeStateHandler()
	{
		m_lp=new LineParser(false);
		m_newstateExists=false;
	}
	~CEnvelopeStateHandler()
	{
		if (m_lp) delete m_lp;
	}
	void ParseState(const char *rppstate)
	{
		m_newstateExists=false;
		m_origenvstate.clear();
		m_envnodes.clear();

		char rpplinebuf[4096];
		stringstream ss;
		ss << rppstate;

		LineParser wdlp(false);
		bool ended=false;
		while (!ended)
		{
			//break;
			ss.getline(rpplinebuf,4096);
			if (ss.eof()) break;
			m_lp->parse(rpplinebuf);
			if (strcmp(m_lp->gettoken_str(0),"PT")==0)
			{
				t_reap_envnode newnode;
				newnode.time=m_lp->gettoken_float(1);
				newnode.value=m_lp->gettoken_float(2);
				newnode.shape=m_lp->gettoken_int(3);
				newnode.selected=(m_lp->gettoken_int(5)&1)==1;
				newnode.mystery=m_lp->gettoken_int(4);
				m_envnodes.push_back(newnode);
			} else
			{
				// not an env point line, add it to our nasty global string vector
				// except if it's the end line of the env text block
				if (strcmp(m_lp->gettoken_str(0),">"))
					m_origenvstate.push_back(rpplinebuf);
			}

		}
	}
	void UpdateToCurrentEnvelope()
	{
		ReaProject *pproj=EnumProjects(-1,0,0);
		if (pproj)
		{
			void *pcurenv= GetSelectedTrackEnvelope(pproj);
			if (pcurenv)
			{
				if (const char* buf = SWS_GetSetObjectState(pcurenv,0))
				{
					this->ParseState(buf);
					SWS_FreeHeapPtr(buf);
				};
			}
		}
	}
	void GenerateNewState()
	{
		//stringstream newenvstate;
		//m_newenvstate.str().clear();
		m_newenvstate.str("");
		int i;
		for (i=0;i<(int)m_origenvstate.size();i++)
			m_newenvstate << m_origenvstate[i] << endl;
		for (i=0;i<(int)m_envnodes.size();i++)
		{
			m_newenvstate << "PT " << m_envnodes[i].time << " " << m_envnodes[i].value << " " << m_envnodes[i].shape <<
				" " << m_envnodes[i].mystery << " " << m_envnodes[i].selected << endl;
		}
		m_newenvstate << ">" << endl;
		m_newstateExists=true;
	}
	void UpdateEnvToNewState()
	{
		GenerateNewState();
		if (m_newstateExists)
		{
			void *pcurenv= GetSelectedTrackEnvelope(EnumProjects(-1,0,0));
			if (pcurenv)
			{
				WDL_FastString str; // Prob should convert m_newenvstate to use WDL_FastString
				str.Set(m_newenvstate.str().c_str());
				SWS_GetSetObjectState(pcurenv, &str);
			}
		}
	}
	void EnvTransform_ShiftInTime(double amount_s)
	{
		this->UpdateToCurrentEnvelope();
		if (m_envnodes.size()>0)
		{
			int i;
			for (i=0;i<(int)m_envnodes.size();i++)
			{
				m_envnodes[i].time+=amount_s;
			}
			//if (m_newstateExists)
			UpdateEnvToNewState();
		}

	}
	void EnvTransform_ScaleInTime(double scalingfactor)
	{
		this->UpdateToCurrentEnvelope();
		if (m_envnodes.size()>0)
		{

		}

	}
	vector<t_reap_envnode> m_envnodes; // parsed envelope nodes in a convenient vector of structs
private:
	stringstream m_newenvstate;
	vector<string> m_origenvstate; // the non-point envelope state strings

	LineParser *m_lp;
	bool m_newstateExists;
};

CEnvelopeStateHandler g_EnvelopeHandler;

void DoShiftEnvelope(COMMAND_T* ct)
{
	BR_Envelope envelope(GetSelectedEnvelope(NULL));
	double amount = (double)ct->user;

	for (int i = 0; i < envelope.CountPoints(); ++i)
	{
		double position;
		envelope.GetPoint(i, &position, NULL, NULL, NULL);
		position += amount;
		envelope.SetPoint(i, &position, NULL, NULL, NULL);
	}
	if (envelope.Commit())
		Undo_OnStateChangeEx(SWS_CMD_SHORTNAME(ct),UNDO_STATE_TRACKCFG,-1);
}
