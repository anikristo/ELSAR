#pragma once
#include <omp.h>
#include <sys/stat.h>

#include "internal/in_memory_sort.h"
#include "internal/rmi.h"

namespace elsar {

/**
 * @brief The external sorting function (ELSAR)
 *
 * @param input_file The name of the input file to be sorted
 * @param output_file The name of the sorted output file to be generated
 * @param tmp_root The root directory for placing temporary files
 * @param num_proc The maximum of threads to be used by the program. Note that
 * the algorithm might use less threads than this parameter depending on memory
 * capacity.
 */
void sort(const char *input_file, const char *output_file, const char *tmp_root,
          const size_t num_proc) {
  // Initialize parameters
  const size_t input_file_sz = fs::file_size(input_file);
  if (input_file_sz == 0) return;

  static const size_t num_recs = input_file_sz / BYTES_PER_REC;

  const size_t available_mem = utils::_avail_mem();

  const int num_partitions = num_recs / AVG_PARTITION_RECS;

  const double partition_width =
      std::floor(1. * utils::MAX_EMBEDDING_VALUE / num_partitions);

  const int num_readers = num_proc;

  const size_t avg_bytes_per_reader_th =
      (num_recs / num_readers) * BYTES_PER_REC; /* except for the last thread */

  const auto avg_mem_for_partition_sorting =
      AVG_PARTITION_RECS *
      (BYTES_PER_REC + sizeof(Embedding) * IN_MEM_SORT_MEM_MULTIPLIER);

  const int num_sorters =
      std::min(num_proc, std::min(input_file_sz, available_mem) /
                             avg_mem_for_partition_sorting);

  // Validation checks
  if ((READ_BATCH_RECS * BYTES_PER_REC * num_readers >= available_mem) or
      (1.4 * avg_mem_for_partition_sorting >= available_mem)) {
    cerr << "This size is not supported yet! Max supported size: "
         << num_proc * available_mem << " bytes" << endl;
    exit(EXIT_FAILURE);
  }

  // Initialize variables
  vector<char *> **fragments = new vector<char *> *[num_readers];
  FILE ***fragment_fids = new FILE **[num_readers];
  size_t **fragment_sizes = new size_t *[num_readers];
  for (int i = 0; i < num_readers + 1; ++i) {
    fragment_fids[i] = new FILE *[num_partitions];
    fragment_sizes[i] = new size_t[num_partitions]{0};
    fragments[i] = new vector<char *>[num_partitions];
  }

#pragma omp parallel for num_threads(num_readers)
  for (int reader_th_idx = 0; reader_th_idx < num_readers; ++reader_th_idx) {
    auto next_byte_to_read = reader_th_idx * avg_bytes_per_reader_th;
    auto last_byte_to_read = next_byte_to_read + avg_bytes_per_reader_th;

    if (reader_th_idx == num_readers - 1) {
      last_byte_to_read = input_file_sz; /* The last thread reads until EOF */
    }

    auto partition_frags_for_reader = fragments[reader_th_idx];
    FILE *input_fid = utils::_open_input_or_fail(input_file);
    fseek(input_fid, next_byte_to_read, SEEK_SET);

    utils::_initialize_fragment_fids_for_th(fragment_fids[reader_th_idx],
                                            num_partitions, tmp_root);

    // Initialize memory for the records read in a batch
    char *recs_buf = new char[READ_BATCH_RECS * BYTES_PER_REC];

    while (next_byte_to_read < last_byte_to_read) {
      auto remaining_recs =
          (last_byte_to_read - next_byte_to_read) / BYTES_PER_REC;

      auto num_recs_to_read = std::min(READ_BATCH_RECS, remaining_recs);
      auto num_recs_read =
          fread(recs_buf, BYTES_PER_REC, num_recs_to_read, input_fid);
      if (num_recs_read != num_recs_to_read) {
        cerr << "ERROR: Could not read file." << endl;
        cerr << strerror(errno) << endl;
        exit(EXIT_FAILURE);
      }
      for (size_t i = 0; i < num_recs_read; ++i) {
        auto emb = (static_cast<int>(recs_buf[i * BYTES_PER_REC]) -
                    utils::MIN_PRINTABLE_CHAR) *
                       utils::PRINTABLE_RANGE +
                   (static_cast<int>(recs_buf[i * BYTES_PER_REC + 1]) -
                    utils::MIN_PRINTABLE_CHAR);

        auto predicted_partition = std::min(
            static_cast<int>(emb / partition_width), num_partitions - 1);

        partition_frags_for_reader[predicted_partition].push_back(
            recs_buf + i * BYTES_PER_REC);
      }

      utils::_flush_fragments(partition_frags_for_reader,
                              fragment_fids[reader_th_idx],
                              fragment_sizes[reader_th_idx], num_partitions);

      next_byte_to_read += num_recs_read * BYTES_PER_REC;
    }
    fclose(input_fid);
    delete[] recs_buf;
  }
  for (int i = 0; i < num_readers; ++i) {
    delete[] fragments[i];
  }
  delete[] fragments;

  utils::_create_output_file(output_file, input_file_sz);

  // Open the output file for each of the sorter threads
  FILE **out_fids_for_sorters = new FILE *[num_sorters];
  for (int i = 0; i < num_sorters; ++i) {
    out_fids_for_sorters[i] = utils::_open_output_or_fail(output_file);
  }

  vector<size_t> total_partition_sizes(num_partitions, 0);
  for (int partition_idx = 0; partition_idx < num_partitions; ++partition_idx) {
    for (int reader_idx = 0; reader_idx < num_readers + 1; ++reader_idx) {
      total_partition_sizes[partition_idx] +=
          fragment_sizes[reader_idx][partition_idx];
    }
  }

  vector<size_t> partition_write_offsets(num_partitions);
  partition_write_offsets[0] = 0;
  for (int i = 1; i < num_partitions; ++i) {
    partition_write_offsets[i] = partition_write_offsets[i - 1] +
                                 total_partition_sizes[i - 1] * BYTES_PER_REC;
  }

#pragma omp parallel for num_threads(num_sorters) schedule(static, 1)
  for (int partition_idx = 0; partition_idx < num_partitions; ++partition_idx) {
    auto partition_size = total_partition_sizes[partition_idx];
    if (partition_size > 0) {
      Embedding *partition_contents = new Embedding[partition_size];
      size_t write_head = 0;
      char *rec_buf = new char[partition_size * BYTES_PER_REC];
      char *sorted_rec_buf = new char[partition_size * BYTES_PER_REC];
      for (int reader_th_idx = 0; reader_th_idx < num_readers;
           ++reader_th_idx) {
        rewind(fragment_fids[reader_th_idx][partition_idx]);

        auto num_recs_read = utils::_read_records_file_into_embeddings(
            fragment_fids[reader_th_idx][partition_idx],
            fragment_sizes[reader_th_idx][partition_idx],
            partition_contents + write_head,
            rec_buf + write_head * BYTES_PER_REC);

        fclose(fragment_fids[reader_th_idx][partition_idx]);
        write_head += num_recs_read;
      }

      elsar::internal::in_memory_sort(partition_contents,
                                      partition_contents + partition_size,
                                      partition_size);

      auto th_id = omp_get_thread_num();

      char *sorted_buf_batch = new char[WRITE_BATCH_SZ * BYTES_PER_REC];
      write_head = 0;
      fseek(out_fids_for_sorters[th_id], partition_write_offsets[partition_idx],
            SEEK_SET);
      while (write_head < partition_size) {
        auto remaining_recs = partition_size - write_head;
        auto num_recs_to_write = std::min(WRITE_BATCH_SZ, remaining_recs);

        // Coalesce
        for (size_t rec_idx = 0; rec_idx < num_recs_to_write; ++rec_idx) {
          memcpy(sorted_buf_batch + rec_idx * BYTES_PER_REC,
                 partition_contents[write_head + rec_idx].record,
                 BYTES_PER_REC);
        }
        fwrite_unlocked(sorted_buf_batch, sizeof(char) * BYTES_PER_REC,
                        num_recs_to_write, out_fids_for_sorters[th_id]);
        write_head += num_recs_to_write;
      }

      delete[] sorted_buf_batch;
      delete[] rec_buf;
      delete[] partition_contents;
    }
  }

  for (int i = 0; i < num_sorters; i++) {
    fclose(out_fids_for_sorters[i]);
  }

  for (int i = 0; i < num_readers; i++) {
    delete[] fragment_sizes[i];
  }
  delete[] fragment_sizes;
  delete[] fragment_fids;
}
}  // namespace elsar
