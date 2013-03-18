// A very simple utility for embedding resources inside a project using C++ static 
// arrays. This tends to be the simplest and most efficient way to embed a resource
// inside an executable.
//
// Compile: cl MakeResource.cpp
// Run: MakeResource.exe Image.gif > Image.cpp

#include <wrl.h>
#include <stdio.h>

namespace wrl = Microsoft::WRL::Wrappers;

#define ASSERT(expr) _ASSERTE(expr)
#ifdef _DEBUG
#define VERIFY(expr) ASSERT(expr)
#else
#define VERIFY(expr) (expr)
#endif

class file_view
{
    BYTE const * m_view;
    LARGE_INTEGER m_size;
    typedef wrl::HandleT<wrl::HandleTraits::HANDLENullTraits> MapHandle;

    file_view(file_view const &);
    file_view & operator=(file_view const &);
public:

    file_view(char const * name) throw() :
        m_view(),
        m_size()
    {
        ASSERT(name);

        wrl::FileHandle const file(CreateFileA(name,
                                               GENERIC_READ,
                                               FILE_SHARE_READ,
                                               nullptr, // default security
                                               OPEN_EXISTING,
                                               FILE_ATTRIBUTE_NORMAL,
                                               nullptr)); // no template

        if (!file.IsValid()) return;
        VERIFY(GetFileSizeEx(file.Get(), &m_size));
        if (m_size.QuadPart == 0) return;

        MapHandle const map(CreateFileMapping(file.Get(),
                                              nullptr, // default security
                                              PAGE_READONLY,
                                              0, 0, // match file size
                                              nullptr)); // no name

        VERIFY(map.IsValid());

        m_view = static_cast<BYTE const *>(MapViewOfFile(map.Get(),
                                                         FILE_MAP_READ,
                                                         0, 0, // offset
                                                         0)); // match file size
    }

    ~file_view() throw()
    {
        if (valid())
        {
            VERIFY(UnmapViewOfFile(m_view));
        }
    }

    bool valid() const throw() // If !valid() call GetLastError for reason
    {
        return nullptr != m_view;
    }

    BYTE const * begin() const throw()
    {
        ASSERT(valid());
        return m_view;
    }

    BYTE const * end() const throw()
    {
        return begin() + m_size.QuadPart;
    }
};

int main(int argc, char ** argv)
{
    if (2 != argc)
    {
        printf("MakeResource.exe input.png > output.cpp\n");
        return 0;
    }

    file_view const file(argv[1]);

    if (!file.valid())
    {
        printf("Failed to load resource.\n");
        return 0;
    }

    printf("static unsigned char MAKE_RESOURCE_DATA[] =\n{");

    unsigned column = 0;

    for (auto it = file.begin(); it != file.end(); ++it)
    {
        if (0 == column)
        {
            printf("\n    ");
        }

        printf("0x%02x, ", *it);

        if (100 == ++column)
        {
            column = 0;
        }
    }

    printf("\n};\n\nunsigned char const * MakeResourceBuffer() { return MAKE_RESOURCE_DATA; }\nunsigned MakeResourceSize() { return sizeof(MAKE_RESOURCE_DATA); }\n");
}
