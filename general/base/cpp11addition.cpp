#include "cpp11addition.h"
#include <sstream>
#include <cassert>
#include <stdlib.h>
#include <limits.h>
#include <stdlib.h>
#include <iostream>

static const std::regex isaunsignednumber("^[0-9]+$");
static const std::regex isasignednumber("^-?[0-9]+$");
static const std::regex isadouble("^-?[0-9]+(\\.[0-9]+)?$");
static const std::regex ishexa("^([0-9a-fA-F][0-9a-fA-F])+$");
#ifdef _WIN32
static const std::regex regexseparators("[/\\]+");
#endif

static const std::string base64_chars =
             "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
             "abcdefghijklmnopqrstuvwxyz"
             "0123456789+/";

static inline bool is_base64(unsigned char c) {
  return (isalnum(c) || (c == '+') || (c == '/'));
}

static const char* const lut = "0123456789ABCDEF";

std::size_t pairhash::operator()(const std::pair<uint8_t, uint8_t> &x) const
{
    return (x.first << 8) + x.second;
}

bool stringreplaceOne(std::string& str, const std::string& from, const std::string& to)
{
    const size_t start_pos = str.find(from);
    if(start_pos == std::string::npos)
        return false;
    str.replace(start_pos, from.length(), to);
    return true;
}

uint8_t stringreplaceAll(std::string& str, const std::string& from, const std::string& to)
{
    if(from.empty())
        return 0;
    size_t start_pos = 0;
    uint8_t count=0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
        count++;
    }
    return count;
}

std::vector<std::string> stringregexsplit(const std::string& input, const std::regex& regex)
{
    // passing -1 as the submatch index parameter performs splitting
    std::sregex_token_iterator
        first{input.begin(), input.end(), regex, -1},
        last;
    return {first, last};
}

std::vector<std::string> stringsplit(const std::string &s, char delim)
{
    std::vector<std::string> elems;

    std::string::size_type i = 0;
    std::string::size_type j = s.find(delim);

    if(j == std::string::npos)
    {
        if(!s.empty())
            elems.push_back(s);
        return elems;
    }
    else
    {
        while (j != std::string::npos) {
           elems.push_back(s.substr(i, j-i));
           i = ++j;
           j = s.find(delim, j);

           if (j == std::string::npos)
              elems.push_back(s.substr(i, s.length()));
        }
        return elems;
    }
}

bool stringEndsWith(std::string const &fullString, std::string const &ending)
{
    if (fullString.length() >= ending.length()) {
        return (0 == fullString.compare (fullString.length() - ending.length(), ending.length(), ending));
    } else {
        return false;
    }
}

bool stringEndsWith(std::string const &fullString, char const &ending)
{
    if (fullString.length()>0) {
        return fullString[fullString.size()-1]==ending;
    } else {
        return false;
    }
}

bool stringStartWith(std::string const &fullString, std::string const &starting)
{
    if (fullString.length() >= starting.length()) {
        return (fullString.substr(0,starting.length())==starting);
    } else {
        return false;
    }
}

bool stringStartWith(std::string const &fullString, char const &starting)
{
    if (fullString.length()>0) {
        return fullString[0]==starting;
    } else {
        return false;
    }
}

std::string& stringimplode(const std::vector<std::string>& elems, char delim, std::string& s)
{
    for (std::vector<std::string>::const_iterator ii = elems.begin(); ii != elems.cend(); ++ii)
    {
        s += (*ii);
        if ( ii + 1 != elems.end() ) {
            s += delim;
        }
    }

    return s;
}

std::string stringimplode(const std::vector<std::string>& elems, char delim)
{
    std::string s;
    return stringimplode(elems, delim, s);
}

std::string stringimplode(const std::queue<std::string>& elems, char delim)
{
    std::string newString;
    std::queue<std::string> copy=elems;
    unsigned int count=0;
    while(!copy.empty())
    {
        if(count>0)
            newString+=delim;
        newString+=copy.front();
        copy.pop();
        ++count;
    }

    return newString;
}

std::string stringimplode(const std::vector<std::string>& elems, const std::string &delim)
{
    std::string newString;
    for (std::vector<std::string>::const_iterator ii = elems.begin(); ii != elems.cend(); ++ii)
    {
        newString += (*ii);
        if ( ii + 1 != elems.end() ) {
            newString += delim;
        }
    }
    return newString;
}

uint8_t stringtouint8(const std::string &string,bool *ok)
{
    if(Q_LIKELY(std::regex_match(string,isaunsignednumber)))
    {
        if(Q_LIKELY(ok!=NULL))
            *ok=true;
        const unsigned int &tempValue=std::stoull(string);
        if(Q_LIKELY(tempValue<=0xFF))
            return tempValue;
        else
        {
            if(ok!=NULL)
                *ok=false;
            return 0;
        }
    }
    else
    {
        if(ok!=NULL)
            *ok=false;
        return 0;
    }
}

