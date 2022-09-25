project(examples)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples)

add_executable(open_close_db open_close/open_close_db.cpp)
target_link_libraries(open_close_db
    PRIVATE
        leveldb
)

add_executable(read_write_db read_write/read_write_db.cpp)
target_link_libraries(read_write_db
    PRIVATE
        leveldb
)
