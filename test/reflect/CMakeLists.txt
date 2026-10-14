add_test_target(test_reflect_count_fields
    SOURCES count_fields_test.cpp
    LIBS sc::reflect
)

add_test_target(test_reflect_get_fields
    SOURCES get_fields_test.cpp
    LIBS sc::reflect
)
