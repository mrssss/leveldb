#include "leveldb/db.h"
#include "leveldb/write_batch.h"

#include "check_status.h"

int main()
{
    leveldb::DB * db;
    leveldb::Options opts;

    opts.create_if_missing = true;
    leveldb::DB::Open(opts, "testdb", &db);

    std::string value;
    
    check_status(db->Get(leveldb::ReadOptions(), "key-a", &value));

    leveldb::WriteBatch batch;
    batch.Delete("key-a");
    batch.Put("key-b", value);

    check_status(db->Write(leveldb::WriteOptions(), &batch));

    return 0;
}
