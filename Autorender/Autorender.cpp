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
#include "../../taglib/include/tag.h"
#include "../../taglib/include/fileref.h"
#include <io.h>

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

int GetCurrentYear(){
	time_t t = 0;
	struct tm *lt = NULL;  
	t = time(NULL);
	lt = localtime( &t );
	int curYear = lt->tm_year;
	curYear = curYear + 1900;
	return curYear;
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

string GetQueuedRendersDir(){
	ostringstream qrStream;
	qrStream << GetResourcePath() << PATH_SLASH_CHAR << "QueuedRenders";
	return qrStream.str();
}

string GetCurrentRenderExtension( WDL_String *prjStr ){
	return ParseFileExtension( GetProjectParameterValueStr( prjStr, "RENDER_FILE" ) );
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
	
	const char *fnChar = fn->c_str();

	for( int i = 0; i < strlen( fnChar ); i++ ){
		for( int j = 0; j < strlen( illegalChars ); j++ ){
			if( fnChar[i] == illegalChars[j] ){
				fn->replace( i, 1, "_");
				break;
			}
		}
	}
}

void ShowAutorenderHelp(COMMAND_T*) {
	string helpText = "Preliminary Autorender Usage:\n\n";
	helpText += "Regions are rendered to seperate audio files.\n";
	helpText += "Region names are used for track names when tagging.\n\n";
	helpText += "To set other tagging parameters, put the following in\n";
	helpText += "the project notes on their own lines like so:\n\n";
	helpText += "TAG_ARTIST Captain Beefheart\n";
	helpText += "TAG_ALBUM Trout Mask Replica\n";
	helpText += "TAG_YEAR 1969\n";
	helpText += "TAG_GENRE WTF\n";
	helpText += "TAG_COMMENT Again, WTF\n\n";
	helpText += "You can also specify the output directory:\n";
	helpText += "RENDER_PATH C:\\Renders\n";
	helpText += "(Mac users may have to do this, I dunno).\n\n";
	helpText += "Also note that Autorender uses your last used render settings,\n";
	helpText += "so make sure that your last manual render used the settings you want.\n\n";
	helpText += "Sorry for the crudeness, will get better, thx.\n";
	MessageBox( GetMainHwnd(), helpText.c_str(), "Basic Autorender Usage", MB_OK );
}

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

	//Get params in the project notes
	string renderPath = GetProjectNotesParameter( prjStr, "RENDER_PATH" );
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

	// remove PATH_SLASH_CHAR from end of string if it exists
	EnsureStrDoesntEndWith( renderPath, PATH_SLASH_CHAR );

	// render path was specified and doesn't exist
	if( !renderPath.empty() && !FileExists( renderPath.c_str() ) ){
		string message = "Render path " + renderPath + " doesn't exist!";
		renderPath.clear();
		MessageBox( GetMainHwnd(), message.c_str(), "Autorender: Bad Render Path", MB_OK );
	}

	while( !FileExists( renderPath.c_str() ) ){
		char renderPathChar[1024] = "";		
		//Prob windows only, what to do? Couldn't figure out how to use WDL_ChooseDirectory (got linker errors), maybe not xplatform anyway
		BrowseForDirectory( "Select render output directory", NULL, renderPathChar, 1024 );
		if( strlen( renderPathChar ) == 0 ){
			//Show error
			return;
		}
		renderPath = renderPathChar;
	}

	// append PATH_SLASH_CHAR from end of string if it doens't exist
	EnsureStrEndsWith( renderPath, PATH_SLASH_CHAR );

	string queuedRendersDir = GetQueuedRendersDir();
	// Would be good to delete all files in the render queue directory here and at the end, 
	// but apparently deleting files is not so easy...

	ostringstream outRenderProjectPrefixStream;
	outRenderProjectPrefixStream << queuedRendersDir << PATH_SLASH_CHAR << "render_Autorender";
	string outRenderProjectPrefix = outRenderProjectPrefixStream.str();
	
	//init the stuff we need for the region loop
	int project_index = 0, marker_index = 0, track_index = 0, idx;
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
			renderTrack.sanitizedTrackName = trackName.str();
			SanitizeFilename( &renderTrack.sanitizedTrackName );
			
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
		string renderedFilePath = renderPath + renderTracks[i].getFileName( renderFileExtension );						
		
		TagLib::FileRef f( renderedFilePath.c_str() );
		if( !f.isNull() ){
			if( !tag_artist.empty() ) f.tag()->setArtist( tag_artist );
			if( !tag_album.empty() ) f.tag()->setAlbum( tag_album );
			if( !tag_genre.empty() ) f.tag()->setGenre( tag_genre );
			if( tag_year > 0 ) f.tag()->setYear( tag_year );
			if( !tag_comment.empty() ) f.tag()->setComment( tag_comment );
			
			f.tag()->setTrack( i + 1 );
			f.tag()->setTitle( renderTracks[i].trackName );
			f.save();
		} else {
			//throw error
		}
	}

#ifdef _WIN32
	system( ( "explorer " + renderPath ).c_str() );
#else
	//Mac open render directory?
#endif

}

static project_config_extension_t g_projectconfig = { NULL };

static COMMAND_T g_commandTable[] = 
{
	{ { DEFACCEL, "Autorender: Batch Render Regions" },	"AUTORENDER", AutorenderRegions, "Batch Render Regions" },
	{ { DEFACCEL, "Autorender: Show Instructions" },	"AUTORENDER_HELP", ShowAutorenderHelp, "Show Instructions" },
#ifdef TESTCODE
	{ { DEFACCEL, "Autorender: TestCode" }, "AUTORENDER_TESTCODE",  TestFunction, "Autorender: TestCode" },
#endif
	{ {}, LAST_COMMAND, }, // Denote end of table
};

static void menuhook(const char* menustr, HMENU hMenu, int flag)
{
	if (strcmp(menustr, "Main file") == 0 && flag == 0){
		//Get ID for Show Render Queue so we don't move around as the menu changes?
		AddSubMenu(hMenu, SWSCreateMenu( g_commandTable), "Autorender", -8 );
	}
/*	
		for( int i = 0; i < len; i++ ){
			AddToMenu(hMenu, g_commandTable[i].menuText, g_commandTable[i].accel.accel.cmd);
		}
	*/

}

int AutorenderInit(){
	SWSRegisterCommands(g_commandTable);

	if (!plugin_register("projectconfig",&g_projectconfig))
		return 0;

    if (!plugin_register("hookcustommenu", (void*)menuhook))
        return 0;

	return 1;
}