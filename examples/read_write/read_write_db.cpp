#include <iostream>

#include "leveldb/db.h"

#include "check_status.h"

int main()
{
    leveldb::DB * db;

    // 打开数据库
    leveldb::Options options;
    options.create_if_missing = true;   
    db->Open(options, "testdb", &db);

    // 读数据
    std::string value;
    check_status(db->Get(leveldb::ReadOptions(), "key-a", &value));

    std::cout << "key-a: " << value << std::endl;

    // 写数据
    check_status(db->Put(leveldb::WriteOptions(), "key-a", "value-a"));

    // 读数据
    check_status(db->Get(leveldb::ReadOptions(), "key-a", &value));

    std::cout << "key-a: " << value << std::endl;

    // 删除数据
    check_status(db->Delete(leveldb::WriteOptions(), "key-a"));

    return 0;
}
