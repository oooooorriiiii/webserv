#pragma once
#ifndef UTILS_HPP
# define UTILS_HPP

# include <iostream>
# include <fstream>
# include <istream>
# include <set>
# include <vector>
# include <sys/types.h>
# include <dirent.h>
# include <sys/types.h>
# include <sys/stat.h>
# include <unistd.h>

namespace ft {
    void                    CreateFile(std::string file, const std::string& body);
    std::set<std::string>   CreateDirectoryList(std::string directoryPath);
    void                    TrimWSP(std::string& str);
   	unsigned int	        StrBase_to_UI_(const std::string& str, std::ios_base& (*base)(std::ios_base&));
   	int	                    StrBase_to_I_(const std::string& str, std::ios_base& (*base)(std::ios_base&));

    template <typename T>
    bool    vecIncludes(const std::vector<T>& vec, T item) {
        typename std::vector<T>::const_iterator it = vec.begin();
        for (; it != vec.end() && *it != item; ++it) {;}
        return (it == vec.end() ? false : true);
    }
}

#endif