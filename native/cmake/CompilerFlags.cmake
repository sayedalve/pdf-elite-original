# cmake/CompilerFlags.cmake

if(MSVC)
    add_compile_options(
        /W4
        /WX
        /utf-8
        /permissive-
        /Zc:__cplusplus
        /Zc:externC
        /Zc:wchar_t
        /EHsc
    )

    add_compile_definitions(
        NOMINMAX
        WIN32_LEAN_AND_MEAN
        VC_EXTRALEAN
        UNICODE
        _UNICODE
        WINVER=0x0A00
        _WIN32_WINNT=0x0A00
        NTDDI_VERSION=0x0A000006
    )

    # Debug specific
    add_compile_options("$<$<CONFIG:Debug>:/Od;/RTC1;/JMC>")

    # Release specific
    add_compile_options("$<$<CONFIG:Release>:/O2;/GL;/Ob2>")
    add_compile_definitions("$<$<CONFIG:Release>:NDEBUG>")
    
    # RelWithDebInfo
    add_compile_options("$<$<CONFIG:RelWithDebInfo>:/O2>")

    # Linker flags
    add_link_options("$<$<CONFIG:Release>:/HIGHENTROPYVA;/guard:cf;/OPT:REF;/OPT:ICF>")
    add_link_options("/DYNAMICBASE" "/NXCOMPAT")
endif()
