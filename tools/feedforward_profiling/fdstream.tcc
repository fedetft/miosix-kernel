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

*/

#ifndef FDSTREAM_TCC
#define FDSTREAM_TCC

// provide the fdstream class methods

template <class charT , class Traits>
basic_fdoutbuf<charT , Traits>::basic_fdoutbuf(int fd_, bool manage_) : fd(fd_), manage(manage_) {
  this->setp(buffer, buffer + buf_size);
}

template <class charT , class Traits>
basic_fdoutbuf<charT , Traits>::~basic_fdoutbuf() {
  if (fd >= 0) {
    this->sync();
    if (manage) {
      while (::close(fd) == -1 && errno == EINTR);
    }
  }
}

// sync returns 0 for success and -1 for error
template <class charT , class Traits>
int basic_fdoutbuf<charT , Traits>::sync() {
  return (fd == -1) ? -1 : flush_buffer(); 
}

template <class charT , class Traits>
void basic_fdoutbuf<charT , Traits>::attach_fd(int fd_, bool manage_) {
  if (fd >= 0) {
    this->sync();
    if (manage) {
      while (::close(fd) == -1 && errno == EINTR);
    }
  }    
  fd = fd_;
  manage = manage_;
}

template <class charT , class Traits>
void basic_fdoutbuf<charT , Traits>::close_fd() {
  if (fd >= 0) {
    this->sync(); 
    while (::close(fd) == -1 && errno == EINTR); 
    fd = -1;
  }
}

// flush_buffer() returns 0 for success and -1 for error
template <class charT , class Traits>
int basic_fdoutbuf<charT , Traits>::flush_buffer() {
  std::size_t count = this->pptr() - this->pbase();
  if (count) {
    std::size_t remaining = count * sizeof(char_type);
    ssize_t result;
    const char* temp_p = reinterpret_cast<const char*>(buffer);
    do {
      result = write(fd, temp_p, remaining);
      if (result > 0) {
	temp_p += result;
	remaining -= result;
      }
    } while (remaining && (result != -1 || errno == EINTR));

    if (remaining) return -1;
    this->pbump(-count);
  }
  return 0;
}

template <class charT , class Traits>
typename basic_fdoutbuf<charT , Traits>::int_type
basic_fdoutbuf<charT , Traits>::overflow(int_type c) {
  // the stream must be full: empty buffer and then put c in buffer
  if (flush_buffer() == -1) return traits_type::eof();
  if (traits_type::eq_int_type(traits_type::eof(), c)) return traits_type::not_eof(c);
  return this->sputc(c);
}

#ifndef FDSTREAM_USE_STD_N_READ_WRITE
template <class charT , class Traits>
std::streamsize basic_fdoutbuf<charT , Traits>::xsputn(const char_type* source_p, std::streamsize num) {
  // if num is less than the space in the buffer, put it in the buffer
  if (num < this->epptr() - this->pptr()) {
    traits_type::copy(this->pptr(), source_p, num);
    this->pbump(num);
  }

  else {
    // make a block write to the target, so first flush anything
    // in the buffer and then use Unix ::write()
    if (flush_buffer() == -1) num = 0;
    else {
      std::size_t remaining = num * sizeof(char_type);
      ssize_t result;
      const char* temp_p = reinterpret_cast<const char*>(source_p);
      do {
	result = write(fd, temp_p, remaining);
	if (result > 0) {
	  temp_p += result;
	  remaining -= result;
	}
      } while (remaining && (result != -1 || errno == EINTR));
      
      if (remaining) num -= remaining/sizeof(char_type);
    }
  }
  return num;
}
#endif /*FDSTREAM_USE_STD_N_READ_WRITE*/


template <class charT , class Traits>
basic_fdinbuf<charT , Traits>::basic_fdinbuf(int fd_, bool manage_): fd(fd_), manage(manage_) {
  reset();
}

template <class charT , class Traits>
basic_fdinbuf<charT , Traits>::~basic_fdinbuf() {
  if (manage && fd >= 0) {
    while (::close(fd) == -1 && errno == EINTR);
  }
}

