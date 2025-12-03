clear
gcc -o haffman haffman.c 
echo "Hello World! This is a test of Huffman coding algorithm." > test.txt
cat test.txt
ls -lh test.txt
./haffman encode test.txt compressed.bin
./haffman decode compressed.bin decompressed.txt