# NifflerDB

NifflerDB is a simple, embeddable and persistent key-value store implemented as a B+ tree.

## Hello World
```c++
#include "niffler/db.h"

int main(int argc, char* argv[])
{
    // true will truncate the file if it exists
    auto niffler = std::make_unique<niffler::db>("hello_world.ndb", true);
    niffler->insert("Hello World!", "Hello World!");
    
    return 0;
}
```

## Public API(thread safe)
```c++
std::unique_ptr<find_result> find(const key& key) const;
bool exists(const key& key) const;
bool insert(const key& key, const void *data, u32 data_size);
bool remove(const key& key);
```

## TODO
* Better error handling
* Support for multiple "buckets"
* Unused pages clean-up
* Memory-mapped I/O?
* Transactions?

## OS Support
Only runs on Windows(at least right now) and has only been tested with the Visual C++ Compiler.