uint16_t stringtouint16(const std::string &string,bool *ok)
{
    if(Q_LIKELY(std::regex_match(string,isaunsignednumber)))
    {
        if(Q_LIKELY(ok!=NULL))
            *ok=true;
        const unsigned int &tempValue=std::stoull(string);
        if(Q_LIKELY(tempValue<=0xFFFF))
            return tempValue;
        else
        {
            if(ok!=NULL)
                *ok=false;
            return 0;
        }
    }
    else
    {
        if(ok!=NULL)
            *ok=false;
        return 0;
    }
}

uint32_t stringtouint32(const std::string &string,bool *ok)
{
    if(Q_LIKELY(std::regex_match(string,isaunsignednumber)))
    {
        if(Q_LIKELY(ok!=NULL))
            *ok=true;
        const unsigned int &tempValue=std::stoull(string);
        if(Q_LIKELY(tempValue<=0xFFFFFFFF))
            return tempValue;
        else
        {
            if(ok!=NULL)
                *ok=false;
            return 0;
        }
    }
    else
    {
        if(ok!=NULL)
            *ok=false;
        return 0;
    }
}

bool stringtobool(const std::string &string,bool *ok)
{
    if(string=="1")
    {
        if(ok!=NULL)
            *ok=true;
        return true;
    }
    if(string=="0")
    {
        if(ok!=NULL)
            *ok=true;
        return false;
    }
    if(string=="true" || string=="TRUE")
    {
        if(ok!=NULL)
            *ok=true;
        return true;
    }
    if(string=="false" || string=="FALSE")
    {
        if(ok!=NULL)
            *ok=true;
        return false;
    }
    if(ok!=NULL)
        *ok=false;
    return false;
}

uint64_t stringtouint64(const std::string &string,bool *ok)
{
    if(Q_LIKELY(std::regex_match(string,isaunsignednumber)))
    {
        if(Q_LIKELY(ok!=NULL))
            *ok=true;
        return std::stoull(string);
    }
    else
    {
        if(ok!=NULL)
            *ok=false;
        return 0;
    }
}

int8_t stringtoint8(const std::string &string,bool *ok)
{
    if(Q_LIKELY(std::regex_match(string,isasignednumber)))
    {
        if(Q_LIKELY(ok!=NULL))
            *ok=true;
        const unsigned int &tempValue=std::stoi(string);
        if(Q_LIKELY(tempValue<=0x7F))
            return tempValue;
        else
        {
            if(ok!=NULL)
                *ok=false;
            return 0;
        }
    }
    else
    {
        if(ok!=NULL)
            *ok=false;
        return 0;
    }
}

int16_t stringtoint16(const std::string &string,bool *ok)
{
    if(Q_LIKELY(std::regex_match(string,isasignednumber)))
    {
        if(Q_LIKELY(ok!=NULL))
            *ok=true;
        const unsigned int &tempValue=std::stoi(string);
        if(Q_LIKELY(tempValue<=0x7FFF))
            return tempValue;
        else
        {
            if(ok!=NULL)
                *ok=false;
            return 0;
        }
    }
    else
    {
        if(ok!=NULL)
            *ok=false;
        return 0;
    }
}

int32_t stringtoint32(const std::string &string,bool *ok)
{
    if(Q_LIKELY(std::regex_match(string,isasignednumber)))
    {
        if(Q_LIKELY(ok!=NULL))
            *ok=true;
        return std::stoi(string);
    }
    else
    {
        if(ok!=NULL)
            *ok=false;
        return 0;
    }
}

int64_t stringtoint64(const std::string &string,bool *ok)
{
    if(Q_LIKELY(std::regex_match(string,isasignednumber)))
    {
        if(Q_LIKELY(ok!=NULL))
            *ok=true;
        return std::stoll(string);
    }
    else
    {
        if(ok!=NULL)
            *ok=false;
        return 0;
    }
}

float stringtofloat(const std::string &string,bool *ok)
{
    if(Q_LIKELY(std::regex_match(string,isadouble)))
    {
        if(Q_LIKELY(ok!=NULL))
            *ok=true;
        return std::stof(string);
    }
    else
    {
        if(ok!=NULL)
            *ok=false;
        return 0;
    }
}

double stringtodouble(const std::string &string,bool *ok)
{
    if(Q_LIKELY(std::regex_match(string,isadouble)))
    {
        if(Q_LIKELY(ok!=NULL))
            *ok=true;
        return std::stod(string);
    }
    else
    {
        if(ok!=NULL)
            *ok=false;
        return 0;
    }
}

std::string binarytoHexa(const std::vector<char> &data)
{
    std::string output;
    output.reserve(2*data.size());
    for(size_t i=0;i<data.size();++i)
    {
        const unsigned char c = data[i];
        output.push_back(lut[c >> 4]);
        output.push_back(lut[c & 15]);
    }
    return output;
}

std::string binarytoHexa(const char * const data,const uint32_t &size)
{
    std::string output;
    output.reserve(2*size);
    for(size_t i=0;i<size;++i)
    {
        const unsigned char c = data[i];
        output.push_back(lut[c >> 4]);
        output.push_back(lut[c & 15]);
    }
    return output;
}

