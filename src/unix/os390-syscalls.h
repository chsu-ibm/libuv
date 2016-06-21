/* Copyright libuv project contributors. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */


#ifndef UV_OS390_SYSCALL_H_
#define UV_OS390_SYSCALL_H_

#include "os390-epoll.h"
#include <dirent.h>

# define UV__O_CLOEXEC            0x80000
#define UV__EPOLL_CLOEXEC         UV__O_CLOEXEC
#define UV__EPOLL_CTL_ADD         1
#define UV__EPOLL_CTL_DEL         2
#define UV__EPOLL_CTL_MOD         3
#define UV__EPOLL_CTL_ADD_MSGQ    4

struct uv__epoll_event {
  uint32_t events;
  uint32_t data;
};

#endif /* UV_OS390_SYSCALL_H_ */
