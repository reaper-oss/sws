/******************************************************************************
/ Autorender.cpp
/
/ Copyright (c) 2011 Shane StClair
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
#include <time.h>
#include "../../WDL/projectcontext.h"

#define TAGLIB_STATIC
#define TAGLIB_NO_CONFIG
#include "../../taglib/include/tag.h"
#include "../../taglib/include/fileref.h"

#define TESTCODE

// START OF TEST CODE

#ifdef TESTCODE
#include "Prompt.h"
#pragma message("TESTCODE Defined")

void PrintDebugString( string debugStr )
{
	if( strcmp( debugStr.substr( debugStr.length() ).c_str(), "\n") != 0 ){
		debugStr += "\n";
	}

	#ifdef _WIN32
		OutputDebugString( debugStr.c_str() );
	#else
		printf( debugStr.c_str() );
	#endif
}

void TestFunc(COMMAND_T* = NULL)
{
	PrintDebugString("Yeah, debug printing works.");
}

void PrintGuids(COMMAND_T* = NULL)
{
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		char debugStr[256];
		char guidStr[64];
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		guidToString((GUID*)GetSetMediaTrackInfo(tr, "GUID", NULL), guidStr);
		sprintf(debugStr, "Track %d %s GUID %s\n", i, (char*)GetSetMediaTrackInfo(tr, "P_NAME", NULL), guidStr);
#ifdef _WIN32
		OutputDebugString(debugStr);
#else
		printf(debugStr);
#endif
	}
}

#endif

// END OF TEST CODE

int GetCurrentYear(){
	time_t t = 0;
	struct tm *lt = NULL;  
	t = time(NULL);
	lt = localtime( &t );
	int curYear = lt->tm_year;
	curYear = curYear + 1900;
	return curYear;
}

string ZeroPadNumber(int num, int digits ){
    std::ostringstream ss;
    ss << setw( digits ) << setfill( '0' ) << num;
    return ss.str();
}

string ParseFileExtension( string path ){
    if( path.find_last_of(".") != string::npos )
        return path.substr( path.find_last_of(".") + 1 );
    return "";
}

class RenderTrack {
	public:
		int trackNumber;
		//double trackStartTime, trackEndTime;
		string trackName;
		string getFileName( string );
		string getPaddedTrackNumber();
};

string RenderTrack::getPaddedTrackNumber(){
	return ZeroPadNumber( trackNumber, 2 );
}

string RenderTrack::getFileName( string ext = "" ){
	string fileName = getPaddedTrackNumber() + " " + trackName;
	if( !ext.empty() ){
		fileName += "." + ext;
	}
	return fileName;
}

WDL_String* GetProjectString(){
    WDL_String* prjStr = new WDL_String("");

	char str[4096];
	EnumProjects(-1, str, 256);	

	ProjectStateContext* prj;
	prj = ProjectCreateFileRead( str );

	if( !prj ){
		//TODO: Throw error
		return prjStr;
	}

    int iLen = 0;
    while( !prj->GetLine( str, 4096 ) ){
		prjStr->Append( str, prjStr->GetLength() + 4098 );
		prjStr->Append( "\n", prjStr->GetLength() + 2 );
    }
    delete prj;

	return prjStr;
}

void WriteProjectFile( string filename, WDL_String* prjStr ){

	ProjectStateContext* outProject = ProjectCreateFileWrite( filename.c_str() );	
	char line[4096];
	int pos = 0;	
	while( GetChunkLine( prjStr->Get(), line, 4096, &pos, false ) ){
		outProject->AddLine( line );
	}
    delete outProject;
}

WDL_HeapBuf GetHeapBufFromChar( const char *str ){
	WDL_HeapBuf m_hb;

    int s = strlen( str );
    char *newbuf = ( char* ) m_hb.Resize( s + 1, false );
    if (newbuf){
		memcpy( newbuf, str, s);
		newbuf[s] = 0;
    }

	return m_hb;
}

void SetProjectParameter( WDL_String *prjStr, string param, string paramValue ){
	char line[4096];
	int pos = 0;
	LineParser lp(false);

	while( GetChunkLine( prjStr->Get(), line, 4096, &pos, false ) ){
		if( !lp.parse( line ) && lp.getnumtokens() ) {		
			if ( strcmp( lp.gettoken_str(0), param.c_str() ) == 0) {
				
				int lineLen = strlen( line ) + 1; //Add 1 for the omitted newline
				int startPos = pos - lineLen;

				string replacementStr = param + string( " " ) + paramValue + string( "\n" );
				WDL_String replacement = replacementStr.c_str();
	
				prjStr->DeleteSub( startPos, lineLen );
				prjStr->Insert( replacement.Get(), startPos );

				break;
			}
		}
	}	
}

string GetProjectParameterValueStr( WDL_String *prjStr, string param, int token = 1 ){
	char line[4096];
	int pos = 0;
	LineParser lp(false);

	while( GetChunkLine( prjStr->Get(), line, 4096, &pos, false ) ){
		if( !lp.parse( line ) && lp.getnumtokens() ) {		
			if ( strcmp( lp.gettoken_str(0), param.c_str() ) == 0) {
				return lp.gettoken_str( token );
			}
		}
	}

	return "";
}

string GetProjectNotesParameter( WDL_String *prjStr, string param ){
	char line[4096];
	int pos = 0;
	LineParser lp(false);
	param = "|" + param; //Project note lines have a | prefix

	bool inProjectNotes = false;

	while( GetChunkLine( prjStr->Get(), line, 4096, &pos, false ) ){
		if( !lp.parse( line ) && lp.getnumtokens() ) {	
			if( inProjectNotes ){
				if ( strcmp( lp.gettoken_str(0), ">" ) == 0) {
					//end of project notes
					return "";
				} else if ( strcmp( lp.gettoken_str(0), param.c_str() ) == 0 ){
					string paramVal = "";
					for( int i = 1; i <= lp.getnumtokens(); i++ ){
						if( i > 1 ) paramVal += " ";
						paramVal += lp.gettoken_str( i );
					}

					return paramVal;
				}				
			} else if ( strcmp( lp.gettoken_str(0), "<NOTES" ) == 0) {
				inProjectNotes = true;
				continue;
			}
		}
	}

	return "";
}

string GetQueuedRendersDir(){
	ostringstream qrStream;
	qrStream << GetResourcePath() << PATH_SLASH_CHAR << "QueuedRenders";
	return qrStream.str();
}

string GetCurrentRenderExtension( WDL_String *prjStr ){
	return ParseFileExtension( GetProjectParameterValueStr( prjStr, "RENDER_FILE" ) );
}

void AutorenderTest(COMMAND_T*) {
	int project_index = 0, marker_index = 0, track_index = 0, idx;
	bool isrgn;
	double pos, rgnend;
	char* track_name;


	WDL_String* prjStr = GetProjectString();	
	
	string tag_artist = GetProjectNotesParameter( prjStr, "TAG_ARTIST" );
	string tag_album = GetProjectNotesParameter( prjStr, "TAG_ALBUM" );
	string tag_genre = GetProjectNotesParameter( prjStr, "TAG_GENRE" );
	string tag_year_str = GetProjectNotesParameter( prjStr, "TAG_YEAR" );
	string tag_comment = GetProjectNotesParameter( prjStr, "TAG_COMMENT" );

	unsigned int tag_year = 0;
	if( tag_year_str.empty() ){
		tag_year = GetCurrentYear();
	} else {
		tag_year = atoi( tag_year_str.c_str() );
	}

	string queuedRendersDir = GetQueuedRendersDir();
	string renderFileExtension = GetCurrentRenderExtension( prjStr );
	string renderPath = "C:\\test\\";

	ostringstream outRenderProjectPrefixStream;
	outRenderProjectPrefixStream << queuedRendersDir << PATH_SLASH_CHAR << "render_Autorender";
	string outRenderProjectPrefix = outRenderProjectPrefixStream.str();

	vector<RenderTrack> renderTracks;

	while( EnumProjectMarkers( marker_index++, &isrgn, &pos, &rgnend, &track_name, &idx) > 0 ){
		if( isrgn == true && idx > track_index ){
			track_index = idx;
			WDL_String trackPrjStr = prjStr->Get();

			RenderTrack renderTrack;
			renderTrack.trackNumber = track_index;

			ostringstream trackName;
			if( strlen( track_name ) > 0 ){
				trackName << track_name;
			} else {
				trackName << "Track " << renderTrack.getPaddedTrackNumber();
			}
			renderTrack.trackName = trackName.str();

			string outRenderProjectPath = outRenderProjectPrefix + renderTrack.getFileName("rpp");
			string renderFilePath = renderPath + renderTrack.getFileName( renderFileExtension );						
			SetProjectParameter( &trackPrjStr, "RENDER_FILE", "\"" + renderFilePath + "\"" );	

			ostringstream renderRange;
			renderRange << "0 " << pos << " " << rgnend;
			SetProjectParameter( &trackPrjStr, "RENDER_RANGE", renderRange.str() );	
			
			WriteProjectFile( outRenderProjectPath, &trackPrjStr );
			renderTracks.push_back( renderTrack );
		}
	}
	
	Main_OnCommand( 41207, 0 ); //Render all queued renders

	// Tag!
	for( unsigned int i = 0; i < renderTracks.size(); i++){		

		TagLib::FileRef f( renderedFilePath.c_str() );
		
		if( !tag_artist.empty() ) f.tag()->setArtist( tag_artist );
		if( !tag_album.empty() ) f.tag()->setAlbum( tag_album );
		if( !tag_genre.empty() ) f.tag()->setGenre( tag_genre );
		if( tag_year > 0 ) f.tag()->setYear( tag_year );
		if( !tag_comment.empty() ) f.tag()->setComment( tag_comment );
		
		f.tag()->setTrack( i + 1 );
		f.tag()->setTitle( renderTracks[i].trackName );
		f.save();
	}

	system( ( "explorer " + renderPath ).c_str() );
}

static project_config_extension_t g_projectconfig = { NULL };

static COMMAND_T g_commandTable[] = 
{
	{ { DEFACCEL, "Autorender: Test" },	"AUTORENDER_TEST", AutorenderTest, "Autorender: Run Test" },
#ifdef TESTCODE
	{ { DEFACCEL, "Autorender: TestCode" }, "AUTORENDER_TESTCODE",  TestFunc, "Autorender: TestCode" },
	{ { DEFACCEL, "SWS: Print track GUIDs" }, "SWS_PRINTGUIDS",  PrintGuids, "Autorender: Print Guids" },
#endif
	{ {}, LAST_COMMAND, }, // Denote end of table
};

static void menuhook(const char* menustr, HMENU hMenu, int flag)
{
	if ( strcmp(menustr, "Main extensions") == 0 && flag == 0 ){
		int len = 1;
#ifdef TESTCODE
		len = len + 2;
#endif
	
		for( int i = 0; i < len; i++ ){
			AddToMenu(hMenu, g_commandTable[i].menuText, g_commandTable[i].accel.accel.cmd);
		}
	}
}

int AutorenderInit()
{
	SWSRegisterCommands(g_commandTable);

	if (!plugin_register("projectconfig",&g_projectconfig))
		return 0;

    if (!plugin_register("hookcustommenu", (void*)menuhook))
        return 0;

	return 1;
}