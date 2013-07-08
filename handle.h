#pragma once

// Created by Kenny Kerr.
// Get the latest version here: http://dx.codeplex.com

#include <windows.h>
#include <crtdbg.h>

#define HANDLE_ASSERT _ASSERTE

#ifdef _DEBUG
#define HANDLE_VERIFY(expression) HANDLE_ASSERT(expression)
#define HANDLE_VERIFY_(expected, expression) HANDLE_ASSERT(expected == expression) 
#else
#define HANDLE_VERIFY(expression) (expression)
#define HANDLE_VERIFY_(expected, expression) (expression) 
#endif

namespace KennyKerr
{
    template <typename Traits>
    class unique_handle
    {
        struct boolean_struct { int member; };
        typedef int boolean_struct::* boolean_type;
        typedef typename Traits::pointer pointer;

        unique_handle(unique_handle const &);
        unique_handle & operator=(unique_handle const &);
        bool operator==(unique_handle const &);
        bool operator!=(unique_handle const &);

        void close() throw()
        {
            if (*this)
            {
                Traits::close(m_value);
            }
        }

        pointer m_value;

    public:

        explicit unique_handle(pointer value = Traits::invalid()) throw() :
            m_value(value)
        {
        }

        ~unique_handle() throw()
        {
            close();
        }

        unique_handle(unique_handle && other) throw() :
            m_value(other.release())
        {
        }

        unique_handle & operator=(unique_handle && other) throw()
        {
            HANDLE_ASSERT(this != &other);

            reset(other.release());
            return *this;
        }

        operator boolean_type() const throw()
        {
            return Traits::invalid() != m_value ? &boolean_struct::member : nullptr;
        }

        pointer get() const throw()
        {
            return m_value;
        }
        
        bool reset(pointer value = Traits::invalid()) throw()
        {
            if (m_value != value)
            {
                close();
                m_value = value;
            }

            return *this;
        }

        pointer release() throw()
        {
            auto value = m_value;
            m_value = Traits::invalid();
            return value;
        }
    };

    struct null_handle_traits
    {
        typedef HANDLE pointer;

        static pointer invalid() throw()
        {
            return nullptr;
        }

        static void close(pointer value) throw()
        {
            HANDLE_VERIFY(CloseHandle(value));
        }
    };

    struct invalid_handle_traits
    {
        typedef HANDLE pointer;

        static pointer invalid() throw()
        {
            return INVALID_HANDLE_VALUE;
        }

        static void close(pointer value) throw()
        {
            HANDLE_VERIFY(CloseHandle(value));
        }
    };

    struct registry_key_traits
    {
        typedef HKEY pointer;

        static pointer invalid() throw()
        {
            return nullptr;
        }

        static void close(pointer value) throw()
        {
            HANDLE_VERIFY_(ERROR_SUCCESS, RegCloseKey(value));
        }
    };

    typedef unique_handle<null_handle_traits> null_handle;
    typedef unique_handle<invalid_handle_traits> invalid_handle;
    typedef unique_handle<registry_key_traits> registry_key;

    static_assert(sizeof(HANDLE) == sizeof(null_handle), "The handle wrapper should not impose any space overhead");
}
