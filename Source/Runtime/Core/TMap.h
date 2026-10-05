#pragma once
#include <unordered_map>

template <typename Key, typename Value, typename Hash = std::hash<Key>>
using TMap = std::unordered_map<Key, Value, Hash>;
