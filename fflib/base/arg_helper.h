#ifndef _ARG_HELPER_H_
#define _ARG_HELPER_H_

#include <string>
#include <vector>
#include <fstream>


#include "base/str_tool.h"

class ArgHelper
{
public:
    ArgHelper(){}
    ArgHelper(int argc, char* argv[])
    {
        for (int i = 0; i < argc; ++i)
        {
            m_args.push_back(argv[i]);
        }
        if (isEnableOption("-f"))
        {
            loadFromFile(getOptionValue("-f"));
        }
    }
    ArgHelper(std::string arg_str_)
    {
        std::vector<std::string> v;
        StrTool::split(arg_str_, v, " ");
        m_args.insert(m_args.end(), v.begin(), v.end());
        if (isEnableOption("-f"))
        {
            loadFromFile(getOptionValue("-f"));
        }
    }
    bool load(int argc, char* argv[])
    {
        for (int i = 0; i < argc; ++i)
        {
            m_args.push_back(argv[i]);
        }
        if (isEnableOption("-f"))
        {
            loadFromFile(getOptionValue("-f"));
        }
        return true;
    }
    std::string getOption(int idx_) const
    {   
        if ((size_t)idx_ >= m_args.size())
        {   
                return ""; 
        }   
        return m_args[idx_];
    }   
    bool isEnableOption(std::string opt_) const
    {
        for (size_t i = 0; i < m_args.size(); ++i)
        {
            if (opt_ == m_args[i])
            {
                    return true;
            }
        }
        return false;
    }

    std::string getOptionValue(std::string opt_) const
    {
        std::string ret;
        for (size_t i = 0; i < m_args.size(); ++i)
        {   
            if (opt_ == m_args[i])
            {   
                size_t value_idx = ++ i;
                if (value_idx >= m_args.size())
                {
                        return ret;
                }
                ret = m_args[value_idx];
                return ret;
            }   
        }	
        return ret;
    }
    std::string setOptionValue(const std::string& opt_, const std::string& ret) 
    {
        for (size_t i = 0; i < m_args.size(); ++i)
        {   
            if (opt_ == m_args[i])
            {   
                size_t value_idx = ++ i;
                if (value_idx >= m_args.size())
                {
                    m_args.push_back(ret);
                }
                return ret;
            }   
        }	
        m_args.push_back(opt_);
        m_args.push_back(ret);
        return ret;
    }
    int loadFromFile(const std::string& file_)
    {
        std::ifstream inf(file_.c_str());
        std::string all;
        std::string tmp;
        while (inf.eof() == false)
        {
            if (!getline(inf, tmp))
            {
                break;
            }
            if (tmp.empty() == false && tmp[0] == '#')//!过滤注释
            {
                continue;
            }
            //printf("get:%s\n", tmp.c_str());
            //sleep(1);
            all += tmp + " ";
            tmp.clear();
        }
        std::vector<std::string> v;
        StrTool::split(all, v, " ");
        m_args.insert(m_args.end(), v.begin(), v.end());
        return 0;
    }
private:
    std::vector<std::string>  m_args;
};

#endif

