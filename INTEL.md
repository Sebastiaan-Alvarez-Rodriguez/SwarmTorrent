# Intel
To quickly construct a 96MB testfile: 
```bash
python3 -c 'print("A"*100000000)' > test/data/a.out
```

Launch a tracker:
```bash
./tracker -p 2323
```

Make a torrentfile for the testfile:
```bash
./peer make -i test/data/a.out -o test/tfs/a.tf -t TCP:4:2323:127.0.0.1
```

Get a SRC peer to torrent for our testfile:
```bash
./peer torrent -p 2322 -w test/dl/ -f test/tfs/a.tf -r
```

Get a DST peer to torrent for our testfile:
```bash
./peer torrent -p 2322 -w test/dl_dest/ -f test/tfs/a.tf
```