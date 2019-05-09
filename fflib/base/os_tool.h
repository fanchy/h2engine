#ifndef _FF_OS_TOOL_H_
#define _FF_OS_TOOL_H_

#include <unistd.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>

#include <string>
#include <vector>


namespace ff
{

struct OSTool
{
    static bool ls(const std::string& path_, std::vector<std::string>& ret_)
    {
        const char* dir = path_.c_str();
        DIR *dp;
        struct dirent *entry;

        if((dp = opendir(dir)) == NULL) {
            fprintf(stderr,"cannot open directory: %s\n", dir);
            return false;
        }

        while((entry = readdir(dp)) != NULL) {
             if(strcmp(".",entry->d_name) == 0 || strcmp("..",entry->d_name) == 0)
                continue;
            ret_.push_back(entry->d_name);
        }
        closedir(dp);
        return true;
    }
    static bool isDir(const std::string& path)
    {
        struct stat buf;
        if (::stat(path.c_str(), &buf)){
            return false;
        }
        return S_ISDIR(buf.st_mode);
    } 
    static bool isFile(const std::string& path){
        struct stat buf;
        if (::stat(path.c_str(), &buf)){
            return false;
        }
        return S_ISREG (buf.st_mode);
    }
    static bool readFile(const std::string& name, std::string& ret){
        FILE* file = ::fopen(name.c_str(), "rb");
        if (file == NULL) return false;

        ::fseek(file, 0, SEEK_END);
        int size = ::ftell(file);
        ::rewind(file);

        char* chars = new char[size + 1];
        chars[size] = '\0';
        for (int i = 0; i < size;) {
            int read = ::fread(&chars[i], 1, size - i, file);
            i += read;
        }

        ::fclose(file);
        ret.append(chars, size);
        delete [] chars;
        chars = NULL;
        return true;
    }
};
}

#endif
