# ELSAR
External Learned Sorting for ASCII Records

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