# Overview

This is a port of the upstream `paho.mqtt.embedded-c` code, presently at v1.0.0, to the `esp-open-rtos` environment. In particular, it includes the equivalent of the `paho-embed-c` library from the `MQTTClient-C` subtree and the `FreeRTOS` platform-specific code associated with it. It is intended to be as true to that source as possible.

This port uses the `lwip` stack as delivered with the `esp-open-rtos` environment. 

Further details of `paho.mqtt.embedded-c` may be found at [https://github.com/eclipse/paho.mqtt.embedded-c]()

# Status

This code is early beta as of October 30, 2017. 

While "it works for me", many features of the underlying code, as well as those of `component.mk` have not been tested. 


# Usage
## Basic Usage
Program `Makefile`

```
PROGRAM = your_mqtt_program
EXTRA_COMPONENTS = extras/paho-embed-mqtt3c
include ../../common.mk
```

Ensure that the `git` submodule at `extras/paho-embed-mqtt3c/paho.mqtt.embedded-c` has been properly initialized and checked out. This port was developed around `tag: v1.1.0` but should work with later versions; the changes are applied as patches, not as whole-file overrides.

## Known Caveats

The patched version of MQTTFreeRTOS from upstream requires either the FreeRTOS TCP layer or a "shim" layer to adapt the calls and types to, for example LWIP calls and types. A shim is not provided with this distribution due to the uncertainty around header-file content, copyright, and GPL of the FreeRTOS sources.

While present in the upstream code, use of the `#define MQTT_TASK` code path has not been tested. Further, no implementation of `ThreadStart()` has been supplied, though the prototype is present in `MQTTSimple.h`


## Advanced Usage
`component.mk` has been designed to allow for the use of other client-specific implementations. If you have written such a configuration, and `#include <MQTTClient.h>` it should be possible to utilize it instead of the default `FreeRTOS` implementation by 

* Placing the source and header(s) into `extras/paho-embed-mqtt3c/mqttclient_platforms/<your-implementation>` 
* Setting the `make` variable `MQTTCLIENT_PLATFORM_HEADER` to `your-implementation.h`

The `make` variable may be set in the application's `Makefile` or on the command line with, for example
```
[path/to/your/program] $ make MQTTCLIENT_PLATFORM_HEADER=your-implementation.h
```

This variable's value is used verbatim to determine `MQTTCLIENT_PLATFORM_DIR` by prepending `extras/mqttclient_platforms/` and removing the trailing `.h` 

If the desired `MQTTCLIENT_PLATFORM_HEADER` header file is not consistently named with the desired source directory, `MQTTCLIENT_PLATFORM_DIR` can be explicitly defined.

`MQTTCLIENT_PLATFORM_DIR` is used as the wildcard-ed `paho-embed-mqtt3c_SRC_DIR` by the esp-open-rtos build system. It is also added to `INC_DIRS`. 

An appropriate `-DMQTTCLIENT_PLATFORM_HEADER=` flag is added to `CFLAGS` for your program and the library, if not already present. 

Due to the behavior of the esp-open-rtos build system, use of the `make` variable is suggested over modifying `CFLAGS`. If `CFLAGS` is modified before the build systems creates its variables and rules, the esp-open-rtos defaults may not be present during the build.

*Note: `MQTTFreeRTOS.h` and `MQTTFreeRTOS.c` are a special case. The upstream sources are patched, on-the fly, beneath `$(BUILD_DIR)`*

## Replacing `extras/paho_mqtt_c`

This is *not* a drop-in replacement for the older `extras/paho_mqtt_c` that has shipped with previous versions of `esp-open-rtos` for at least the following reasons:

### Symbol Names

`extras/paho_mqtt_c` chose to modify the name of almost every symbol in the code from its upstream value. This port does *not* change the names of symbols relative to upstream.

### `MQTTClient` Return Values

`extras/paho_mqtt_c` adds several values to the return values of `MQTTClient` The only values present in upstream and this port `MQTTClient.h` are

```
enum returnCode { BUFFER_OVERFLOW = -2, FAILURE = -1, SUCCESS = 0 };
```

Example code for `extras/paho_mqtt_c` relies on the non-standard `MQTT_DISCONNECTED` return code. Use of the convenience method

```
int MQTTIsConnected(MQTTClient* client)
```
or its implementation of

```
client->isconnected
```
is suggested as an alternative. 

All the usual caveats about not being able to reliably detect when a network socket is unexpectedly disconnected still apply.

# Author

Jeff Kletsky

# License

## Open-Source Components

*"The eclipse/paho.mqtt.embedded-c Code"* consists of the original `eclipse/paho.mqtt.embedded-c` code, typically checked out from [https://github.com/eclipse/paho.mqtt.embedded-c](). *The eclipse/paho.mqtt.embedded-c Code* is, at this time, generally licensed under the Eclipse Public License v1.0 and Eclipse Distribution License v1.0. Nothing in this document shall be construed to expand the terms of license of any content within that distribution.

## All Other Components

All content of this project, with the exception of *The eclipse/paho.mqtt.embedded-c Code,* is copyright and licensed as described in `LICENSE.txt` in the root directory of this project. 

The content of that file  as of October 30, 2017 is replicated here, for convenience.

```
Copyright (c) 2017, Jeff Kletsky
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.

    * Neither the name of the copyright holder nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
```

