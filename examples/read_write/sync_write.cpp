#include <benchmark/benchmark.h>

#include "leveldb/db.h"

namespace
{

std::shared_ptr<leveldb::DB> open_db(const std::string & db_name)
{
    leveldb::DB * db;
    leveldb::Options opts;

    opts.create_if_missing = true;

    leveldb::DB::Open(opts, db_name, &db);

    return std::shared_ptr<leveldb::DB>(db);
}

void benchmark_sync_write(benchmark::State& state)
{
    auto db = open_db("sync_benchmark");

    leveldb::WriteOptions write_options;
    write_options.sync = true;

    for (auto _ : state)
        db->Put(write_options, "key_sync", "value_sync");
}
BENCHMARK(benchmark_sync_write);

void benchmark_async_write(benchmark::State& state) {
    auto db = open_db("async_benchmark");

    leveldb::WriteOptions write_options;
    write_options.sync = false;

    for (auto _ : state)
        db->Put(write_options, "key_async", "value_async");
}
BENCHMARK(benchmark_async_write);
}

BENCHMARK_MAIN();
