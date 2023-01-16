## Crash course: devices and datasheets (using GPIO as an example)

***NOTE: still editing: send any feedback***

This note gives a give a crash course in thinking about devices and
their datasheets.  It's both incomplete and yet somewhat repetitive 
with both itself and the GPIO writeup.  But 
hopeflly much better than nothing. 

------------------------------------------------------------
###  Device driver: What's that.

Each hardware device an operating system supports requires a "device
driver" that can configure and use the device.  Since there are thousands
of devices, there are thousands of drivers.  Because of device numbers
and device complexity, about 90% of Linux or BSD is device drivers.

If you have a few minutes you can verify these claims yourself: download
a Linux kernel tar-ball, unpack it, and look in the `drivers/` directory
--- you'll see thousands of subdirectories containing many millions of
lines of code.

However, weirdly, no one talks about driver code much other than 
to sneer at its high number of bugs or low elegance.  While you
can find tens of thousands of resarch papers and hundreds of book chapters
on "core" operating system topics such as virtual memory, processes or
file systems, comparatively little gets written about devices despite
them making up the monster proportion of the code.

Partly this is because devices are necessarily ad hoc.  Among other
things, this makes it hard to distill them down to general principles.
Partly it is because the status of hacking on the core kernel is higher;
drivers are where you pawn off a newbie.

