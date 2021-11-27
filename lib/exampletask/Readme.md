Extending the Core
==================
This directory contains an example on how you can extend the base functionality of the gateway.
Basically you can define own boards here and can add one or more tasks that will be started by the core.
You can also add additional libraries that will be used for your task.
In this example we define an addtional board (environment) with the name "testboard".
When building for this board we add the -DTEST_BOARD to the compilation - see [platformio.ini](platformio.ini).
The additional task that we defined will only be compiled and started for this environment (see the #ifdef TEST_BOARD in the code).
You can add your own directory below "lib". The name of the directory must contain "task".

Files
-----
   * [platformio.ini](platformio.ini)
    extend the base configuration - we add a dummy library here and define our buil environment (board)
   * [GwExampleTask.h](GwExampleTask.h) the name of this include must match the name of the directory (ignoring case) with a "gw" in front. This file includes our special hardware definitions and registers our task at the core (DECLARE_USERTASK in the code).
   * [GwExampleTaks.cpp](GwExampleTask.cpp) includes the implementation of our task. This tasks runs in an own thread - see the comments in the code.
   * [GwExampleHardware.h](GwExampleHardware.h) includes our pin definitions for the board.

 Hints
 -----
 Just be careful not to interfere with names from the core - so it is a good practice to prefix your files and class like in the example.

 Developing
 ----------
 To develop I recommend forking the gateway repository and adding your own directory below lib (with the string task in it's name).
 As your code goes into a separate directory it should be very easy to fetch upstream changes without the need to adapt your code.

 Future Plans
 ------------
 If there will be a need we can extend this extension API by means of adding config items and specific java script code and css for the UI.
 
