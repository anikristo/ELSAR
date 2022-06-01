#include "elsar/internal/utils.h"
#include "elsar/sort.h"

int main(int argc, char* argv[]) {
  if (argc < 2 or argc > 5) {
    cout << "USAGE: " << argv[0]
         << " [in-file] [out-file] optional:[tmp-root],[num-threads]\n";
    exit(-1);
  }

  auto input_file = argv[1];
  auto output_file = argv[2];
  auto tmp_root = argc >= 4 ? argv[3] : ".";
  auto num_threads = argc == 5 ? atoll(argv[4])
                               : std::min(thread::hardware_concurrency(),
                                          elsar::utils::MAX_NUM_PROC);
  elsar::sort(input_file, output_file, tmp_root, num_threads);

  return 0;
}
