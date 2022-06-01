#pragma once

#include <string>

namespace elsar {

// Namespace constants
static constexpr size_t BYTES_PER_REC = 100; /* bytes */
static constexpr size_t KEY_SZ = 10;         /* bytes */
static constexpr char MAX_ASCII_CODE = '~';
static const size_t IN_MEM_SORT_MEM_MULTIPLIER = 3;

// Algorithm parameters
static const size_t DEFAULT_RMI_ARCH[] = {1, 1000};
static const size_t WRITE_BATCH_SZ = 1e3;         /* records */
static const int TRAINING_SAMPLE_RECS = 1e7;      /* records */
static const int AVG_PARTITION_RECS = 10'964'912; /* records */
static const size_t READ_BATCH_RECS = 1e6;        /* records */

static const size_t TRAINING_SAMPLE_BYTES =
    TRAINING_SAMPLE_RECS * BYTES_PER_REC;

// Type definitions
typedef char *record_t;
typedef unsigned long converted_t;
typedef char *pointer_t;
}  // namespace elsar
