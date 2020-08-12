High performance logging for Miosix

Some applications - possibly real-time - require to log data, and do so from
multiple threads. While a simple library that opens a file and writes to is
sufficient for simple applications, there are some problems:
- the Miosix filesystem uses typically an SD card as storage, and SD and other
  FLASH based storage devices may pause to do wear leveling. SD cards, even good
  quality class 10 ones, may pause for up to 1 second.
- the Miosix OS, contrary to Linux, does not do much buffering at the filesystem
  layer. This is desirable, since when an OS runs on as little as a few tens of
  KB of RAM, the last thing you want is an OS that uses an unquatifiable amount
  of memory.

This example code shows a high-performance logging class that
- has a nonblocking log() member function, which can be called concurrently from
  multiple threads, to log a user-defined class or struct.
  Tscpp (https://github.com/fedetft/tscpp) is used to serialize data.
  Being nonblocking, it can be called also in real-time threads of your codebase
  with confidence.
- buffers data to compensate for the delays of the storage medium

To configure the logger for your application, to trade off buffer space vs write
data rate, you can edit Logger.h

    static const unsigned int filenameMaxRetry = 100; ///< Limit on new filename
    static const unsigned int maxRecordSize    = 128; ///< Limit on logged data
    static const unsigned int numRecords       = 128; ///< Size of record queues
    static const unsigned int bufferSize       = 4096;///< Size of each buffer
    static const unsigned int numBuffers       = 4;   ///< Number of buffers
    static constexpr bool logStatsEnabled      = true;///< Log logger stats?
