add_custom_target(break_lines
    COMMAND powershell
        ${CMAKE_CURRENT_SOURCE_DIR}/break_line.ps1
        -Root ${CMAKE_CURRENT_SOURCE_DIR}/Json -Width 100
    COMMENT "Break Lines in ${CMAKE_CURRENT_SOURCE_DIR}/Json"
    VERBATIM
)