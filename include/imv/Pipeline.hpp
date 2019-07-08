#pragma once
#include "imv/HashProbe.hpp"
#include "common/runtime/Database.hpp"
size_t filter_probe_scalar(size_t begin, size_t end, Database& db, runtime::Hashmap* hash_table, void** output_build, uint32_t*output_probe, uint64_t* pos_buff=nullptr);

size_t filter_probe_simd_amac(size_t begin, size_t end, Database& db, runtime::Hashmap* hash_table, void** output_build, uint32_t*output_probe, uint64_t* pos_buff=nullptr);
size_t filter_probe_simd_gp(size_t begin, size_t end, Database& db, runtime::Hashmap* hash_table, void** output_build, uint32_t*output_probe, uint64_t* pos_buff=nullptr);
size_t filter_probe_simd_imv(size_t begin, size_t end, Database& db, runtime::Hashmap* hash_table, void** output_build, uint32_t*output_probe, uint64_t* pos_buff=nullptr);
size_t filter_probe_imv(size_t begin, size_t end, Database& db, runtime::Hashmap* hash_table, void** output_build, uint32_t*output_probe, uint64_t* pos_buff=nullptr);
size_t filter_probe_imv1(size_t begin, size_t end, Database& db, runtime::Hashmap* hash_table, void** output_build, uint32_t*output_probe, uint64_t* pos_buff=nullptr);