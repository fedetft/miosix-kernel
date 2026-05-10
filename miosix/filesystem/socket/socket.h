/***************************************************************************
 *   Copyright (C) 2026 by Niccolò Betto                                   *
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

#pragma once

#include "socket_impl.h"

#include "config/miosix_settings.h"
#include "filesystem/file.h"
#include "kernel/sync.h"

#ifdef WITH_NETWORKING

namespace miosix {

/**
 * Socket
 */
class Socket : public FileBase {
  public:
    Socket();
    ~Socket();

    /**
     * Write data to the file, if the file supports writing.
     * \param data the data to write
     * \param len the number of bytes to write
     * \return the number of written characters, or a negative number in case
     * of errors
     */
    virtual ssize_t write(const void *data, size_t len);

    /**
     * Read data from the file, if the file supports reading.
     * \param data buffer to store read data
     * \param len the number of bytes to read
     * \return the number of read characters, or a negative number in case
     * of errors
     */
    virtual ssize_t read(void *data, size_t len);

    /**
     * Move file pointer, if the file supports random-access.
     * \param pos offset to sum to the beginning of the file, current position
     * or end of file, depending on whence
     * \param whence SEEK_SET, SEEK_CUR or SEEK_END
     * \return the offset from the beginning of the file if the operation
     * completed, or a negative number in case of errors
     */
    virtual off_t lseek(off_t pos, int whence);

    /**
     * Truncate the file
     * \param size new file size
     * \return 0 on success, or a negative number on failure
     */
    virtual int ftruncate(off_t size);

    /**
     * Return file information.
     * \param pstat pointer to stat struct
     * \return 0 on success, or a negative number on failure
     */
    virtual int fstat(struct stat *pstat) const;

    /**
     * Perform various operations on a file descriptor
     * \param cmd specifies the operation to perform
     * \param opt optional argument that some operation require
     * \return the exact return value depends on CMD, -1 is returned on error
     */
    virtual int fcntl(int cmd, int opt);

    /**
     * Perform various operations on a file descriptor
     * \param cmd specifies the operation to perform
     * \param arg optional argument that some operation require
     * \return the exact return value depends on CMD, -1 is returned on error
     */
    virtual int ioctl(int cmd, void *arg);

    /**
     * Read data from a socket, and store it in multiple buffers
     * \param iov array of iovec structures describing the buffers where the
     * received message will be stored
     * \param iovcnt number of elements in the iov array
     * \param flags receive flags
     * \return the number of bytes received, or a negative number on failure
     */
    virtual ssize_t readv(const struct iovec *iov, int iovcnt);

    /**
     * Send a message on a socket, from multiple buffers
     * \param iov array of iovec structures describing the buffers containing
     * the message to send
     * \param iovcnt number of elements in the iov array
     * \param flags send flags
     * \return the number of bytes sent, or a negative number on failure
     */
    virtual ssize_t writev(const struct iovec *iov, int iovcnt);

    /**
     * Create a socket
     * \param domain communication domain
     * \param type socket type
     * \param protocol protocol to be used with the socket
     * \return 0 on success, or a negative number on failure
     */
    virtual int socket(int domain, int type, int protocol);

    /**
     * Bind a socket to an address
     * \param name pointer to a sockaddr structure containing the address to
     * bind to
     * \param namelen length of the supplied sockaddr structure
     * \return 0 on success, or a negative number on failure
     */
    virtual int bind(const struct sockaddr *name, socklen_t namelen);

    /**
     * Connect a socket to an address
     * \param name pointer to a sockaddr structure containing the address to
     * connect to
     * \param namelen length of the supplied sockaddr structure
     * \return 0 on success, or a negative number on failure
     */
    virtual int connect(const struct sockaddr *name, socklen_t namelen);

    /**
     * Listen for connections on a socket
     * \param backlog maximum length of the queue of pending connections
     * \return 0 on success, or a negative number on failure
     */
    virtual int listen(int backlog);

    /**
     * Accept a connection on a socket
     * \param newsock Socket object for the accepted socket
     * \param addr pointer to a sockaddr structure where the address of the
     * connecting socket shall be returned, can be null
     * \param addrlen pointer to length of supplied sockaddr object on input,
     * length of the stored address on output, can be null if addr is null
     * \return 0 on success, or a negative number on failure
     */
    virtual int accept(intrusive_ref_ptr<Socket> newsock, 
                       struct sockaddr *addr, socklen_t *addrlen);

