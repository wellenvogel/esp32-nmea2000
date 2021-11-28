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
   * [GwExampleTask.h](GwExampleTask.h) the name of this include must match the name of the directory (ignoring case) with a "gw" in front. This file includes our special hardware definitions and registers our task at the core (DECLARE_USERTASK in the code). Optionally it can define some capabilities (using DECLARE_CAPABILITY) that can be used in the config UI (see below).
   Avoid including headers from other libraries in this file as this could interfere with the main code. Just only include them in your .cpp files (or in other headers).
   * [GwExampleTaks.cpp](GwExampleTask.cpp) includes the implementation of our task. This tasks runs in an own thread - see the comments in the code.
   We can have as many cpp (and header files) as we need to structure our code.  
   * [config.json](config.json)<br>
     This file allows to add some config definitions that are needed for our task. For the possible options have a look at the global [config.json](../../web/config.json). Be careful not to overwrite config defitions from the global file. A good practice wood be to prefix the names of definitions with parts of the library name. Always put them in a separate category so that they do not interfere with the system ones.
     The defined config items can later be accessed in the code (see the example in [GwExampleTask.cpp](GwExampleTask.cpp)).

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

 Future Plans
 ------------
 If there will be a need we can extend this extension API by means of adding  specific java script code and css for the UI.
 
