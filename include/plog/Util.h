#pragma once
#include <time.h>
#include <fcntl.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <io.h>
#include <Windows.h>
#else
#include <sys/io.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <pthread.h>
#endif

namespace plog
{
    namespace util
    {
        inline void localtime_s(struct tm* t, const time_t* time)
        {
#ifdef _WIN32
            ::localtime_s(t, time);
#else
            ::localtime_r(time, t);
#endif
        }

        inline unsigned int gettid()
        {
#ifdef _WIN32
            return ::GetCurrentThreadId();
#elif defined(__unix__)
            return ::syscall(__NR_gettid);
#elif defined(__APPLE__)
            return static_cast<unsigned int>(::syscall(SYS_thread_selfid));
#endif
        }

        class File
        {
        public:
            File() : m_fd(-1)
            {
            }

            File(const char* fileName) : m_fd(-1)
            {
                open(fileName);
            }

            ~File()
            {
                close();
            }

            void open(const char* fileName)
            {
#ifdef _WIN32
                ::_sopen_s(&m_fd, fileName, O_CREAT | O_APPEND | O_TEXT | O_WRONLY, _SH_DENYWR, S_IREAD | S_IWRITE);
#else
                m_fd = ::open(fileName, O_CREAT | O_APPEND | O_WRONLY, S_IREAD | S_IWRITE);
#endif
            }

            void write(const void* buf, size_t count)
            {
#ifdef _WIN32
                ::_write(m_fd, buf, count);
#else
                ::write(m_fd, buf, count);
#endif
            }

            off_t getSize()
            {
                struct stat st = {};
                fstat(m_fd, &st);
                
                return st.st_size;
            }

            void close()
            {
                if (-1 != m_fd)
                {
#ifdef _WIN32
                    ::_close(m_fd);
#else
                    ::close(m_fd);
#endif
                    m_fd = -1;
                }
            }

            static void unlink(const char* fileName)
            {
#ifdef _WIN32
                ::_unlink(fileName);
#else
                ::unlink(fileName);
#endif
            }

            static void rename(const char* oldFilename, const char* newFilename)
            {
                ::rename(oldFilename, newFilename);
            }

        private:
            File(const File&);
            File& operator=(const File&);

        private:
            int m_fd;
        };

        class Mutex
        {
        public:
            Mutex()
            {
#ifdef _WIN32
                ::InitializeCriticalSection(&m_sync);
#else
                ::pthread_mutex_init(&m_sync, 0);
#endif
            }

            ~Mutex()
            {
#ifdef _WIN32
                ::DeleteCriticalSection(&m_sync);
#else
                ::pthread_mutex_destroy(&m_sync);
#endif
            }

            friend class MutexLock;

        private:
            void lock()
            {
#ifdef _WIN32
                ::EnterCriticalSection(&m_sync);
#else
                ::pthread_mutex_lock(&m_sync);
#endif
            }

            void unlock()
            {
#ifdef _WIN32
                ::LeaveCriticalSection(&m_sync);
#else
                ::pthread_mutex_unlock(&m_sync);
#endif
            }

        private:
            Mutex(const Mutex&);
            Mutex& operator=(const Mutex&);

        private:
#ifdef _WIN32
            CRITICAL_SECTION m_sync;
#else
            pthread_mutex_t m_sync;
#endif
        };

        class MutexLock
        {
        public:
            MutexLock(Mutex& mutex) : m_mutex(mutex)
            {
                m_mutex.lock();
            }

            ~MutexLock()
            {
                m_mutex.unlock();
            }

        private:
            Mutex& m_mutex;
        };

        inline std::string toString(const wchar_t* wstr)
        {
#ifdef _WIN32
            //std::mbstate_t state = std::mbstate_t();
            //int len = 1 + std::wcsrtombs(0, &wstr, 0, &state);
            int len = ::WideCharToMultiByte(0, 0, wstr, wcslen(wstr), 0, 0, 0, 0);
                
            std::string str(len, 0);
            ::WideCharToMultiByte(0, 0, wstr, wcslen(wstr), &str[0], str.size(), 0, 0);
            //std::wcsrtombs(&str[0], &wstr, str.size(), &state);

            return str;
#else
            // TODO: implement
            return std::string();
#endif
        }

        inline void splitFileName(const char* fileName, std::string& fileNameNoExt, std::string& fileExt)
        {
            const char* dot = std::strrchr(fileName, '.');
            
            if (dot)
            {
                fileNameNoExt.assign(fileName, dot);
                fileExt.assign(dot + 1);
            }
            else
            {
                fileNameNoExt.assign(fileName);
                fileExt.clear();
            }
        }

        //TODO: add file open func
        //HANDLE h = ::CreateFile(fileName, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_DELETE, 0, OPEN_ALWAYS, 0, 0);
        //int fd = _open_osfhandle(reinterpret_cast<intptr_t>(h), _O_APPEND);
        //m_file = _fdopen(fd, "a");
    }
}