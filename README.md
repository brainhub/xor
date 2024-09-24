# A command utility to perform an XOR operation on the contents of two files

The tool `xor` takes two input files as input, and creates a third file, whose contents are the result of a bit-wise XOR operation on the input files.

## Example 1

    $ xor -a file1.txt -b file2.txt -o output.bin

Such an operation is reversible. 

## Example 2

Using files from the above example, 
    
    $ xor -a file1.txt -b output.bin -o output2
    $ diff output2 file2.txt && echo The same
    The same

    $ xor -a file2.txt -b output.bin -o output3
    $ diff output3 file1.txt && echo The same
    The same

## Handling of size difference

The tool sets the output size equal to the size of the largest file.

This means that when the input is a special file of unlimited size, such as `/dev/urandom`, the tool will never stop.  

#3 Example 3

The following command creates the file `unlimited.bin`, whose contents are `file1.txt`, and then it keeps adding zeroes after that. This invocation never completes (because the end-of-file condition cannot be reached for `/dev/zero`).

    $ xor -a file1.txt -b /dev/zero -o unlimited.bin   

