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

#include "os390-syscalls.h"
#include <stdlib.h>
#include <errno.h>

int	_number_of_epolls;
struct _epoll_list* _global_epoll_list[MAX_EPOLL_INSTANCES];

static int _removefd(struct _epoll_list *lst, int fd)
{
	if(fd == -1)
	  return 0;

	int realsize = lst->aio == NULL ? lst->size : lst->size + 1;
        int deletion_point = realsize;                         
        for (int i = 0; i < realsize; ++i)                     
        {                                                                  
            if(lst->items[i].fd == fd)                                            
            {                                                              
                deletion_point = i;                                        
                break;                                                     
            }                                                              
        }                                                                  

        if (deletion_point < realsize - 1)
        {                                                                  
	    /* deleting a file descriptor */
            for (int i = deletion_point; i < realsize; ++i)    
            {                                                              
                lst->items[i] = lst->items[i+1];
            }                                                              
            --(lst->size);
            return 1;
        }
	else if(deletion_point == realsize - 1) {
	    /* deleting the message queue */
	    lst->aio = NULL;
	    return 1;
	}
        else
            return 0;
}

static int _doesExist(struct _epoll_list *lst, int fd, int *index)
{

        for (int i = 0; i < lst->size; ++i)                     
        {                                                                  
            if(lst->items[i].fd == fd)                                            
            {
                *index=i;
                return 1;
            }
        }                                                                  
        return 0;
}

static void _modify(struct _epoll_list *lst, int index, struct epoll_event events)
{
	struct pollfd *i = &lst->items[index];
        i->events = 0;
        if(events.events & POLLIN)
            i->events |= POLLIN; 
        if(events.events & POLLOUT)
            i->events |= POLLOUT; 
        if(events.events & POLLHUP)
            i->events |= POLLHUP; 

}

static int _append(struct _epoll_list *lst, int fd, struct epoll_event events)
{
	if (lst->size == MAX_ITEMS_PER_EPOLL - 1)
		return ENOMEM;

	// remember, lst->size contains the msgq
	if(lst->aio != NULL) {
	  lst->items[lst->size+1].fd = lst->items[lst->size].fd;
	  lst->items[lst->size+1].events = lst->items[lst->size].events; 
	}

	lst->items[lst->size].fd = fd;
	_modify(lst, lst->size, events); 
	++lst->size;

	return 0;
}

int epoll_create1(int flags)
{
    struct _epoll_list* p = (struct _epoll_list*)malloc(sizeof(struct _epoll_list));
    memset(p, 0, sizeof(struct _epoll_list));
    int index = _number_of_epolls++;
    _global_epoll_list[index] = p;

    if(pthread_mutex_init(&p->lock, NULL)) {
        errno = ENOLCK;
        return -1;
    }
    p->size = 0;
    p->aio = NULL;
    return index; 
}

int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event)
{
    struct _epoll_list *lst = _global_epoll_list[epfd];

    if(op == EPOLL_CTL_DEL){
        if(!_removefd(lst, fd))
            return ENOENT;
    }
    else if(op == EPOLL_CTL_ADD_MSGQ)
    {
	pthread_mutex_lock(&lst->lock);
	lst->items[lst->size].fd = fd;
	lst->items[lst->size].events = POLLIN ;
	lst->aio = &lst->items[lst->size];
	pthread_mutex_unlock(&lst->lock);
	return 0;
    }
    else if(op == EPOLL_CTL_ADD)
    {
        int index;
	pthread_mutex_lock(&lst->lock);
        if( _doesExist(lst, fd, &index) )
        {
	    pthread_mutex_unlock(&lst->lock);
            errno = EEXIST;
            return -1;
        }
	int retval = _append(lst, fd, *event);
	pthread_mutex_unlock(&lst->lock);
	return retval;
    }
    else if(op == EPOLL_CTL_MOD)
    {
        int index;
	pthread_mutex_lock(&lst->lock);
        if( !_doesExist(lst, fd, &index) )
        {
	    pthread_mutex_lock(&lst->lock);
            errno = ENOENT;
            return -1;
        }
	_modify(lst, index, *event);
	pthread_mutex_unlock(&lst->lock);
    }
    else 
    {
        abort();
    }
    return 0;
}

int epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout)
{
    struct _epoll_list *lst = _global_epoll_list[epfd];

    unsigned int size;
    _SET_FDS_MSGS(size, lst->aio == NULL ? 0 : 1, lst->size);

    struct pollfd *pfds = lst->items;
    int returnval = poll( pfds, size, timeout );
    if(returnval == -1)
        return returnval;
    else
        returnval = _NFDS(returnval) + _NMSGS(returnval);

    int reventcount=0;
    // size + 1 will include the msgq
    int realsize = lst->aio == NULL ? lst->size : lst->size + 1;
    for (int i = 0; i < realsize && i < maxevents; ++i)                     
    {
        struct epoll_event ev = { 0, 0 };
        ev.data.fd = pfds[i].fd;
        if(!pfds[i].revents)
            continue;

        if(pfds[i].revents & POLLRDNORM)
            ev.events = ev.events | POLLIN;
        
        if(pfds[i].revents & POLLWRNORM)
            ev.events = ev.events | POLLOUT;

        if(pfds[i].revents & POLLHUP)
        {
            ev.events = ev.events | POLLHUP;
	    pthread_mutex_lock(&lst->lock);
            _removefd(lst, ev.data.fd);
	    pthread_mutex_unlock(&lst->lock);
        }

	pfds[i].revents = 0;
        events[reventcount++] = ev; 
            
    }

    return reventcount;
}

int epoll_file_close(int fd)
{
	for( int i = 0; i < _number_of_epolls; ++i )
	{
		struct _epoll_list *lst = _global_epoll_list[i];
		int index;
	        pthread_mutex_lock(&lst->lock);
		if(_doesExist(lst, fd, &index) )
			_removefd(lst, fd);
	        pthread_mutex_unlock(&lst->lock);
	}
	return 0;
}

