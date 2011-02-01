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

#include "RenderTrack.h"

#include "../../WDL/projectcontext.h"

#define TAGLIB_STATIC
#define TAGLIB_NO_CONFIG
#include "../taglib/tag.h"
#include "../taglib/fileref.h"

int GetCurrentYear(){
	time_t t = 0;
	struct tm *lt = NULL;  
	t = time(NULL);
	lt = localtime( &t );
	int curYear = lt->tm_year;
	curYear = curYear + 1900;
	return curYear;
}

// Globals
string g_render_path;
string g_tag_artist;
string g_tag_album;
string g_tag_genre;
int g_tag_year = GetCurrentYear();
string g_tag_comment;

#define CONSOLE_WINDOWPOS_KEY "ReaConsoleWindowPos"

//#define TESTCODE

// START OF TEST CODE

#ifdef TESTCODE
#include "Prompt.h"
#pragma message("TESTCODE Defined")

void PrintDebugString( string debugStr ){
	if( strcmp( debugStr.substr( debugStr.length() ).c_str(), "\n") != 0 ){
		debugStr += "\n";
	}

	#ifdef _WIN32
		OutputDebugString( debugStr.c_str() );
	#else
		printf( debugStr.c_str() );
	#endif
}

void TestFunction(COMMAND_T* = NULL){
	PrintDebugString("Yeah, debug printing works.");
}

#endif

// END OF TEST CODE

void TrimString( string &str ){
    // Trim Both leading and trailing spaces
    size_t startpos = str.find_first_not_of(" \t"); // Find the first character position after excluding leading blank spaces
    size_t endpos = str.find_last_not_of(" \t"); // Find the first character position from reverse af
 
    // if all spaces or empty return an empty string
    if(( string::npos == startpos ) || ( string::npos == endpos)){
        str = "";
    } else {
        str = str.substr( startpos, endpos-startpos+1 );
	}
}


