# About

This program reports on the open file descriptors of a Linux system along with corresponding data, such as calling process ID, filename, and inode number.

## Example

<img width="758" alt="image" src="https://github.com/andre-fong/System-FD-Table/assets/99469779/5edde39a-0651-4d2f-98b4-0909b4dba6e7">

# Installation

1. Clone the repo
  ```sh
  git clone https://github.com/andre-fong/System-FD-Table.git
  ```
2. Compile
  ```sh
  make
  ```
3. Run
  ```sh
  ./a.out
  ```

# How to Run

When running, the following command line arguments are accepted:

* <code>--per-process</code>
  * Display process FD table (PID, FD)
* <code>--systemWide</code>
  * Display system-wide FD table (PID, FD, filename)
* --<code>Vnodes</code>
  * Display Vnodes FD table (FD, inode)
* <code>--composite</code>
  * Display full table (PID, FD, filename, inode)
* <code>--threshold=X</code>
  * List offending processes at the end with (strictly) more than X file descriptors
  * Listed format: PID (FD count), etc…
* <code>&lt;integer&gt;</code>
  * Only display information for the process with the PID given by the positional argument
  * Target PID will be printed at the start
* <code>--output_TXT</code>
  * Save the composite table of the program to compositeTable.txt
  * Can be run with positional argument to save the composite table of a certain PID
* <code>--output_binary</code>
  * Save the composite table of the program to compositeTable.bin
  * Can be run with positional argument to save the composite table of a certain PID


## Example Usage

```sh
./a.out --composite 255 --threshold=10 --output_binary
```

Run the system FD table tool, outputting the **composite** FD table of all FDs opened by **PID 255**. 
**Output the composite table to compositeTable.bin**.

Also mark all other processes that have over **10 open FDs** (threshold).

# Implementation

* `sys/stat.h`: defined the stat struct and the function stat, which was helpful in getting the inode of a symbolic link
* `readlink(2)`: was helpful in getting the filename of a symbolic link
* `proc(5)`: included helpful information, ie., PIDs are numeric directories inside /proc, /proc/[pid]/fd lists open file descriptors for a process with PID pid
* `dirent.h`: defined the dirent struct and DIR type, useful in opening and listing contents of /proc, /proc/[pid]/fd, etc.
* `sys/types.h`: defined a lot of useful types (ssize_t, pid_t, etc.) for this assignment
