---
title:  openDAQ Stream Protocol Specification
subtitle: Version 1.2.0
titlepage: true
toc: true
toc-own-page: true
papersize: a4
fontsize: 9pt
numbersections: true
linkcolor: blue
document-type: Public
---


# Overview

The data streaming mechanism is intended to enable client programs to receive
data from data acquisition devices, further called devices.
The protocol was designed with the following characteristics in mind:

-   Use only one socket connection per instance of data acquisition to
    limit the number of open sockets. Multiple data acquisition instances are possible but
    devices may prevent this for performance reasons.

-   Minimize network traffic.

-   Extensible signal description and spurious event notification.

-   Transmit Meta information about the acquired signals to make the
    data acquisition self-contained. This means that it is not necessary to
    gather information via the setup interface to interpret the acquired
    signals.

# Architecture

There are three main components involved. 

- A [transport layer](#transport-layer) and 
- a [presentation layer](#presentation-layer) allow the interpretation of data sent over the stream socket by the device. 
- [Command Interface(s)](#command-interfaces)
allow to subscribe or unsubscribe signals to a streaming instance.

# Transport Layer

The transport layer consists of a header and an optional length field.
Both 32 bit words are transmitted in little endian.

The structure of the transport layer header with the following data block payload are depicted below.

![A single block on transport layer](images/transport.png)


## Signal Info Field

![The Signal Info Field](images/sig_info.png)

### Reserved

This field is reserved for future use and must be set to `00b`.

### Type

The `Type` sub-field allows to distinguish the type of payload, which can be either
[Signal Data](#signal-data) or [Meta Information](#meta-information):

Signal Data: 0x01

Meta Information: 0x02

### Size

Indicates the length in bytes of the data block that follows.

For data blocks bigger than 256 byte, `Size` equals 0x00. In this case 
the length of the data block is to be taken from the (optional) `Data Byte Count` field.

## Signal Number

The `Signal Number` field indicates to which signal the following data
block belongs to. It MUST be unique within a single device. The `Signal Number` is required
to differentiate more than one single signal carried over the same stream.

`0` is the `Signal Number` reserved for [Stream Related Meta Information](#stream-related-meta-information).

## Data Byte Count

This field is only present if `Size` in the header equals 0x00. If
so, `Data Byte Count` represents the length in byte of the data block that
follows. This 32 bit word is always transmitted in little endian.

# Presentation Layer

## Terminology

### Signal
A signal is a data source delivering signal values.

### Signal Number
Identifies the signal on the transport layer.

### Signal Id
A unique string identifying the signal on the representation layer.

### Signal Definition
A signal value consists at least of one member. Structs can be used to combine several members to a compound signal value. In addition dimensions can be used to express vectors and matrices.
The resulting structure is the signal definition.

### Signal Member
The signal definition contains at least one signal member.
A signal member describes either a [base data type](#base-data-types) carrying some measured
information or something creating a hierarchy for a more complex description contains several members.

### Signal Value
The members of the signal definition define the complete signal value. It also defines what data is being transferred.

### Resolution And Absolute Reference
Describes how data is to be interpreted. Both are optional. the resolution is represented as a rational number. When there is an absolute reference, each value plus this absolute reference represents an absolute value.

### Signal Data

The `Data` section contains signal data (measurement data acquired by the device) related to the
respective `Signal_Number`. [Meta Information](#meta-information) are necessary to interpret Signal Data.

### Table
Several signals can be grouped to one table, when they are sharing the same domain / time signal or can be synchronized up to one time grid. So, all signals in a table are to be in step.

### Table Id

The table id identifies which table that signal belongs to. It is optional. If it is set, it guarantees that the signals can be represented in a table and that this does not have to be found out by the client.

### Explicit Signal
Explicit signals transmit a value with each table row.
Several values can be transferred in one package.

### Implicit Signal
Implicit signals don't transmit a value with each table row.

Implicit signals transmit a value whenever their [non-explicit rule](#rules) is not followed. 
In order to have the values aligned with the table row, 
each value of an implicit signal is transmitted with a value index which indicates where the value did not follow the rule.
This has to happen before sending any explicit data of this table with the same or higher value index.

Implicit signals have an implicit rule definition (e.g. “linear” or “constant”) at the top level of the signal definition.

If the sender wants to notify the receiver that the table progressed, but the rule still is followed, 
the sender can simply leave out the value and send the value index only as a table progress marker.

Several pairs of value index and value can be transferred in one package.
There may be only one progress marker per package. A progress marker may only be at the end of a package.

### Automatic Additional Signal
When subscribing a signal, additional signals might be subscribed automatically. 
If this is the case, meta information about the additional signals is to be 
transferred before the meta information about the signal originally subscribed.

This allows the client to know when all signals are described and data is to be expected.

### Value Index
An uint64 sent with each value of an implicit signal. It is used to align the value of the implicit signal with the table steps.




### Meta Information

The `Data` section contains additional ("Meta") information related to
the respective `Signal_Number`. Some [Signal Related Metainformation](#signal-related-meta-information) is REQUIRED to correctly
interpret the respective [Signal Data](#signal-data). 

Meta Information may also carry information about certain events which MAY happen on a
device like changes of the output rate or time resynchronization.

A Meta information block always consists of a Metainfo_Type and a Metainfo_Data block.

#### Presentation in this Document

Meta information carries structrured information. In this document we use json to depict the meta information structure and its content.

### Notifications

Since the configuration of a signal may change at any time, updated meta information may be notified at any time. Only the changed parameters will be transferred,
hence only parts of the meta information will be transferred.

![A Meta Information block](images/meta_block.png)

### Metainfo_Type

The Metainfo_Type indicates the protocol of the data in the Metainfo_Data.
This 32 bit word is always transmitted in little endian.

`msgpack` encoding: `type` = 2

The endianness of the meta information Metainfo_Data block depends on 
the meta information format (msgpack uses big endian).

### Metainfo_Block

It contains the actual meta information data. For `type`= 2 (`msgpack`) it has the following structure:

~~~~ {.javascript}
{
  "method": <string>
  "params" {  }
  "valueIndex": <uint64>
}
~~~~

-`method`: A string with the type of meta information.
-`params`: This is optional. The parameters of the meta information. Can be left out when there are no parameters.
-`valueIndex`: This is optional. It is used to align the meta information with the associated data. If this is left out, the change takes affect at once.

\pagebreak






## Stream Related Meta Information

Everything concerning the stream ([Signal Number](#signal-number) `= 0` on the transport layer)

### API Version

~~~~ {.javascript}
{
  "method": "apiVersion",
  "params": {
    "version": "1.0.0"
   }
}
~~~~

The version follows the [semver scheme](https://semver.org/).

This meta information is always sent directly after connecting to the
stream socket.


### Init Meta

The Init Meta Information provides the stream id (required for
[subscribing signals](#command-interfaces)) and a set of
optional features supported by the device. The optional features are specified in a separate document.

~~~~ {.javascript}
{
  "method": "init",
  "params": {
    "streamId": <string>,
    "supported": {
      "<feature_name>": <feature_description>
      ...
    },
    "commandInterfaces": {
      "<command_interface_a>": {
        ... // service details
      },
      "<command_interface_b>": {
        ... // service details
      }
	}
  }
}
~~~~

`"streamId"`: A unique ID identifying the stream instance. It is required for
     using the [Command Interface](#command-interfaces).

`"supported"`: An Object which holds all optional features supported by the device. If no optional features are supported, this object MAY be empty.
     The "supported" field's keys always refer to the respective optional feature name.

`"commandInterfaces"`: An Object which MUST hold at least one command interface (descriptions)
     provided by the device. A command interface is required to
     [subscribe](#subscribe-signal) or [unsubscribe](#unsubscribe-signal) a signal.
     The key `<command_interface>` MUST be a String which specifies the name of
     the [command interface](#command-interfaces). The associated Object value
     describes the command interface in further detail.


### Stream Meta Information

Is used to transfer additional information concerning the stream.


~~~~ {.javascript}
{
  "method": "stream",
  "params": {
    "interpretation": { <optional information for further interpretation> }    
  }
}
~~~~


`"interpretation"`: This optional object contains additional information concerning this stream.
     The information is not necessary for processing the protocol but for further interpretation by the client.





### Available signals

If connecting to the streaming server, the ids of all signals that are currently available MUST be delivered.
If new signals appear afterwards, those new signals MUST be introduced by sending an `available` with the new signal names.

~~~~ {.javascript}
{
  "method": "available",
  "params": {
	"signalIds" : [<signal id>,...]
  }
}
~~~~

### Unavailable signals

If signals disappear while being connected, there MUST be an `unavailable` with the ids of all signals that disappeared.

~~~~ {.javascript}
{
  "method": "unavailable",
  "params": {
	"signalIds" : [<signal id>,...]
  }
}
~~~~

### Alive Meta information

It is being written to the ringbuffer every 0.5 seconds. 
It tells about the fill level percentage of the device ringbuffer holding the measured data. 

~~~~ {.javascript}
{
  "method": "alive",
  "params": {
	"fillLevel" : < 0..100 >
  }
}
~~~~
-`fillLevel`: Fill level of the ringbuffer

## Signal Specific Meta Information

### Subscribe Acknowledge Meta Information

After a successful [subscribe request](#subscribe-request), this acknowldege is sent for each subscribed signal.

The string value of the subscribe key always carries the unique signal id of the signal.
It constitutes the link between the subsribed signal id and the `Signal_Number` used on the transport layer.

~~~~ {.javascript}
{
  "method": "subscribe",
  "params": {
    "signalId" : <signal id>
  }
}
~~~~

`signalId`: Signal id of the subscribed signal.



### Unsubscribe Acknowledge Meta Information

After a successful [unsubscribe request](#unsubscribe-request), this acknowldege is sent for each unsubscribed signal.

The unsubscribe Meta information indicates that there MUST NOT be sent any data with the same `Signal_Number` upon next subscribe.
This Meta information is emitted after a signal got unsubscribed.
No more data with the same `Signal_Number` MUST be sent after the unsubscribe acknowledgement.


~~~~ {.javascript}
{
  "method": "unsubscribe"
}
~~~~


### Signal Description

Each signal is described in a signal related meta information `signal`.
There are some example of signal descriptions in a separate document.

~~~~ {.javascript}
{
  "method": "signal",
  "params": {
    "definition": {
      <contains at least one signal member description>
    },
    "tableId": <string>,
    ["valueIndex": <uint64>],
    {
      "relatedSignals": 
      [
        { 
          "type" : <relation type>, 
          "signalId" : <id of the related signal>
        } 
      ]
    },
    ["interpretation": {}]
  }
}
~~~~

- `tableId`: Id of the table the signal belongs to
- `valueIndex`: Optional, and relevant only if domain signal of the table follows an implicit rule such as an implicit linear time signal.
   Tells the value index of the first value of this signal in the table. If missing, the first value comes at the very beginning (valueIndex = 0).

#### Signal Definition Object

A signal value of a signal consist of one or more members.
All signal members and their properties are described in the `definition` object in the `signal` meta information.
There are some examples of signal member descriptions and time families in a separate document.

Each member...

- MUST have the property `name`
- MUST have the property [`rule`](#rules) if the dataType is one of the base types.
- MUST have the property [`dataType`](#data-types)
- MAY have an `absoluteReference`.
- MAY have an `resolution` object.
- MAY have a `unit` object.
- MAY have a `range` object.
- MAY have a `postScaling` object.
- MAY have a `dimensions` object.

Those properties are described using a signal member object:

~~~~ {.javascript}
{
  "name": <string>,
  "rule": <type of rule as string>,
  "dataType": <data type as string>,
  ["dimensions": { <object describing the dimensions>},]
  ["absoluteReference": <string>,]
  ["resolution" : {
    "num": <unsigned int 64>,
    "denom": <unsigned int 64>
  },]
  ["unit": { <object describing the unit>},]
  ["range": { <object describing the range of the member value>},]
  ["postScaling": { <object describing how the member value is to be scaled on the client side>},]
}
~~~~

A signal with just one signal member has just one [base data type](#base-data-types) value. 
When the signal has a more complex value, [struct](#struct) is used to describe it.
When the signal delivers a vector or matrix of the described values, [dimensions](#dimensions) is used to describe it.

- `absoluteReference`: This is optional. If unit quantity is `time`, it is the absolute time all time stamps are based on. It is a [TAI (no leap seconds) time](#https://en.wikipedia.org/wiki/International_Atomic_Time)
given in ISO8601:2004 format (YYYY-MM-DDThh:mm:ss.f, example: 1970-01-01 for the UNIX epoch)
- `resolution`: A rational number (numerator/denominator) defining the tick size.


##### Unit Object

The `unit` object gives information about the unit of the signal member.

~~~~ {.javascript}
{
  "unit": {
    "displayName" : <string>,
    "unitId" : <int>
    "quantity" : <string>
  }
}
~~~~

- "displayName": Textual description of the unit
- "unitId": Unit id as [defined by OPC-UA](http://www.opcfoundation.org/UA/EngineeringUnits/UNECE/UNECE_to_OPCUA.csv)
- "quantity": Tells what quantity it is ( `acceleration`, `electric current`, `length`, `mass`, `soundpressure`, `strain`, `time`, `voltage`). This is according to the 

##### Range Object

Gives information about the highest and/or lowest possible value. Both values are optional.

~~~~ {.javascript}
{
  "range": {
    "high" : <number, default is unlimited>,
    "low" : <number, default is -unlimited>
  }
}
~~~~

##### Post Scaling Object

Tells how the value need to be scaled with an offset and a scale factor. All parameters are optional.

~~~~ {.javascript}
{
  "postScaling": {
    "offset" : <number, default is 0>,
    "scale" : <number, default is 1>
  }
}
~~~~




#### Table Id

The id of the table the signal belongs to.

#### Related Signals

It contains relation type and signal id of signals that add additional information to interpret this signal.

~~~~ {.javascript}
{
  "relatedSignals": 
    [ 
      {
        "type" : <relation type>, 
        "signalId" : <id of the related signal> 
      }
    ]
}
~~~~

- `type`: The relation type tells how to interprete the related signal
	- `domain` signal could be the time. Absolutely necessary for a data signal
	- `status` signal carrying status information for this data signal.
- `signalId`: Id of the related signal

#### Signal Interpretation

This is optional and contains information about the signal as a whole for further interpretation.

## Signal Data

After the meta information describing the signal has been received, measured values are to be interpreted as follows:

### Explicit Signals

- The size of a complete signal value derives from the sum of the sizes of all explicit members
- Members are sent in the same sequence as in the meta information describing the signal
- Only members with an explicit rule are transferred.
- Non explicit members are calculated according their rule (i.e. constant, linear). They take no room within the transferred signal data blocks.
- The size of each value is the sum of the size of all explicit signal members

### Implicit Signals

- Members are sent in the same sequence as in the meta information describing the signal
- The size of each value is the sum of the sizes of all explicit members. When transmitted the value is always prepended with the index at which the change takes place (an uint64).



# Rules

A signal member might follow a specific rule. There will be an absolute start value and a rule to calculate the following values. 
This is used to dramatically reduce the amount of data to be transferred. The receiving client uses the rule to calculate the values.

## Range

Some rules use the type range. it has the form.

~~~~ {.javascript}
{ 
  "low" : <number>,
  "high": <number>
}
~~~~

`number` can be any scalar numeric data type.

## Rule Types

There are different rule types. More rule types can be added in the future.

### Linear Rule {#Linear_Rule}
For equidistant signal members use the linear rule.

It is described by an absolute start value and a relative delta between two neighboring values.
A time signal is an example for a signal with implicit linear rule. On time resynchronization, a new time start value is send together with a value index.
This is send before any explicit data with this value index.

![Equidistant 2 dimensional points](images/equidistant_points.png)


A linear axis is described as follows:

~~~~ {.javascript}
{
  "rule": "linear",
  "linear": {
    "start": <number|range>,
    "delta": <number>,
    ["size": <unsigned number>]
  },
}
~~~~

- `rule`: Type of implicit rule
- `linear`: Properties of the implicit linear rule
- `linear/start`: Absolute value/range of the next following value
- `linear/delta`: The difference between two values
- `linear/size`: If this optional parameter is given, the rule creates `size` values. The rule runs from `start` to `start` + (`delta` * `size`). Otherwise it run indefinitely beginning with `start`.


### Log Rule {#Log_Rule}
For log signal members use the log rule.

It is described by an absolute start value and a relative delta between two neighboring values with logarithmic scaling.

A log axis is described as follows:

~~~~ {.javascript}
{
  "rule": "log",
  "log": {
    "start": <number|range>,
    "delta": <number>,
    "base" : <unsigned number>,
    ["size": <unsigned number>]
  },
}
~~~~

- `rule`: Type of implicit rule
- `log`: Properties of the implicit linear rule
- `log/start`: Absolute value/range of the next following value
- `log/delta`: The difference between two values
- `log/base`: The base of the log function.
- `log/size`: If this optional parameter is given, the rule creates `size` values.


### Constant Rule
The rule is simple: There is a start value. The value equals the start value until a new start value is posted.
A status signal is an example for a signal with implicit constant rule. A new constant value together with a value index is send on change of the status.
This is send before any explicit data with this value index.

~~~~ {.javascript}
{
  "rule": "constant",
  "constant": {
    "start": <number|range>,
  },
}
~~~~

- `constant/start`: Absolute value/range of the following values

### List Rule
This rule has a fixed list of values. Those can be numbers, strings or ranges. 
Size is implicitly defined by the number of elements in `values`.

~~~~ {.javascript}
{
  "rule": "list",
  "list": {
    "values": [<values>]
  },
}
~~~~

- `list/values`: A vector of numbers, strings or ranges. 

\pagebreak


### Explicit Rule
![2 dimensional points](images/non_equidistant_points.png)

When there is no rule to calculate values depending on a start value the explicit rule is being used: Each value is transferred.

~~~~ {.javascript}
{
  "rule": "explicit"
}
~~~~

Explicit rule does not have any parameters.

\pagebreak

# Data Types

The data type names are used in the meta information to describe the structure of the signal data

## Base Data Types

The following base data types are supported:

* `int8`
* `uint8`
* `int16`
* `uint16`
* `int32`
* `uint32`
* `int64`
* `uint64`
* `int128`
* `uint128`
* `real32`
* `real64`
* `complex32`
* `complex64`
* `bitField`

## Complex Numbers
The types `complex32` and `complex64` are used to express complex numbers both have the two elements `real` and `imag`.
For `complex32` both are of type `real32`. For `complex64` both are of type `real64`.


## Dimensions

Dimensions adds dimensions to a [signal members](#signal-member). A signal could have 0 to n dimensions. If missing the default 0 is used.
Each contained dimension:
- has to follow a rule
- may have a unit

~~~~ {.javascript}
{
  "dimensions": 
  [
    {
      "name": <value> 
      "rule" : <ruleType>,
      <rule details>
      ["unit": <unit of the dimension>]
    }
  ]
}
~~~~

- `dimensions` contains an array of objects. When there is element, the signal member does hold a vector. When there are two elements, the signal holds a matrix.
- `dimensions/name`: Name of the dimension
- `dimensions/rule`: Implicit rule type for the dimension. Explicit rule is not allowed here!
- `dimensions/<rule details>`: Rule details depending on rule type 
- `dimensions/unit`: The unit of the dimension. This is optional.

### Transferred Data

The explicit content of the value is transferred based on the dimensions description. So, if sample is described with a deminsion with a count 20, 20 values are transfered.

## Struct

A combination of named members which may be of different types.

~~~~ {.javascript}
{
  "dataType" : "struct",
  "struct": [
    {
      <member description>
    },
      ...
    {
      <member description>
    }
  ]
}
~~~~

- `<member description>`: Description of a struct member. Can be a base data type or struct

### Transferred Data

The explicit content of all members is transferred.

\pagebreak

## Bit Field

A bit field is an unsigned integer where each bit has a described meaning. 

~~~~ {.javascript}
{
  "dataType" : "bitField",
  "bitField": {
    "dataType": < an unsigned integer i.e uint32 >,
    "bit": [
      {
        "index" : 0,
        "uuid" : <a uuid>
        "description" : <descripting text>
      },
      {
        "index" : 1,
        "uuid" : <another uuid>
        "description" : <another descripting text>
      },
    ]
  }
}
~~~~

- `dataType`: Any unsigned integer mentioned in the [base types](#base-types).
- `bitDescription`: An array with objects describing the relevant bits.
  Only used bits need to be descriped, so indices may be left out.
  * `index` : The bit index starts with 0 (is zero based) being the least significant bit.
  * `uuid` : Universally unique identifier to allow recognition by software
  * `description` : A describing text

### Transferred Data

values of type `bitField/dataType`


# Command Interface

The command interface is used to subscribe/unsubscribe signals to/from an existing stream.
There are just those two methods available.

## Subscribe Request

- After connecting to a streaming server, the client MAY request to subscribe signals at any time via
  command interface as described in the [Init Meta Information](#init-meta).
- Once succesfully subscribed, the device acknowledges the operation with a
  [Subscribe Acknowledge Meta Information](#subscribe-acknowledge-meta-information) for each signal and all relevant
  [Signal Releated Meta Information](#signal-related-meta-information). Afterwards the device MUST
  sent the respective [Signal Data](#signal-data) as it becomes available internally and
  MUST NOT leave out Signal Data or
  [Signal Related Meta Information](#signal-related-meta-information).
- Once subscribed, the client MAY [unsubscribe](#unsubscribe-signal) signals at any time via command interface.

The requests carries the stream id and an array of the signal ids to be subscribed.



## Unsubscribe Request

- Subscribed signals MAY unsubscribed at any time.
- After succesfully unsubscribing signals, the already acquired data of the signal is transferred.
- Unsubscribe is acknowledged by [Unsubscribe Acknowledge Meta Information](#unsubscribe-acknowledge-meta-information). No data from this signal is to be delivered afterwards.

The request carries the stream id and an array of signal ids to be unsubscribed.

# History

## Version 0.1.0

- Initial internal version

## Version 0.2.0

- Transport layer header is always transmitted in little endian

## Version 0.3.0

- Added `alive` meta information
- Meta information `available` and `unavailable` changed to an object having a key `signalIds` with an array of signal ids.
- Time object is optional
- Endianness of signal data removed
- Introduced Index Signal
- Introduced data type of Timestamps
- Added `leapSeconds` to time object

## Version 0.4.0

- Meta information `subscribe` changed: `params` now contain an object having a key `signalId`.

## Version 0.5.0

- Added data types int128, uint128

## Version 0.6.0

- Index Signal got removed
- Introducing tables
- Removed `leapSeconds` from time object

## Version 0.7.0

- Epoch is optional
- Introducing related signals
- Introducing data type bitfield

## Version 0.7.1

- Fixed bitfield to bitField


## Version 0.7.2

- Introduce [Automatic Additional Signal](#automatic-additional-signal)

## Version 0.7.3

- Introduce optional unit for signal member

## Version 0.7.4

- Add unit id to unit object

## Version 1.0.0

- Changed to openDAQ streaming protocol
- Replaced Arrays with Dimensions
- Replaced Timefamily with Resolution
- Add a relatedSignals reference as required for data signals
- Added optional parameter `size` to `Linear` and `log` rule
- Add `quantity`to unit object. Quantity `time` is used for signals carrying the time.
- `Time` object got removed, epoch and resolution moved up one level.
- Related signal types `domain` and `status`
- `epoch` got renamed to `origin`

## Version 1.0.1
- `signal` meta information gest optional `valueIndex`. This can be used to add 
  another data signal to an existing table.
- `origin` got renamed to `absoluteReference`

## Version 1.2.0

- Introduced `range` and `postScaling` in signal members
