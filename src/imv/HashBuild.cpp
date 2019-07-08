#include "imv/HashBuild.hpp"
#include <iostream>
size_t build_raw(size_t begin, size_t end, Database& db, runtime::Hashmap* hash_table, Allocator* allo, int entry_size) {
  size_t found = 0;
  auto& ord = db["orders"];
  auto o_orderkey = ord["o_orderkey"].data<types::Integer>();
  int build_key_off = sizeof(runtime::Hashmap::EntryHeader);
  for (size_t i = begin; i < end; ++i) {
    Hashmap::EntryHeader* ptr = (Hashmap::EntryHeader*) allo->allocate(entry_size);
    *(int*) (((char*) ptr) + build_key_off) = o_orderkey[i].value;
    hash_t hash_value = hash()(o_orderkey[i], primitives::seed);
    //auto head= ht1.entries+ptr->h.hash;
    hash_table->insert_tagged(ptr, hash_value);
    ++found;
  }
  return found;
}
size_t build_simd(size_t begin, size_t end, Database& db, runtime::Hashmap* hash_table, Allocator*allo, int entry_size) {
  size_t found = 0;
  auto& ord = db["orders"];
  auto o_orderkey = ord["o_orderkey"].data<types::Integer>();
  int build_key_off = sizeof(runtime::Hashmap::EntryHeader);
  __m512i v_build_key, v_offset, v_base_entry_off, v_entry_addr, v_key_off = _mm512_set1_epi64(build_key_off), v_build_hash_mask, v_zero =
      _mm512_set1_epi64(0), v_all_ones = _mm512_set1_epi64(-1), v_conflict, v_base_offset = _mm512_set_epi64(7, 6, 5, 4, 3, 2, 1, 0), v_seed =
      _mm512_set1_epi64(primitives::seed), v_build_hash, v_old_next, v_old_next_ptr;

  __mmask8 m_valid_build = -1, m_no_conflict, m_rest;
  __m256i v256_zero = _mm256_set1_epi32(0);
  v_base_entry_off = _mm512_mullo_epi64(v_base_offset, _mm512_set1_epi64(entry_size));
  for (size_t i = begin; i < end;) {
    /// step 1: gather build keys (using gather to compilate with loading discontinuous values)
    v_offset = _mm512_add_epi64(_mm512_set1_epi64(i), v_base_offset);
    i += VECTORSIZE;
    if (i >= end) {
      m_valid_build = (m_valid_build >> (i - end));
    }
    auto v256_build_key = _mm512_mask_i64gather_epi32(v256_zero, m_valid_build, v_offset, o_orderkey, 4);
    v_build_key = _mm512_cvtepi32_epi64(v256_build_key);

    /// step 2: allocate new entries
    Hashmap::EntryHeader* ptr = (Hashmap::EntryHeader*) allo->allocate(VECTORSIZE * entry_size);

    /// step 3: write build keys to new entries
    v_entry_addr = _mm512_add_epi64(_mm512_set1_epi64((uint64_t) ptr), v_base_entry_off);
    _mm512_i64scatter_epi64(0, _mm512_add_epi64(v_entry_addr, v_key_off), v_build_key, 1);

    /// step 4: hashing the build keys (note the hash value cannot be used to directly fetch buckets)
    v_build_hash = runtime::MurMurHash()(v_build_key, v_seed);
    v_build_hash_mask = ((Vec8u(v_build_hash) & Vec8u(hash_table->mask)));
    /// step 5: insert new entries into the buckets
    m_rest = m_valid_build;
#if 1
    while (m_rest) {
      // note the write conflicts
      v_conflict = _mm512_conflict_epi64(v_build_hash_mask);
      m_no_conflict = _mm512_testn_epi64_mask(v_conflict, v_all_ones);
      m_no_conflict = _mm512_kand(m_no_conflict, m_rest);
      found += _mm_popcnt_u32(m_no_conflict);
      // fetch old next
      v_old_next = _mm512_mask_i64gather_epi64(v_zero, m_no_conflict, v_build_hash_mask, hash_table->entries, 8);
      v_old_next_ptr = hash_table->ptr(v_old_next);
      // write old next to the next of new entries
      _mm512_mask_i64scatter_epi64(0, m_no_conflict, v_entry_addr, (v_old_next_ptr), 1);
      // overwrite the old next with new entries
      _mm512_mask_i64scatter_epi64(hash_table->entries, m_no_conflict, v_build_hash_mask, hash_table->update(v_old_next, (v_entry_addr), (v_build_hash)), 8);
      // update
      m_rest = _mm512_kandn(m_no_conflict, m_rest);
      v_build_hash_mask = _mm512_mask_blend_epi64(m_rest, v_all_ones, v_build_hash_mask);
    }

#else
    Vec8u entry = Vec8u(v_entry_addr);
    Vec8u hash_value = Vec8u(v_build_hash);
    for (int j = 0; j + i - VECTORSIZE < end && j<VECTORSIZE; ++j) {
      ++found;
      hash_table->insert_tagged((Hashmap::EntryHeader*) entry.entry[j], hash_value.entry[j]);
      for(int k=0;k<j;++k) {
        if((hash_value.entry[k] & hash_table->mask)==(hash_value.entry[j] & hash_table->mask)) {
          std::cout<<"conflits"<<std::endl;
        }
      }
    }

#endif
  }
  return found;
}