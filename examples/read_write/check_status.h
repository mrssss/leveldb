#pragma once

#include "leveldb/db.h"
#include <iostream>

namespace
{

inline void check_status(const leveldb::Status & s)
{
    if (!s.ok())
        std::cerr << s.ToString() << std::endl;
}
}