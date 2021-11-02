imported from https://github.com/aduvenhage/ais-decoder
and from https://github.com/AK-Homberger/NMEA2000-AIS-Gateway
AIS NMEA Message Decoder (v2.0)
This project was created to learn more about AIS and see how easy it would be to create a decoder for the NMEA strings. The NMEA string decoding is implemented according to: 'http://catb.org/gpsd/AIVDM.html'.

Also please, checkout https://github.com/aduvenhage/ais_beast for an attemp to make a faster (C++ only) decoder.

The decoder is designed to work off of raw data (processed in blocks) received from, for example, a file or a socket.  The raw data sentences (or lines) may be seperated by '[LF]' or '[CR][LF]'.

The key component to implement was the 6bit nibble packing and unpacking of arbitrarily sized signed and unsigned integers as well as strings (see PayloadBuffer in ais_decoder.h).  The decoder interface also delivers clean strings (i.e. all characters after and including '@' removed and all trailing whitespace removed).

The decoder consists of a base class that does the decoding, with pure virtual methods for each AIS messages type.  A user of the decoder has to inherit from the decoder class and implement/override 'onTypeXX(...)' style methods, meta/payload extraction, as well as error handling methods (see the examples, for how this is done).  Basic error checking, including CRC checks, are done and errors are also reported through decoder interface.

The current 'onTypeXX(...)' message callback are unique for each message type (types 1, 2, 3, 4, 5, 9, 11, 18, 19, 24, 27 currently supported).  No assumtions are made on default or blank values and all values are returned as integers -- the user has to scale and convert the values like position and speed to floats and the desired units.

The method 'enableMsgTypes(...)' can be used to enable/disable the decoding of specific messages. For example 'enableMsgTypes({1, 5})' will cause only type 1 and type 5 to be decoded internally, which could increase decoding performance, since the decoder will just skip over other message types.  The method takes a list or set of integers, for example '{1, 2, 3}' or '{5}'.

The individual data sentences (per line) may also include meta data before or after the NMEA sentences.  The decoder makes use of a sentence parser class that should be extended by the user to extract the NMEA data from each sentence (see example applications and default_sensor_parser.h).  The meta data is provided as a header and a footer string to the user via one of the pure virtual methods on the decoder interface.  For multi-line messages only the header and footer of the first sentence is reported (reported via 'onMessage(...)').

The decoder also provides access to the META and raw sentence data as messages are being decoded.  The following methods can be called from inside the 'onMessage()', 'onTypeXX()' or 'onDecodeError()' methods:
 - 'header()' returns the extracted META data header
 - 'footer()' returns the extracted META data footer
 - 'payload()' returns the full NMEA payload
 - 'sentences()' returns list of original sentences that contributed

Some time was also spent on improving the speed of the NMEA string processing to see how quickly NMEA logs could be processed.  Currently the multi-threaded file reading examples (running a thread per file) achieve more than 3M NMEA messages per second, per thread.  When running on multiple logs concurrently (8 threads is a good number on modern hardware) 12M+ NMEA messages per second is possible.  During testing it was also found that most of the time was spent on the 6bit nibble packing and unpacking, not the file IO.

SWIG is used to provide Python bindings.  And the decoder can also be built and installed through python.

## Checklist
- [x] Basic payload 6bit nibble stuffing and unpacking
- [x] ASCII de-armouring
- [x] CRC checking
- [x] Multi-Sentence message handling
- [x] Decoder base class
- [x] Support types 1, 2, 3, 4, 5, 9, 11, 18, 19, 24, 27
- [x] Test with large data-sets (files)
- [x] Validate payload sizes (reject messages, where type and size does not match)
- [x] Build-up message stats (bytes processed, messages processed, etc.)
- [x] Profile and improve speed 
- [x] Validate fragment count and fragment number values
- [x] Investigate faster ascii de-armouring and bit packing techniques (thanks to Frans van den Bergh)
- [x] Add python interface
- [x] Support NMEA files/data with non-standard meta data, timestamp data, etc.
- [x] Improve multi-line performance (currently copying data, which is slow)
- [x] Add support for custom sentence headers and footers (meta data)
- [x] Allow for NMEA-to-NMEA filtering by storing and providing access to source sentences with each decoded message

