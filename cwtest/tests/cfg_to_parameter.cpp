#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/eventfd.h>

#include <netinet/tcp.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <ITCH/functions.h>
#include <Util/Parameter.h>
#include <Util/Network_Utils.h>
#include <Util/File_Utils.h>

#include <initializer_list>

#include <boost/iostreams/device/file.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/tokenizer.hpp>
#include <boost/foreach.hpp>
#include <boost/algorithm/string/trim_all.hpp>

#include <stdarg.h>
#include <stdint.h>
#include <chrono>
#include <thread>
#include <vector>


using namespace std;
using namespace hf_cwtest;
using namespace hf_tools;

struct current_date_time{
	string YYYY;
	string MM;
	string DD;
	string hh;
	string mm;
	string ss;
	string timeString;
	current_date_time(){
	    time_t     tnow;
	    time(&tnow);
	    struct tm*  t_now = localtime(&tnow); 
	    //time zone should be set first
	    YYYY = std::to_string(t_now->tm_year+1900);
	    MM = std::to_string(t_now->tm_mon +101).substr(1,2);
	    DD = std::to_string(t_now->tm_mday +100).substr(1,2);
	    hh = std::to_string(t_now->tm_hour +100).substr(1,2);
	    mm = std::to_string(t_now->tm_min +100).substr(1,2);
	    ss = std::to_string(t_now->tm_sec +100).substr(1,2);
	}
};

static Parameter m_parameters;
static void replace_c(string& aLine, char old_c, char new_c=' ')
{
	for (string::iterator it=aLine.begin(); it != aLine.end(); it++){
                if (*it == old_c) *it=new_c;
	}
}

static size_t occurrence_c(const char* line, char which){
	size_t k=0;
	while (*line){
		if (*line++ == which) k++;
	}
	return k;
}

static void get_tokens(const string& aLine, vector<string>& list, char delim=' ')
{
    const char *p0=aLine.c_str();
    const char *pN=p0;
    bool line_end=false;
    while (!line_end){
        pN=p0;
        while (*pN && *pN != delim ) pN++;
        if (!*pN) line_end=true;
	if (pN-p0)
        list.push_back(trim(string(p0, size_t(pN-p0))));
        if (line_end) break;
        p0=++pN;
    }
}

string calculate_multiply_terms(const string& aLine)
{
	vector<string> tokens;
	get_tokens(aLine, tokens, '*');
	double tmpV=1;
	for (size_t p=0; p<tokens.size(); p++){
		string sVal=tokens[p];
		replace_c(sVal, '"');
		replace_c(sVal, '\'');
		sVal=trim(sVal);
		if (sVal[0] == '$')
		{
			string var=sVal.substr(1);
			if (m_parameters.has(var)) sVal=m_parameters[var].getString();
		}
		tmpV *= stod(sVal);
	}
	return to_string(tmpV);
}

string joint_terms(const string& aLine, char sep='+')
{
	vector<string> tokens;
	get_tokens(aLine, tokens, sep);
	string retS;
	for (size_t p=0; p<tokens.size(); p++){
		string sVal=tokens[p];
		replace_c(sVal, '"');
		replace_c(sVal, '\'');
		sVal=trim(sVal);
		if (sVal[0] == '$')
		{
			string var=sVal.substr(1);
			if (m_parameters.has(var)) sVal=m_parameters[var].getString();
		}
		retS += sVal;
	}
	return retS;
}

string calculate_sum_terms(const string& aLine)
	//diff from join by checking if having '/" in calling function
{
	vector<string> tokens;
	get_tokens(aLine, tokens, '*');
	size_t tmpV=0;
	for (size_t p=0; p<tokens.size(); p++){
		string sVal=tokens[p];
		replace_c(sVal, '"');
		replace_c(sVal, '\'');
		sVal=trim(sVal);
		if (sVal[0] == '$')
		{
			string var=sVal.substr(1);
			if (m_parameters.has(var)) sVal=m_parameters[var].getString();
		}
		try {
			tmpV += stol(sVal);
		} catch (...){
			return joint_terms(aLine);
		}
	} 
	return to_string(tmpV);
}

