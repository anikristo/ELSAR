# ELSAR
External Learned Sorting for ASCII Records

[![Total alerts](https://img.shields.io/lgtm/alerts/g/anikristo/ELSAR.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/anikristo/ELSAR/alerts/) 
[![Language grade: C/C++](https://img.shields.io/lgtm/grade/cpp/g/anikristo/ELSAR.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/anikristo/ELSAR/context:cpp)
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)

## To generate data 
```
./third_party/gensort -a <num_recs> /data/input_file
```

## To compile ELSAR
```
./compile.sh
```

## To run ELSAR
```
./run.sh <input_file> <output_file> <temp_root> <num_threads>
```

## To verify data's checksum and sortedness 
```
./third_party/valsort /data/input_file
```

__Disclaimer__
This code has been tested on Linux Ubuntu 20.04 with input sizes up to 1.5TB. 
