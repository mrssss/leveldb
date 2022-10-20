leveldb
=======

_Jeff Dean, Sanjay Ghemawat_

leveldb库提供了一个可持久化的键-值对存储。键和值可以是任意的字节序列。在键-值对存储中，键是
按照用户指定的比较函数来进行排序的。

## 打开一个数据库

一个leveldb的数据库有一个和文件系统中的目录相关的名字。所有数据库相关的内容都会存放在这个文
件夹下。下面是一个怎么打开一个数据库的例子，如果这个库不存在的话，它会创建一个:

```c++
#include <cassert>
#include "leveldb/db.h"

leveldb::DB* db;
leveldb::Options options;
options.create_if_missing = true;
leveldb::Status status = leveldb::DB::Open(options, "/tmp/testdb", &db);
assert(status.ok());
...
```

你想要在数据库存在时抛出一个错误，那么你可以在调用`leveldb::DB::Open`之前调
用下面这一行:

```c++
options.error_if_exists = true;
```

## Status

你可能已经注意到前面使用到的`leveldb::Status`这个类型了。leveldb中大多数函数
会通过返回一个`leveldb::Status`类型的值来表示是否有错误产生。你可以通过检查它
的结果来确定函数是否正常的执行。
下面这段代码可以打印出错误的信息:

```c++
leveldb::Status s = ...;
if (!s.ok()) cerr << s.ToString() << endl;
```

## 关闭一个数据库

当你执行完对数据库的操作之后，可以直接删除数据库对象即可。例如:

```c++
... open the db as described above ...
... do something with db ...
delete db;
```

## 读写操作

数据库提供了更新(Put)，删除(Delete)和读取(Get)的方法来修改和查询数据库。
例如，下面的这段代码把key1中存储的值移动到key2中:

```c++
std::string value;
leveldb::Status s = db->Get(leveldb::ReadOptions(), key1, &value);
if (s.ok()) s = db->Put(leveldb::WriteOptions(), key2, value);
if (s.ok()) s = db->Delete(leveldb::WriteOptions(), key1);
```

## 原子更新

需要注意的是，如果进程在添加key2之后、删除key1之前挂掉了，那么这个值会存储在
多个键中。这样的问题可以通过使用`WriteBatch`类来避免掉。这个类可以原子性的更
新一批改动到数据库中:

```c++
#include "leveldb/write_batch.h"
...
std::string value;
leveldb::Status s = db->Get(leveldb::ReadOptions(), key1, &value);
if (s.ok()) {
  leveldb::WriteBatch batch;
  batch.Delete(key1);
  batch.Put(key2, value);
  s = db->Write(leveldb::WriteOptions(), &batch);
}
```

`WriteBatch`会保留一系列对数据库的改动，这些改动会按照顺序更新在数据库上面。
我们在Put之前调用Delete，这样如果key1和key2相同，我们也不会错误的把这个值给删除掉。

使用`WriteBatch`的另外一个好处是，通过把许多独立的更新操作放进同一个batch中
写入数据库，可以加速批量更新的速度。

## 同步写操作

默认情况下，leveldb的写操作都是异步的:它把写操作从进程提交给操作系统之后就返回了。
至于什么时候从内存持久化到磁盘是异步发生的。如果有特殊的写操作要求是同步的话，可以
通过打开同步标志来控制同步写操作。这样的话，只有当数据写入了持久化存储之后，写操
作的函数调用才会返回。(在实现了POSIX的系统中，这是通过在执行写操作之前调用`fsync(...)`
或`fdatasync(...)`或`msync(..., MS_SYNC)`来实现)。

```c++
leveldb::WriteOptions write_options;
write_options.sync = true;
db->Put(write_options, ...);
```

通常异步写操作会比同步写操作快1000倍以上。异步写操作的缺点在于，如果机器宕机了可
能会导致最后的一些少量的更新会丢失。如果只是进程的崩溃，即使sync标志是false，也
不会导致任何数据的丢失，一个数据更新从进程内存提交给操作系统之后就被当做操作完成了。

异步操作通常可以安全地被使用。例如，当要写入一块很大的数据的时候程序崩溃了，你可
以在崩溃恢复后重新写入这一批数据。混合架构也是可行的：每N个写操作都以一个同步的
写操作结束(其他都是异步写操作)。当程序崩溃的时候，重新写入上次同步写操作之后的所
有数据(同步写操作可以用来更新从什么位置继续写操作的标记)。