string ParseFileExtension( string path ){
    if( path.find_last_of(".") != string::npos )
        return path.substr( path.find_last_of(".") + 1 );
    return "";
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
/*
WDL_HeapBuf GetHeapBufFromChar( const char *str ){
	WDL_HeapBuf m_hb;

    int s = (int)strlen( str );
    char *newbuf = ( char* ) m_hb.Resize( s + 1, false );
    if (newbuf){
		memcpy( newbuf, str, s);
		newbuf[s] = 0;
    }

	return m_hb;
}
*/

void SetProjectParameter( WDL_String *prjStr, string param, string paramValue ){
	char line[4096];
	int pos = 0;
	LineParser lp(false);

	while( GetChunkLine( prjStr->Get(), line, 4096, &pos, false ) ){
		if( !lp.parse( line ) && lp.getnumtokens() ) {		
			if ( strcmp( lp.gettoken_str(0), param.c_str() ) == 0) {
				
				int lineLen = (int)strlen( line ) + 1; //Add 1 for the omitted newline
				int startPos = pos - lineLen;

				string replacementStr = param + string( " " ) + paramValue + string( "\n" );
				WDL_String replacement;
				replacement.Set(replacementStr.c_str());
	
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

/*
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
					TrimString( paramVal );
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
*/

string GetQueuedRendersDir(){
	ostringstream qrStream;
	qrStream << GetResourcePath() << PATH_SLASH_CHAR << "QueuedRenders";
	return qrStream.str();
}

string GetCurrentRenderExtension( WDL_String *prjStr ){
	return ParseFileExtension( GetProjectParameterValueStr( prjStr, "RENDER_FILE" ) );
}

void ReplaceChars( string *str, const char *illegalChars, const char *replaceChar = "" ){
	const char *strChar = str->c_str();

	for( int i = 0; i < (int)strlen( strChar ); i++ ){
		for( int j = 0; j < (int)strlen( illegalChars ); j++ ){
			if( strChar[i] == illegalChars[j] ){
				str->replace( i, 1, replaceChar);
				break;
			}
		}
	}
}

void RemoveQuotes( string *str ){
	const char *stripChar = "\"";
	ReplaceChars( str, stripChar );
}

void SanitizeFilename( string *fn ){

#ifdef _WIN32
	//Windows illegal chars
	const char *illegalChars = "\\/<>|\":?*";
#else
	// TODO: Mac illegal chars
	const char *illegalChars = ":";
	// First char can't be a dot
	// Don't need now because track number is always a prefix, but might if
	// we allow an option to omit them
	//if( fn->substr(0,1) == "." ) fn->replace(0,1,"_");
#endif

	ReplaceChars( fn, illegalChars, "_" );
}


/*
void ShowAutorenderHelp(COMMAND_T*) {
	string helpText = "No need for instructions right now\n\n";
	helpText += "Maybe later.\n";
	MessageBox( GetMainHwnd(), helpText.c_str(), "Autorender Usage", MB_OK );
}
*/

void EnsureStrEndsWith( string &str, const char endChar ){
	if( str.c_str()[ str.length() - 1 ] != endChar ){
		str += endChar;
	}
}

void EnsureStrDoesntEndWith( string &str, const char endChar ){
	while( str.c_str()[ str.length() - 1 ] == endChar ){
		str.erase( str.length() - 1 );
	}
}

bool BrowseForRenderPath( char *renderPathChar ){
	bool result;
	result = BrowseForDirectory( "Select render output directory", NULL, renderPathChar, 1024 );
	return result;
}

wchar_t* WideCharPlz( const char* inChar ){		
	DWORD dwNum = MultiByteToWideChar(CP_UTF8, 0, inChar, -1, NULL, 0);
	wchar_t *wChar;
	wChar = new wchar_t[ dwNum ];
	MultiByteToWideChar(CP_UTF8, 0, inChar, -1, wChar, dwNum );
	return wChar;
}

void AutorenderRegions(COMMAND_T*) {
	Main_OnCommand( 40026, 0 ); //Save current project

	//Get the project config as a WDL_String
	WDL_String* prjStr = GetProjectString();	

	string renderFileExtension = GetCurrentRenderExtension( prjStr );
	if( renderFileExtension.empty() ){
		//have to have a renderFileExtension, show error and exit
		MessageBox( GetMainHwnd(), "Couldn't get render extension. Manually render a dummy file with the desired settings and run again.", "Autorender: Render Extension Error", MB_OK );
		return;
	}

	// remove PATH_SLASH_CHAR from end of string if it exists
	EnsureStrDoesntEndWith( g_render_path, PATH_SLASH_CHAR );

	// render path was specified and doesn't exist
	if( !g_render_path.empty() && !FileExists( g_render_path.c_str() ) ){
		string message = "Render path " + g_render_path + " doesn't exist!";
		g_render_path.clear();
		MessageBox( GetMainHwnd(), message.c_str(), "Autorender: Bad Render Path", MB_OK );
	}

	while( !FileExists( g_render_path.c_str() ) ){
		char renderPathChar[1024];
		bool browseResult;
		browseResult = BrowseForRenderPath(renderPathChar);
		if( !browseResult ){
			return;
		}
		g_render_path = renderPathChar;
	}

	// append PATH_SLASH_CHAR from end of string if it doens't exist
	EnsureStrEndsWith( g_render_path, PATH_SLASH_CHAR );

	string queuedRendersDir = GetQueuedRendersDir();
	// Would be good to delete all files in the render queue directory here and at the end, 
	// but apparently deleting files is not so easy...

	ostringstream outRenderProjectPrefixStream;
	outRenderProjectPrefixStream << queuedRendersDir << PATH_SLASH_CHAR << "render_Autorender";
	string outRenderProjectPrefix = outRenderProjectPrefixStream.str();
	
	//init the stuff we need for the region loop
	int marker_index = 0, track_index = 0, idx;
	bool isrgn;
	double pos, rgnend;
	char* track_name;
	vector<RenderTrack> renderTracks;
	map<int,bool> foundIdx;

	//Loop through regions, build render rpps, build RenderTrack vector
	while( EnumProjectMarkers( marker_index++, &isrgn, &pos, &rgnend, &track_name, &idx) > 0 ){
		//if( isrgn == true && idx > track_index ){
		if( isrgn == true && foundIdx.find( idx ) == foundIdx.end() ){
			foundIdx[ idx ] = true;

			track_index++;
			WDL_String trackPrjStr(prjStr);

			RenderTrack renderTrack;
			renderTrack.trackNumber = track_index;

			ostringstream trackName;
			if( strlen( track_name ) > 0 ){
				trackName << track_name;
			} else {
				trackName << "Track " << renderTrack.getPaddedTrackNumber();
			}
			renderTrack.trackName = trackName.str();
			renderTrack.sanitizedTrackName = trackName.str();
			SanitizeFilename( &renderTrack.sanitizedTrackName );
			
			string outRenderProjectPath = outRenderProjectPrefix + renderTrack.getFileName("rpp");
			string renderFilePath = g_render_path + renderTrack.getFileName( renderFileExtension );						
			SetProjectParameter( &trackPrjStr, "RENDER_FILE", "\"" + renderFilePath + "\"" );	

			ostringstream renderRange;
			renderRange << "0 " << pos << " " << rgnend;
			SetProjectParameter( &trackPrjStr, "RENDER_RANGE", renderRange.str() );	
			
			WriteProjectFile( outRenderProjectPath, &trackPrjStr );
			renderTracks.push_back( renderTrack );
		}
	}
	
	if( renderTracks.size() == 0 ){
		//error, or prompt user for entire project rendering with tagging
#ifdef _WIN32
	MessageBox( GetMainHwnd(), "No regions found", "Autorender Error", MB_OK );
#else
	//Mac message?
#endif
		return;
	}

	Main_OnCommand( 41207, 0 ); //Render all queued renders

	// Tag!
	for( unsigned int i = 0; i < renderTracks.size(); i++){		
		string renderedFilePath = g_render_path + renderTracks[i].getFileName( renderFileExtension );						
		wchar_t* w_rendered_path = WideCharPlz( renderedFilePath.c_str() );

		TagLib::FileRef f( w_rendered_path );
		if( !f.isNull() ){			
			wchar_t* w_tag_artist = WideCharPlz( g_tag_artist.c_str() );
			wchar_t* w_tag_album = WideCharPlz( g_tag_album.c_str() );
			wchar_t* w_tag_genre = WideCharPlz( g_tag_genre.c_str() );
			wchar_t* w_tag_comment = WideCharPlz( g_tag_comment.c_str() );
			wchar_t* w_track_title = WideCharPlz( renderTracks[i].trackName.c_str() );

			if( wcslen( w_tag_artist ) ) f.tag()->setArtist( w_tag_artist );
			if( wcslen( w_tag_album ) ) f.tag()->setAlbum( w_tag_album );
			if( wcslen( w_tag_genre ) ) f.tag()->setGenre( w_tag_genre );
			if( g_tag_year > 0 ) f.tag()->setYear( g_tag_year );
			if( wcslen( w_tag_comment ) ) f.tag()->setComment( w_tag_comment );

			f.tag()->setTrack( i + 1 );
			f.tag()->setTitle( w_track_title );
			
			f.save();
		} else {
			//throw error?
		}
	}

#ifdef _WIN32
	system( ( "explorer " + g_render_path ).c_str() );
#else
	//Mac open render directory?
#endif

}


void processDialogFieldStr( HWND hwndDlg, WPARAM wParam, string &target, bool &hasChanged ){
	char dlg_field[512];
	GetDlgItemText(hwndDlg, (int)wParam, dlg_field, 512);
	string dlg_field_str = dlg_field;
	RemoveQuotes( &dlg_field_str );
	if( dlg_field_str.compare( target ) ){
		target = dlg_field_str;
		hasChanged = true;
	}
}

void processDialogFieldInt( HWND hwndDlg, WPARAM wParam, int &target, bool &hasChanged ){
	int dlg_field;
	dlg_field = GetDlgItemInt(hwndDlg, (int)wParam, NULL, false);
	if( dlg_field != target ){
		target = dlg_field;
		hasChanged = true;
	}
}

INT_PTR WINAPI doAutorenderMetadata(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam){
	bool hasChanged = false;

    switch (uMsg){
            case WM_INITDIALOG:
				RestoreWindowPos(hwndDlg, CONSOLE_WINDOWPOS_KEY, false);
				SetDlgItemText(hwndDlg, IDC_ARTIST, g_tag_artist.c_str() );
				SetDlgItemText(hwndDlg, IDC_ALBUM, g_tag_album.c_str() );
				SetDlgItemText(hwndDlg, IDC_GENRE, g_tag_genre.c_str() );
				SetDlgItemInt(hwndDlg, IDC_YEAR, g_tag_year, false );
				SetDlgItemText(hwndDlg, IDC_COMMENT, g_tag_comment.c_str() );
				SetDlgItemText(hwndDlg, IDC_RENDER_PATH, g_render_path.c_str() );

				return 0;
            case WM_COMMAND:
				switch (LOWORD(wParam)){
					case IDBROWSE:
						char renderPathChar[1024];
						bool browseResult;
						browseResult = BrowseForRenderPath(renderPathChar);
						if( browseResult ){
							SetDlgItemText(hwndDlg, IDC_RENDER_PATH, renderPathChar );
						}
						break;
                    case IDOK:
						processDialogFieldStr( hwndDlg, IDC_ARTIST, g_tag_artist, hasChanged );
						processDialogFieldStr( hwndDlg, IDC_ALBUM, g_tag_album, hasChanged );
						processDialogFieldStr( hwndDlg, IDC_GENRE, g_tag_genre, hasChanged );
						processDialogFieldInt( hwndDlg, IDC_YEAR, g_tag_year, hasChanged );
						processDialogFieldStr( hwndDlg, IDC_COMMENT, g_tag_comment, hasChanged );
						processDialogFieldStr( hwndDlg, IDC_RENDER_PATH, g_render_path, hasChanged );
						EnsureStrEndsWith( g_render_path, PATH_SLASH_CHAR );

						if( hasChanged ){
							Undo_OnStateChangeEx("Set autorender metadata", UNDO_STATE_MISCCFG, -1);
						}
                        // fall through!								
                    case IDCANCEL:							
                        SaveWindowPos(hwndDlg, CONSOLE_WINDOWPOS_KEY);
                        EndDialog(hwndDlg,0);
                        break;
				}
				return 0;
            case WM_DESTROY:
                // We're done
                return 0;
    }
    return 0;
}

void ShowAutorenderMetadata(COMMAND_T*){
	DialogBox( g_hInst, MAKEINTRESOURCE(IDD_AUTORENDER_METADATA), g_hwndParent, doAutorenderMetadata );
}


static bool ProcessExtensionLine(const char *line, ProjectStateContext *ctx, bool isUndo, struct project_config_extension_t *reg){
    LineParser lp(false);
    if (lp.parse(line) || lp.getnumtokens() < 1)
		return false;
    if (strcmp(lp.gettoken_str(0), "<AUTORENDER") == 0){
        char linebuf[4096];
        while(true){
            if (!ctx->GetLine(linebuf,sizeof(linebuf)) && !lp.parse(linebuf)){
				if (lp.gettoken_str(0)[0] == '>'){
					break;
				}
				
				if( !strcmp(lp.gettoken_str(0), "ARTIST") ){
					g_tag_artist = lp.gettoken_str(1);
				} else if ( !strcmp(lp.gettoken_str(0), "ALBUM") ){
					g_tag_album = lp.gettoken_str(1);
				} else if ( !strcmp(lp.gettoken_str(0), "GENRE") ){
					g_tag_genre = lp.gettoken_str(1);
				} else if ( !strcmp(lp.gettoken_str(0), "YEAR") ){
					g_tag_year = lp.gettoken_int(1);
				} else if ( !strcmp(lp.gettoken_str(0), "COMMENT") ){
					g_tag_comment = lp.gettoken_str(1);
				} else if ( !strcmp(lp.gettoken_str(0), "RENDER_PATH") ){
					g_render_path = lp.gettoken_str(1);
				}

			} else {
				break;
			}
        }
        return true;
    }
    return false;
}

void writeAutorenderSettingInt( ProjectStateContext *ctx, const char* settingName, int setting ){
	char str[512];
	sprintf( str, "%s", settingName);	
	sprintf( str + strlen( str ), " %i", setting );	
	ctx->AddLine(str);
}

void writeAutorenderSettingString( ProjectStateContext *ctx, const char* settingName, string setting ){
	char str[512];	
	sprintf( str, "%s", settingName);	
	sprintf( str + strlen( str ), " \"%s\"", setting.c_str() );	
	ctx->AddLine(str);
}

static void SaveExtensionConfig(ProjectStateContext *ctx, bool isUndo, struct project_config_extension_t *reg){
	ctx->AddLine("<AUTORENDER");
	
	if( !g_tag_artist.empty() ){
		writeAutorenderSettingString( ctx, "ARTIST", g_tag_artist );
	}
	
	if( !g_tag_album.empty() ){
		writeAutorenderSettingString( ctx, "ALBUM", g_tag_album );
	}

	if( !g_tag_genre.empty() ){
		writeAutorenderSettingString( ctx, "GENRE", g_tag_genre );
	}

	if( g_tag_year > 0 ){
		writeAutorenderSettingInt( ctx, "YEAR", g_tag_year );
	}

	if( !g_tag_comment.empty() ){
		writeAutorenderSettingString( ctx, "COMMENT", g_tag_comment );
	}

	if( !g_render_path.empty() ){
		writeAutorenderSettingString( ctx, "RENDER_PATH", g_render_path );
	}

	ctx->AddLine(">");
}

static void BeginLoadProjectState(bool isUndo, struct project_config_extension_t *reg){
}

static project_config_extension_t g_projectconfig = { ProcessExtensionLine, SaveExtensionConfig, BeginLoadProjectState, NULL };

static COMMAND_T g_commandTable[] = {
	{ { {FCONTROL|FALT|FSHIFT|FVIRTKEY,'R',0}, "Autorender: Batch Render Regions" },	"AUTORENDER", AutorenderRegions, "Batch Render Regions" },
	{ { DEFACCEL, "Autorender: Edit Project Metadata" }, "AUTORENDER_METADATA", ShowAutorenderMetadata, "Edit Project Metadata" },
//	{ { DEFACCEL, "Autorender: Show Instructions" }, "AUTORENDER_HELP", ShowAutorenderHelp, "Show Instructions" },
#ifdef TESTCODE
	{ { DEFACCEL, "Autorender: TestCode" }, "AUTORENDER_TESTCODE",  TestFunction, "Autorender: TestCode" },
#endif
	{ {}, LAST_COMMAND, }, // Denote end of table
};

static void menuhook(const char* menustr, HMENU hMenu, int flag){
	if (strcmp(menustr, "Main file") == 0 && flag == 0){
		AddSubMenu(hMenu, SWSCreateMenu( g_commandTable), "Autorender", 40929 );
	}
}

int AutorenderInit(){
	SWSRegisterCommands(g_commandTable);

	if (!plugin_register("projectconfig",&g_projectconfig))
		return 0;

    if (!plugin_register("hookcustommenu", (void*)menuhook))
        return 0;

	return 1;
}