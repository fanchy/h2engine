#ifndef _STRTOOL_H_
#define _STRTOOL_H_


#include <string.h>
#include <iostream>
#include <sstream>
#include <vector>


struct StrTool
{
    static std::string trim(const std::string& str)
    {
        std::string::size_type pos = str.find_first_not_of(' ');
        if (pos == std::string::npos)
        {
            return str;
        }
        std::string::size_type pos2 = str.find_last_not_of(' ');
        if (pos2 != std::string::npos)
        {
            return str.substr(pos, pos2 - pos + 1);
        }
        return str.substr(pos);
    }

    static int split(const std::string& str, std::vector<std::string>& ret_, std::string sep = ",")
    {
        if (str.empty())
        {
            return 0;
        }

        std::string tmp;
        std::string::size_type pos_begin = str.find_first_not_of(sep);
        std::string::size_type comma_pos = 0;

        while (pos_begin != std::string::npos)
        {
            comma_pos = str.find(sep, pos_begin);
            if (comma_pos != std::string::npos)
            {
                tmp = str.substr(pos_begin, comma_pos - pos_begin);
                pos_begin = comma_pos + sep.length();
            }
            else
            {
                tmp = str.substr(pos_begin);
                pos_begin = comma_pos;
            }

            if (!tmp.empty())
            {
                ret_.push_back(tmp);
                tmp.clear();
            }
        }
        return 0;
    }

    static std::string replace(const std::string& str, const std::string& src, const std::string& dest)
    {
        std::string ret;

        std::string::size_type pos_begin = 0;
        std::string::size_type pos       = str.find(src);
        while (pos != std::string::npos)
        {
            //cout <<"replacexxx:" << pos_begin <<" " << pos <<"\n";
            ret.append(str.c_str() + pos_begin, pos - pos_begin);
            ret += dest;
            pos_begin = pos + src.length();
            pos       = str.find(src, pos_begin);
        }
        if (pos_begin < str.length())
        {
            ret.append(str.begin() + pos_begin, str.end());
        }
        return ret;
    }

    size_t utf8_words_num(const char* s_)
    {
        size_t ret = 0;
        const char* p = s_;
        for (unsigned char c = (unsigned char)(*p); c != 0; c = (unsigned char)(*p))
        {
            ++ret;
            if (c <= 127)
            {
                p += 1;
            }
            else if (c < 192)
            {
                p   += 2;
            }
            else if (c < 223)
            {
                p   += 3;
            }
            else
            {
                p   += 4;
            }
        }
        return ret;
    }
    template <typename T>
    static std::string num2str(T v){
        std::ostringstream oss;
        oss << v;
        return oss.str();
    }
    static bool loadCsvFromString(const std::string& data, std::vector<std::vector<std::string> >& ret, std::string delimiter = ",")
	{
		std::string linesep = "\n";
		std::string::size_type pos = data.find(linesep);
		if (pos != std::string::npos && pos > 0 && data[pos - 1] == '\r'){
			linesep = "\r\n";
		}

		std::vector<std::string> allLines;
		StrTool::split(data, allLines, linesep);
		std::vector<std::string> tmpRow;
		for (size_t i = 0; i < allLines.size(); ++i){
			ret.push_back(tmpRow);
			StrTool::split(allLines[i], ret.back(), delimiter);

			for (size_t j = 0; j < ret.back().size(); ++j){
				std::string& eachFiled = ret.back()[j];
				if (eachFiled.empty() == false && eachFiled[0] == '"'){
					std::string newVal;
					size_t m = 0;
					while (m < eachFiled.size()){
						if (eachFiled[m] == '"'){
							if (m + 1 < eachFiled.size() && eachFiled[m + 1] == '"'){
								newVal += '"';
								++m;
							}
						}
						else{
							newVal += eachFiled[m];
						}
						++m;
					}
					eachFiled = newVal;
				}
			}
		}
		return true;
	}
};

typedef StrTool StrTool_t;
#endif

