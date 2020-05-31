/******************************************************************************
/ Autorender.cpp
/
/ Copyright (c) 2011-2018 Shane St Savage
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
	#include <Shlwapi.h>
#endif

//#define UNICODE

#include "RenderRegion.h"

#include "../cfillion/cfillion.hpp" // CF_ShellExecute
#include "../SnM/SnM_Dlg.h"
#include "../Prompt.h"

#include <time.h>
#include <WDL/localize/localize.h>
#include <WDL/projectcontext.h>

#include <taglib/tag.h>
#include <taglib/fileref.h>

int GetCurrentYear(){
	time_t t = 0;
	struct tm *lt = NULL;
	t = time(NULL);
	lt = localtime( &t );
	int curYear = lt->tm_year;
	curYear = curYear + 1900;
	return curYear;
}

string GetRenderQueueTimeString(){
  time_t rawtime;
  struct tm * timeinfo;
  char buffer[14];
  time ( &rawtime );
  timeinfo = localtime ( &rawtime );

  strftime( buffer,14,"%y%m%d_%H%M%S",timeinfo);

  //if this hasn't been made clear before, i suck at c++
  return string( buffer );
}

// Globals
string g_render_path;
string g_tag_artist;
string g_tag_album;
string g_tag_genre;
int g_tag_year = GetCurrentYear();
string g_tag_comment;
bool g_doing_render = false;

//Pref globals
string g_pref_default_render_path;

#define METADATA_WINDOWPOS_KEY "AutorenderWindowPos"
#define PREFS_WINDOWPOS_KEY "AutorenderPrefsWindowPos"
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

string ParseFileExtension(string path){
	if (path.find_last_of(".") != string::npos)
		return path.substr(path.find_last_of(".") + 1);
	return "";
}

void ParsePath( const char* path, char* parentDir ){
	//set parentDir to the parent directory or an empty string if none exists
	strcpy( parentDir, path );
	char *lastSlash = strrchr( parentDir, PATH_SLASH_CHAR );
	if( lastSlash ){
		lastSlash[0] = 0;
	} else {
		parentDir = 0;
	}
}

void GetProjectRealPath( char* prjPath ){
	char rpp[MAX_PATH];
	EnumProjects(-1, rpp, MAX_PATH);
	ParsePath( rpp, prjPath );
}

string ARGetProjectName(){
	char prjPath[MAX_PATH];
	string prjPathStr = "";
	EnumProjects(-1, prjPath, MAX_PATH);
	if( strlen( prjPath ) == 0 ) return prjPathStr;
	prjPathStr = prjPath;
    if( prjPathStr.find_last_of( PATH_SLASH_CHAR ) == string::npos ) return prjPathStr;
	prjPathStr = prjPathStr.substr( prjPathStr.find_last_of( PATH_SLASH_CHAR ) + 1 );
	if( prjPathStr.find_last_of( ".rpp" ) == string::npos ) return prjPathStr;
	prjPathStr = prjPathStr.substr( 0, prjPathStr.find_last_of( ".rpp" ) );
	return prjPathStr;
}

void GetProjectString(WDL_FastString* prjStr){
	char str[4096];
	EnumProjects(-1, str, MAX_PATH);

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

void WriteProjectFile( string filename, WDL_FastString* prjStr ){
	//CheckDirTree( filename, true ); This done in GetQueuedRenders
	ProjectStateContext* outProject = ProjectCreateFileWrite( filename.c_str() );

	if(!outProject)
		return;

	char line[4096];
	int pos = 0;
	while( GetChunkLine( prjStr->Get(), line, 4096, &pos, false ) ){
		outProject->AddLine("%s",line);
	}
	delete outProject;
}

void WDLStringReplaceLine( WDL_FastString *prjStr, int pos, const char *oldLine, const char *newLine ){
	//if newLine does't have a trailing \n, things will break because WDL_FastString will insert > at the wrong place!
	//maybe should check for this here?
	int lineLen = (int)strlen( oldLine ) + 1; //Add 1 for the omitted newline
	int startPos = pos - lineLen;
	prjStr->DeleteSub( startPos, lineLen );
	prjStr->Insert( newLine, startPos );
}

void SetProjectParameter( WDL_FastString *prjStr, string param, string paramValue, string insertAfterParam ){
	char line[4096];
	int pos = 0;
	LineParser lp(false);
	string paramString = param + string(" ") + paramValue + string("\n");

	while (GetChunkLine(prjStr->Get(), line, 4096, &pos, false)){
		if( !lp.parse( line ) && lp.getnumtokens() ) {
			if ( strcmp( lp.gettoken_str(0), param.c_str() ) == 0) {
				WDLStringReplaceLine(prjStr, pos, line, paramString.c_str());
				return;
			}
		}
	}

	if (!insertAfterParam.empty()) {
		//param wasn't found, insert after insertAfterParam
		pos = 0;
		while (GetChunkLine(prjStr->Get(), line, 4096, &pos, false)){
			if (!lp.parse(line) && lp.getnumtokens()) {
				if (strcmp(lp.gettoken_str(0), insertAfterParam.c_str()) == 0) {
					string replacementStr = line + string("\n") + paramString;
					WDLStringReplaceLine(prjStr, pos, line, replacementStr.c_str());
					return;
				}
			}
		}
	}
}

void SetProjectParameter(WDL_FastString *prjStr, string param, string paramValue){
	SetProjectParameter(prjStr, param, paramValue, "");
}

string GetProjectParameterValueStr( WDL_FastString *prjStr, string param, int token = 1 ){
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

string GetQueuedRendersDir(){
	ostringstream qrStream;
	qrStream << GetResourcePath() << PATH_SLASH_CHAR << "QueuedRenders";
	string qDir = qrStream.str();
	CheckDirTree( qDir );
	return qDir;
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
	const char *illegalChars = "\\/<>|\":?*.";
	ReplaceChars(fn, illegalChars, "_");
}

void ShowAutorenderHelp(COMMAND_T*) {
	string helpText = __LOCALIZE("This is how it's done:\r\n\r\n","sws_DLG_158");
	helpText.append(__LOCALIZE("1. Create and name regions to be rendered\r\n","sws_DLG_158"));
	helpText.append(__LOCALIZE("and tagged.\r\n","sws_DLG_158"));
	helpText.append(__LOCALIZE("2. [Optional] Select Edit Project Metadata\r\n","sws_DLG_158"));
	helpText.append(__LOCALIZE("and set tag metadata and render path.\r\n","sws_DLG_158"));
	helpText.append(__LOCALIZE("3. Batch Render Regions!\r\n\r\n\r\n","sws_DLG_158"));
	helpText.append(__LOCALIZE("Important notes:\r\n","sws_DLG_158"));
	helpText.append(__LOCALIZE("Autorender uses the last used render settings.\r\n","sws_DLG_158"));
	helpText.append(__LOCALIZE("If you need to change render settings, run a dummy\r\n","sws_DLG_158"));
	helpText.append(__LOCALIZE("render and save the project before batch rendering.\r\n","sws_DLG_158"));
	helpText.append(__LOCALIZE("If no regions are present, the entire project\r\n","sws_DLG_158"));
	helpText.append(__LOCALIZE("will be rendered and tagged.\r\n\r\n","sws_DLG_158"));
	DisplayInfoBox(GetMainHwnd(), __LOCALIZE("Autorender usage","sws_DLG_158"), helpText.c_str());
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
	result = BrowseForDirectory(__LOCALIZE("Select render output directory","sws_DLG_158"), NULL, renderPathChar, 1024 );
	return result;
}

void OpenRenderPath(COMMAND_T *){
	if( !g_render_path.empty() && FileExists( g_render_path.c_str() ) ){
		CF_ShellExecute( g_render_path.c_str() );
	} else {
		MessageBox( GetMainHwnd(), __LOCALIZE("Render path not set or invalid. Set render path in Autorender metadata.","sws_mbox"), __LOCALIZE("Autorender - Error","sws_mbox"), MB_OK );
	}
}

void ForceSaveAndLoad( WDL_FastString *str ){
	Undo_OnStateChangeEx(__LOCALIZE("Autorender: Load project data","sws_undo"), UNDO_STATE_MISCCFG, -1);
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

void GetRenderedFiles(string dir, vector<RenderRegion> regions, map <string, RenderRegion> &files){
	DIR *dp;
	struct dirent *dirp;
	if ((dp = opendir(dir.c_str())) != NULL){
		while ((dirp = readdir(dp)) != NULL){
			string fileName = string(dirp->d_name);

			if (!fileName.compare(".") || !fileName.compare("..")) {
				continue;
			}

			for (std::vector<RenderRegion>::iterator region = regions.begin(); region != regions.end(); ++region) {
				string regionFileNamePrefix = region->getFileName("", 0);
				//TODO make sure filename sanitizing works as expected (REAPER internally handling during region rendering
				if (!fileName.compare(0, regionFileNamePrefix.length(), regionFileNamePrefix)) {
					string path = string(dir + PATH_SLASH_CHAR + fileName);
					files.insert(pair <string, RenderRegion>(path, *region));
					break;
				}
			}
		}
	}
	closedir(dp);
}

void MakePathAbsolute( char* path, char* basePath ){
#ifdef _WIN32
	if (PathIsRelative(path))
#else
	if (path[0] != '/' && path[0] != '~') // Reaper probably never uses homedir-rooted paths, but check just in case.
#endif
	{
		char filename[MAX_PATH];
		strcpy(filename, path);
		sprintf(path, "%s%c%s", basePath, PATH_SLASH_CHAR, filename);
	}
}

void MakeMediaFilesAbsolute( WDL_FastString *prjStr ){
	char line[4096];
	int pos = 0;

	LineParser lp(false);
	bool inTrack = false;
	bool inTrackItem = false;
	bool inTrackItemSource = false;
	int trackIgnoreChunks = 0;
	int trackItemIgnoreChunks = 0;
	int trackItemSourceIgnoreChunks = 0;
	const char *firstChar;
	string firstTokenStr;

	int lineNum = 0; //for debugging

	//Reaper API's GetProjectPath() returns the path to the project's audio dir, not to .rpp!
	char projPath[MAX_PATH];
	GetProjectRealPath( projPath );

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
					WDL_FastString sanitizedMediaFilePath;
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


void AutorenderRegions(COMMAND_T*)
{
  if (IsProjectDirty && IsProjectDirty(NULL))
  {
    // keep this msg on a single line for the langpack generator
		int r=MessageBox(GetMainHwnd(), __LOCALIZE("The current project is not saved.\r\nDo you want to save it?\r\n\r\nNote: if you have changed render settings, you need to run a dummy render and save the project first (last settings will not be taken into account otherwise).","sws_mbox"),
      __LOCALIZE("Autorender","sws_mbox"), MB_YESNOCANCEL);
    if (r==IDCANCEL) return;
    if (r==IDYES) Main_OnCommand(40026,0);
  }

	g_doing_render = true;

	//Get the project config as a WDL_FastString
	WDL_FastString prjStr;
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
		string message = (string)__LOCALIZE("Render path","sws_mbox") + " " + g_render_path + " " + (string)__LOCALIZE("does not exist!","sws_mbox");
		g_render_path.clear();
		MessageBox( GetMainHwnd(), message.c_str(), __LOCALIZE("Autorender - Error","sws_mbox"), MB_OK );
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

	//Project tweaks - only do after render path check! (Don't want to overwrite users settings in the original file)
	MakeMediaFilesAbsolute( &prjStr );

	string queuedRendersDir = GetQueuedRendersDir(); // This also checks to make sure that the dir exists
	NukeDirFiles( queuedRendersDir, "rpp" ); // Deletes all .rpp files in the queuedRendersDir

	ostringstream outRenderProjectPrefixStream;
	outRenderProjectPrefixStream << queuedRendersDir << PATH_SLASH_CHAR;
	outRenderProjectPrefixStream << "qrender_";

	string outRenderProjectPrefix = outRenderProjectPrefixStream.str();

	//init the stuff we need for the region loop
	int marker_index = 0, region_index = 0, idx;
	bool isrgn;
	double pos, rgnend;
	const char* region_name;
	vector<RenderRegion> renderRegions;
	map<int,bool> foundIdx;

	//Loop through regions, build RenderRegion vector (this is just for tracking what to tag)
	while( EnumProjectMarkers( marker_index++, &isrgn, &pos, &rgnend, &region_name, &idx) > 0 ){
		if( isrgn == true && foundIdx.find( idx ) == foundIdx.end() ){
			foundIdx[ idx ] = true;

			RenderRegion renderRegion;
			if( strlen( region_name ) > 0 ){
				renderRegion.regionName = region_name;
				renderRegion.sanitizedRegionName = region_name;
				SanitizeFilename( &renderRegion.sanitizedRegionName );
			}

			renderRegion.regionNumber = ++region_index;
			renderRegions.push_back( renderRegion );
		}
	}

	foundIdx.clear();

	if( renderRegions.size() == 0 ){
		//Render entire project with tagging
		string prjNameStr = ARGetProjectName();
		RenderRegion renderRegion;
		renderRegion.regionNumber = 1;
		renderRegion.regionName = prjNameStr;
		renderRegion.sanitizedRegionName = prjNameStr;
		SanitizeFilename( &renderRegion.sanitizedRegionName );
		renderRegion.entireProject = true;
		renderRegions.push_back( renderRegion );
	}

	// Get number of digits to pad the region numbers with...at least two
	int regionNumberPad = (int) max( 2.0, floor( log10( (double) renderRegions.size() ) ) + 1 );

	//Set nameless regions to region_XX
	for( unsigned int i = 0; i < renderRegions.size(); i++){
		if( renderRegions[i].regionName.empty() ){
			renderRegions[i].regionName = "region_" + renderRegions[i].getPaddedRegionNumber( regionNumberPad );
			renderRegions[i].sanitizedRegionName = renderRegions[i].regionName;
		}
	}

	//Build render queue
	//a single project with fixed render parameters is added to the queue, which renders all regions
	string outRenderProjectPath = outRenderProjectPrefix;
	outRenderProjectPath += GetRenderQueueTimeString() + "_" + ARGetProjectName() + "_autorender.rpp";

	if (renderRegions.size() == 1 && renderRegions[0].entireProject) {
		string regionFilename = renderRegions[0].getFileName("", 2);
		if (g_render_path.empty()){
			SetProjectParameter(&prjStr, "RENDER_FILE", "\"" + regionFilename + "\"");
		} else {
			SetProjectParameter(&prjStr, "RENDER_FILE", "\"" + g_render_path + PATH_SLASH_CHAR + regionFilename + "\"");
		}

		SetProjectParameter(&prjStr, "RENDER_RANGE", "1 0 0 18 1000");
	} else {
		if (!g_render_path.empty()){
			SetProjectParameter(&prjStr, "RENDER_FILE", "\"" + g_render_path + "\"");
		}

		SetProjectParameter(&prjStr, "RENDER_PATTERN", "\"$timelineorder $region\"", "RENDER_FILE");
		SetProjectParameter(&prjStr, "RENDER_RANGE", "3 0 0 18 1000");
	}

	SetProjectParameter(&prjStr, "RENDER_STEMS", "0");
	SetProjectParameter(&prjStr, "RENDER_ADDTOPROJ", "0");

	WriteProjectFile(outRenderProjectPath, &prjStr);

	Main_OnCommand( 41207, 0 ); //Render all queued renders

	map<string, RenderRegion> renderedFiles;
	GetRenderedFiles(g_render_path, renderRegions, renderedFiles);

	// Tag!
	for (std::map<string, RenderRegion>::iterator renderedFile = renderedFiles.begin(); renderedFile != renderedFiles.end(); ++renderedFile){
		string renderedFilePath = renderedFile->first;
		RenderRegion renderRegion = renderedFile->second;

		TagLib::FileRef f( win32::widen(renderedFilePath).c_str() );

		if( !f.isNull() ) {
			if( !g_tag_artist.empty() )
			  f.tag()->setArtist( {g_tag_artist, TagLib::String::UTF8} );
			if( !g_tag_album.empty() )
			  f.tag()->setAlbum( {g_tag_album, TagLib::String::UTF8} );
			if( !g_tag_genre.empty() )
			  f.tag()->setGenre( {g_tag_genre, TagLib::String::UTF8} );
			if( !g_tag_comment.empty() )
			  f.tag()->setComment( {g_tag_comment, TagLib::String::UTF8} );
			f.tag()->setTitle( {renderRegion.regionName, TagLib::String::UTF8} );

			if( g_tag_year > 0 ) f.tag()->setYear( g_tag_year );

			f.tag()->setTrack( renderRegion.regionNumber );
			f.save();
		} else {
			//throw error?
		}
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
    // NF: use IsWindowEnabled() from SWELL, see commit 44371a80
	dlg_field = IsWindowEnabled( GetDlgItem(hwndDlg, (int)wParam) ) && IsDlgButtonChecked(hwndDlg, (int)wParam) != 0;
	// dlg_field = IsDlgButtonChecked(hwndDlg, (int)wParam) != 0;

	if( dlg_field != target ){
		target = dlg_field;
		hasChanged = true;
	}
}

void loadPrefs(){
	char def_render_path[MAX_PATH];
	GetPrivateProfileString( SWS_INI, DEFAULT_RENDER_PATH_KEY, "", def_render_path, MAX_PATH, get_ini_file() );
	g_pref_default_render_path = def_render_path;
}

INT_PTR WINAPI doAutorenderMetadata(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (INT_PTR r = SNM_HookThemeColorsMessage(hwndDlg, uMsg, wParam, lParam))
		return r;

	bool hasChanged = false;
	switch (uMsg){
            case WM_INITDIALOG:
				RestoreWindowPos(hwndDlg, PREFS_WINDOWPOS_KEY, false);
				SetDlgItemText(hwndDlg, IDC_ARTIST, g_tag_artist.c_str() );
				SetDlgItemText(hwndDlg, IDC_ALBUM, g_tag_album.c_str() );
				SetDlgItemText(hwndDlg, IDC_GENRE, g_tag_genre.c_str() );
				SetDlgItemInt(hwndDlg, IDC_YEAR, g_tag_year, false );
				SetDlgItemText(hwndDlg, IDC_COMMENT, g_tag_comment.c_str() );
				SetDlgItemText(hwndDlg, IDC_RENDER_PATH, g_render_path.c_str() );
				return 0;
            case WM_COMMAND:
				switch (LOWORD(wParam)){
					case IDC_BROWSE:
						char renderPathChar[1024];
						bool browseResult;
						browseResult = BrowseForRenderPath(renderPathChar);
						if( browseResult ) SetDlgItemText(hwndDlg, IDC_RENDER_PATH, renderPathChar );
						break;
                    case IDOK:
						processDialogFieldStr( hwndDlg, IDC_ARTIST, g_tag_artist, hasChanged );
						processDialogFieldStr( hwndDlg, IDC_ALBUM, g_tag_album, hasChanged );
						processDialogFieldStr( hwndDlg, IDC_GENRE, g_tag_genre, hasChanged );
						processDialogFieldInt( hwndDlg, IDC_YEAR, g_tag_year, hasChanged );
						processDialogFieldStr( hwndDlg, IDC_COMMENT, g_tag_comment, hasChanged );
						processDialogFieldStr( hwndDlg, IDC_RENDER_PATH, g_render_path, hasChanged );

						if( hasChanged ) Undo_OnStateChangeEx(__LOCALIZE("Set autorender metadata","sws_undo"), UNDO_STATE_MISCCFG, -1);
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

INT_PTR WINAPI doAutorenderPreferences(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (INT_PTR r = SNM_HookThemeColorsMessage(hwndDlg, uMsg, wParam, lParam))
		return r;

	bool hasChangedDontCare = false;
    switch (uMsg){
            case WM_INITDIALOG:
				loadPrefs();
				RestoreWindowPos(hwndDlg, METADATA_WINDOWPOS_KEY, false);
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
	ctx->AddLine("%s",str);
}

void writeAutorenderSettingString( ProjectStateContext *ctx, const char* settingName, string setting ){
	// Need to use makeEscapedConfigString to make sure that if the user enters "'` etc into the
	// metadata that the RPP isn't corrupted
	WDL_FastString sanitizedStr;
	makeEscapedConfigString(setting.c_str(), &sanitizedStr);
	WDL_FastString str(settingName);
	str.Append(" ");
	str.Append(sanitizedStr.Get());
	ctx->AddLine("%s",str.Get());
}

static void SaveExtensionConfig(ProjectStateContext *ctx, bool isUndo, struct project_config_extension_t *reg){
	// Don't save if the data's blank.  Otherwise every project file will have the AUTORENDER section
	// In itself this is fine, but then if the user uninstalls the SWS extension
	// then there will be a warning with every project file about unknown ext data
	// If only the year is set, don't write anything either.
	if (!g_tag_artist.empty() || !g_tag_album.empty() || !g_tag_genre.empty() ||
		!g_tag_comment.empty() || !g_render_path.empty() )
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
}

static project_config_extension_t g_projectconfig = { ProcessExtensionLine, SaveExtensionConfig, BeginLoadProjectState, NULL };

//!WANT_LOCALIZE_1ST_STRING_BEGIN:sws_actions
static COMMAND_T g_commandTable[] = {
	{ { DEFACCEL, "SWS/Shane: Batch Render Regions" },	"AUTORENDER", AutorenderRegions, "Batch Render Regions" },
	{ { DEFACCEL, "SWS/Shane: Autorender: Edit Project Metadata" }, "AUTORENDER_METADATA", ShowAutorenderMetadata, "Edit Project Metadata" },
	{ { DEFACCEL, "SWS/Shane: Autorender: Open Render Path" }, "AUTORENDER_OPEN_RENDER_PATH", OpenRenderPath, "Open Render Path" },
	{ { DEFACCEL, "SWS/Shane: Autorender: Show Instructions" }, "AUTORENDER_HELP", ShowAutorenderHelp, "Show Instructions" },
	{ { DEFACCEL, "SWS/Shane: Autorender: Global Preferences" }, "AUTORENDER_PREFERENCES", AutorenderPreferences, "Global Preferences" },
#ifdef TESTCODE
	{ { DEFACCEL, "SWS Autorender: [Internal] TestCode" }, "AUTORENDER_TESTCODE",  TestFunction, "Autorender: TestCode" },
#endif
	{ {}, LAST_COMMAND, }, // Denote end of table
};
//!WANT_LOCALIZE_1ST_STRING_END

int AutorenderInit(){
	SWSRegisterCommands(g_commandTable);

	if (!plugin_register("projectconfig",&g_projectconfig))
		return 0;
	return 1;
}

void AutorenderExit()
{
	plugin_register("-projectconfig",&g_projectconfig);
}
