# Intel
To quickly construct a 96MB testfile: 
```bash
python3 -c 'print("A"*100000000, end="")' > test/dl_src/a.out
```

Launch a tracker:
```bash
./tracker -p 2323
```

In a separate terminal, make a torrentfile for the testfile:
```bash
./peer make -i test/dl_src/a.out -o test/tfs/a.tf -t TCP:4:2323:127.0.0.1
```

Get a SRC peer to torrent for our testfile:
```bash
./peer torrent -p 2322 -w test/dl_src/ -f test/tfs/a.tf -r
```

In another separate terminal, get a DST peer to torrent for our testfile:
```bash
./peer torrent -p 2321 -w test/dl_dst/ -f test/tfs/a.tf
```