void read_vector_param_value(Parameter& v_param_to_fill, string& textBuf, FILE *cfg)
{
	//textBuf has '['
	char cfgbuf[160];
   	while (!feof(cfg) && textBuf.find(']') == string::npos){ 
		fgets(cfgbuf, 160, cfg); 
		//cout << cfgbuf << endl;
		char *pH=strchr(cfgbuf, '#'); 
		if (pH) *pH=0; 
		string aLine =trim(cfgbuf); 
		if (aLine.find(',')==string::npos)
			aLine += ',';
		textBuf += aLine;
   	}
	size_t sq_left=textBuf.find('[');
	if (sq_left == string::npos) sq_left=0;
	else sq_left++;
	size_t sq_right=textBuf.find(']');
	sq_right--;
	string v_text=textBuf.substr(sq_left, sq_right-sq_left+1);
	if (v_text.find('{') != string::npos){
		vector<Parameter> paramV;
		vector<string> tokens;
		get_tokens(v_text, tokens, '}');
		for (size_t s=0; s<tokens.size(); s++){
			if (tokens[s].length() < 2) continue;
			vector<string> pairs;
			size_t brace_pos=tokens[s].find('{');
			if (brace_pos == string::npos) brace_pos=0;
			else brace_pos++;
			get_tokens(tokens[s].substr(brace_pos), pairs, ',');
			Parameter h_param;
			hash_map<string, Parameter> kv;
			for (size_t p=0; p<pairs.size(); p++){
				size_t i_col=pairs[p].find(':');
				string sKey=pairs[p].substr(0, i_col);
				replace_c(sKey, '\'');
				replace_c(sKey, '\"');
				sKey=trim(sKey);
				string sVal=pairs[p].substr(i_col+1);
				replace_c(sVal, '\'');
				replace_c(sVal, '\"');
				sVal=trim(sVal);
				if (sVal[0] == '$')
				{
					string var=sVal.substr(1);
					if (m_parameters.has(var))
					sVal=m_parameters[var].getString();
				}
				Parameter param_value;
				param_value.setString(sVal);
				kv[sKey]=param_value;
			}
			h_param.setDict(kv);
			paramV.push_back(h_param);
		}
		v_param_to_fill.setVector(paramV);
		return;
	} 
	//if (textBuf.find('\'') != string::npos || textBuf.find('"') != string::npos){
		//vector of string
	vector<string> paramV;
	vector<string> v_data;
	get_tokens(v_text, v_data, ',');
	for (size_t v=0; v< v_data.size(); v++){
		string var=trim(v_data[v]);
		if (var.find('+') != string::npos){
			if (var.find('\'') != string::npos || var.find('"') != string::npos)
			{
				paramV.push_back(joint_terms(var));
				continue; 
			}
			var = calculate_sum_terms(var);
		}
		replace_c(var, '\'');
		replace_c(var, '"');
		var=trim(var);
		if (var[0]=='$'){
			string vk=v_data[v].substr(1);
			if (m_parameters.has(vk)) var=m_parameters[vk].getString();
		}
		paramV.push_back(var);
	}
	v_param_to_fill.setStringVector(paramV);
	return;

/* temporarily treat all as string and let apps do the convertion
	 * else if (textBuf.find('.') != string::npos){
		//vector of double
		vector<string> v_data;
		get_tokens(textBuf, v_data, ',');
		vector<double> doubleV;
		Parameter d_value;
		d_value.setDoubleVector(doubleV);
		for (size_t v=0; v< v_data.size(); v++){
			doubleV.push_back(stod(v_data[v]));
		}
		text_buf.clear();
		return;
	} else {
		vector<string> v_data;
		get_tokens(textBuf, v_data, ',');
		vector<int> intV;
		Parameter i_value;
		i_value.setIntVector(IntV);
		for (size_t v=0; v< v_data.size(); v++){
			try {
			intV.push_back(stol(v_data[v]));
			}
			catch(...){
				cout << "Bad integer " << v_data[v] << endl;
			}
		}
	}
	*/
}