uint8_t hexToDecUnit(const std::string& data)
{
     auto fromHex = [](char c)
     {
        if (isdigit(c)) return c - '0';
        switch(c)
        {
            case '0':
                return 0;
            case '1':
                return 1;
            case '2':
                return 2;
            case '3':
                return 3;
            case '4':
                return 4;
            case '5':
                return 5;
            case '6':
                return 6;
            case '7':
                return 7;
            case '8':
                return 8;
            case '9':
                return 9;

            case 'a':
            case 'A':
                return 10;
            case 'b':
            case 'B':
                return 11;
            case 'c':
            case 'C':
                return 12;
            case 'd':
            case 'D':
                return 13;
            case 'e':
            case 'E':
                return 14;
            case 'f':
            case 'F':
                return 15;
        }
        return 0;
    };
    return fromHex(data[0]) << 4 | fromHex(data[1]);
}

std::vector<char> hexatoBinary(const std::string &data)
{
    if(Q_LIKELY(std::regex_match(data,ishexa)))
    {
        std::vector<char> out;
        out.reserve(data.length()/2);
        for(size_t i=0;i<data.length();i+=2)
        {
            const std::string &partpfchain=data.substr(i,2);
            uint8_t x=hexToDecUnit(partpfchain);
            out.push_back(x);
        }
        return out;
    }
    else
        return std::vector<char>();
}

void binaryAppend(std::vector<char> &data,const std::vector<char> &add)
{
    if(add.empty())
        return;
    if(data.empty())
    {
        data=add;
        return;
    }
    const int oldsize=data.size();
    data.resize(oldsize+add.size());
    memcpy(data.data()+oldsize,add.data(),add.size());
}

void binaryAppend(std::vector<char> &data,const char * const add,const uint32_t &addSize)
{
    if(addSize==0)
        return;
    if(data.empty())
    {
        data.resize(addSize);
        memcpy(data.data(),add,addSize);
        return;
    }
    const int oldsize=data.size();
    data.resize(oldsize+addSize);
    memcpy(data.data()+oldsize,add,addSize);
}

std::vector<char> base64toBinary(const std::string &string)
{
    int index=0;
    int sub_index=0;
    int encoded_string_remaining=string.size();
    int encoded_string_pos=0;
    unsigned char char_array_4[4], char_array_3[3];
    std::vector<char> ret;

    while(encoded_string_remaining-- && (string[encoded_string_pos]!='=') && is_base64(string[encoded_string_pos]))
    {
        char_array_4[index++]=string[encoded_string_pos];
        encoded_string_pos++;
        if(index==4)
        {
            for(index=0;index<4;index++)
                char_array_4[index]=base64_chars.find(char_array_4[index]);

            char_array_3[0]=(char_array_4[0]<<2) + ((char_array_4[1]&0x30)>>4);
            char_array_3[1]=((char_array_4[1]&0xf)<<4) + ((char_array_4[2]&0x3c)>>2);
            char_array_3[2]=((char_array_4[2]&0x3)<<6) + char_array_4[3];

            for(index=0;(index<3);index++)
                ret.push_back(char_array_3[index]);

            index=0;
        }
    }

    if(index)
    {
        for(sub_index=index;sub_index<4;sub_index++)
            char_array_4[sub_index]=0;

        for(sub_index=0;sub_index<4;sub_index++)
            char_array_4[sub_index]=base64_chars.find(char_array_4[sub_index]);

        char_array_3[0]=(char_array_4[0]<<2) + ((char_array_4[1]&0x30)>>4);
        char_array_3[1]=((char_array_4[1]&0xf)<<4) + ((char_array_4[2]&0x3c)>>2);
        char_array_3[2]=((char_array_4[2]&0x3)<<6) + char_array_4[3];

        for (sub_index=0;(sub_index<index-1);sub_index++)
            ret.push_back(char_array_3[sub_index]);
    }

    return ret;
}


std::string FSabsoluteFilePath(const std::string &string)
{
    std::string newstring=string;
    stringreplaceAll(newstring,"//","/");
    #ifdef _WIN32
    stringreplaceAll(newstring,"\\\\","\\");
    std::vector<std::string> parts=stringregexsplit(newstring,regexseparators);
    #else
    std::vector<std::string> parts=stringsplit(newstring,'/');
    #endif

    #ifndef _WIN32
    unsigned int index=1;
    #else
    unsigned int index=2;
    #endif
    while(index<parts.size())
    {
        if(parts.at(index)=="..")
        {
            parts.erase(parts.begin()+index);
            #ifndef _WIN32
            if(index>0 && (index>1 || !parts.at(index-1).empty()))
            #else
            if(index>1)
            #endif
            {
                parts.erase(parts.begin()+index-1);
                index--;
            }
        }
        else
            index++;
    }

    #ifndef _WIN32
    if(parts.empty() || (parts.size()==1 && parts.at(0).empty()))
        return "/";
    #endif
    return stringimplode(parts,'/');
}

std::string FSabsolutePath(const std::string &string)
{
    const std::string &tempFile=FSabsoluteFilePath(string);
    const std::size_t &found=tempFile.find_last_of("/\\");
    if(found!=std::string::npos)
        return tempFile.substr(0,found)+'/';
    else
        return tempFile;
}
