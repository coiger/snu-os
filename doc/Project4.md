# Project 4: Hello, File System!

* Assigned: 2022-05-18 Wednesday 17:00:00 KST
* No Design Review
* **Due: 2022-06-09 Thursday 20:59:59 KST**

## Introduction

This is the last OS project. Yay! :thumbsup:

In this project, you will develop a new kernel-level mechanism for embedding location information into the ext2 file system. 
You will also control the accessibility of the directories and files inside your custom ext2 file system based on the location metadata.

This is a team project. Accept the Github classroom assignment invitation. Each team will get access to its own GitHub repository (e.g. swsnu/scheduler-team-n) for collaboration and submission. Commit your final code and README.md to the master or main branch for submission.


## 1. Setting the device's GPS location (10 pts.)

It would be nice if we used a physical device with GPS sensors to acquire location information. 
Unfortunately, we don't use real devices this semester. What should we do?
Recall `sys_set_orientation` back in project 2.
We will implement a system call to update the (fake) 'current location' of our kernel. 

First, write the following definition of `struct gps_location` on `include/linux/gps.h`.

```c
struct gps_location {
  int lat_integer;
  int lat_fractional;
  int lng_integer;
  int lng_fractional;
  int accuracy;
};
```

Then, write a new system call in `kernel/gps.c` that updates the current location of the device. 
The kernel should store the location information in kernel memory space.
Don't forget to synchronize concurrent access to this piece of information.

The new system call number should be 399(32-bit) or 294(64-bit), and the prototype should be:

```c
/*
 * Set the current device location. Syscall number 399 or 294.
 *
 * Args:
 *   loc: gps_location to set as the current device orientation.
 *
 * Returns:
 *   Zero on success
 *   -EINVAL if loc is NULL or its attributes are invalid.
 *   -EFAULT if loc is outside the accessible address space.
 *
 */
long sys_set_gps_location(struct gps_location *loc);
```

This sets

* latitude = `loc->lat_integer` + `loc->lat_fractional` * (10^-6)
* longitude = `loc->lng_integer` + `loc->lng_fractional` * (10^-6)

The validity of each attribute is determined as follows. 
* 0 <= `loc->lat_fractional`, `loc->lng_fractional` < 1,000,000 (Fractional parts of the latitude and longitude represent up to the sixth decimal point.)
* -90 <= latitude <= 90
* -180 <= longitude <= 180 (180.xx is an invalid longitude!)
* 0 <= accuracy < 1000

`Accuracy` refers to how close the device's calculated position is from the truth, expressed as a radius and measured in meters ([source](https://www.gps.gov/systems/gps/performance/accuracy/)). We need this attribute to determine whether the two locations are close or not.
The reason why we define the integer part and fractional part separately in `struct gps_location` is because the Linux kernel does not support floating-point representations.
Refer to section 4. You'll see how you should use this information to do access control.


## 2. Update the GPS location of files on creation or modification (20 pts.)

You should amend the file system to give **Regular files** and **directories** a location metadata.