void read_hash_param_value(Parameter& h_param, FILE *cfg)
	//called by the case key:{
	//which will make the pair key:h_param;
	//looking for the closing brace }
	//to complete h_param
{
	hash_map<string, Parameter>& p_dict=h_param.getDict();
	size_t lq_count=1;
	size_t rq_count=0;
	string text_buf;
	char cfgbuf[160];
   	while (!feof(cfg) && lq_count > 0){
		fgets(cfgbuf, 160, cfg); 
		//cout << cfgbuf << endl;
		char *pH=strchr(cfgbuf, '#'); 
		if (pH) *pH=0; 
		if (strlen(cfgbuf) < 1) continue;
		char* pCol=cfgbuf; 
		lq_count += occurrence_c(pCol, '{');
		rq_count += occurrence_c(pCol, '}'); 
		lq_count -= rq_count;
		text_buf += trim(cfgbuf);
		if (pCol[strlen(pCol)-1]==':') continue;
		size_t iCol=text_buf.find(':');
		if (iCol == string::npos){
			//throw runtime_error("hash value has no key");
			continue;
		}
		if (iCol==text_buf.length()-1) continue;
		cout << m_parameters << endl;
		string key=trim(text_buf.substr(0, iCol));
		replace_c(key, '\'');
		replace_c(key, '"');
		key=trim(key);
		Parameter& p_value=p_dict[key];
		if (text_buf.find('{') != string::npos){
			p_value.setDict(hash_map<string, Parameter>());
			hash_map<string, Parameter>& dict=p_value.getDict();
			read_hash_param_value(p_value, cfg);
			//p_dict[key]=p_value;
			text_buf.clear();
			lq_count--;
			continue;
		}
		size_t sq_pos=text_buf.find('[');
		if (sq_pos != string::npos){
			string v_buf=text_buf.substr(sq_pos);
			read_vector_param_value(p_value, v_buf, cfg);
			//p_dict[key]=v_value;
			text_buf.clear();
			continue;
		}
		string var=trim(text_buf.substr(iCol+1));
		if (var.find('+') != string::npos){
			if (var.find('\'') != string::npos || var.find('"') != string::npos)
			{
				p_value.setString(joint_terms(var));
				text_buf.clear();
				continue; 
			}
			var = calculate_sum_terms(var);
		}
		if (var.find('*') != string::npos){
			p_value.setString(calculate_multiply_terms(var));
			text_buf.clear();
			continue;
		}
		replace_c(var, '\'');
		replace_c(var, '"');
		var=trim(var);
		if (var[0]=='$'){
			string vk=var.substr(1);
			if (m_parameters.has(vk)) var=m_parameters[vk].getString();
		}
		p_value.setString(var);
		text_buf.clear();
/*
		if (val.find('\'') != string::npos || val.find('"') != string::npos){
			char qt=val[0];
			replace_c(val, '\'');
			replace_c(val, '"');
			val=trim(val);
		}
			//Parameter s_value;
		if (val[0]=='$'){
			string vk=val.substr(1);
			if (m_parameters.has(vk))
			val=m_parameters[vk].getString();
		}
		// temporary treat all as string and let app do conversion
		p_value.setString(trim(val));
			//p_dict[key]=s_value;
		text_buf.clear();
			//continue;
		*/
	
		/* temporary treat all as string and let app do conversion
		if (val.find('.') != string::npos){
			Parameter d_value;
			d_value.setDouble(stod(val));
			p_dict[key]=d_value;
			text_buf.clear();
			continue;
		}
		Parameter i_value;
		try {
			i_value.setInt(stol(val));
			p_dict[key]=i_value;
		} catch (...){
			cout << "Bad integer " << val << endl;
		}
		text_buf.clear();
		*/
	}
}

int read_cfg(char* file_name)
{
	Parameter& aParam=m_parameters;
	FILE* cfg=0;
	cfg=fopen(file_name, "r");
	if (!cfg){
		fprintf(stderr, "%s config_file_name unknown\n", file_name);
                exit(EXIT_FAILURE);
	}
	aParam.setDict(hash_map<string, Parameter>());
	hash_map<string, Parameter>& h_mp=aParam.getDict();
	//m_parameters=aParam;
	//TO-DO .... set CW Global parameter to h_mp
	//       1. TIME_ZONE (TZ)
	//       2. TODAY : YYYY, MM, DD (by TZ)
	struct current_date_time today;
	Parameter& tmp=h_mp["YYYY"];
	tmp.setString(today.YYYY);
	cout << aParam << endl;
	Parameter& mmp=h_mp["MM"];
	mmp.setString(today.MM);
	cout << aParam << endl;
	Parameter& dmp=h_mp["DD"];
	dmp.setString(today.DD);
	Parameter& kmp=h_mp["YYYYMMDD"];
	kmp.setString(today.YYYY+today.MM+today.DD);
	cout << aParam << endl;


	//       3. G-TODAY : GYYYY, GMM, GDD (TZ(0))
	//also need to consider when $key; currently only $value
	read_hash_param_value(aParam, cfg);
	cout << m_parameters << endl;
}

int main(int argc, char **argv)
{
	if (argc != 2) {
		fprintf(stderr, "Usage: %s config_file_name\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	//string config=argv[1];
	read_cfg(argv[1]);//config);
	return 0;
}

