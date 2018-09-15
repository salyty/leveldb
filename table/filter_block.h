// Copyright (c) 2012 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.
//
// A filter block is stored near the end of a Table file.  It contains
// filters (e.g., bloom filters) for all data blocks in the table combined
// into a single filter block.

#ifndef STORAGE_LEVELDB_TABLE_FILTER_BLOCK_H_
#define STORAGE_LEVELDB_TABLE_FILTER_BLOCK_H_

#include <stddef.h>
#include <stdint.h>
#include <string>
#include <vector>
#include "leveldb/slice.h"
#include "util/hash.h"

namespace leveldb {

class FilterPolicy;

// A FilterBlockBuilder is used to construct all of the filters for a
// particular Table.  It generates a single string which is stored as
// a special block in the Table.
//
// The sequence of calls to FilterBlockBuilder must match the regexp:
//      (StartBlock AddKey*)* Finish

// 一个SSTable 只创建一个 FilterBlock（属于 MetaBlock 的一种）
// 这个FilterBlock包含对该表的所有 DataBlock 的布隆过滤器。
// 每个DataBlock根据它在表中的偏移量block_offset落到指定的 filter[filter_index]中，
// filter_index = (block_offset / kFilterBase)
class FilterBlockBuilder {
 public:
  explicit FilterBlockBuilder(const FilterPolicy*);
  //对于每个 DataBlock， 先调用一次StartBlock，然后再调用AddKey将该 block 的所有 key 填进去
  void StartBlock(uint64_t block_offset);
  void AddKey(const Slice& key);
  //最后，填充 filterOffsetArray 和 offset of filterOffsetArray
  Slice Finish();

 private:
  void GenerateFilter();

  //在创建的过程中，policy 不变，result_和filter_offsets_不断增加。
  //start_，tmp_keys_，keys_每次创建一个新的 filter 后清空重置
  const FilterPolicy* policy_;
  std::string keys_;              // Flattened key contents
  std::vector<size_t> start_;     // Starting index in keys_ of each key
  //整个 Meta Block 的数据，一步步增长，先填充 filterArray, 再追加 filter_offsets_
  std::string result_;            // Filter data computed so far
  std::vector<Slice> tmp_keys_;   // policy_->CreateFilter() argument
  //每个 filter 的偏移量
  std::vector<uint32_t> filter_offsets_;

  // No copying allowed
  FilterBlockBuilder(const FilterBlockBuilder&);
  void operator=(const FilterBlockBuilder&);
};

class FilterBlockReader {
 public:
 // REQUIRES: "contents" and *policy must stay live while *this is live.
  FilterBlockReader(const FilterPolicy* policy, const Slice& contents);
  bool KeyMayMatch(uint64_t block_offset, const Slice& key);

 private:
  const FilterPolicy* policy_;
  const char* data_;    // Pointer to filter data (at block-start)
  const char* offset_;  // Pointer to beginning of offset array (at block-end)
  //filter 数量
  size_t num_;          // Number of entries in offset array
  size_t base_lg_;      // Encoding parameter (see kFilterBaseLg in .cc file)
};

}

#endif  // STORAGE_LEVELDB_TABLE_FILTER_BLOCK_H_
