# Intel
To quickly construct a 96MB testfile: 
```bash
python3 -c 'print("A"*100000000)' > test/data/a.out
```

Make a torrentfile for it:
```bash
./peer make -i test/data/a.out -o test/tfs/a.tf -t TCP:4:2323:127.0.0.1
```
