# fty-common-messagebus2

## Description
This project aims to provide somme common methods to address communication over several message bus.
It provide an high level interface to handle comunication of Message. The format of the Message also defined in this project.

It comes today with 3 implementations:
* MQTT
* AMQP
* Malamute

Those 3 implementations are implementing the fty-commom-messagebus2 interface and are carring Message.

## Interface & Message
The basic idea is to have interface and message working like a post service:
Message meta data, defines where to send the message, what's the subject of the message and who send it, etc... (like what you would put in an envelop).
Message payload is fully agnostic from the bus (Like what you would put in the envelop).
The bus implementation should carry out the message to the destination.

To be valid, a message must have the following fields
- FROM (who send)
- TO (destination queue/topic)
- SUBJECT

If you need someone to reply to your message, you have to add few fields:
- REPLY_TO (which queue/topic to reply)
- CORRELATION_ID (unique id to identify the exchange)

The message definiton is available the [header](common/public_include/fty/messagebus/Message.h)
The interfaces is documentation is available in the [header](common/public_include/fty/messagebus/MessageBus.h)


## Dependencies
* [fty-cmake](https://github.com/42ity/fty-cmake/)
* [fty_common_mlm](https://github.com/42ity/fty-common-mlm)
* [fty_common_logging](https://github.com/42ity/fty-common-logging)
* [PahoMqttC](https://github.com/eclipse/paho.mqtt.c)
* [PahoMqttCpp](https://github.com/eclipse/paho.mqtt.cpp)
* [googletest](https://github.com/google/googletest)

## How to build

To build fty-common-messagebus2 project run:

```cmake
cmake -B build -DBUILD_ALL=ON
cmake --build build
```

#### Build options

| Option                       | description                                  | acceptable value      | default value           |
|------------------------------|----------------------------------------------|-----------------------|-------------------------|
| BUILD_ALL_ADD_ON             | Build all addons                             | ON\|OFF               | ON                      |
| BUILD_AMQP                   | Enable AMQP addon                            | ON\|OFF               | ON                      |
| BUILD_MALAMUTE               | Enable Malamute addon                        | ON\|OFF               | ON                      |
| BUILD_MQTT                   | Enable Mqtt addon                            | ON\|OFF               | ON                      |
| BUILD_SAMPLES                | Enable samples build                         | ON\|OFF               | OFF                     |
| BUILD_TESTING                | Add test compilation                         | ON\|OFF               | ON                      |
| BUILD_DOC                    | Build documentation                          | ON\|OFF               | OFF                     |


## How to use the dependency in your project

Add the dependency in CMakeList.txt:

```cmake
etn_target(${PROJECT_NAME}
  SOURCES
    .....
  USES
    .....
    fty-common-messagebus2-<bus name>
    .....
)
```

## Howto

See all samples in samples folder.

### Mqtt samples

* [PubSub](samples/mqtt/src/FtyCommonMessagebusMqttSamplePubSub.cpp)
* [WaitRequest](samples/mqtt/src/FtyCommonMessagebusMqttSampleAsyncReply.cpp)
* [SendRequest](samples/mqtt/src/FtyCommonMessagebusMqttSampleSendRequest.cpp)

### Amqp samples

* [PubSub](samples/amqp/src/FtyCommonMessagebusAmqpSamplePubSub.cpp)
* [WaitRequest](samples/amqp/src/FtyCommonMessagebusAmqpSampleAsyncReply.cpp)
* [SendRequest](samples/amqp/src/FtyCommonMessagebusAmqpSampleSendRequest.cpp)

### Malamute samples

* [PubSub](samples/malamute/src/FtyCommonMessagebusMlmSamplePubSub.cpp)
* [WaitRequest](samples/malamute/src/FtyCommonMessagebusMlmSampleReply.cpp)
* [SendRequest](samples/malamute/src/FtyCommonMessagebusMlmSampleRequest.cpp)

## Change log

[Change log](CHANGELOG.md) provides informations about bug fixing, improvement, etc.