`WriteBatch`提供了一种异步写的替代方案。可以将多个更新放到同一个WriteBatch中，
并且使用同步的写操作更新数据。(把`write_options.sync`设置为true)。这样的话，
同步写操作的额外开销会被分摊到一个batch中的所以写操作上。

## 并发

在同一时刻，只能有一个进程打开某个数据库。为了防止误用，leveldb会从操作系统那里申
请一个文件锁。在同一个进城内，同样的一个`leveldb::DB`对象可以安全地被多个并发的
线程共享。例如，不同的线程可以在不需要外部同步机制的情况下对同一个数据库进行写入、
获取迭代器和读的操作（leveldb的有内部的同步机制）。但是其他的一些对象（Iterator
和`WriteBatch`）会需要额外的同步机制。如果两个线程共享同一个对象，它们必须使用自
己的锁机制。更多的细节可以看公共的头文件中的内容。

## 迭代器

下面的例子说明了怎么打印数据库中所有的键-值对。

```c++
leveldb::Iterator* it = db->NewIterator(leveldb::ReadOptions());
for (it->SeekToFirst(); it->Valid(); it->Next()) {
  cout << it->key().ToString() << ": "  << it->value().ToString() << endl;
}
assert(it->status().ok());  // Check for any errors found during the scan
delete it;
```

下面的例子展示了怎么处理在范围[start, limit)内的键。

```c++
for (it->Seek(start);
   it->Valid() && it->key().ToString() < limit;
   it->Next()) {
  ...
}
```

你也可以逆向处理所有的键值对（逆向迭代器比前向迭代器慢一些）。

```c++
for (it->SeekToLast(); it->Valid(); it->Prev()) {
  ...
}
```

## 快照

快照提供了一个完整的包含了所有键值存储状态的持久化的只读视图。`ReadOptions::snapshot`
不是空代表着需要读特定版本的数据库快照。如果`ReadOptions::snapshot`为空，会读取
当前数据库的状态。


Snapshots are created by the `DB::GetSnapshot()` method:
快照是通过`DB::GetSnapshot()`方法来创建的：

```c++
leveldb::ReadOptions options;
options.snapshot = db->GetSnapshot();
... apply some updates to db ...
leveldb::Iterator* iter = db->NewIterator(options);
... read using iter to view the state when the snapshot was created ...
delete iter;
db->ReleaseSnapshot(options.snapshot);
```

需要注意的是当不需要使用一个快照的时候，可以使用`DB::ReleaseSnapshot`接口。
然后leveldb的实现就不会再去维护这个快照相关的状态数据了。

## 切片

`it->key()`和`it->value()`会返回一个`leveldb::Slice`类型的实例。切片是一个
包含有长度和指向外部字节数据的指针的简单的数据结构。切片是对`std::string`的简单
替代。这样对于大一点的键和值来说，可以省了很多拷贝数据的开销。除此之外，leveldb的
方法不会返回以null结尾的C语言风格的字符串，也就是说leveldb的键和值中可以包含`'\0'`
字节。

C++字符串和null结尾的C风格字符串可以很轻易的转换成一个切片：

```c++
leveldb::Slice s1 = "hello";

std::string str("world");
leveldb::Slice s2 = str;
```

切片也同样可以很容易的转换成C++字符串

```c++
std::string str = s1.ToString();
assert(str == std::string("hello"));
```

使用Slice的时候需要它的调用者来负责它指针指向的外部的字节数组的生命周期，否则会
遇到下面的这种bug：

```c++
leveldb::Slice slice;
if (...) {
  std::string str = ...;
  slice = str;
}
Use(slice);
```

当出了if块的时候str对象会被释放掉，slice中的指针就会指向一块已经被释放的内存。

## 比较器

在前面的例子中，用到了默认的排序函数根据键来排序，默认采用了字节序。我们也可以在
打开数据库的使用提供一个自定义的比较器。例如，如果一个数据库中的键包含两个数字，
我们可以先根据第一个数字进行排序；如果相等，再使用第二个数字进行排序。

首先，我们会定义一个`leveldb::Comparator`的子类来表示这个规则：

```c++
class TwoPartComparator : public leveldb::Comparator {
 public:
  // Three-way comparison function:
  //   if a < b: negative result
  //   if a > b: positive result
  //   else: zero result
  int Compare(const leveldb::Slice& a, const leveldb::Slice& b) const {
    int a1, a2, b1, b2;
    ParseKey(a, &a1, &a2);
    ParseKey(b, &b1, &b2);
    if (a1 < b1) return -1;
    if (a1 > b1) return +1;
    if (a2 < b2) return -1;
    if (a2 > b2) return +1;
    return 0;
  }

  // Ignore the following methods for now:
  const char* Name() const { return "TwoPartComparator"; }
  void FindShortestSeparator(std::string*, const leveldb::Slice&) const {}
  void FindShortSuccessor(std::string*) const {}
};
```