template <class charT , class Traits>
void basic_fdinbuf<charT , Traits>::reset() {
  this->setg(buffer + putback_size,     // beginning of putback area
	     buffer + putback_size,     // read position
	     buffer + putback_size);    // end position
}

template <class charT , class Traits>
void basic_fdinbuf<charT , Traits>::attach_fd(int fd_, bool manage_) {
  reset();
  if (manage && fd >= 0) {
    while (::close(fd) == -1 && errno == EINTR);
  }
  fd = fd_;
  manage = manage_;
}

template <class charT , class Traits>
void basic_fdinbuf<charT , Traits>::close_fd() {
  if (fd >= 0) {
    reset();
    while (::close(fd) == -1 && errno == EINTR);
    fd = -1;}
}

template <class charT , class Traits>
typename basic_fdinbuf<charT , Traits>::int_type
basic_fdinbuf<charT , Traits>::underflow() {
  // insert new characters into the buffer when it is empty

  // is read position before end of buffer?
  if (this->gptr() < this->egptr()) return traits_type::to_int_type(*this->gptr());

  int putback_count = this->gptr() - this->eback();
  if (putback_count > putback_size) putback_count = putback_size;

  /* copy up to putback_size characters previously read into the putback
   * area -- use traits_type::move() because if the size of file modulus buf_size
   * is less than putback_size, the source and destination ranges can
   * overlap on the last call to underflow() (ie the one which leads
   * to traits_type::eof() being returned)
   */
  traits_type::move(buffer + (putback_size - putback_count),
		    this->gptr() - putback_count,
		    putback_count);

  // read at most buf_size new characters
  ssize_t result;
  do {
    result = read(fd, buffer + putback_size, buf_size * sizeof(char_type));
  } while (result == -1 && errno == EINTR);

  if (sizeof(char_type) > 1 && result > 0) { // check for part character fetch - this block will be
                                             // optimised out where char_type is a plain one byte char
    std::size_t part_char = result % sizeof(char_type);
    if (part_char) {
      ssize_t result2;
      std::streamsize remaining = sizeof(char_type) - part_char;
      char* temp_p = reinterpret_cast<char*>(buffer);
      temp_p += putback_size * sizeof(char_type) + result;
      do {
	result2 = read(fd, temp_p, remaining);
	if (result2 > 0) {
	  temp_p += result2;
	  remaining -= result2;
	  result += result2;
	}
      } while (remaining                              // we haven't met the request
	       && result2                             // not end of file
	       && (result2 != -1 || errno == EINTR)); // no error other than syscall interrupt
    }
  }
 
  if (result <= 0) {
    // error (result == -1) or end-of-file (result == 0)
    // we might as well make the put back area readable so reset
    // the buffer pointers to enable this
    this->setg(buffer + (putback_size - putback_count), // beginning of putback area
	       buffer + putback_size,                   // start of read area of buffer
	       buffer + putback_size);                  // end of buffer - it's now empty
    return traits_type::eof();
  }
  
  // reset buffer pointers
  this->setg(buffer + (putback_size - putback_count),             // beginning of putback area
	     buffer + putback_size,                               // read position
	     buffer + (putback_size + result/sizeof(char_type))); // end of buffer

  // return next character
  return traits_type::to_int_type(*this->gptr());
}