The location information of them should be updated when they are **created** or **modified**.
[Learn more](https://unix.stackexchange.com/questions/2464/timestamp-modification-time-and-created-time-of-a-file)

Include location attributes to the kernel filesystem machinery, with getter and setter operations.

### Add GPS-related attributes on `struct ext2_inode` and `struct ext2_inode_info`

First of all, you need to add five GPS-related attributes to `struct ext2_inode` and `struct ext2_inode_info`. 
The former struct represents data on-disk and the latter data in-memory. 

Append the following fields _in order_ at the _end_ of the two structs:
```
i_lat_integer (32-bit)
i_lat_fractional (32-bit)
i_lng_integer (32-bit)
i_lng_fractional (32-bit)
i_accuracy (32-bit)
```
You will need to pay close attention to the endianness of the fields you add to the ext2 physical inode structure. 

### Modify struct `inode_operations` 

Secondly, you should modify the inode operations interface. Add the following two members to `struct inode_operations` definition in `include/linux/fs.h`:

```c
int (*set_gps_location)(struct inode *);
int (*get_gps_location)(struct inode *, struct gps_location *);
```

Note that function pointer `set_gps_location` does _not_ accept any GPS location structure as its argument - it should use the kernel GPS data (set by `sys_set_gps_location`) to set gps location metadata.

Implement `set_gps_location` function that works **for ext2 file system**. (It is not a system call!)
Do not consider other file systems such as ext3 or ext4.
Look in the `fs/` directory for all the file system code, and in the `fs/ext2/` directory for ext2 specific code. 

### Call `set_gps_location` properly on regular file creation or modification 

The only thing left to do is to call `set_gps_location` whenever a regular file/a directory is created or modified. 

You don't have to care about other kinds of files, but if you want to track location information for them, feel free to do so and describe on `README.md`



## 3. User-space testing for location information (10 pts.)

### Write a sytem call to get gps location of a given file/directory.

To retrieve the location information, write a new system call numbered 400 (or 295) with the following prototype:

```c
/*
 * Set the current device location. Syscall number 400 or 295.
 *
 * Args:
 *   pathname: absolute or relative path that describes
 *       the location of a specific regular file or a directory
 *   loc: user-space buffer that the gps location
 *       of a file would be copied to. 
 *
 * Returns:
 *    Zero on success
 *   -EINVAL if one of the arguments is NULL.
 *   -EFAULT if loc is outside the accessible address space.
 *   -ENOENT if there is no regular file or directory named with a pathname
 *   -EACCES if path walking to the destination is not possible according to geotag-based access control
 *   -ENODEV if no GPS coordinates are embedded in the file.
 *
 */
long sys_get_gps_location(const char *pathname, struct gps_location *loc);
```

On success, the system call should return 0, and `*loc` should be populated with the location information of the regular file or a directory specified by `pathname`. 
Implement this system call in `kernel/gps.c`.

We will not test for other possible errors that are not mentioned above. If there are other error cases you want to take care of, feel free to do so.

Note that you are to implement a pair of functions that are quite similar in functionality. Namely, a location getter for an inode and a system call that retrieves the location information from the path of a file.

(2022-06-03 edited:  '-EACCES if the regular file or directory is not readable by the current user.' requirement is removed. You may decide to return this error on this case or you can just print the GPS location and return zero.) 


### Use e2fsprogs to create an ext2 filesystem image in user-space
As you have modified the physical representation of the ext2 file system on the disk, you also need a special tool that creates such a file system in user-space. In this project, we will make use of the ext2 file system utilities that [e2fsprogs](http://e2fsprogs.sourceforge.net/) provides. 

Navigate to the link provided above to download `e2fsprogs`.

Then, **modify the appropriate file(s)** in `e2fsprogs/lib/ext2fs/` to match the new physical layout.

After making the aforementioned modifications, compile the utilities.
```bash
os@ubuntu:e2fsprogs$ ./configure
os@ubuntu:e2fsprogs$ make
```

The binary you will be the most interested in is `e2fsprogs/misc/mke2fs`. This tool should now create an ext2 file system with your modifications. Before this, you should find the empty loop device using `losetup -f`. 

```bash
os@ubuntu:~$ sudo losetup -f
/dev/loopXX
```
XX is a positive integer that changes based on your system environment.

Create a modified ext2 file system using the modified `mke2fs` tool on your host PC.

```bash
os@ubuntu:proj4$ dd if=/dev/zero of=proj4.fs bs=1M count=1
os@ubuntu:proj4$ sudo losetup /dev/loopXX proj4.fs
os@ubuntu:proj4$ sudo ../e2fsprogs/misc/mke2fs -I 256 -L os.proj4 /dev/loopXX
os@ubuntu:proj4$ sudo losetup -d /dev/loopXX
```
The file `proj4.fs` now contains the modified ext2 file system! Copy it into QEMU, just like your test binaries.

In QEMU, create a directory called `/root/proj4` and mount the `proj4.fs` on it. 

```bash 
root:~> mkdir /root/proj4
root:~> mount -o loop -t ext2 proj4.fs /root/proj4
```

Files and directory operations that you execute on `/root/proj4` will now be ext2 operations. Check whether the newly created regular files in `/root/proj4` are properly geo-tagged! 


## 4. Location-based file access control (15 pts.)

Access permission to a geo-tagged **regular files** or **directories** should be granted only when the system's current GPS location is _geometrically close_ to the GPS location of the file.

**But root directory in `proj4.fs` should be free of the access restriction.**

For example...
* If you created a regular file at the 302 building, you won't be able to modify it in Hawaii.
* If you created a directory in the Seoul National University station, you can't ever add new file inside that directory at the 302 building.
* But, you should be able to create regular files/directories inside root directory in `proj4.fs` from anywhere. 


Let us define the geometrical distance between two locations (both described by their longitudes and latitudes) as the shortest path between the two *along the surface of the Earth*.
The two locations are geometrically close if their geometrical distance is smaller than the sum of their `accuracy` values.

In calculating the geometrical distance, the system will have to do floating-point arithmetic.
As mentioned before, because floating-point operations are not supported in the Linux kernel, the floating-point-ish arithmetic should be implemented only with integer arithmetic.

Fortunately, the range of latitude and longitude are bounded, and the precision of the fractional part only requires 6 figures.
So, fixed-point arithmetic is enough for calculating distance.

We provide [a simple example](https://github.com/swsnu/osspr2022/blob/main/handout/fixed_point.c) for fixed-point arithmetic that you can refer to.
The example contains macros for basic arithmetic and also a cosine table to do some math on spherical space. Or you can implement different method of calculations. Since there must be some errors whichever method you take, we will not strictly examine the result of the calculations. Yet, make sure you take integer overflows into account.

In order to simplify the calcuation, you can make some geometrical assumptions. For example, you may assume that the Earth is a perfect sphere. Write in README.md any assumptions and approximations you made.

In addition to the location-based access control, the new ext2 file system should respect the existing access control mechanism.
That is, proximity to the GPS location of a file/a directory should work as an **extra condition** for the system granting the access permission to the file.
If permission cannot be granted under the existing file access control mechanism, access to a file/a directory should be disallowed even if its GPS location matches the system's.

If you want to implement extra policies for other types of files, you are free to do so.
It is advisable to document extra policies on `README.md` .



## 5. Test Code (10 pts.)
In this project, you should write two test programs `test/gpsupdate.c`, `test/print_loc.c`. 

* `gpsupdate.c` takes five command line arguments: `lat_integer`, `lat_fractional`, `lng_integer`, `lng_fractional`, and `accuracy`. It updates the system's GPS location via `sys_set_gps_location` based on the given arguments. 

* `print_loc.c` takes one command line argument, which is a path to a regular file or a directory. Then it prints out the GPS coordinates and `accuracy` of the regular file or a directory, plus the Google Maps link of the corresponding location. Its output format should be as following:

```
Latitude = 10.000000, Longitude = 20.123456
Accuracy = 10 [m]
Google Link = https://maps.google.com/?q=10.000000,20.123456
```
  
Finally, you should place your `proj4.fs` in the root directory. 
Your `proj4.fs` should contain at least 2 directory with 2 files in each directory, and at least 2 files outside those directories.
Each of file/directories should have different GPS coordinates.

Below is an example of how files should be located in `proj4.fs`
```
.
├── dir_1
│   ├── file_1
│   └── file_2
├── dir_2
│   ├── file_3
│   └── file_4
├── file_5
└── file_6
```


## Submission

- Code
  - Commit to the master branch of your team repo.
	- The `README.md` should describe your implementation, how to build your kernel (if you changed anything), and lessons learned.
  - `test/gpsupdate.c`, `test/print_loc.c`, `proj4.fs` is mandatory.
- Slides and Demo (`n` is your team number)
	- Email: osspr.2022.ta@gmail.com
	- Title: [Project 4] Team `n`
	- Attachments: team-`n`-slides.{ppt,pdf,...}, team-`n`-demo.{mp4,avi,...}
	- One slide file (up to 6 pages, include lessons learned & discussions on the last page)
	- One demo video (up to 3 minutes)


## We're here to help you

Any trouble? Questions on the [issue board](https://github.com/swsnu/osspr2022/issues) are more than welcome.
We also highly encourage discussions between students.
Start early, be collaborative, and most importantly, have fun! Good luck :)
