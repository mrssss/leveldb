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

add_executable(write_batch read_write/write_batch.cpp)
target_link_libraries(write_batch
    PRIVATE
        leveldb
)

add_executable(sync_write_benchmark read_write/sync_write.cpp)
target_link_libraries(sync_write_benchmark
    PRIVATE
        leveldb
        benchmark
)