In any case, for this class: much of the action is in devices.  A big win
of writing a small system from scratch on a hardware platform like the
r/pi is that we can easily connect devices and do things with them that
a general-purpose system such as your laptop can't.  Especially true if
the device has tight real time requirements.  For examlif e, Try to plug
in an accelerometer to your laptop, or get consisent small nanosecond
accurate timings on MacOS (hahaha, sorry, that's mean), or build any of
the wearable devices you might buy, etc.

By the end class hopefully you will have the superpower that if you
see some cool new device, you can quickly grab its datasheet, code up a
driver for it without panic or much fuss, and then be able to exploit
whatever interesting power it gives.  This ability will also make it
easy for you to have an interesting idea and then back solve for how to
build it by looking at the the massive catelogue of devices out there,
secure in the knowledge that you can write any code you need.

Partly you'll develop this superpower because after you get through about
N datasheets and architecture manual chapters the N+1th just isn't that
bad. (I'd say N=10 is a reasonable guess, but it could take fewer.)
Partly it is also because there are general patterns you can exploit
and get used to; this note will talk about some of them.

So, let me tell you about devices.

--------------------------------------------------------------------
### Some hacks for reading datasheets

If expectation management makes happiness, then the key to getting
through datasheets is a clear-eyed view of the ways they often suck,
just expecting it, and doing enough that their surprises become routine
and you can flow through without a lot of drama.

The following comments also partially apply to architecture manauals,
though these often  get vetted much more heavily.

Some crude patterns for datasheets:

  - To start off on a positive note: typically 15-20% (or more)
    of a datasheet can be skipped because it deals with packing
    dimensions, or spends a bunch of time defining a common device
    protocol (e.g., SPI or I2C).  It's a happy feeling to flip past
    it all.

  - When you read a datasheet you mainly want to know "how":  how to
    configure a device to do what you want, how to know when data is
    ready, how to read its data, how to transmit output.

    Unfortunately, most datasheets are written in the wrong voice:
    passively, with pages and pages of "what" and "where" definitions but
    not much on "how".   In a sense they are a dictionary defining the
    device language, whereas you need to know how to speak device
    sentences for configuration and use.

    The most extreme example of non-operational, definitions-only prose
    I recall is the I2C device protocol "chapter" in the RISC-V  HiFive1
    manual: which simply gave a table of device addreses with the sentence
    that you should go "read the the I2C spec".

  - Related: fields are generally defined linearly --- e.g., in order
    of their memory address --- rather than grouped operationally.
    so you'll may have to read the datasheet  all the way through and
    try to infer what fields have to be done at same time

  - In addition, since many devices are made overseas, you can have
    the added challenge that the passive voice prose is being remixed by
    a non-English speaker.  It's just the way it is.  Often devices most
    attractive at a cost level have the hardest to understand datasheets.
    (Of course: if you can read the native language of the manufacturer, don't
    forget to check for a natively written datasheet!)

  - While the primary user goal is, definitionally, to use
    the device, weirdly few datasheets actually give examples for how
    to configure and use their device.

    As a result, you may have to spend hours trying to figure out
    something that would only take minutes given a simple example.
    Bizarre.  Clearly they have such examples (otherwise how would
    they test?): so put some simple ones in!  Show a "hello world".
    Show best practices.  Show some common mistakes.

    I'd say the single biggest, easy change that device manufacturers
    could make is to put examples.  IMO, it should be an ANSI standard
    requirement.

    In any case: known problem.  It's just the way it is.   One of
    your skills will be to get used to taking a bunch of "what" prose,
    and turning it into a working "how".  Part of this will be inferring
    implicit rules for how to combine or sequence multiple device 
    actions.  Do you have to write A before B?  Do you have to clear
    X before configuring the device?

    On the plus side, a bad datasheet is a moat keeping your competition
    out and (hopefully) this class makes you able to swim, fast.

  - To contradict the previous points: some of the better manufacturers
    will put out examples and suggestions.  But, note: these are often
    in something called an "Application Note" rather than the datasheet
    itself.  

    Mechanical suggestion: if you have a device X, search "application
    note X".  Likely useful.

  - Datasheets and hardware manuals often have crucial restrictions
    or important rules buried in the middle of a chapter, in the middle
    of a bunch of other prose.  You have to expect this dynamic and get
    in the habit of reading everything very carefully.

    A good example is from our "debug hardware" lab where we use
    the machine's single stepping execution method to validate code
    correctness.

    If you look at the bottom of page 13-16 in the ARM1176 architecture
    manual (the middle of the debug hardware configuration chapter),
    you see this major rule for single stepping:

      - "[if a] mismatch occurs while the processor is running in a
          privileged mode ... it is ignored"

    I.e., if you try to single-step in privileged mode rather than
    user-mode, nothing happens.  The preceeding chapter 13 pages have
    a bunch of prose on configuring the debug hardware --- nowhere do
    they imply processor state matters.  It's very easy to miss this
    unflagged sentence and try to do single stepping of kernel code,
    have it not work, and not be able to figure out what is going on. (I
    wasted some hours.)

    Again, major restriction buried in prose with no fore-shadowing.

  - More for architecture manuals:  just because something is in there
    and your CPU chip claims that it implementats that version, it does
    not mean you have that functionaliy!

    One example from this class: if you read the arm1176 manual it has
    an interesting set of performance counters where you can set them
    to count down from a given number and give you an exception if they
    hit zero. So, for example, you could use them to run user code for
    at most 50 cycles.  However, it turns out that while the r/pi A+
    and pi zero provide working counters, neither has this exception
    connected --- if a counter hits 0, nothing happens.  Not mentioned
    in the document; we had to find it discussed in some forum posts.

    Second example: there is a whole chapter in the arm1176 on how
    to put code and data in "fast memory" (sort of like pinning in a
    cache) but, again, the arm1176 chip used for the A+ and zero 
    silently does not provide this.  Making matters worse,
    when you "initialize" the functionality the configuration 
    registers do change, and kind-of look right.

    I wasted a couple of days on this for the first 240lx offering.
    The only place I found any comment was an old forum post.  (But
    in this case, it's possible I missed something in the manual ---
    but that is a lesson too!)


----------------------------------------------------------------
#### Bigger picture: controlling devices

Generally, whenever you need to control a device, you'll do something
like the following:

0. Get the datasheet for the device (i.e., the oft poorly-written PDF that describes it).
1. Figure out which memory locations control which device actions.
2. Figure out what magic values have to be written to cause the device to do something.
3. Figure out when you read from them how to interpret the results.

Fairly common patterns:

1. Devices typically require some kind of initialization, so often
   the first thing you'll do is turn the device on and then do a
   sequence of writes to set the device in a specific configuration.
   In some cases, rather than turning it on, you'll disable it so it
   doesn't interact with the outside world while being setup.

   The tables in the Broadcom give the initial value the device memory
   will have when powered on (the "reset" column).

   This part of the process is often the most complicated, with several
   steps: its made worse because getting any step wrong leads to the
   same result: the device doesn't work.

2. When you're waiting for results there will often be some kind of
   status or "ready" bit you have to check to see that there is data.
   Similarly, before you write data (e.g., to send a character using
   the UART) you will often have to wait for the device to have space.

3. Similarly, for results there is often a way to setup interrupts (we
   will do this in week 3) so that a piece of code runs rather than
   requiring you to spin checking if there is data (or space).

4. When you set values, you have to determine if you must preserve
   old values or can ignore them.

--------------------------------------------------------------------
### Initialization patterns

Some common device configuration details for complex devices:

   - As mentioned above, device enable can be extrmely expensive and
     just because your final store instruction to set the device's
     enable field was "completed" from the point of view of the CPU,
     which has started running the intructions after: ***this does not
     mean the device is up and running!***

     A complex devices (gyroscope, accelerometer, network transciever)
     is not going to be instantaneously active.  It's not uncommon for
     such devices to need 10 or more milliseconds (on the pi: million
     of cycles).  As a result, you'll want to do a close read looking
     for any table in the datasheet that says how long to wait.

     While datasheets generally have this information the table may
     be buried deep in the center of the PDF or the wording kind of
     weird.  In any case, for any complex device, it's not going to be
     instantaneously active: you'll want to do a close read looking for
     any table in the datasheet that says how long to wait.  Like device
     examples I'd say this should be collected on the first few pages.

     Similarly, if you're looking at device code and you don't some kind
     of delay after initialization, the code is likely broken.  If you
     do see a delay, but it doesn't have a specific comment or datasheet
     page number, I'd say it's also likely broken.  E.g., people flying
     blind may just stick in a 30ms delay or so "just in case".

   - For devices that have multi-step configurations, usually you will
     set the device to an "on" but disabled state when you (re-)configure
     it.  Otherise, it may start sending garbage out the world when you
     are halfway done with configuration.

   - This is arguable, but keep in mind: While devices have values
     they should be reset to on restart, it's usually better not to
     assume them --- either check that's the actual value, or set it
     explicitly (so, for example, configure can be done twice with
     different values).  

     If there are FIFO queues you almost certainly want to clear them so
     that after a re-config you don't send or receive garbage.  An easy
     race condition: enable the device and then clear the FIFOs --- a
     messaage from a previous session could arrive.  You want to clear
     with the device disabled.

We discuss a few of the more tricky issues below.  These apply
both to GPIO and to devices more generally.

----------------------------------------------------------------
#### Errata: Everything is broken

Check for datasheet errata! Datasheets often have errors.  The Broadcom
document is no different. Fortunately it has been worked over enough that
it has a [nice errata](https://elinux.org/BCM2835_datasheet_errata).
Sometimes you can figure out bugs from contradictions, sometimes
from errata. One perhaps surprisingly good source is looking at the
Linux kernel code, since Linux devs often have back-channel access
to manufacturers, who are highly incentivized to have Linux work with
their devices.

The hardest task is to be the first one to boot up a system or configure
a device.  Any datasheet mistake can easily prevent all progress.
Isolating the root cause using differential analysis is hard because
a datasheet error in any of the multiple setup steps  causes the same
effect: the device doesn't work.  Multiply the above by: are you sure
both your code and your understanding of the datasheet is correct?

----------------------------------------------------------------
#### Device accesses = remote procedure calls.

While the device operations are initiated using memory loads and stores,
what actually occurs "behind the scenes" is much closer to an arbitrarily
complex remote procedure calls where:

 1. The network is the I/O or memory bus the CPU and device use to
    talk to each other.

 2. The register source value being stored "to memory" is
    in fact a message that is sent to the device.  This message
    can trigger complicated and/or slow device actions, especially
    for configuration  and, thus, take much longer than a 
    true memory store.  One way to see this is to use a cycle   
    counter to measure the cost of different device operations.

 3. The destination value loaded "from memory" is a message
    sent from the device to the CPU.     If this load was part of a
    "send-reply" pattern where the code first stored and then waited
    for a result, this store-load pair can take an arbitrary long
    time to complete.

----------------------------------------------------------------
#### Interrupts versus polling for device events

When something happens on a device, how do you know about it?  You can
either poll (explicitly check) or use interrupts (so the device signals
the CPU which then jumps to an interrupt routine).  We discuss common
patterns below.

At risk of getting too far into the weeds: for some devices, device-to-CPU
messages can be triggered because of some external event, rather than
being a reply to a request explicitly sent by code running on the CPU.
The correctness problem is that if the driver code does not read the
event soon enough, a second event will either (silently) overwrite it
or get discarded (e.g., microphone, gyroscope or accelerometer readings).

As a partial solution, some devices provide a FIFO queue to buffer
results.  This helps, but doesn't solve the issue: FIFOs are finite,
so waiting too long still leads to discarded results.  This problem is
not some fake thing I'm making up; you'll see the issue pretty soon:

  - When you implement your UART code, the UART hardware has a 8-byte
    receive FIFO --- if bytes arrive when this FIFO is full, the hardware
    will silently discard.

  - Similarly the NRF transceiver we use for networking has a 4 message
    receive FIFO: if you wait too long and a fifth message arrives,
    one gets dropped.  

    (For both cases I'm being vague because writing this made me realize
    I didn't test if its the oldest or newest --- if you run an experiment
    let us know!)

These bugs suck.  They often won't show up when the system is ticking
along slowly or with few nodes (e.g., as during testing) but only
sporadically when you go full speed or add more nodes or tasks and
then things will just lock up occasionally.  (Compounding the problem:
"are you sure it's not a hardware bug?")

To avoid dropping device results, the code must either sit in a "fast
enough" loop "polling" for these events (e.g., by checking a status
location over and over: "is there anything?")  or use _interrupts_ so
that its device handling code runs soon-enough when there is something
to do.  Polling is simple, but makes it hard to do other things ---
do too many or take too long doing one and you missed your window.
Interrupts make it easier to do other things, but are a great way to
break your system since they they allow the interrupt routine to be
invoked on any instruction, begging for race conditions.

In this class we will always build device code using polling first and
only switch to interrupts reluctantly.

It may seem too cute, but I think this is completely isomorphic: You
already understand interrupts versus polling because of your phone.
If you turn off the ringer, you'll have to manually go and repeatedly
check if a message arrived.  The more time-critical the message is (e.g.,
if you have to reply before the sender gets mad) the more frequently you
have to check.  On the other hand, you can instead turn on the ringer so
you get notified (interrupted) when something comes in and just go work on
other things.  You also see issues with context switching.  Similarly with
buzzers on washer or dryer machines, doorbells, your mom yelling.

As a postscript: For reasons I don't quite understand a lot of people
think you have to immediately use interrupts if you do a device.  This
is false.  You can always poll.  And, in fact, polling is the superior
solution in many of the use cases we will see.  E.g., if you are building
a single-purpose device, rather than a general purpose OS, you can sit
in a loop pretty easily checking all the different devices you have to
see if something interesting occured.  A polling loop makes it feasible
to much more thoroughly test your system --- maybe even to the point
that you are surprised if it breaks.  However, if you add interrupts,
I'd say you now almost-certainly have an impossilbe to test system ---
interrupts allow the interrupt routine to be called on any instruction,
exponentially blow up the state space of your code.  You can't exhaust
this space with brute force testing.  (There are other tricks you can
play, but operating systems people almost never know about them; we will
do a few in cs240LX but that doesn't help us now.)

----------------------------------------------------------------
#### How: device memory + compiler optimization = bugs

As discussed in the [COMPILE](../1-compile/volatile/README.md) note:
The C compiler does not know about hardware devices nor does it know
that device memory is "special" and can spontaneously change without
visible any store in the program text. Thus, the compiler can (and
often will) optimize pointers to device memory in a way that breaks
the intent of your code. For example, assume `status` is a pointer
to a device status register and we want to spin until it holds the
value `1` before using the device:

        unsigned *status = (void*)0x12345678;
        ...
        while(*status != 1)
            ;

The compiler will look at the loop, see that there is no write to
`status` and potentially rewrite it internally as code that looks
like:

        if(*status != 1) {
            // infinite loop
            while(1)
                ;
        }

This is very different than what you intended. As we discuss in
another note, the traditional method OSes have used for this problem
is to mark device pointers with the `volatile` type qualifier, which
(roughly) tells the compiler the pointed to location can spontaneously
change and thus the compiler should not add or remove any `volatile`
load or store or reorder it with either its own accesses or with
those of other `volatile` loads or stores (regular, non-volatile
access have no guarantees).

----------------------------------------------------------------
#### Rules for writing device code

To do a new device:

  - Do a quick internet search for the device name and "errata".
    I'd also suggest redoing it by adding "forum" or "bare metal"
    to pull up more obscure issues.

  - First off: for external devices, figure out the device input
    and output voltage.  Always make sure you get this right!

    Obviously, do not connect a 3v device to a 5v pi pin!  But, also
    don't connect a 5v device to a 3v pi pin.  If you're lucky this
    will cause the device to do nothing (e.g., all device reads return
    `0`). But it might cause it to almost work -- sort-of working stuff
    is hard to debug since it causes Heisenbugs, error that come and
    go non-deterministically.

    Similarly: Never use a device directly that puts out more or less
    than 3v --- what the r/pi's pins expect.

  - Find the values that must be true on reset.  Read the device after
    reboot and see that these are what you get.  If you don't the
    device could be broken (it happens, especially these days with
    so many counterfeits).  Or you code could be broken --- e.g.,
    the addresses you use.
 
    If things don't work: it's likely to be your code.  However,
    hardware does go bad.
 
    Buy at least two devices: if the first doesn't work, try swapping
    in the second.  Also try switching pi's --- sometimes pins get
    fried, or jumps are too loose.
 
    Note easy mistakes: use pins where you don't have to count a bunch.
 
  - When you write a value to the device for a field that is both
    read and write, read it back and make sure that that you wrote is
    what you get back (note: some device registers will have bits that
    can change spontaneously).

    Trivial, but this hack has detected many bugs for me over the years.
    If nothing else, it increases your confidence.
 
  - Use `put32` and `get32` to read or write the device locations
    so that the compiler optimization does not blow up your code.
 
  - Use polling to get device results rather than jumping right into
    interrupts.
    
  - If you have two devices that communicate, set up your pi in a
    "loop back" configuration where it can send and receive to itself.
    This will be at least 2x (maybe more) faster to develop since the
    single pi knows exactly what it intends to do and what occured.
 
  - If you are doing networking: in our experience, sending is pretty
    robust, but receiving --- where an antennea has to cleanly decode
    signals --- can be very sentitive to dirty power and cause receive
    to fail.  This can be hard to figure out since the fact send "works"
    means you will assume there is no hardware problem.

  - When you write the code, add the datasheet page numbers for
    why you did things.  You won't remember.  Plus, it let's
    someone (like a TA) look at your code and see why you did 
    something and if it makes sense.
 
 - A very minor point but I have made this mistake: a revesion 2.0
   might just be indicated with a "+" rather than a big red "version 2"
   or a different device number.   If you're careles it's easy to buy the
   older version (e.g., because its getting liquidated at good prices)
   but pick up the later datasheet and spend a ton of time writing code
   to use functionality that simply does not exist on the device you have.
   I've done this.

 - When you set values, you have to determine if you must preserve
   old values (so must read-modify-write) or can ignore them
   and just set what you want (the GPIO `SET` and `CLR` registers).

---------------------------------------------------------------------
#### Missing:

Some stuff that is missing:
 - The debug hardware on the rv is a good example of weird prose that
   you have to think about but does what you want.
 - The Ou cache bug.
 - Bitbang.
 - Digital analyzer to make electrical printk.
 - SBZ

----------------------------------------------------------------
#### Intermixing memory and device memory operations.

When these operations are intermixed with normal loads and stores,
their cost can lead to the following hard-to-debug
problem when 

(I had one that literally took two days a few years back.)

On many machines, including the pi, when you perform a store to a device,
the store can return before the device operation completes. This hack
can give a big speed improvement by, among other things, allowing you
to pipeline operations to the same device.  However, it also can lead
to subtle ordering mistakes.

The pi does guarantee that all reads and writes to the same device occur
in order (these operations are "sequentially consistent" w.r.t. each
other). However it _does not_ guarantee that a write to one device A
followed by a second write to device B will complete in that order. If
they are entirely independent, this may not matter.  However, if the
device operations were supposed to happen in that order (A then B),
the code is broken.

The way we handle this is to put in a "memory barrier" that
(over-simplifying) guarantees that all previous loads and stores to memory
or devices have completed before execution goes beyond the barrier. You
can use these to impose ordering. We will discuss them at length
later. There are some very subtle issues, especially when dealing with
virtual memory hardware. We don't worry about this for the current lab.


----------------------------------------------------------------
#### Correctness: Read reset values to santity check.

Of 

----------------------------------------------------------------
#### Correctness: I get 12517 from the devide, is that right?

Who knows?

Often have no idea if the value of the device is right.
So find special cases where you can detect.
For an analogue to digital converter, attach an attenuator that
you can cycle between off and on (and multple that have different
recistence)

part of this:  datasheets will give value


E.g, play an 80hz signal and see what you get from your microphone.

Often there is 

