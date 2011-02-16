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
#ifdef _WIN32
	#include "dirent.h"
#endif

//#define UNICODE
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
bool g_prepend_track_number = false;
string g_region_prefix;
bool g_remove_region_prefix = true;
bool g_doing_render = false;

//Pref globals
bool g_pref_allow_stems = false;
bool g_pref_allow_addtoproj = false;
string g_pref_default_render_path;

#define METADATA_WINDOWPOS_KEY "AutorenderWindowPos"
#define PREFS_WINDOWPOS_KEY "AutorenderPrefsWindowPos"
#define ALLOW_STEMS_KEY "AutorenderAllowStemRender"
#define DEFAULT_RENDER_PATH_KEY "AutorenderDefaultRenderPath"

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

#ifndef _WIN32
#define max(x,y) ((x)<(y)?(y):(x))
#endif

vector<string> &split(const string &s, char delim, vector<string> &elems) {
    stringstream ss(s);
    string item;
    while(getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

vector<string> split(const string &s, char delim) {
    vector<string> elems;
    return split(s, delim, elems);
}

void CheckDirTree( const string &path, bool isFilename = false ){
	vector<string> dirComps = split( path, PATH_SLASH_CHAR );
	string buildPath = "";

#ifdef _WIN32
	bool skipRoot = true;
#else
	bool skipRoot = false;
#endif

	unsigned int lastIndex = (int)(isFilename ? dirComps.size() - 2 : dirComps.size() - 1);

	for( unsigned int i = 0; i <= lastIndex; i++ ){
		if( i > 0 ) buildPath += PATH_SLASH_CHAR;
		buildPath.append( dirComps[i] );
		if( i > 0 || !skipRoot ) CreateDirectory( buildPath.c_str(), NULL );
	}
}

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

string GetProjectName(){
	char prjPath[256];
	string prjPathStr = "";
	EnumProjects(-1, prjPath, 256);
	if( strlen( prjPath ) == 0 ) return prjPathStr;
	prjPathStr = prjPath;
    if( prjPathStr.find_last_of( PATH_SLASH_CHAR ) == string::npos ) return prjPathStr;
	prjPathStr = prjPathStr.substr( prjPathStr.find_last_of( PATH_SLASH_CHAR ) + 1 );
	if( prjPathStr.find_last_of( ".rpp" ) == string::npos ) return prjPathStr;
	prjPathStr = prjPathStr.substr( 0, prjPathStr.find_last_of( ".rpp" ) );
	return prjPathStr;
}

void GetProjectString(WDL_String* prjStr){
	char str[4096];
	EnumProjects(-1, str, 256);	

	ProjectStateContext* prj = ProjectCreateFileRead( str );

	if( !prj ){
		//TODO: Throw error
		return;
	}

    while( !prj->GetLine( str, 4096 ) ){
		prjStr->Append( str, prjStr->GetLength() + 4098 );
		prjStr->Append( "\n", prjStr->GetLength() + 2 );
    }
    delete prj;
}

void WriteProjectFile( string filename, WDL_String* prjStr ){
	//CheckDirTree( filename, true ); This done in GetQueuedRenders
	ProjectStateContext* outProject = ProjectCreateFileWrite( filename.c_str() );	
	char line[4096];
	int pos = 0;	
	while( GetChunkLine( prjStr->Get(), line, 4096, &pos, false ) ){
		outProject->AddLine( line );
	}
    delete outProject;
}

/*
 void GetHeapBufFromChar(WDL_HeapBuf* buf, const char *str)
{
	strcpy((char*)buf->Resize(strlen(str)+1), str);
}
*/

void WDLStringReplaceLine( WDL_String *prjStr, int pos, const char *oldLine, const char *newLine ){
	//if newLine does't have a trailing \n, things will break because WDL_String will insert > at the wrong place!
	//maybe should check for this here?
	int lineLen = (int)strlen( oldLine ) + 1; //Add 1 for the omitted newline
	int startPos = pos - lineLen;
	prjStr->DeleteSub( startPos, lineLen );
	prjStr->Insert( newLine, startPos );
	int i = 0;
}

void SetProjectParameter( WDL_String *prjStr, string param, string paramValue ){
	char line[4096];
	int pos = 0;
	LineParser lp(false);

	while( GetChunkLine( prjStr->Get(), line, 4096, &pos, false ) ){
		if( !lp.parse( line ) && lp.getnumtokens() ) {		
			if ( strcmp( lp.gettoken_str(0), param.c_str() ) == 0) {
				string replacementStr = param + string( " " ) + paramValue + string( "\n" );				
				WDLStringReplaceLine( prjStr, pos, line, replacementStr.c_str() );
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
	string qDir = qrStream.str();
	CheckDirTree( qDir );
	return qDir;
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
	// Mac illegal chars
	const char *illegalChars = ":";
	// First char can't be a dot, doens't matter if we're prepending track numbers
	if( !g_prepend_track_number && fn->substr(0,1) == "." ) fn->replace(0,1,"_");
#endif

	ReplaceChars( fn, illegalChars, "_" );
}


void ShowAutorenderHelp(COMMAND_T*) {
	string helpText = "This is how it's done:\n\n";
	helpText.append("1. Create and name regions to be rendered\n");
	helpText.append("    and tagged.\n");
	helpText.append("2. [Optional] Select Edit Project Metadata\n");
	helpText.append("    and set tag metadata and render path.\n");
	helpText.append("3. Batch Render Regions!\n\n\n");
	helpText.append("Notes:\n\n");
	helpText.append("Autorender uses the last used render settings.\n");
	helpText.append("If you need to set your render format, run a dummy\n");
	helpText.append("render the normal way before batch rendering.\n\n");
	helpText.append("If no regions are present, the entire project\n");
	helpText.append("will be rendered and tagged.\n\n");

	MessageBox( GetMainHwnd(), helpText.c_str(), "Autorender Usage", MB_OK );
}


void EnsureStrEndsWith( string &str, const char endChar ){
	if( str.c_str()[ str.length() - 1 ] != endChar ){
		str += endChar;
	}
}

bool EnsureStrDoesntEndWith( string &str, const char endChar ){
	bool removedChars = false;
	while( str.c_str()[ str.length() - 1 ] == endChar ){
		str.erase( str.length() - 1 );
		removedChars = true;
	}
	return removedChars;
}

void FixPath( string &path ){
	EnsureStrEndsWith( path, PATH_SLASH_CHAR );
}

bool BrowseForRenderPath( char *renderPathChar ){
	bool result;
	result = BrowseForDirectory( "Select render output directory", NULL, renderPathChar, 1024 );
	return result;
}

#ifdef _WIN32
void ExecuteWindowsProcess( char* cmd ){
	STARTUPINFO info={sizeof(info)};
	PROCESS_INFORMATION processInfo;
	if (CreateProcess(NULL, cmd, NULL, NULL, TRUE, 0, NULL, NULL, &info, &processInfo)){
		::WaitForSingleObject(processInfo.hProcess, INFINITE);
		CloseHandle(processInfo.hProcess);
		CloseHandle(processInfo.hThread);
	}
}
#endif

void OpenPathInFindplorer( const char* path ){
	if( strlen( path ) ){
		char cmd[512];
#ifdef _WIN32
		strcpy( cmd, "explorer \"");
		strcpy( cmd + strlen( cmd ), path );
		strcpy( cmd + strlen( cmd ), "\"" );
		ExecuteWindowsProcess( cmd );
#else
		strcpy( cmd, "open \"");
		strcpy( cmd + strlen( cmd ), path );
		strcpy( cmd + strlen( cmd ), "\"" );
		system( cmd );
#endif
	}
}


void OpenRenderPath(COMMAND_T *){
	if( !g_render_path.empty() && FileExists( g_render_path.c_str() ) ){
		OpenPathInFindplorer( g_render_path.c_str() );
	} else {
		MessageBox( GetMainHwnd(), "Render path not set or invalid. Set render path in Autorender metadata.", "Render Path Not Set", MB_OK );
	}
}

#ifdef _WIN32
wchar_t* WideCharPlz( const char* inChar ){		
	DWORD dwNum = MultiByteToWideChar(CP_UTF8, 0, inChar, -1, NULL, 0);
	wchar_t *wChar;
	wChar = new wchar_t[ dwNum ];
	MultiByteToWideChar(CP_UTF8, 0, inChar, -1, wChar, dwNum );
	return wChar;
}
#endif

void ForceSaveAndLoad( WDL_String *str ){
	Undo_OnStateChangeEx("Autorender: Load project data", UNDO_STATE_MISCCFG, -1);
	Main_OnCommand( 40026, 0 ); //Save current project
	GetProjectString( str );
}

void toLowerCase( string &str ){
	std::transform( str.begin(), str.end(), str.begin(), (int(*)(int)) std::tolower );
}

bool hasPrefix( string const &fullString, string const &prefix){
	if (fullString.length() >= prefix.length()) {
        return (0 == fullString.compare( 0, prefix.length(), prefix));
    } else {
        return false;
    }
}

bool hasEnding( string const &fullString, string const &ending){
	if (fullString.length() >= ending.length()) {
        return (0 == fullString.compare( fullString.length() - ending.length(), ending.length(), ending));
    } else {
        return false;
    }
}

bool hasEndingCaseInsensitive( string const &fullString, string const &ending){
	string lFullString = fullString;
	toLowerCase( lFullString );
	string lEnding = ending;
	toLowerCase( lEnding );
	return hasEnding( lFullString, lEnding );
}

void NukeDirFiles( string dir, string ext = "" ){
	DIR *dp;
	struct dirent *dirp;
	if( ( dp = opendir( dir.c_str() ) ) != NULL ){
		while( ( dirp = readdir( dp ) ) != NULL ){
			string thisFile = dir + PATH_SLASH_CHAR + dirp->d_name;
			if( thisFile.compare(".") && thisFile.compare("..") && ( ext.empty() || hasEndingCaseInsensitive( thisFile, ext ) ) )
				DeleteFile( thisFile.c_str() );  //Delete any file except . and ..
		}
	}
	closedir( dp );	
}

void MakePathAbsolute( char* path, char* basePath ){
#ifdef _WIN32
	char cwd[512];
	GetCurrentDirectory( 512, cwd );
	SetCurrentDirectory( basePath );
	GetFullPathName( path, 1024, path, NULL );
	SetCurrentDirectory( cwd );
#else
	/*
	char cwd[512];
	cwd = getcwd();
	chdir( basePath );
	realpath( path, path )
	chdir( cwd );
	*/
#endif
}

void MakeMediaFilesAbsolute( WDL_String *prjStr ){
	char line[4096];
	int pos = 0;

	LineParser lp(false);
	bool inTrack = false;
	bool inTrackItem = false;
	bool inTrackItemSource = false;
	char projPath[512];
	GetProjectPath(projPath, 512 );
	int trackIgnoreChunks = 0;
	int trackItemIgnoreChunks = 0;
	int trackItemSourceIgnoreChunks = 0;
	const char *firstChar;
	string firstTokenStr;

	int lineNum = 0; //for debugging

	while( GetChunkLine( prjStr->Get(), line, 4096, &pos, false ) ){
		lineNum++;
		if( !lp.parse( line ) && lp.getnumtokens() ) {
			firstTokenStr = lp.gettoken_str(0);
			firstTokenStr = firstTokenStr.substr(0,1);
			firstChar = firstTokenStr.c_str();

			if ( strcmp( lp.gettoken_str(0), ">" ) == 0 ){
				//end of a chunk
				if( inTrackItemSource ){
					if( trackItemSourceIgnoreChunks == 0 ){
						inTrackItemSource = false;
					} else {
						--trackItemSourceIgnoreChunks;
					}
				} else if( inTrackItem ) {
					if( trackItemIgnoreChunks == 0 ){
						inTrackItem = false;
					} else {
						--trackItemIgnoreChunks;
					}
				} else if( inTrack ){
					if( trackIgnoreChunks == 0 ){
						inTrack = false;
					} else {
						--trackIgnoreChunks;
					}
				}
			} else if( inTrackItemSource ){
				if( strcmp( lp.gettoken_str(0), "FILE" ) == 0 ){
					string replacementStr = "FILE ";				
					char *mediaPath = (char*) lp.gettoken_str(1);
					MakePathAbsolute( mediaPath, projPath );
					WDL_String sanitizedMediaFilePath;
					makeEscapedConfigString( mediaPath, &sanitizedMediaFilePath);
					replacementStr.append( sanitizedMediaFilePath.Get() );
					if( lp.getnumtokens() > 1 ){
						replacementStr.append( " " );
						replacementStr.append( lp.gettoken_str( 2 ) );
					}
					replacementStr.append( string( "\n" ) );
					WDLStringReplaceLine( prjStr, pos, line, replacementStr.c_str() );
				} else if ( strcmp( firstChar, "<" ) == 0 ){
					++trackItemSourceIgnoreChunks;
				}
			} else if ( inTrackItem ){
				if( strcmp( lp.gettoken_str(0), "<SOURCE" ) == 0 ){
					inTrackItemSource = true;
				} else if ( strcmp( firstChar, "<" ) == 0 ){
					++trackItemIgnoreChunks;
				}
			} else if ( inTrack ){
				if( strcmp( lp.gettoken_str(0), "<ITEM" ) == 0 ){
					inTrackItem = true;
				} else if ( ( strcmp( firstChar, "<" ) == 0 ) || ( strcmp( firstChar, "<\0" ) == 0 ) ){
					trackIgnoreChunks++;
				}
			} else if ( strcmp( lp.gettoken_str(0), "<TRACK" ) == 0 ){
				inTrack = true; 
			}
		}
	}	
}


void AutorenderRegions(COMMAND_T*) {
	g_doing_render = true;

	//Get the project config as a WDL_String
	WDL_String prjStr;
	ForceSaveAndLoad( &prjStr );

	//use default path if no render path specified
	if( g_render_path.empty() && !g_pref_default_render_path.empty() ){
		g_render_path = g_pref_default_render_path;
	}

	// remove PATH_SLASH_CHAR from end of string if it exists
	bool removedChars = EnsureStrDoesntEndWith( g_render_path, PATH_SLASH_CHAR );
	if( removedChars ){
		ForceSaveAndLoad( &prjStr );
	}

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
			g_doing_render = false;
			return;
		}
		g_render_path = renderPathChar;		
		ForceSaveAndLoad( &prjStr );
	}

	string renderFileExtension = GetCurrentRenderExtension( &prjStr );
	if( renderFileExtension.empty() ){
		//have to have a renderFileExtension, show error and exit
		MessageBox( GetMainHwnd(), "Couldn't get render extension. Manually render a dummy file with the desired settings and run again.", "Autorender: Render Extension Error", MB_OK );
		g_doing_render = false;
		return;
	}

	//Project tweaks - only do after render path check! (Don't want to overwrite users settings in the original file)
	SetProjectParameter( &prjStr, "RENDER_ADDTOPROJ", "0" );	
	if( !g_pref_allow_stems ) SetProjectParameter( &prjStr, "RENDER_STEMS", "0" );
	MakeMediaFilesAbsolute( &prjStr );

	string queuedRendersDir = GetQueuedRendersDir(); // This also checks to make sure that the dir exists
	NukeDirFiles( queuedRendersDir, "rpp" ); // Deletes all .rpp files in the queuedRendersDir

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

	//Loop through regions, build RenderTrack vector
	while( EnumProjectMarkers( marker_index++, &isrgn, &pos, &rgnend, &track_name, &idx) > 0 ){
		if( isrgn == true && foundIdx.find( idx ) == foundIdx.end() ){
			foundIdx[ idx ] = true;

			RenderTrack renderTrack;
			if( strlen( track_name ) > 0 ){
				renderTrack.trackName = track_name;
				// If a region prefix has been specified, check to see if this region has it and skip if not
				if( !g_region_prefix.empty() ){					
					if( !hasPrefix( renderTrack.trackName, g_region_prefix ) ) continue;
					if( g_remove_region_prefix ){
						renderTrack.trackName.erase( 0, g_region_prefix.length() );
					}
				}

				renderTrack.sanitizedTrackName = renderTrack.trackName;
				SanitizeFilename( &renderTrack.sanitizedTrackName );
			} else {
				// If a region prefix has been specified, skip this track
				if( !g_region_prefix.empty() ) continue;
			}

			renderTrack.trackNumber = ++track_index;
			renderTrack.trackStartTime = pos;
			renderTrack.trackEndTime = rgnend;			
			renderTracks.push_back( renderTrack );
		}
	}

	foundIdx.clear(); 
	
	if( renderTracks.size() == 0 ){
		//Render entire project with tagging
		string prjNameStr = GetProjectName();
		WDL_String trackPrjStr(prjStr);
		RenderTrack renderTrack;
		renderTrack.trackNumber = 1;
		renderTrack.trackName = prjNameStr;
		renderTrack.sanitizedTrackName = prjNameStr;
		SanitizeFilename( &renderTrack.sanitizedTrackName );
		renderTrack.entireProject = true;
		renderTracks.push_back( renderTrack );
	}

	// Get number of digits to pad the track numbers with...at least two
	int trackNumberPad = (int) max( 2, floor( log10( (double) renderTracks.size() ) ) + 1 );

	// Set to zero if prepending track numbers is turned off
	int prependTrackNumberPad = g_prepend_track_number ? trackNumberPad : 0;

	//Set nameless tracks to Track XX
	for( unsigned int i = 0; i < renderTracks.size(); i++){		
		if( renderTracks[i].trackName.empty() ){
			renderTracks[i].trackName = "Track " + renderTracks[i].getPaddedTrackNumber( trackNumberPad );
			renderTracks[i].sanitizedTrackName = renderTracks[i].trackName;
		}
	}

	if( !g_prepend_track_number ){
		//Set duplicate numbers so that tracks can't overwrite each other when track number prepending is off
		//Although, this could still happen (e.g. if two tracks were named Song and another was named Song(2) )
		map<string,int> trackDuplicates;
		for( unsigned int i = 0; i < renderTracks.size(); i++){		
			if( trackDuplicates[ renderTracks[i].trackName ] ){
				renderTracks[i].duplicateNumber = ++trackDuplicates[ renderTracks[i].trackName ];
			} else {
				trackDuplicates[ renderTracks[i].trackName ] = 1;
			}
		}
		trackDuplicates.clear();
	}

	//Build render queue
	for( unsigned int i = 0; i < renderTracks.size(); i++){		
		WDL_String trackPrjStr(prjStr);

		string outRenderProjectPath = outRenderProjectPrefix + renderTracks[i].getFileName("rpp", trackNumberPad );
		string renderFilePath = g_render_path + PATH_SLASH_CHAR + renderTracks[i].getFileName( renderFileExtension, prependTrackNumberPad );						
		SetProjectParameter( &trackPrjStr, "RENDER_FILE", "\"" + renderFilePath + "\"" );

		if( renderTracks[i].entireProject ){
			SetProjectParameter( &trackPrjStr, "RENDER_RANGE", "1 0 0" ); //Render entire project			
		} else {
			ostringstream renderRange;
			renderRange << "0 " << renderTracks[i].trackStartTime << " " << renderTracks[i].trackEndTime;
			SetProjectParameter( &trackPrjStr, "RENDER_RANGE", renderRange.str() );	
		}
		
		WriteProjectFile( outRenderProjectPath, &trackPrjStr );
	}

	Main_OnCommand( 41207, 0 ); //Render all queued renders

	// Tag!
	for( unsigned int i = 0; i < renderTracks.size(); i++){		
		string renderedFilePath = g_render_path + PATH_SLASH_CHAR + renderTracks[i].getFileName( renderFileExtension, prependTrackNumberPad );
#ifdef _WIN32
		wchar_t* w_rendered_path = WideCharPlz( renderedFilePath.c_str() );
		TagLib::FileRef f( w_rendered_path );
#else
		TagLib::FileRef f( renderedFilePath.c_str() );
#endif
		if( !f.isNull() ){
#ifdef _WIN32
			wchar_t* w_tag_artist = WideCharPlz( g_tag_artist.c_str() );
			wchar_t* w_tag_album = WideCharPlz( g_tag_album.c_str() );
			wchar_t* w_tag_genre = WideCharPlz( g_tag_genre.c_str() );
			wchar_t* w_tag_comment = WideCharPlz( g_tag_comment.c_str() );
			wchar_t* w_track_title = WideCharPlz( renderTracks[i].trackName.c_str() );

			if( wcslen( w_tag_artist ) ) f.tag()->setArtist( w_tag_artist );
			if( wcslen( w_tag_album ) ) f.tag()->setAlbum( w_tag_album );
			if( wcslen( w_tag_genre ) ) f.tag()->setGenre( w_tag_genre );
			if( wcslen( w_tag_comment ) ) f.tag()->setComment( w_tag_comment );

			f.tag()->setTitle( w_track_title );
			
			delete [] w_tag_artist;
			delete [] w_tag_album;
			delete [] w_tag_genre;
			delete [] w_tag_comment;
			delete [] w_track_title;
#else
			if( !g_tag_artist.empty() ) f.tag()->setArtist( g_tag_artist.c_str() );
			if( !g_tag_album.empty() ) f.tag()->setAlbum( g_tag_album.c_str() );
			if( !g_tag_genre.empty() ) f.tag()->setGenre( g_tag_genre.c_str() );
			if( !g_tag_comment.empty() ) f.tag()->setComment( g_tag_comment.c_str() );
			f.tag()->setTitle( renderTracks[i].trackName.c_str() );
#endif			
			if( g_tag_year > 0 ) f.tag()->setYear( g_tag_year );
			f.tag()->setTrack( i + 1 );
			f.save();
		} else {
			//throw error?
		}
#ifdef _WIN32
		delete [] w_rendered_path;
#endif
	}

	OpenRenderPath( NULL );
	g_doing_render = false;
	
	//NukeDirFiles( queuedRendersDir, "rpp" ); //Maybe cleanup .rpp here too?
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

void processDialogFieldCheck( HWND hwndDlg, WPARAM wParam, bool &target, bool &hasChanged ){
	bool dlg_field;
	//No IsWindowEnabled on OSX...maybe in swell?
	//dlg_field = IsWindowEnabled( GetDlgItem(hwndDlg, (int)wParam) ) && IsDlgButtonChecked(hwndDlg, (int)wParam) != 0;
	dlg_field = IsDlgButtonChecked(hwndDlg, (int)wParam) != 0;

	if( dlg_field != target ){
		target = dlg_field;
		hasChanged = true;
	}
}

void loadPrefs(){
	g_pref_allow_stems = GetPrivateProfileInt( SWS_INI, ALLOW_STEMS_KEY, 0, get_ini_file() ) != 0;
	char def_render_path[256];
	GetPrivateProfileString( SWS_INI, DEFAULT_RENDER_PATH_KEY, "", def_render_path, 256, get_ini_file() );
	g_pref_default_render_path = def_render_path;
}


INT_PTR WINAPI doAutorenderMetadata(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam){
	bool hasChanged = false;

	switch (uMsg){
            case WM_INITDIALOG:
				RestoreWindowPos(hwndDlg, PREFS_WINDOWPOS_KEY, false);
				SetDlgItemText(hwndDlg, IDC_ALLOW_STEMS, g_tag_artist.c_str() );
				SetDlgItemText(hwndDlg, IDC_ALBUM, g_tag_album.c_str() );
				SetDlgItemText(hwndDlg, IDC_GENRE, g_tag_genre.c_str() );
				SetDlgItemInt(hwndDlg, IDC_YEAR, g_tag_year, false );
				SetDlgItemText(hwndDlg, IDC_COMMENT, g_tag_comment.c_str() );
				SetDlgItemText(hwndDlg, IDC_RENDER_PATH, g_render_path.c_str() );
				SetDlgItemText(hwndDlg, IDC_REGION_PREFIX, g_region_prefix.c_str() );
				CheckDlgButton(hwndDlg, IDC_REMOVE_PREFIX_FROM_TRACK_NAME, g_remove_region_prefix);
				CheckDlgButton(hwndDlg, IDC_PREPEND_TRACK_NUMBER, g_prepend_track_number);
				

				return 0;
            case WM_COMMAND:
				switch (LOWORD(wParam)){
					case IDC_BROWSE:
						char renderPathChar[1024];
						bool browseResult;
						browseResult = BrowseForRenderPath(renderPathChar);
						if( browseResult ) SetDlgItemText(hwndDlg, IDC_RENDER_PATH, renderPathChar );
						break;
					case IDC_REGION_PREFIX:
						char prefixChar[256];
						GetDlgItemText(hwndDlg, IDC_REGION_PREFIX, prefixChar, 256);
						EnableWindow( GetDlgItem(hwndDlg, IDC_REMOVE_PREFIX_FROM_TRACK_NAME), strlen( prefixChar ) > 0 );
						break;
                    case IDOK:
						processDialogFieldStr( hwndDlg, IDC_ARTIST, g_tag_artist, hasChanged );
						processDialogFieldStr( hwndDlg, IDC_ALBUM, g_tag_album, hasChanged );
						processDialogFieldStr( hwndDlg, IDC_GENRE, g_tag_genre, hasChanged );
						processDialogFieldInt( hwndDlg, IDC_YEAR, g_tag_year, hasChanged );
						processDialogFieldStr( hwndDlg, IDC_COMMENT, g_tag_comment, hasChanged );
						processDialogFieldStr( hwndDlg, IDC_RENDER_PATH, g_render_path, hasChanged );
						processDialogFieldStr( hwndDlg, IDC_REGION_PREFIX, g_region_prefix, hasChanged );
						processDialogFieldCheck( hwndDlg, IDC_REMOVE_PREFIX_FROM_TRACK_NAME, g_remove_region_prefix, hasChanged );
						processDialogFieldCheck( hwndDlg, IDC_PREPEND_TRACK_NUMBER, g_prepend_track_number, hasChanged );						

						if( hasChanged ) Undo_OnStateChangeEx("Set autorender metadata", UNDO_STATE_MISCCFG, -1);
                        // fall through!								
                    case IDCANCEL:							
                        SaveWindowPos(hwndDlg, PREFS_WINDOWPOS_KEY);
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

const char* bool_to_char( bool b){
	return b ? "1" : "0";
}

INT_PTR WINAPI doAutorenderPreferences(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam){
	bool hasChangedDontCare = false;

    switch (uMsg){
            case WM_INITDIALOG:
				loadPrefs();
				RestoreWindowPos(hwndDlg, METADATA_WINDOWPOS_KEY, false);
				CheckDlgButton(hwndDlg, IDC_ALLOW_STEMS, g_pref_allow_stems);
				SetDlgItemText(hwndDlg, IDC_DEFAULT_RENDER_PATH, g_pref_default_render_path.c_str() );
				return 0;
            case WM_COMMAND:
				switch (LOWORD(wParam)){
					case IDC_BROWSE:
						char renderPathChar[1024];
						bool browseResult;
						browseResult = BrowseForRenderPath(renderPathChar);
						if( browseResult ){
							SetDlgItemText(hwndDlg, IDC_DEFAULT_RENDER_PATH, renderPathChar );
						}
						break;
                    case IDOK:
						g_pref_allow_stems = IsDlgButtonChecked(hwndDlg, IDC_ALLOW_STEMS) != 0;
						WritePrivateProfileString(SWS_INI, ALLOW_STEMS_KEY, bool_to_char( g_pref_allow_stems ), get_ini_file());

						processDialogFieldStr( hwndDlg, IDC_DEFAULT_RENDER_PATH, g_pref_default_render_path, hasChangedDontCare );
						WritePrivateProfileString(SWS_INI, DEFAULT_RENDER_PATH_KEY, g_pref_default_render_path.c_str(), get_ini_file());
                        // fall through!
                    case IDCANCEL:
                        SaveWindowPos(hwndDlg, METADATA_WINDOWPOS_KEY);
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

void AutorenderPreferences(COMMAND_T*){
	DialogBox( g_hInst, MAKEINTRESOURCE(IDD_AUTORENDER_PREFERENCES), g_hwndParent, doAutorenderPreferences );
}



static bool ProcessExtensionLine(const char *line, ProjectStateContext *ctx, bool isUndo, struct project_config_extension_t *reg){
	if( g_doing_render ) return false;

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
				} else if ( !strcmp(lp.gettoken_str(0), "REGION_PREFIX") ){
					g_region_prefix = lp.gettoken_str(1);
				} else if ( !strcmp(lp.gettoken_str(0), "REMOVE_REGION_PREFIX") ){
					g_remove_region_prefix = lp.gettoken_int(1) != 0;
				} else if ( !strcmp(lp.gettoken_str(0), "PREPEND_TRACK_NUMBER") ){
					g_prepend_track_number = lp.gettoken_int(1) != 0;
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
	sprintf( str, "%s %i", settingName, setting);
	ctx->AddLine(str);
}

void writeAutorenderSettingString( ProjectStateContext *ctx, const char* settingName, string setting ){
	// Need to use makeEscapedConfigString to make sure that if the user enters "'` etc into the
	// metadata that the RPP isn't corrupted
	WDL_String sanitizedStr;
	makeEscapedConfigString(setting.c_str(), &sanitizedStr);
	WDL_String str(settingName);
	str.Append(" ");
	str.Append(sanitizedStr.Get());
	ctx->AddLine(str.Get());
}

static void SaveExtensionConfig(ProjectStateContext *ctx, bool isUndo, struct project_config_extension_t *reg){
	// Don't save if the data's blank.  Otherwise every project file will have the AUTORENDER section
	// In itself this is fine, but then if the user uninstalls the SWS extension
	// then there will be a warning with every project file about unknown ext data
	// If only the year is set, don't write anything either.
	if (!g_tag_artist.empty() || !g_tag_album.empty() || !g_tag_genre.empty() ||
		!g_tag_comment.empty() || !g_render_path.empty() || !g_region_prefix.empty()
		|| !g_prepend_track_number || !g_remove_region_prefix )
	{
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

		if( !g_region_prefix.empty() ){
			writeAutorenderSettingString( ctx, "REGION_PREFIX", g_region_prefix );
		}
		
		writeAutorenderSettingInt( ctx, "REMOVE_REGION_PREFIX", int( g_remove_region_prefix ) );

		writeAutorenderSettingInt( ctx, "PREPEND_TRACK_NUMBER", int( g_prepend_track_number ) );

		ctx->AddLine(">");
	}
}

static void BeginLoadProjectState(bool isUndo, struct project_config_extension_t *reg){
	if( g_doing_render ) return;
	loadPrefs();
	g_tag_artist.clear();
	g_tag_album.clear();
	g_tag_genre.clear();
	g_tag_year = GetCurrentYear();
	g_tag_comment.clear();
	g_render_path.clear();	
	g_prepend_track_number = true;
	g_region_prefix.clear();
	g_remove_region_prefix = true;
}

static project_config_extension_t g_projectconfig = { ProcessExtensionLine, SaveExtensionConfig, BeginLoadProjectState, NULL };

static COMMAND_T g_commandTable[] = {
	{ { {FCONTROL|FALT|FSHIFT|FVIRTKEY,'R',0}, "SWS Autorender: Batch Render Regions" },	"AUTORENDER", AutorenderRegions, "Batch Render Regions" },
	{ { DEFACCEL, "SWS Autorender: Edit Project Metadata" }, "AUTORENDER_METADATA", ShowAutorenderMetadata, "Edit Project Metadata" },
	{ { DEFACCEL, "SWS Autorender: Open Render Path" }, "AUTORENDER_OPEN_RENDER_PATH", OpenRenderPath, "Open Render Path" },	
	{ { DEFACCEL, "SWS Autorender: Show Instructions" }, "AUTORENDER_HELP", ShowAutorenderHelp, "Show Instructions" },
	{ { DEFACCEL, "SWS Autorender: Global Preferences" }, "AUTORENDER_PREFERENCES", AutorenderPreferences, "Global Preferences" },
#ifdef TESTCODE
	{ { DEFACCEL, "SWS Autorender: TestCode" }, "AUTORENDER_TESTCODE",  TestFunction, "Autorender: TestCode" },
#endif
	{ {}, LAST_COMMAND, }, // Denote end of table
};

static void menuhook(const char* menustr, HMENU hMenu, int flag){
	if (strcmp(menustr, "Main file") == 0 && flag == 0){
		AddSubMenu(hMenu, SWSCreateMenu( g_commandTable), "SWS Autorender", 40929 );
	}
}

int AutorenderInit(){
	SWSRegisterCommands(g_commandTable);

	if (!plugin_register("projectconfig",&g_projectconfig))
		return 0;
#ifdef _SWS_MENU
    if (!plugin_register("hookcustommenu", (void*)menuhook))
        return 0;
#endif
	return 1;
}