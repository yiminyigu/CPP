#ifndef STRINGUTILITIES_H
#define STRINGUTILITIES_H
///////////////////////////////////////////////////////////////////////
// StringUtilities.h - small, generally useful, helper classes       //
// ver 1.0                                                           //
// Language:    C++, Visual Studio 2017                              //
// Application: Most Projects, CSE687 - Object Oriented Design       //
// Author:      Ammar Salman, Syracuse University                    //
//              assalman@syr.edu                                     //
// Source:      Jim Fawcett, Syracuse University, CST 4-187          //
//              jfawcett@twcny.rr.com                                //
///////////////////////////////////////////////////////////////////////
/*
* Package Operations:
* -------------------
* This package provides functions:
* - Title(text)           display title
* - title(text)           display subtitle
* - putline(n)            display n newlines
* - trim(str)             remove leading and trailing whitespace
* - split(str, 'delim')   break string into vector of strings separated by delim char 
* - showSplit(vector)     display splits
* - sToW(str)             convert std::string to std::wstring
* - wToS(wstr)            convert std::wstring to std::string 
*
* Required Files:
* ---------------
*   StringUtilities.h
*
* Maintenance History:
* --------------------
* ver 1.1 : 11 Feb 2019
* - added string conversion functions (moved from Process.h)
* ver 1.0 : 12 Jan 2018
* - first release
* - refactored from earlier Utilities.h
*
* Notes:
* ------
* - Designed to provide all functionality in header file.
* - Implementation file only needed for test and demo.
*
* Planned Additions and Changes:
* ------------------------------
* - none yet
*/
#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <functional>
#include <locale>
#include "../../customLogger/customLogger.h"

namespace Utilities
{
  /////////////////////////////////////////////////////////////////////
  // String Helper functions
 
  //----< display underlined title >-----------------------------------

  inline void Title(bool logOpen, const std::string& text, char underline = '=')
  {
	std::ostringstream os;
	os << "\n  " << text;
	os << "\n " << std::string(text.size() + 2, underline);
	customLogging::StaticLogger::write(logOpen, os.str());
	
  }
  //----< display underlined subtitle >--------------------------------

  inline void title(bool logOpen,const std::string& text, char underline = '-')
  {
	std::ostringstream os;
	os << "\n  " << text;
	os << "\n " << std::string(text.size() + 2, underline);
	customLogging::StaticLogger::write(logOpen, os.str());
  }
  //----< display j newlines >-----------------------------------------

  inline void putline(bool logOpen, size_t j = 1)
  {
	  std::ostringstream os;
    for (size_t i = 0; i < j; ++i)
		os << "\n";
	customLogging::StaticLogger::write(logOpen, os.str());
  }
  /*--- remove whitespace from front and back of string argument ---*/
  /*
  *  - does not remove newlines
  */
  template <typename T>
  inline std::basic_string<T> trim(const std::basic_string<T>& toTrim)
  {
    if (toTrim.size() == 0)
      return toTrim;
    std::basic_string<T> temp;
    std::locale loc;
    typename std::basic_string<T>::const_iterator iter = toTrim.begin();
    while (isspace(*iter, loc) && *iter != '\n')
    {
      if (++iter == toTrim.end())
      {
        break;
      }
    }
    for (; iter != toTrim.end(); ++iter)
    {
      temp += *iter;
    }
    typename std::basic_string<T>::reverse_iterator riter;
    size_t pos = temp.size();
    for (riter = temp.rbegin(); riter != temp.rend(); ++riter)
    {
      --pos;
      if (!isspace(*riter, loc) || *riter == '\n')
      {
        break;
      }
    }
    if (0 <= pos && pos < temp.size())
      temp.erase(++pos);
    return temp;
  }

  /*--- split sentinel separated strings into a vector of trimmed strings ---*/

  template <typename T>
  inline std::vector<std::basic_string<T>> split(const std::basic_string<T>& toSplit, T splitOn = ',')
  {
    std::vector<std::basic_string<T>> splits;
    std::basic_string<T> temp;
    typename std::basic_string<T>::const_iterator iter;
    for (iter = toSplit.begin(); iter != toSplit.end(); ++iter)
    {
      if (*iter != splitOn)
      {
        temp += *iter;
      }
      else
      {
        splits.push_back(trim(temp));
        temp.clear();
      }
    }
    if (temp.length() > 0)
      splits.push_back(trim(temp));
    return splits;
  }
  /*--- show collection of string splits ------------------------------------*/

  template <typename T>
  inline void showSplits(bool logOpen,const std::vector<std::basic_string<T>>& splits)
  {
	  std::ostringstream os;
    for (auto item : splits)
    {
      if (item == "\n")
		  os << "\n--" << "newline";
      else
		  os << "\n--" << item;
    }
	os << "\n";
	customLogging::StaticLogger::write(logOpen,os.str());
  }

  //----< helper function to convert strings >---------------------------

  inline std::string wToS(const std::wstring& ws)
  {
    std::string conv;
    for (size_t i = 0; i < ws.size() -1; i++)
    {
      conv.push_back(static_cast<char>(ws[i]));
    }
    return conv;
  }
  //----< helper function to convert strings >---------------------------

  inline std::wstring sToW(const std::string& s)
  {
    std::wstring conv;
    for (auto ch : s)
    {
      conv.push_back(static_cast<wchar_t>(ch));
    }
    return conv;
  }
}
#endif
