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

## Example 3

The following command creates the file `unlimited.bin`, whose contents are `file1.txt`, and then it keeps adding zeroes after that. This invocation never completes (because the end-of-file condition cannot be reached for `/dev/zero`).

    $ xor -a file1.txt -b /dev/zero -o unlimited.bin

## High-performance update mode

The tool detects the update scenario and takes advantage of it as follows.

When one of input filed and the output file satisfy the following conditions:
* they are regular files
* name of one input file matched the output file name (call this file as "output" file)
* the size of output file is larger than the other file (call this file as "intput" file)
then the output file is updated in-place.

When the input file is small, this update is an instant operation. Furthermore, some fileystems, such as btrfs, perform copy on write, so the following stems can perform instant masking for _arbitrary_ large files:

    $ cp LARGE copy
    $ xor -a LARGE -b mask -o copy
    $ diff LARGE copy
    Binary files LARGE and copy differ
    $ xor -a copy -b mask -o copy
    # so far these are instant operations on btrfs

    $ diff LARGE copy && echo the same
    the same