接着创建一个使用这个自定义比较器的数据库：

```c++
TwoPartComparator cmp;
leveldb::DB* db;
leveldb::Options options;
options.create_if_missing = true;
options.comparator = &cmp;
leveldb::Status status = leveldb::DB::Open(options, "/tmp/testdb", &db);
...
```

### 后向兼容性

当创建一个数据库的时候，会给这个数据库附加上比较器Name方法的返回结果，在以后每次
打开这个数据库的时候，都会检查这个名字。如果这个名字改变了，`leveldb::DB::Open`
会调用失败。所以，当且仅当：
1. 数据库键的格式发生变化
2. 比较函数不能和现有的数据库兼容的时候
3. 允许丢掉现有数据库中存储的所有数据
这三个条件同时满足的情况下，才能够改变比较器的名称。

尽管如此，你还是可以有计划的逐步的修改你的键的格式。比如你可以在每个键的末尾存储
一个version字段（一个字节就够用了）。当你希望切换到新的键格式的时候（比如，在前
面的例子中，我们想给键加第三个数字）：
1. 使用同一个比较器的名称
2. 新的键使用新的version编号
3. 更新比较器函数，让它能够根据version编号来决定怎么处理这个键

## 性能调优

可以通过改变在`include/options.h`中定义的类型的默认值来做性能调优。

### Block大小

leveldb会将临近的键组织到同一个block中，这样的一个block是用来持久化的最基本的
单元。默认的block大小是大约4096个未压缩的字节。

通常进行批量读的应用程序会倾向于增加这个值。而对于经常进行单点读取小变量的应用程
序则会倾向于设置一个小一点的block size。这样性能会有所提升。

但是，使用小于1KB的或者大于几MB的block size不会对性能有所帮助。block size的
性能调优需要在1KB～MB的范围之间进行。

除此之外，大一些的block size对于压缩算法更有帮助。

### 压缩

在写入到持久化的存储之前，每个block会被单独的进行压缩。现在，默认的压缩算法的
速度是非常快的，所以一般压缩功能都会打开，仅有一小部分没有办法压缩的数据是不进
行压缩操作的。
在极少数的用例中，应用程序想要关闭所有的数据压缩的功能，但是我们也需要在看到
benchmarks测试显示有性能提升的时候才能够这么做（关闭数据压缩功能）：

```c++
leveldb::Options options;
options.compression = leveldb::kNoCompression;
... leveldb::DB::Open(options, name, ...) ....
```

### 缓存

数据库中的内容是存储在文件系统的一系列的文件中的。每个文件中存储了压缩过的block
的序列。如果`options.block_cache`被设置成非空，那么最常被访问到的解压后的
block的内容会被缓存起来。

```c++
#include "leveldb/cache.h"

leveldb::Options options;
options.block_cache = leveldb::NewLRUCache(100 * 1048576);  // 100MB cache
leveldb::DB* db;
leveldb::DB::Open(options, name, &db);
... use the db ...
delete db
delete options.block_cache;
```

Note that the cache holds uncompressed data, and therefore it should be sized
according to application level data sizes, without any reduction from
compression. (Caching of compressed blocks is left to the operating system
buffer cache, or any custom Env implementation provided by the client.)

When performing a bulk read, the application may wish to disable caching so that
the data processed by the bulk read does not end up displacing most of the
cached contents. A per-iterator option can be used to achieve this:

```c++
leveldb::ReadOptions options;
options.fill_cache = false;
leveldb::Iterator* it = db->NewIterator(options);
for (it->SeekToFirst(); it->Valid(); it->Next()) {
  ...
}
delete it;
```

### Key Layout

Note that the unit of disk transfer and caching is a block. Adjacent keys
(according to the database sort order) will usually be placed in the same block.
Therefore the application can improve its performance by placing keys that are
accessed together near each other and placing infrequently used keys in a
separate region of the key space.

For example, suppose we are implementing a simple file system on top of leveldb.
The types of entries we might wish to store are:

    filename -> permission-bits, length, list of file_block_ids
    file_block_id -> data

We might want to prefix filename keys with one letter (say '/') and the
`file_block_id` keys with a different letter (say '0') so that scans over just
the metadata do not force us to fetch and cache bulky file contents.

### Filters

