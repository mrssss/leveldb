#include <iostream>

#include "leveldb/db.h"

#include "check_status.h"

int main()
{
    leveldb::DB * db;

    leveldb::Options options;
    options.create_if_missing = true;

    leveldb::DB::Open(options, "iteration", &db);

    auto db_guard = std::shared_ptr<leveldb::DB>(db);

    db_guard->Put(leveldb::WriteOptions(), "key-a", "value-a");
    db_guard->Put(leveldb::WriteOptions(), "key-b", "value-b");
    db_guard->Put(leveldb::WriteOptions(), "key-c", "value-c");
    db_guard->Put(leveldb::WriteOptions(), "key-d", "value-d");
    db_guard->Put(leveldb::WriteOptions(), "key-e", "value-e");

    leveldb::Iterator * it = db_guard->NewIterator(leveldb::ReadOptions());

    for (it->SeekToFirst(); it->Valid(); it->Next())
        std::cout << it->key().ToString() << ": " << it->value().ToString() << std::endl;

    return 0;
}
