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
    extend the base configuration - we add a dummy library here and define additional build environments (boards)
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
   * [config.json](exampleConfig.json)<br>
     This file allows to add some config definitions that are needed for our task. For the possible options have a look at the global [config.json](../../web/config.json). Be careful not to overwrite config defitions from the global file. A good practice wood be to prefix the names of definitions with parts of the library name. Always put them in a separate category so that they do not interfere with the system ones.
     The defined config items can later be accessed in the code (see the example in [GwExampleTask.cpp](GwExampleTask.cpp)).<br>
     
     Starting from Version 20250305 you should normally not use this file name any more as those configs would be added for all build environments. Instead define a parameter _custom_config_ in your [platformio.ini](platformio.ini) for the environments you would like to add some configurations for. This parameter accepts a list of file names (relative to the project root, separated by ,).

   * [index.js](example.js)<br>
     You can add javascript code that will contribute to the UI of the system. The WebUI provides a small API that allows you to "hook" into some functions to include your own parts of the UI. This includes adding new tabs, modifying/replacing the data display items, modifying the status display or accessing the config items.
     For the API refer to [../../web/index.js](../../web/index.js#L2001).
     To start interacting just register for some events like api.EVENTS.init. You can check the capabilities you have defined to see if your task is active.
     By registering an own formatter [api.addUserFormatter](../../web/index.js#L2054) you can influence the way boat data items are shown.
     You can even go for an own display by registering for the event *dataItemCreated* and replace the dom element content with your own html. By additionally having added a user formatter you can now fill your own html with the current value.
     By using [api.addTabPage](../../web/index.js#L2046) you can add new tabs that you can populate with your own code. Or you can link to an external URL.<br>
     Please be aware that your js code is always combined with the code from the core into one js file.<br>
     For fast testing there is a small python script that allow you to test the UI without always flushing each change.
     Just run it with
     ```
     tools/testServer.py nnn http://x.x.x.x/api
     ```
     with nnn being the local port and x.x.x.x the address of a running system. Open `http://localhost:nnn` in your browser.<br>
     After a change just start the compilation and reload the page.<br>
     
     Starting from Version 20250305 you should normally not use this file name any more as those js code would be added for all build environments. Instead define a parameter _custom_js_ in your [platformio.ini](platformio.ini) for the environments you would like to add the js code for. This parameter accepts a list of file names (relative to the project root, separated by ,). This will also allow you to skip the check for capabilities in your code.

   * [index.css](index.css)<br>
     You can add own css to influence the styling of the display.<br>
     
     Starting from Version 20250305 you should normally not use this file name any more as those styles would be added for all build environments. Instead define a parameter _custom_css_ in your [platformio.ini](platformio.ini) for the environments you would like to add some styles for. This parameter accepts a list of file names (relative to the project root, separated by , or as multi line entry)

   * [script.py](script.py)<br>
     Starting from version 20251007 you can define a parameter "custom_script" in your [platformio.ini](platformio.ini).
     This parameter can contain a list of file names (relative to the project root) that will be added as a [platformio extra script](https://docs.platformio.org/en/latest/scripting/index.html#scripting). The scripts will be loaded at the end of the main [extra_script](../../extra_script.py).
     You can add code there that is specific for your build.
     Example:
     ```
      # PlatformIO extra script for obp60task
      epdtype = "unknown"
      pcbvers = "unknown"
      for x in env["BUILD_FLAGS"]:
          if x.startswith("-D HARDWARE_"):
              pcbvers = x.split('_')[1]
          if x.startswith("-D DISPLAY_"):
              epdtype = x.split('_')[1]

      propfilename = os.path.join(env["PROJECT_LIBDEPS_DIR"], env     ["PIOENV"], "GxEPD2/library.properties")
      properties = {}
      with open(propfilename, 'r') as file:
          for line in file:
              match = re.match(r'^([^=]+)=(.*)$', line)
              if match:
                  key = match.group(1).strip()
                  value = match.group(2).strip()
                  properties[key] = value

      gxepd2vers = "unknown"
      try:
          if properties["name"] == "GxEPD2":
              gxepd2vers = properties["version"]
      except:
          pass

      env["CPPDEFINES"].extend([("BOARD", env["BOARD"]), ("EPDTYPE",      epdtype), ("PCBVERS", pcbvers), ("GXEPD2VERS", gxepd2vers)])

      print("added hardware info to CPPDEFINES")
      print("friendly board name is '{}'".format(env.GetProjectOption     ("board_name")))
     ```


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
 * add a request handler for web requests (since 202411xx) - see registerRequestHandler in the API
 

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

 