Because of the way leveldb data is organized on disk, a single `Get()` call may
involve multiple reads from disk. The optional FilterPolicy mechanism can be
used to reduce the number of disk reads substantially.

```c++
leveldb::Options options;
options.filter_policy = NewBloomFilterPolicy(10);
leveldb::DB* db;
leveldb::DB::Open(options, "/tmp/testdb", &db);
... use the database ...
delete db;
delete options.filter_policy;
```

The preceding code associates a Bloom filter based filtering policy with the
database.  Bloom filter based filtering relies on keeping some number of bits of
data in memory per key (in this case 10 bits per key since that is the argument
we passed to `NewBloomFilterPolicy`). This filter will reduce the number of
unnecessary disk reads needed for Get() calls by a factor of approximately
a 100. Increasing the bits per key will lead to a larger reduction at the cost
of more memory usage. We recommend that applications whose working set does not
fit in memory and that do a lot of random reads set a filter policy.

If you are using a custom comparator, you should ensure that the filter policy
you are using is compatible with your comparator. For example, consider a
comparator that ignores trailing spaces when comparing keys.
`NewBloomFilterPolicy` must not be used with such a comparator. Instead, the
application should provide a custom filter policy that also ignores trailing
spaces. For example:

```c++
class CustomFilterPolicy : public leveldb::FilterPolicy {
 private:
  leveldb::FilterPolicy* builtin_policy_;

 public:
  CustomFilterPolicy() : builtin_policy_(leveldb::NewBloomFilterPolicy(10)) {}
  ~CustomFilterPolicy() { delete builtin_policy_; }

  const char* Name() const { return "IgnoreTrailingSpacesFilter"; }

  void CreateFilter(const leveldb::Slice* keys, int n, std::string* dst) const {
    // Use builtin bloom filter code after removing trailing spaces
    std::vector<leveldb::Slice> trimmed(n);
    for (int i = 0; i < n; i++) {
      trimmed[i] = RemoveTrailingSpaces(keys[i]);
    }
    builtin_policy_->CreateFilter(trimmed.data(), n, dst);
  }
};
```

Advanced applications may provide a filter policy that does not use a bloom
filter but uses some other mechanism for summarizing a set of keys. See
`leveldb/filter_policy.h` for detail.

## Checksums

leveldb associates checksums with all data it stores in the file system. There
are two separate controls provided over how aggressively these checksums are
verified:

`ReadOptions::verify_checksums` may be set to true to force checksum
verification of all data that is read from the file system on behalf of a
particular read.  By default, no such verification is done.

`Options::paranoid_checks` may be set to true before opening a database to make
the database implementation raise an error as soon as it detects an internal
corruption. Depending on which portion of the database has been corrupted, the
error may be raised when the database is opened, or later by another database
operation. By default, paranoid checking is off so that the database can be used
even if parts of its persistent storage have been corrupted.

If a database is corrupted (perhaps it cannot be opened when paranoid checking
is turned on), the `leveldb::RepairDB` function may be used to recover as much
of the data as possible

## Approximate Sizes

The `GetApproximateSizes` method can used to get the approximate number of bytes
of file system space used by one or more key ranges.

```c++
leveldb::Range ranges[2];
ranges[0] = leveldb::Range("a", "c");
ranges[1] = leveldb::Range("x", "z");
uint64_t sizes[2];
db->GetApproximateSizes(ranges, 2, sizes);
```

The preceding call will set `sizes[0]` to the approximate number of bytes of
file system space used by the key range `[a..c)` and `sizes[1]` to the
approximate number of bytes used by the key range `[x..z)`.

## Environment

All file operations (and other operating system calls) issued by the leveldb
implementation are routed through a `leveldb::Env` object. Sophisticated clients
may wish to provide their own Env implementation to get better control.
For example, an application may introduce artificial delays in the file IO
paths to limit the impact of leveldb on other activities in the system.

```c++
class SlowEnv : public leveldb::Env {
  ... implementation of the Env interface ...
};

SlowEnv env;
leveldb::Options options;
options.env = &env;
Status s = leveldb::DB::Open(options, ...);
```

## Porting

leveldb may be ported to a new platform by providing platform specific
implementations of the types/methods/functions exported by
`leveldb/port/port.h`.  See `leveldb/port/port_example.h` for more details.

In addition, the new platform may need a new default `leveldb::Env`
implementation.  See `leveldb/util/env_posix.h` for an example.

## Other Information

Details about the leveldb implementation may be found in the following
documents:

1. [Implementation notes](impl.md)
2. [Format of an immutable Table file](table_format.md)
3. [Format of a log file](log_format.md)