    /**
     * Get the address of the socket itself
     * \param name pointer to a sockaddr structure where the address of the
     * socket shall be returned
     * \param namelen pointer to length of supplied sockaddr object on input,
     * length of the stored address on output
     * \return 0 on success, or a negative number on failure
     */
    virtual int getsockname(struct sockaddr *name, socklen_t *namelen);

    /**
     * Get the address of the peer connected to a socket
     * \param name pointer to a sockaddr structure where the address of the peer
     * socket shall be returned
     * \param namelen pointer to length of supplied sockaddr object on input,
     * length of the stored address on output
     * \return 0 on success, or a negative number on failure
     */
    virtual int getpeername(struct sockaddr *name, socklen_t *namelen);

    /**
     * Send a message on a socket
     * \param dataptr pointer to the message to send
     * \param size length of the message to send
     * \param flags send flags
     * \return the number of bytes sent, or a negative number on failure
     */
    virtual ssize_t send(const void *dataptr, size_t size, int flags);

    /**
     * Send a message on a socket, to a specific destination
     * \param dataptr pointer to the message to send
     * \param size length of the message to send
     * \param flags send flags
     * \param to pointer to a sockaddr structure containing the destination
     * address
     * \param tolen length of the supplied sockaddr structure
     * \return the number of bytes sent, or a negative number on failure
     */
    virtual ssize_t sendto(const void *dataptr, size_t size, int flags,
                           const struct sockaddr *to, socklen_t tolen);

    /**
     * Receive a message from a socket
     * \param mem buffer where the received message will be stored
     * \param len length of the supplied buffer
     * \param flags receive flags
     * \return the number of bytes received, or a negative number on failure
     */
    virtual ssize_t recv(void *mem, size_t len, int flags);

    /**
     * Receive a message from a socket, and store the source address
     * \param mem buffer where the received message will be stored
     * \param len length of the supplied buffer
     * \param flags receive flags
     * \param from pointer to a sockaddr structure where the source address will
     * be stored, can be null if not interested in the source address
     * \param fromlen pointer to length of supplied sockaddr object on input,
     * length of the stored address on output, can be null if from is null
     * \return the number of bytes received, or a negative number on failure
     */
    virtual ssize_t recvfrom(void *mem, size_t len, int flags,
                             struct sockaddr *from, socklen_t *fromlen);

    /**
     * Shut down part of a full-duplex connection on a socket
     * \param how specifies what to shut down
     * \return 0 on success, or a negative number on failure
     */
    virtual int shutdown(int how);

    /**
     * Set the value of a socket option
     * \param level protocol level of the option
     * \param optname option name
     * \param optval pointer to the buffer containing the option value
     * \param optlen length of the supplied buffer
     * \return 0 on success, or a negative number on failure
     */
    virtual int setsockopt(int level, int optname, const void *optval,
                           socklen_t optlen);

    /**
     * Get the value of a socket option
     * \param level protocol level of the option
     * \param optname option name
     * \param optval pointer to the buffer where the option value will be
     * returned
     * \param optlen pointer to the length of the supplied buffer
     * \return 0 on success, or a negative number on failure
     */
    virtual int getsockopt(int level, int optname, void *optval,
                           socklen_t *optlen);

    /**
     * Send a message on a socket, to a specific destination
     * \param message pointer to an msghdr structure describing the message to
     * send and the destination address
     * \param flags send flags
     * \return the number of bytes sent, or a negative number on failure
     */
    virtual ssize_t sendmsg(const struct msghdr *message, int flags);

    /**
     * Receive a message from a socket, and store the source address and
     * ancillary data
     * \param message pointer to an msghdr structure describing the message to
     * receive, and where to store the source address and ancillary data
     * \param flags receive flags
     * \return the number of bytes received, or a negative number on failure
     */
    virtual ssize_t recvmsg(struct msghdr *message, int flags);

  private:
    void close();

    lwip_sock *sock;

/*
 * The lwIP functions this socket implementation is derived from were passed a
 * socket fd, and that fd was used in debug print functions, which are still
 * present in the code.
 * The fd parameter has been removed since it is now handled at the FD table
 * level, meaning that the socket object doesn't have any way to know its fd.
 * Additially, fds are not unique 
 * To keep code changes to a minimum compared to the original lwIP code and to
 * keep debug print functions somewhat working, the fd (previously named 's') is
 * transparently replaced with the address of the socket object itself, cast to
 * integer to satisfy the %d format specifier.
 * In the future consider rewriting debug prints to %x and pass the this pointer
 */
#ifdef LWIP_DEBUG
    unsigned int s = reinterpret_cast<uintptr_t>(this);
#else
    unsigned int s = 0;
#endif
};

} // namespace miosix

#endif // WITH_NETWORKING
