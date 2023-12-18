Extending the Core
==================
This directory contains an example on how you can extend the base functionality of the gateway.
Maybe you have another interesting hardware or need some additional functions but would like to use the base functionality of the gateway.
You can define own hardware configurations (environments) here and can add one or more tasks that will be started by the core.
You can also add additional libraries that will be used to build your task.
In this example we define an addtional board (environment) with the name "testboard".
When building for this board we add the -DTEST_BOARD to the compilation - see [platformio.ini](platformio.ini).
The additional task that we defined will only be compiled and started for this environment (see the #ifdef TEST_BOARD in the code).
You can add your own directory below "lib". The name of the directory must contain "task".

Files
-----
   * [platformio.ini](platformio.ini)<br>
    This file is completely optional.
    You only need this if you want to
    extend the base configuration - we add a dummy library here and define one additional build environment (board)
   * [GwExampleTask.h](GwExampleTask.h) the name of this include must match the name of the directory (ignoring case) with a "gw" in front. This file includes our special hardware definitions and registers our task at the core.<br>
   This registration can be done statically using [DECLARE_USERTASK](https://github.com/wellenvogel/esp32-nmea2000/blob/9b955d135d74937a60f2926e8bfb9395585ff8cd/lib/api/GwApi.h#L202) in the header file. <br>
   As an alternative we just only register an [initialization function](https://github.com/wellenvogel/esp32-nmea2000/blob/9b955d135d74937a60f2926e8bfb9395585ff8cd/lib/exampletask/GwExampleTask.h#L19) using DECLARE_INITFUNCTION and later on register the task function itself via the [API](https://github.com/wellenvogel/esp32-nmea2000/blob/9b955d135d74937a60f2926e8bfb9395585ff8cd/lib/exampletask/GwExampleTask.cpp#L32).<br>
   This gives you more flexibility - maybe you only want to start your task if certain config values are set.<br>
   The init function itself should not interact with external hardware and should run fast. It needs to return otherwise the main code will not start up.<br>
   Avoid including headers from other libraries in this file as this could interfere with the main code. Just only include them in your .cpp files (or in other headers).
   Optionally it can define some capabilities (using DECLARE_CAPABILITY) that can be used in the config UI (see below)
   There are some special capabilities you can set:
     *  HIDEsomeName: will hide the configItem "someName"
     *  HELP_URL: will set the url that is loaded when clicking the HELP tab (user DECLARE_STRING_CAPABILITY)<br>

   * [GwExampleTaks.cpp](GwExampleTask.cpp) includes the implementation of our task. This tasks runs in an own thread - see the comments in the code.
   We can have as many cpp (and header files) as we need to structure our code.  
   * [config.json](config.json)<br>
     This file allows to add some config definitions that are needed for our task. For the possible options have a look at the global [config.json](../../web/config.json). Be careful not to overwrite config defitions from the global file. A good practice wood be to prefix the names of definitions with parts of the library name. Always put them in a separate category so that they do not interfere with the system ones.
     The defined config items can later be accessed in the code (see the example in [GwExampleTask.cpp](GwExampleTask.cpp)).

 Interfaces
 ----------
 The task init function and the task function interact with the core using an [API](../api/GwApi.h) that they get when started.
 The API has a couple of functions that only can be used inside an init function and others that can only be used inside a task function.
 Avoid any other use of core functions other then via the API - otherwise you carefully have to consider thread synchronization!
 The API allows you to
 * access config data
 * write logs
 * send NMEA2000 messages
 * send NMEA0183 messages
 * get the currently available data values (as shown at the data tab)
 * get some status information from the core
 * send some requests to the core (only for very special functionality)
 * add and increment counter (starting from 20231105)
 * add some fixed [XDR](../../doc/XdrMappings.md) Mapping - see [example](https://github.com/wellenvogel/esp32-nmea2000/blob/9b955d135d74937a60f2926e8bfb9395585ff8cd/lib/exampletask/GwExampleTask.cpp#L63).
 * add capabilities (since 20231105 - as an alternative to a static DECLARE_CAPABILITY )
 * add a user task (since 20231105 - as an alternative to a static DECLARE_USERTASK)
 * store or read task interface data (see below)


 __Interfacing between Task__

Sometimes you may want to exchange data between different user tasks.<br> As this needs thread sychronization (and a place to store this data) there is an interface to handle this in a safe manner (since 20231105).<br>
The task that would like to provide some data for others must declare a [class](https://github.com/wellenvogel/esp32-nmea2000/blob/9b955d135d74937a60f2926e8bfb9395585ff8cd/lib/exampletask/GwExampleTask.h#L24) that stores this data. This must be declared in the task header file and you need to make this known to the code using DECLARE_TASKIF.<br>
Before you can use this interface for writing you need to ["claim"](https://github.com/wellenvogel/esp32-nmea2000/blob/9b955d135d74937a60f2926e8bfb9395585ff8cd/lib/exampletask/GwExampleTask.cpp#L55) it - this prevents other tasks from also writing this data.
Later on you are able to [write](https://github.com/wellenvogel/esp32-nmea2000/blob/9b955d135d74937a60f2926e8bfb9395585ff8cd/lib/exampletask/GwExampleTask.cpp#L278) this data via the api.<br>
Any other task that is interested in your data can read it at any time. The read function will provide an update counter - so the reading side can easily see if the data has been written.
The core uses this concept for the interworking between the [button task](../buttontask/) - writing - and the [led task](../ledtask/) - [reading](https://github.com/wellenvogel/esp32-nmea2000/blob/9b955d135d74937a60f2926e8bfb9395585ff8cd/lib/ledtask/GwLedTask.cpp#L52).

 Hints
 -----
 Just be careful not to interfere with C symbols from the core - so it is a good practice to prefix your files and class like in the example.

 Developing
 ----------
 To develop I recommend forking the gateway repository and adding your own directory below lib (with the string task in it's name).
 As your code goes into a separate directory it should be very easy to fetch upstream changes without the need to adapt your code.
 Typically after forking the repo on github (https://github.com/wellenvogel/esp32-nmea2000) and initially cloning it you will add my repository as an "upstream repo":
 ```
 git remote add upstream https://github.com/wellenvogel/esp32-nmea2000.git
 ```
 To merge in a new version use:
 ```
 git fetch upstream
 git merge upstream/master
 ```
 Refer to https://docs.github.com/en/pull-requests/collaborating-with-pull-requests/working-with-forks/syncing-a-fork
 
 By following the hints in this doc the merge should always succeed without conflicts.

 
