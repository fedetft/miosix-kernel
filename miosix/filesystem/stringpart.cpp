/***************************************************************************
 *   Copyright (C) 2013-2024 by Terraneo Federico                          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   As a special exception, if other files instantiate templates or use   *
 *   macros or inline functions from this file, or you compile this file   *
 *   and link it with other works to produce a work based on this file,    *
 *   this file does not by itself cause the resulting work to be covered   *
 *   by the GNU General Public License. However the source code for this   *
 *   file must still be made available in accordance with the GNU General  *
 *   Public License. This exception does not invalidate any other reasons  *
 *   why a work based on this file might be covered by the GNU General     *
 *   Public License.                                                       *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

//Makes memrchr available in newer GCCs
#define _GNU_SOURCE
#include <string.h>

#include "stringpart.h"
#include <cassert>

#ifdef TEST_ALGORITHM
#include <cstdio>
#endif //TEST_ALGORITHM

using namespace std;

namespace miosix {

//
// class StringPart
//

StringPart::StringPart(string& str, size_t idx, size_t off)
        : str(&str), index(idx), offset(off), saved('\0'), owner(false),
          type(CPPSTR)
{
    if(index==string::npos || index>=str.length()) index=str.length();
    else {
        saved=str[index];
        str[index]='\0';
    }
    offset=min(offset,index);
}

StringPart::StringPart(char* s, size_t idx, size_t off)
        : cstr(s), index(idx), offset(off), saved('\0'), owner(false),
          type(CSTR)
{
    assert(cstr); //Passed pointer can't be null
    size_t len=strlen(cstr);
    if(index==string::npos || index>=len) index=len;
    else {
        saved=cstr[index];
        cstr[index]='\0';
    }
    offset=min(offset,index);
}

StringPart::StringPart(const char* s)
        : ccstr(s), offset(0), saved('\0'), owner(false), type(CCSTR)
{
    assert(ccstr); //Passed pointer can't be null
    index=strlen(s);
}

StringPart::StringPart(StringPart& rhs, size_t idx, size_t off)
      : cstr(&saved), index(0), offset(0), saved('\0'), owner(false), type(CSTR)
{
    rhs.substr(*this,idx,off);
}

void StringPart::substr(StringPart& target, size_t idx, size_t off)
{
    target.clear();
    target.type=type;
    switch(type)
    {
        case CSTR:
            target.cstr=cstr;
            break;
        case CCSTR:
            target.type=CSTR; //To make a substring of a CCSTR we need to make a copy
            if(empty()==false) target.assign(*this);
            break;
        case CPPSTR:
            target.str=str;
            break;
    }
    if(idx!=string::npos && idx<length())
    {
        target.index=offset+idx;//Make index relative to beginning of original str
        if(target.type==CPPSTR)
        {
            target.saved=(*target.str)[target.index];
            (*target.str)[target.index]='\0';
        } else {
            target.saved=target.cstr[target.index];
            target.cstr[target.index]='\0';
        }
    } else target.index=index;
    target.offset=min(offset+off,target.index); //Works for CCSTR as offset is always zero
}

StringPart::StringPart(const StringPart& rhs)
      : cstr(&saved), index(0), offset(0), saved('\0'), owner(false), type(CSTR)
{
    if(rhs.empty()==false) assign(rhs);
}

StringPart& StringPart::operator= (const StringPart& rhs)
{
    if(this==&rhs) return *this; //Self assignment
    //Don't forget that we may own a pointer to someone else's string,
    //so always clear() to detach from a string on assignment!
    clear();
    if(rhs.empty()==false) assign(rhs);
    return *this;
}

bool StringPart::startsWith(const StringPart& rhs) const
{
    if(this->length()<rhs.length()) return false;
    return memcmp(this->c_str(),rhs.c_str(),rhs.length())==0;
}

size_t StringPart::findFirstOf(char c, size_t pos) const
{
    size_t l=length();
    if(pos>=l) return std::string::npos;
    const char *begin=c_str();
    const void *index=memchr(begin+pos,c,l-pos);
    if(index==nullptr) return std::string::npos;
    return reinterpret_cast<const char*>(index)-begin;
}

size_t StringPart::findLastOf(char c) const
{
    const char *begin=c_str();
    //Not strrchr() to take advantage of knowing the string length
    const void *index=memrchr(begin,c,length());
    if(index==nullptr) return std::string::npos;
    return reinterpret_cast<const char*>(index)-begin;
}

const char *StringPart::c_str() const
{
    switch(type)
    {
        case CSTR: return cstr+offset;
        case CCSTR: return ccstr; //Offset always 0
        default: return str->c_str()+offset;
    }
}

char StringPart::operator[] (size_t index) const
{
    switch(type)
    {
        case CSTR: return cstr[offset+index];
        case CCSTR: return ccstr[index]; //Offset always 0
        default: return (*str)[offset+index];
    }
}

void StringPart::clear()
{
    if(type==CSTR)
    {
        cstr[index]=saved;//Worst case we'll overwrite terminating \0 with an \0
        if(owner) delete[] cstr;
    } else if(type==CPPSTR) {
        if(index!=str->length()) (*str)[index]=saved;
        //assert(owner==false);
    } //For CCSTR there's nothing to do
    cstr=&saved; //Reusing saved as an empty string
    saved='\0';
    index=offset=0;
    owner=false;
    type=CSTR;
}

void StringPart::assign(const StringPart& rhs)
{
    cstr=new char[rhs.length()+1];
    strcpy(cstr,rhs.c_str());
    index=rhs.length();
    offset=0;
    owner=true;
}

} //namespace miosix

#ifdef TEST_ALGORITHM

// g++ -Wall -fsanitize=address -DTEST_ALGORITHM -o t stringpart.cpp && ./t

using namespace std;
using namespace miosix;

int main()
{
    //Empty class
    {
        StringPart sp;
        assert(sp.empty());
        assert(sp.length()==0);
        assert(sp[0]=='\0');
        assert(sp.c_str()[0]=='\0');
    }
    //Construction from std::string
    {
        string s="0123456789";
        string reference=s;
        {
            StringPart sp(s);
            assert(sp.empty()==false);
            assert(sp.length()==10);
            assert(strcmp(sp.c_str(),reference.c_str())==0);
            assert(s==reference);
            assert(sp[0]=='0');
        }
        assert(s==reference);
        {
            StringPart sp(s,5);
            assert(sp.empty()==false);
            assert(sp.length()==5);
            assert(strcmp(sp.c_str(),"01234")==0);
            assert(strcmp(s.c_str(),"01234")==0); //Should have done shallow copy
            assert(sp[0]=='0');
        }
        assert(s==reference);
        {
            StringPart sp(s,5,2);
            assert(sp.empty()==false);
            assert(sp.length()==3);
            assert(strcmp(sp.c_str(),"234")==0);
            assert(strcmp(s.c_str(),"01234")==0); //Should have done shallow copy
            assert(sp[0]=='2');
        }
        assert(s==reference);
        {
            StringPart sp(s,5,5);
            assert(sp.empty());
            assert(sp.length()==0);
            assert(strcmp(sp.c_str(),"")==0);
            assert(strcmp(s.c_str(),"01234")==0); //Should have done shallow copy
            assert(sp[0]=='\0');
        }
        assert(s==reference);
    }
    //Construction from char *
    {
        char s[32], reference[32];
        strcpy(s,"0123456789");
        strcpy(reference,s);
        {
            StringPart sp(s);
            assert(sp.empty()==false);
            assert(sp.length()==10);
            assert(strcmp(sp.c_str(),reference)==0);
            assert(strcmp(s,reference)==0);
            assert(sp[0]=='0');
        }
        assert(strcmp(s,reference)==0);
        {
            StringPart sp(s,5);
            assert(sp.empty()==false);
            assert(sp.length()==5);
            assert(strcmp(sp.c_str(),"01234")==0);
            assert(strcmp(s,"01234")==0); //Should have done shallow copy
            assert(sp[0]=='0');
        }
        assert(strcmp(s,reference)==0);
        {
            StringPart sp(s,5,2);
            assert(sp.empty()==false);
            assert(sp.length()==3);
            assert(strcmp(sp.c_str(),"234")==0);
            assert(strcmp(s,"01234")==0); //Should have done shallow copy
            assert(sp[0]=='2');
        }
        assert(strcmp(s,reference)==0);
        {
            StringPart sp(s,5,5);
            assert(sp.empty());
            assert(sp.length()==0);
            assert(strcmp(sp.c_str(),"")==0);
            assert(strcmp(s,"01234")==0); //Should have done shallow copy
            assert(sp[0]=='\0');
        }
        assert(strcmp(s,reference)==0);
    }
    //Construction from const char *
    {
        const char s[]="0123456789";
        const char reference[]="0123456789";
        {
            StringPart sp(s);
            assert(sp.empty()==false);
            assert(sp.length()==10);
            assert(strcmp(sp.c_str(),reference)==0);
            assert(strcmp(s,reference)==0);
            assert(sp[0]=='0');
        }
        assert(strcmp(s,reference)==0);
    }
    //Substring from std::string
    {
        string s="0123456789";
        string reference=s;
        //Direct substring
        {
            StringPart sp(s);
            StringPart sp2(sp,5,2);
            assert(sp2.empty()==false);
            assert(sp2.length()==3);
            assert(strcmp(sp2.c_str(),"234")==0);
            assert(strcmp(s.c_str(),"01234")==0); //Should have done shallow copy
            assert(sp2[0]=='2');
        }
        assert(s==reference);
        {
            StringPart sp(s);
            string other="Hello";
            StringPart sp2(other,3);
            sp.substr(sp2,5,2);
            assert(other=="Hello");
            assert(sp2.empty()==false);
            assert(sp2.length()==3);
            assert(strcmp(sp2.c_str(),"234")==0);
            assert(strcmp(s.c_str(),"01234")==0); //Should have done shallow copy
            assert(sp2[0]=='2');
        }
        assert(s==reference);
        //Substring of a substring
        {
            StringPart sp(s,8,1);
            StringPart sp2(sp,5,2);
            assert(sp2.empty()==false);
            assert(sp2.length()==3);
            assert(strcmp(sp2.c_str(),"345")==0);
            assert(strcmp(s.c_str(),"012345")==0); //Should have done shallow copy
            assert(sp2[0]=='3');
        }
        assert(s==reference);
        {
            StringPart sp(s,8,1);
            string other="Hello";
            StringPart sp2(other,3);
            sp.substr(sp2,5,2);
            assert(other=="Hello");
            assert(sp2.empty()==false);
            assert(sp2.length()==3);
            assert(strcmp(sp2.c_str(),"345")==0);
            assert(strcmp(s.c_str(),"012345")==0); //Should have done shallow copy
            assert(sp2[0]=='3');
        }
        assert(s==reference);
    }
    //Substring from char *
    {
        char s[32], reference[32];
        strcpy(s,"0123456789");
        strcpy(reference,s);
        //Direct substring
        {
            StringPart sp(s);
            StringPart sp2(sp,5,2);
            assert(sp2.empty()==false);
            assert(sp2.length()==3);
            assert(strcmp(sp2.c_str(),"234")==0);
            assert(strcmp(s,"01234")==0); //Should have done shallow copy
            assert(sp2[0]=='2');
        }
        assert(strcmp(s,reference)==0);
        {
            StringPart sp(s);
            string other="Hello";
            StringPart sp2(other,3);
            sp.substr(sp2,5,2);
            assert(other=="Hello");
            assert(sp2.empty()==false);
            assert(sp2.length()==3);
            assert(strcmp(sp2.c_str(),"234")==0);
            assert(strcmp(s,"01234")==0); //Should have done shallow copy
            assert(sp2[0]=='2');
        }
        assert(strcmp(s,reference)==0);
        //Substring of a substring
        {
            StringPart sp(s,8,1);
            StringPart sp2(sp,5,2);
            assert(sp2.empty()==false);
            assert(sp2.length()==3);
            assert(strcmp(sp2.c_str(),"345")==0);
            assert(strcmp(s,"012345")==0); //Should have done shallow copy
            assert(sp2[0]=='3');
        }
        assert(strcmp(s,reference)==0);
        {
            StringPart sp(s,8,1);
            string other="Hello";
            StringPart sp2(other,3);
            sp.substr(sp2,5,2);
            assert(other=="Hello");
            assert(sp2.empty()==false);
            assert(sp2.length()==3);
            assert(strcmp(sp2.c_str(),"345")==0);
            assert(strcmp(s,"012345")==0); //Should have done shallow copy
            assert(sp2[0]=='3');
        }
        assert(strcmp(s,reference)==0);
    }
    //Substring from const char *
    {
        const char s[]="0123456789";
        const char reference[]="0123456789";
        //Direct substring
        {
            StringPart sp(s);
            StringPart sp2(sp,5,2);
            assert(sp2.empty()==false);
            assert(sp2.length()==3);
            assert(strcmp(sp2.c_str(),"234")==0);
            assert(strcmp(s,reference)==0); //Should have done deep copy
            assert(sp2[0]=='2');
        }
        assert(strcmp(s,reference)==0);
        {
            StringPart sp(s);
            string other="Hello";
            StringPart sp2(other,3);
            sp.substr(sp2,5,2);
            assert(other=="Hello");
            assert(sp2.empty()==false);
            assert(sp2.length()==3);
            assert(strcmp(sp2.c_str(),"234")==0);
            assert(strcmp(s,reference)==0); //Should have done deep copy
            assert(sp2[0]=='2');
        }
        assert(strcmp(s,reference)==0);
        //Substring of an empty const char * corner case
        {
            const char s[]="";
            StringPart sp(s);
            string other="Hello";
            StringPart sp2(other,3);
            sp.substr(sp2,5,2);
            assert(other=="Hello");
            assert(sp2.empty());
            assert(sp2.length()==0);
            assert(strcmp(sp2.c_str(),"")==0);
            assert(sp2[0]=='\0');
        }
    }
    //startsWith, findFirstOf, findLastOf
    {
        string s="/dir1/dir2/dir3/dir4/dir5";
        string reference=s;
        {
            StringPart sp(s,19,6); //"dir2/dir3/dir"
            assert(sp.startsWith(StringPart("dir2"))==true);
            assert(sp.startsWith(StringPart("dir3"))==false);
            assert(sp.findFirstOf('/')==4);
            assert(sp.findFirstOf('/',5)==9);
            assert(sp.findFirstOf('0')==string::npos);
            assert(sp.findLastOf('/')==9);
            assert(sp.findLastOf('0')==string::npos);
        }
        assert(s==reference);
    }
    printf("Test passed\n");
}

#endif //TEST_ALGORITHM