- [ ] Validate talker IDs
- [ ] Look at multiple threads/decoders working on the same file, for very large files
- [ ] Add minimal networking to work with RTL-AIS (https://github.com/dgiardini/rtl-ais.git) and also to forward raw data

## Build
This project uses CMAKE to build.  To build through command line on linux, do the following:

- git clone https://github.com/aduvenhage/ais-decoder.git
- mkdir ais-decoder-build
- cd ais-decoder-build
- cmake ../ais-decoder -DCMAKE_BUILD_TYPE=RELEASE
- make
- sudo make install


## Examples
The project includes some examples of how to use the AIS decoder lib.


## Create a python module
The module is built around the 'ais_quick' interface. See 'examples/quick'. This project uses [SWIG](http://www.swig.org/) to compile a python module.  The SWIG interface file is located at 'python/ais_decoder.i'.

### To build and install using 'setuptools'
The setup script does try to build and install the C++ library automatically and depends on CMAKE.  If this fails see the [build](#build) instructions.

```
cd python
sudo python setup.py build
sudo python setyp.py install
```

### To build manually (tested with MacOS)
Follow the decoder lib [build](#build) instructions first -- if the library is installed on the system, the SWIG steps are easier.

```
cd python
swig -Wall -c++ -python ais_decoder.i
c++ -c -fPIC ais_decoder_wrap.cxx -I /System/Library/Frameworks/Python.framework/Versions/2.7/include/python2.7/
c++ -shared ais_decoder_wrap.o -lpython -lais_decoder -o _ais_decoder.so
```

Make sure you use the correct python lib for the version you will be working with.

### Build Notes
On Linux you will have to install the following:
- 'cmake'
- 'g++'
- 'swig'
- 'python-dev' or 'python3-dev' *for Python3*
- 'python3-distutils' *for Python3*

## Import and use python module
In python, do the following to test:

```
import ais_decoder

str = "!AIVDM,1,1,,A,13HOI:0P0000VOHLCnHQKwvL05Ip,0*23\n"
ais_decoder.pushAisSentence(str, len(str))

n = ais_decoder.numAisMessages()
msg = ais_decoder.popAisMessage().asdict()
```


'pushAisSentence(...)' scans for one sentence only and sentences should always end with a newline.  'popAisMessage().asdict()' returns a Python dictionary with the message fields. Message fragments, for multi-fragment messages, are managed and stored internally.  'msg' will be empty if no output is ready yet.

The interface also has 'pushChunk(data, len)' that accepts any number of messages. Incomplete sentences at the end of the supplied chunk will be buffered until the next call.

```
import ais_decoder

try:
    str = "!AIVDM,1,1,,A,13HOI:0P0000VOHLCnHQKwvL05Ip,0*23\n!AIVDM,1,1,,A,13HOI:0P0000VOHLCnHQKwvL05Ip,0*23\n!AIVDM,1,1,,A,13HOI:0P0000VOHLCnHQKwvL05Ip,0*23\n!AIVDM,1,1,,A,13HOI:0P0000VOHLCnHQKwvL05Ip,0*23\n!AIVDM,1,1,,A,13HOI:0P0000VOHLCnHQKwvL05Ip,0*23\n!AIVDM,1,1,,A,13HOI:0P0000VOHLCnHQKwvL05Ip,0*23\n!AIVDM,1,1,,A,13HOI:0P0000VOHLCnHQKwvL05Ip,0*23\n!AIVDM,1,1,,A,13HOI:0P0000VOHLCnHQKwvL05Ip,0*23\n!AIVDM,1,1,,A,13HOI:0P0000VOHLCnHQKwvL05Ip,0*23\n!AIVDM,1,1,,A,13HOI:0P0000VOHLCnHQKwvL05Ip,0*23\n!AIVDM,1,1,,A,13HOI:0P0000VOHLCnHQKwvL05Ip,0*23\n"
    ais_decoder.pushAisChunk(str, len(str))

    n = ais_decoder.numAisMessages()
    print("num messages = ", n)

    while True:
        if ais_decoder.numAisMessages() == 0:
            break

        msg = ais_decoder.popAisMessage().asdict()
        print(msg)

except RuntimeError as err:
    print("Runtime error. ", err)
except:
    print("Error.")
```

Interface functions can throw Python 'RuntimeError' exceptions on critical errors, but message decoding errors are reported back as a AIS message with 'msg=0' and appropriate error information.
