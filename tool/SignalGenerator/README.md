# openDAQ LT Streaming Signal Generator

Generates synthetic streamed measured data.

The following signal patterns are supported:

* Constant value
* Sine with configurable frequency, amplitude and offset
* Rectangle with configurable frequency, amplitude, offset and duty cycle
* Saw tooth with confirgurable frequency, amplitude, offset
* Impulse with confirgurable frequency, amplitude, offset

Each signal has an independent output rate. Each signal has an indepetent start time.


There are three output modes:

* `siggen2websocket`: Signal generator behaves as websocket streaming server. Any streaming client may connect.
* `siggen2socket`: Signal generator behaves as tcp streaming server. Any streaming client may connect.
* `siggen2file`: Signal generator writes to file.
* `siggen2stdout`: Signal generator writes human readable output to standard out

The signal generator gives you signal data without being asked for. Most devices in contrast will not deliver signal data untils signals are subscribed.

Processing of all signals takes place with a fixed processing rate. A Processing time may be limited or indefinite.
Choosing a longer processing period leads to higher latency and bigger data blocks. Processing can be done in realtime according to the processing rate.
Processing can also be done without waiting which is usefull when writing to file.

## Build

Use cmake to build

## Configuration

There is a default signal configuration. A configuration file with the desired configuration can be provided instead. See `config.json` as example.

```json
{
  "signals" : {
    "configured_signal" : {
      "dataType" : "real32",
      "function" : "sine",
      "amplitude" : 3.5,
      "offset" : 3.1,
      "frequency" : 10,
      "samplePeriod" : "1ms",
      "explicitTime": false
    },
    "another_signal" : {
      "dataType" : "int32",
      "function" : "rectangle",
      "amplitude" : 3,
      "offset" : 3,
      "frequency" : 1,
      "samplePeriod" : "1ms",
      "explicitTime": false
    }    
  },
  "executionTime": "10s",
  "processPeriod": "1s"
}
```

- The document has a section `signals` for describing all signals and their function
	* `dataType` affects `amplitude` and `offset`. Possible types are `real32`, `real64`, `ìnt32`, `int64`
	* `function` possible funciton tyopes are: `sine`, `rectangle`, `impulse`, `constant`, `sawtooth`
- `executionTime` is the duration of execution. Leave this away to have indefinite execution. After the execution time has elapsed, the signal generator stops execution.
- `processPeriod` the cycle time for processing. Choosing a small value here results in smaller data blocks and lower latency.


### Used Libraries

- [boost](https://www.boost.org/)
- [nlohmann/json](https://github.com/nlohmann/json)
