/* Copyright (C) 2001 and 2004 Chris Vine


 The following code declares classes to read from and write to
 Unix file descriptors.

 The whole work comprised in files fdstream.h and fdstream.tcc is
 distributed by Chris Vine under the GNU Lesser General Public License
 as follows:

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public License
   as published by the Free Software Foundation; either version 2.1 of
   the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library (see the file LGPL.TXT which came
   with this source code package); if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
   02111-1307, USA.

However, it is not intended that the object code of a program whose
source code instantiates a template from this file should by reason
only of that instantiation be subject to the restrictions of use in
the GNU Lesser General Public License.  With that in mind, the words
"and instantiations of templates (of any length)" shall be treated as
inserted in the fourth paragraph of section 5 of that licence after
the words "and small inline functions (ten lines or less in length)".
This does not affect any other reason why object code may be subject
to the restrictions in that licence (nor for the avoidance of doubt
does it affect the application of section 2 of that licence to
modifications of the source code in this file).

 -----------------------------------------------------------------

 The attach(), close() and xsputn() methods added by Chris Vine, 2001.

 All the classes were rewritten, and also provided in template form
 for wide characters, by Chris Vine 2004.  Note that these classes
 have not yet been tested with wide characters, where the char_type is
 other than a plain C char (if you are using wide characters, do some
 test checks first - and in particular note that there is a direct
 byte for byte read() and write() of wide characters into bytes on the
 relevant target, without taking on board the inefficiencies of endian
 adjustment for platform independence).

 By default, like the fstream::fstream(int fd) and fstream::attach(fd)
 extensions in libstdc++-v2, the destructors of these classes close
 the file descriptors concerned, which helps exception safety (the
 attach() method will also close any previous file descriptor).  If
 this behaviour is not wanted, pass `false' as the second parameter of
 fdostream/fdistream constructor or of the attach() method .  This
 will enable the same file descriptor to be used successively for,
 say, reading and writing, or to be shared between fdistream and
 fdostream objects (but take great care if doing the latter, since the
 file pointers can easily get out of sync).

 The files provide a block read and write in fdinbuf::xsgetn() and
 fdoutbuf::xsputn().  They operate (after appropriately vacating and
 resetting the buffers) by doing a block read and write by calling
 Unix read() and write() and are very efficient for large block reads
 (those significantly exceeding the buffer size).  If users want all
 reads and writes to go through the buffers, by using
 std::basic_streambuf<charT, Traits>::xsputn() and
 std::basic_streambuf<charT, Traits>::xsgetn() then the variable
 FDSTREAM_USE_STD_N_READ_WRITE can be defined for the purpose.
 (libstdc++-3 provides efficient inbuilt versions of these
 std::basic_streambuf functions for block reads not significantly
 larger than the buffer size.)

 If FDSTREAM_USE_STD_N_READ_WRITE is not defined, then a call to
 fdinbuf::xsgetn() with a request for less than 4 characters will
 result in less than 4 characters available for putback (only the
 number of characters returned by xsgetn() is guaranteed to be
 available for putback, assuming that some positive number greater
 than 0 is returned).

*/

#ifndef FDSTREAM_H
#define FDSTREAM_H

// see above for what this does
//#define FDSTREAM_USE_STD_N_READ_WRITE 1

#include <istream>
#include <ostream>
#include <streambuf>
#include <unistd.h>
#include <errno.h>

/*
The following convenience typedefs appear at the end of this file:
typedef basic_fdinbuf<char> fdinbuf;
typedef basic_fdoutbuf<char> fdoutbuf;
typedef basic_fdostream<char> fdostream;
typedef basic_fdistream<char> fdistream;

*/


template <class charT , class Traits = std::char_traits<charT> >
class basic_fdoutbuf: public std::basic_streambuf<charT, Traits> {

public:
  typedef charT char_type;
  typedef Traits traits_type;
  typedef typename traits_type::int_type int_type;

private:
  int fd;    // file descriptor
  bool manage;

  static const int buf_size = 1024;   // size of the data write buffer
  char_type buffer[buf_size];
  int flush_buffer();
protected:
  virtual int sync();
  virtual int_type overflow(int_type);
#ifndef FDSTREAM_USE_STD_N_READ_WRITE
  virtual std::streamsize xsputn(const char_type*, std::streamsize);
#endif
public:
  basic_fdoutbuf(int fd_ = -1, bool manage_ = true); // a default _fd value of -1 means that we will use attach() later
  virtual ~basic_fdoutbuf();

  void attach_fd(int fd_, bool manage_ = true);
  void close_fd();
  int get_fd() const {return fd;}
};

template <class charT , class Traits = std::char_traits<charT> >
class basic_fdostream: public std::basic_ostream<charT, Traits> {
  basic_fdoutbuf<charT, Traits> buf;
public:
  basic_fdostream(int fd, bool manage = true): std::basic_ostream<charT, Traits>(0),
                                               buf(fd, manage) { // pass the descriptor at construction
    this->rdbuf(&buf);
  }
  basic_fdostream(): std::basic_ostream<charT, Traits>(0) {      // attach the descriptor later
    rdbuf(&buf);
  }
  void attach(int fd, bool manage = true) {buf.attach_fd(fd, manage);}
  void close() {buf.close_fd();}
  int filedesc() const {return buf.get_fd();}
};


template <class charT , class Traits = std::char_traits<charT> >
class basic_fdinbuf : public std::basic_streambuf<charT, Traits> {

public:
  typedef charT char_type;
  typedef Traits traits_type;
  typedef typename traits_type::int_type int_type;

private:
  int fd;    // file descriptor
  bool manage;

  static const int putback_size = 4;     // size of putback area
  static const int buf_size = 1024;      // size of the data buffer
  char_type buffer[buf_size + putback_size];  // data buffer
  void reset();
protected:
  virtual int_type underflow();
#ifndef FDSTREAM_USE_STD_N_READ_WRITE
  virtual std::streamsize xsgetn(char_type*, std::streamsize);
#endif
public:
  basic_fdinbuf(int fd_ = -1, bool manage_ = true); // a default value of -1 means that we will use attach() later
  virtual ~basic_fdinbuf();

  void attach_fd(int fd_, bool manage_ = true);
  void close_fd();
  int get_fd() const {return fd;}
};

template <class charT , class Traits = std::char_traits<charT> >
class basic_fdistream : public std::basic_istream<charT, Traits> {
  basic_fdinbuf<charT , Traits> buf;
public:
  basic_fdistream (int fd, bool manage = true) : std::basic_istream<charT, Traits>(0),
                                                 buf(fd, manage) { // pass the descriptor at construction
    this->rdbuf(&buf);
  }
  basic_fdistream () : std::basic_istream<charT, Traits>(0) {      // attach the descriptor later
    rdbuf(&buf);
  }
  void attach(int fd, bool manage = true) {buf.attach_fd(fd, manage);}
  void close() {buf.close_fd();}
  int filedesc() const {return buf.get_fd();}
};

typedef basic_fdinbuf<char> fdinbuf;
typedef basic_fdoutbuf<char> fdoutbuf;
typedef basic_fdostream<char> fdostream;
typedef basic_fdistream<char> fdistream;

#include "fdstream.tcc"

#endif /*FDSTREAM_H*/
