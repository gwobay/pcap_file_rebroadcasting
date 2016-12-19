#pragma once

#include <string>
		
namespace hf_cwtest{
	using namespace std;	
string trim(const string& inString){
	char *sCh=inString.c_str();
	char *p0=sCh;
	char *p9=sCh+inString.length()-1;
	while (p0<p9 && (*p0 <= ' ' || *p0 > '~')) p0++;
	while (p9>p0 && (*p9 <= ' ' || *p9 > '~')) p9--;
	if (*p9 <= ' ') return string("");
	return string(p0, p9-p0+1);
}
		
}
