#ifndef SERVERSSL

#include "EpollClient.h"

#include <iostream>
#include <unistd.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <cstring>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "Epoll.h"
#include "EpollSocket.h"
#include "../../general/base/GeneralVariable.h"

using namespace CatchChallenger;

EpollClient::EpollClient(const int &infd) :
    #ifndef SERVERNOBUFFER
    bufferSize(0),
    #endif
    infd(infd)
{
    #ifndef SERVERNOBUFFER
    memset(buffer,0,4096);
    #endif
}

EpollClient::~EpollClient()
{
    close();
}

void EpollClient::reopen(const int &infd)
{
    close();
    this->infd=infd;
}

void EpollClient::close()
{
    #ifndef SERVERNOBUFFER
    bufferSize=0;
    #endif
    if(infd!=-1)
    {
        char tempBuffer[4096];
        //purge the buffer to prevent multiple EPOLLIN due to level trigger (vs edge trigger)
        while(read(tempBuffer,sizeof(tempBuffer))>0)
        {}
        /* Closing the descriptor will make epoll remove it
        from the set of descriptors which are monitored. */
        Epoll::epoll.ctl(EPOLL_CTL_DEL, infd, NULL);
        ::close(infd);
        //std::cout << "Closed connection on descriptor " << infd << std::endl;
        infd=-1;
    }
}

ssize_t EpollClient::read(char *buffer,const size_t &bufferSize)
{
    if(infd==-1)
        return -1;
    const auto &bytesAvailableVar=bytesAvailable();
    //need more performance? change the API for 0 copy API
    if(bytesAvailableVar<=0)//non blocking for read
    {
        //good alternative?: Not work
        /*if(errno == 11)
            return ::read(infd, buffer, bufferSize);*/

        //valid but can be slow:
        const int &flags = fcntl(infd, F_GETFL, 0);
        if(flags == -1)
        {
            std::cerr << "fcntl get flags error" << std::endl;
            return -1;
        }

        if(flags & O_NONBLOCK)
            return ::read(infd, buffer, bufferSize);
        return bytesAvailableVar;
    }
    ssize_t count;
    if(bytesAvailableVar>0 && bytesAvailableVar<(ssize_t)bufferSize)
        count=::read(infd, buffer, bytesAvailableVar);
    else
        count=::read(infd, buffer, bufferSize);
    if(count == -1)
    {
        /* If errno == EAGAIN, that means we have read all
        data. So go back to the main loop. */
        if(errno != EAGAIN)
        {
            std::cerr << "Read socket error" << std::endl;
            close();
            return -1;
        }
    }
    return count;
}

ssize_t EpollClient::write(const char *buffer, const size_t &bufferSize)
{
    if(infd==-1)
        return -1;
    const ssize_t &size = ::write(infd, buffer, bufferSize);
    if(size != (ssize_t)bufferSize)
    {
        if(errno != EAGAIN)
        {
            if(errno!=104)
                std::cerr << "Write socket error, errno: " << errno << std::endl;
            else
                std::cerr << "Write socket error, Connection reset by peer (errno: 104)" << std::endl;
            close();
            return -1;
        }
        else
        {
            std::cerr << "Write socket full: EAGAIN for size:" << bufferSize << std::endl;
            #ifndef SERVERNOBUFFER
            if(this->bufferSize<BUFFER_MAX_SIZE)
            {
                if(size<0)
                {
                    if((this->bufferSize+bufferSize)<BUFFER_MAX_SIZE)
                    {
                        memcpy(this->buffer+this->bufferSize,buffer,bufferSize);
                        this->bufferSize+=bufferSize;
                        return bufferSize;
                    }
                    else
                    {
                        memcpy(this->buffer+this->bufferSize,buffer,BUFFER_MAX_SIZE-this->bufferSize);
                        this->bufferSize=BUFFER_MAX_SIZE;
                        return BUFFER_MAX_SIZE-this->bufferSize;
                    }
                }
                else
                {
                    const int &diff=bufferSize-size;
                    if((this->bufferSize+diff)<BUFFER_MAX_SIZE)
                    {
                        memcpy(this->buffer+this->bufferSize,buffer+size,diff);
                        this->bufferSize+=bufferSize;
                        return bufferSize;
                    }
                    else
                    {
                        memcpy(this->buffer+this->bufferSize,buffer+size,BUFFER_MAX_SIZE-this->bufferSize);
                        this->bufferSize=BUFFER_MAX_SIZE;
                        return BUFFER_MAX_SIZE-this->bufferSize;
                    }
                }
            }
            #else
            return size;
            #endif
        }
    }
    else
        return size;
}

#ifndef SERVERNOBUFFER
void EpollClient::flush()
{
    if(bufferSize>0)
    {
        char buf[512];
        size_t count=512;
        if(bufferSize<count)
            count=bufferSize;
        memcpy(buf,buffer,count);
        ssize_t size = ::write(infd, buf, count);
        if(size<0)
        {
            if(errno != EAGAIN)
            {
                std::cerr << "Write socket buffer error" << std::endl;
                close();
            }
        }
        else
        {
            bufferSize-=size;
            memmove(buffer,buffer+size,bufferSize);
        }
    }
}
#endif

BaseClassSwitch::EpollObjectType EpollClient::getType() const
{
    return BaseClassSwitch::EpollObjectType::Client;
}

bool EpollClient::isValid() const
{
    return infd!=-1;
}

long int EpollClient::bytesAvailable() const
{
    if(infd==-1)
        return -1;
    long int nbytes=0;
    // gives shorter than true amounts on Unix domain sockets.
    if(ioctl(infd, FIONREAD, &nbytes)>=0)
    {
        if(nbytes<0 || nbytes>1024*1024*1024)
        {
            /*if(errno!=11)
                std::cerr << "ioctl(infd, FIONREAD, &nbytes) return incorrect value: " << nbytes << ", errno: " << errno << std::endl;*/
            return -1;
        }
        return nbytes;
    }
    else
        return -1;
}
#endif
