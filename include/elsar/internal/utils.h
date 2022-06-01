#pragma once

#include <fcntl.h>
#include <sys/stat.h>

#include <cstring>
#include <experimental/filesystem>
#include <fstream>
#include <thread>

#include "embedding.h"
#include "globals.h"
#include "in_memory_sort.h"

using namespace std;
namespace fs = std::experimental::filesystem;

namespace elsar {
namespace utils {

inline static converted_t _convert_key(const char *);

constexpr unsigned char MIN_PRINTABLE_CHAR = 32;
constexpr unsigned char MAX_PRINTABLE_CHAR = 127;
constexpr unsigned int MAX_NUM_PROC = 99;
constexpr unsigned char PRINTABLE_RANGE =
    MAX_PRINTABLE_CHAR - MIN_PRINTABLE_CHAR;
static const int MAX_EMBEDDING_VALUE =
    PRINTABLE_RANGE * PRINTABLE_RANGE + PRINTABLE_RANGE;

inline static converted_t _convert_key(const char *key) {
  const short num_chars_to_convert = 9;
  converted_t value = 0;
  for (auto i = 0; i < num_chars_to_convert; ++i) {
    value += static_cast<converted_t>((key[i]) - MIN_PRINTABLE_CHAR) *
             static_cast<converted_t>(
                 pow(PRINTABLE_RANGE, num_chars_to_convert - i - 1));
  }
  return value;
}

size_t _read_records_file_into_embeddings(FILE *fid, size_t num_recs_to_read,
                                          Embedding *const converted_batch,
                                          char *const recs_buf) {
  auto num_recs_read =
      fread_unlocked(recs_buf, BYTES_PER_REC, num_recs_to_read, fid);
  if (num_recs_read != num_recs_to_read) {
    cerr << "ERROR: Could not read file." << endl;
    cerr << strerror(errno) << endl;
    exit(EXIT_FAILURE);
  }

  for (size_t rec_idx = 0; rec_idx < num_recs_read; ++rec_idx) {
    converted_batch[rec_idx].converted_key =
        utils::_convert_key(recs_buf + rec_idx * BYTES_PER_REC);
    converted_batch[rec_idx].record = &recs_buf[rec_idx * BYTES_PER_REC];
  }
  return num_recs_read;
}

inline size_t _file_sz(const char *filename) {
  struct stat st;
  stat(filename, &st);
  return st.st_size;
}

size_t _avail_mem() {
  string buf;
  ifstream meminfo_file("/proc/meminfo");
  while (meminfo_file >> buf) {
    if (buf == "MemAvailable:") {
      size_t mem;
      if (meminfo_file >> mem) {
        return mem * 1000;  // mem is reported in kB
      } else {
        return 0;
      }
    }
    // ignore rest of the line
    meminfo_file.ignore(numeric_limits<streamsize>::max(), '\n');
  }
  return 0;
}

FILE *_open_output_or_fail(const char *filename) {
  FILE *fid = fopen(filename, "rb+");
  if (!fid) {
    cerr << "Unable to open output file: " << filename << endl;
    cerr << strerror(errno) << endl;
    exit(EXIT_FAILURE);
  }
  return fid;
}

void _flush_fragments(vector<char *> *frags, FILE **frag_fids,
                      size_t *frag_sizes, const int num_partitions) {
  for (int partition_idx = 0; partition_idx < num_partitions; ++partition_idx) {
    auto fid = frag_fids[partition_idx];
    for (auto embedding_itr = frags[partition_idx].begin();
         embedding_itr != frags[partition_idx].end(); ++embedding_itr) {
      fwrite_unlocked(*embedding_itr, sizeof(char), BYTES_PER_REC, fid);
    }
    frag_sizes[partition_idx] += frags[partition_idx].size();
    frags[partition_idx].clear();
  }
}

template <class RandomIt>
void _write_recs_to_output(FILE *out_fid, size_t file_offset, RandomIt begin,
                           RandomIt end) {
  fseek(out_fid, file_offset, SEEK_SET);
  for (auto itr = begin; itr != end; ++itr) {
    fwrite_unlocked(itr->record, sizeof(char), BYTES_PER_REC, out_fid);
  }
}

FILE *_open_input_or_fail(const char *filename) {
  FILE *fid = fopen(filename, "rb");
  if (!fid) {
    cerr << "ERROR: Could not open file:" << filename << endl;
    cerr << strerror(errno) << endl;
    exit(EXIT_FAILURE);
  }

  return fid;
}

FILE *_open_tmp_file_or_fail(const char *tmpfs_root) {
  int tmp_fd = openat(AT_FDCWD, tmpfs_root, O_EXCL | O_RDWR | O_TMPFILE, 0600);
  if (tmp_fd < 0) {
    cerr << "Unable to create tmpfile" << endl;
    cerr << strerror(errno) << endl;
    exit(EXIT_FAILURE);
  }

  auto fid = fdopen(tmp_fd, "w+b");
  if (!fid) {
    cerr << "ERROR: Unable to fdopen tmpfile." << endl;
    cerr << strerror(errno) << endl;
    exit(EXIT_FAILURE);
  }

  return fid;
}

void _initialize_fragment_fids_for_th(FILE **frag_fids_for_th, int num_frags,
                                      const char *tmpfs_root) {
  for (int i = 0; i < num_frags; ++i) {
    frag_fids_for_th[i] = _open_tmp_file_or_fail(tmpfs_root);
  }
}

void _create_output_file(const char *filename, const size_t file_sz) {
  FILE *fid = fopen(filename, "wb");
  fclose(fid);
  fs::resize_file(filename, file_sz);
}

}  // namespace utils
}  // namespace elsar
