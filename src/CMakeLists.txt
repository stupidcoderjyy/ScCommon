set(TARGET sc_compile_options)

add_library(${TARGET} INTERFACE)
add_library(sc::compile_options ALIAS ${TARGET})

if (MSVC)
    target_compile_options(${TARGET} INTERFACE
        /utf-8
        $<$<CONFIG:Debug>:/Od /Zi /MDd>
        $<$<CONFIG:Release>:/O2 /MD>
    )
else ()
    target_compile_options(${TARGET} INTERFACE
        $<$<CONFIG:Debug>:-O0 -g>
        $<$<CONFIG:Release>:-O2>
    )
endif ()

add_subdirectory(json)
add_subdirectory(reflect)
add_subdirectory(common)
