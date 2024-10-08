## High CPI Challenge

This code is designed to achieve very high cycles-per-instruction (CPI) on an x86-64 
machine. The basic rules of the challenge are

* single threaded code
* the CPU must *actively* be executing instructions
* "on-device" only... no waiting for IO from another machine
* CPI is measured by `perf stat`

No, this is not actually useful in any way.

### Usage:

The program builds with `make` on an x86-64 Linux machine.

To run it, you can do `./slow <approx_shift>`. `approx_shift` is just a number, 
experimentally, 2345 works well and gives ~615 CPI on my machine. 

### Design:

To achieve high cycles-per-instruction, we need to keep both the numerator large and the 
denomenator small. This means that _every_ instruction needs to be slow. So, the loop 
consists of a single type of instruction that we can make _extra_ slow:

`jmp *(target)`

We construct a cycle of `N` jumps as follows:

```
jmp[0]: jmp *(target0)
jmp[1]: jmp *(target1)
jmp[2]: jmp *(target2)
jmp[3]: jmp *(target3)

target0: jmp[P[0]]
target1: jmp[P[1]]
target2: jmp[P[2]]
target3: jmp[P[3]]
```

Where `P` is some cyclic permutation. For instance, if `P = [ 1, 2, 3, 0 ]`,
then our program would execute in the following order:

```
jmp[0]
jmp[1]
jmp[2]
jmp[3]
jmp[0]
...
```

In this case, with `N = 4` and also this particular "high-locality" `P`, our program
will run quite fast. To make the program nice and slow, `N` needs to be large and `P` needs
to have poor cache locality. This way, for every `jmp *(target)` we first have a cache miss
_loading the instruction_ followed by another cache miss when loading `target`. These
loads all depend on each other meaning that they must be executed sequentially.

To generate "low-locality" `P`s, we select `N` to be a large prime and `shift` to be
some other nonzero number.

```
jmp[0]
jmp[shift % N]
jmp[(2 * shift) % N]
jmp[(3 * shift) % N]
...
```

The cycle will be of size `N - 1` and have poor locality if `shift` is sufficiently large
and well-chosen. 

### Measurement

Once the program starts running, it will print out it's PID. You can then
run 
```
perf stat -e cycles,instructions -I 1000 -p <pid>
```
to see values for `cycles` and `instructions` ever second.

On my machine (a AMD EPYC 7763 64-Core Processor) it outputs numbers roughly like this:
```
     7.008462327      3,528,633,557      cycles
     7.008462327          5,601,273      instructions              #    0.00  insn per cycle
```

which is ~630 CPI. Pretty high!

### Caveats

* This code is extremely non-portable.
* Higher CPI might be possibly by selecting addresses more carefully (placing all loads on 
page boundaries, etc.) but I haven't bothered doing that.


# The Infernal Machine: `slower.cpp`

630 CPI is nice, but I think we can do even better. So, `slower.cpp` executes the same program
as `slow.cpp`... inside of a virtual machine.

Running/building this works pretty similar to `slow`, except you may need to use `sudo`
to both run and profile the virtual machine.

I use the following commands:
```
sudo ./slower 2345
```
and 
```
sudo perf kvm stat -e cycles,instructions -I 1000 -p <pid>
```

On my machine with AMD's hardware accelerated "nested page translation" (NPT),
the numbers look great!

```
     7.008185293      3,516,477,735      cycles
     7.008185293          2,687,541      instructions              #    0.00  insn per cycle
```

giving us a nice and efficient ~1308 CPI

