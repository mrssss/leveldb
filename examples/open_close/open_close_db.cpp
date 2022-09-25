#include <iostream>

#include "leveldb/db.h"

int main()
{
    leveldb::DB * db;
    leveldb::Options options;

    options.create_if_missing = true;   // 如果数据库不存在，则创建它
    // options.error_if_exists = true;     // 如果数据库存在，则抛出一个错误
    
    leveldb::Status status = leveldb::DB::Open(options, "testdb", &db);         // 相对地址
    // leveldb::Status status = leveldb::DB::Open(options, "/tmp/testdb", &db);    // 绝对地址
    
    if (!status.ok())
        std::cerr << status.ToString() << std::endl;

    delete db;      // 关闭数据库

    return 0;
}