#ifndef FDSTREAM_USE_STD_N_READ_WRITE
template <class charT , class Traits>
std::streamsize basic_fdinbuf<charT , Traits>::xsgetn(char_type* dest_p, std::streamsize num) {
  std::streamsize copied_to_target = 0;
  std::streamsize available = this->egptr() - this->gptr();

  // if num is less than or equal to the characters already in the buffer,
  // extract from buffer
  if (num <= available) {
    traits_type::copy(dest_p, this->gptr(), num);
    this->gbump(num);
    copied_to_target = num;
  }

  else {
    // first copy out the buffer
    if (available) {
      traits_type::copy(dest_p, this->gptr(), available);
      copied_to_target = available;
    }

    // we now know from the first 'if' block that there is at least one more character required

    ssize_t remaining = (num - copied_to_target) * sizeof(char_type); // this is in bytes
    std::size_t bytes_fetched = 0;

    ssize_t result = 1; // a dummy value so that the second if block
                        // is always entered if next one isn't

    if (num - copied_to_target > buf_size) {
      // this is a big read, and we are going to read up to everything we need
      // except one character with a single block system read() (leave one character
      // still to get on a buffer refill read in order (a) so that the final read()
      // will not unnecessarily block after the xsgetn() request size has been met and
      // (b) so that we have an opportunity to fill the buffer for any further reads)
      remaining -= sizeof(char_type); // leave one remaining character to fetch
      char* temp_p = reinterpret_cast<char*>(dest_p);
      temp_p += copied_to_target * sizeof(char_type);
      do {
	result = read(fd, temp_p, remaining);
	if (result > 0) {
	  temp_p += result;
	  remaining -= result;
	  bytes_fetched += result;
	}
      } while (remaining                             // more to come
	       && result                             // not end of file
	       && (result != -1 || errno == EINTR)); // no error other than syscall interrupt

      copied_to_target += bytes_fetched/sizeof(char_type); // if the stream hasn't failed this must
                                                           // be a whole number of characters
      if (!copied_to_target) return 0;  // nothing available - bail out
      remaining += sizeof(char_type);   // reset the requested characters to add back the last one
      bytes_fetched = 0;                // reset bytes_fetched
    }

    char* buffer_pos_p = reinterpret_cast<char*>(buffer);
    buffer_pos_p += putback_size * sizeof(char_type);
    // at this point bytes_fetched will have been set or reset to 0

    if (result > 0) { // no stream failure
      // now fill up the buffer as far as we can to extract sufficient
      // bytes for the one or more remaining characters requested
      do {
	result = read(fd, buffer_pos_p, buf_size * sizeof(char_type) - bytes_fetched);
	if (result > 0) {
	  buffer_pos_p += result;
	  remaining -= result;
	  bytes_fetched += result;
	}
      } while (remaining > 0                         // we haven't met the request
	       && result                             // not end of file
	       && (result != -1 || errno == EINTR)); // no error other than syscall interrupt

      if (!copied_to_target && !bytes_fetched) return 0;   // nothing available - bail out
    }

    if (sizeof(char_type) > 1 && result > 0) { // check for part character fetch - this block will be
                                               // optimised out where char_type is a plain one byte char
      std::size_t part_char = bytes_fetched % sizeof(char_type);
      if (part_char) {
	remaining = sizeof(char_type) - part_char;
	do {
	  result = read(fd, buffer_pos_p, remaining);
	  if (result > 0) {
            buffer_pos_p += result;
	    remaining -= result;
	    bytes_fetched += result;
	  }
	} while (remaining                             // we haven't met the request
		 && result                             // not end of file
		 && (result != -1 || errno == EINTR)); // no error other than syscall interrupt
      }
    }
    std::streamsize chars_fetched = bytes_fetched/sizeof(char_type);// if the stream hasn't failed this
                                                                    // must now be a whole number of
                                                                    // characters we have put directly
                                                                    // in the buffer
    // now put the final instalment of requested characters into the target from the buffer
    std::streamsize copy_num = chars_fetched;
    std::streamsize putback_count = 0;
    if (copy_num) {
      if (copy_num > num - copied_to_target) copy_num = num - copied_to_target;
      traits_type::copy(dest_p + copied_to_target, buffer + putback_size, copy_num);
      copied_to_target += copy_num;
    }

    if (copy_num < putback_size) { // provide some more putback characters
      std::streamsize start_diff = copied_to_target;
      if (start_diff > putback_size) start_diff = putback_size;
      putback_count = start_diff - copy_num;
      traits_type::copy(buffer + (putback_size - putback_count),
			dest_p + (copied_to_target - start_diff),
			putback_count);
    }
    // reset buffer pointers
    this->setg(buffer + (putback_size - putback_count),    // beginning of putback area
	       buffer + (putback_size + copy_num),         // read position
	       buffer + (putback_size + chars_fetched));   // end of buffer
  }
  return copied_to_target;
}
#endif /*FDSTREAM_USE_STD_N_READ_WRITE*/

#endif /*FDSTREAM_TCC*/
