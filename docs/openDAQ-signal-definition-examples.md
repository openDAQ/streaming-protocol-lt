---
Title:  HBK Signal Definition Examples
Author: Matthias Loy, Helge Rasmussen
Version: 0.1
---

In this document you find examples how signals are described in the hbk streaming protocol.

When describing what signal data is sent, the examples always assume we are sending one value. More values can of course be sent in one packet if necessary.


# Signal Member Examples

## A Voltage Sensor

The signal has 1 scalar value. Synchronous output rate is 100 Hz

- Each signal value consists of one member
- This member is a scaled 32 bit real64 base data type which is explicit
- The time is linear.

The device sends the following `signal` meta information:

### Time signal:

~~~~ {.javascript}
SignalId : "voltageTimeSignal"
{
  "method": "signal",
  "params" : {
      "definition" : {
      "name": "voltage_time",
      "rule: "linear",
      "linear": {
        "delta": 10
      }
      "dataType": "uint64",
      "unit": {
        "displayName" : "s",
      }
      "time" : {
        "resolution" : {
          "num" :1,
          "denom" : 1000
        },
        "absoluteReference": "1970-01-01"
    }
  }
}
~~~~

Unit for the time is seconds resolution is 1/1000 of a second. Time stamp would be in milliseconds since the epoch.


Transferred data: 

A uint64 containining index of the time (0) and a uint64 with the time at that index,
after that no more time information is sent unless there is a break in the linear time such as a pause.

~~~~
valueIndex (uint64)
time (uint64)
~~~~


### Data signal:

~~~~ {.javascript}
{
  "method": "signal",
  "params" : {
    "tableId": "voltage_table",
    "definition" : {
      "name": "voltage",
      "rule: "explicit",
      "dataType": "real64"
    }
  }
  ["relatedSignals": [ { "type" : domain, "signalId" : voltageTimeSignal} ]
}
~~~~

Transferred data for one signal value:

~~~~
value (real64)
~~~~



## A CAN Decoder

The signal has a simple scalar member.

- The value is expressed as a base data type
- The member is explicit.
- The time is explicit.

The device sends the following `signal` meta information:


### Time signal:
~~~~ {.javascript}
SignalId : "canTimeSignal"
{
  "method": "signal",
  "params" : {
      "definition" : {
      "name": "decoder_time",
      "rule: "explicit",
      "dataType": "uint64",
      "time" : {
        "resolution" : {
          "num" : 1,
          "denom" : 1000000
        },
        "absoluteReference": "1970-01-01"
    }
  }
}
~~~~

Transferred data for one signal value: 

~~~~
time (uint64)
~~~~

`time` is microseconds since eopch.



### Data signal:
~~~~ {.javascript}
{
  "method": "signal",
  "params" : {
    "definition" : {
      "name": "decoder",
      "rule: "explicit",
      "dataType": "uint32",
    }
    ["relatedSignals": [ { "type" : domain, "signalId" : canTimeSignal} ]
  }
}
~~~~

Transferred data for one signal value:

~~~~
uint32
~~~~


## A Simple Counter

This is for counting events that happens at any time (explicit rule).

- The signal value is expressed as a base data type
- The member `counter` is linear with an increment of 2, it runs in one direction
- The device sends an initial absolute value of `counter` within the meta information describing the signal.
- `time` is explicit.

### Time signal:
~~~~ {.javascript}
{
  "method": "signal",
  "params" : {
    "tableId": "counter_table",
    "definition" : {
      "name": "counter_time",
      "rule: "explicit",
      "dataType": "uint64",
      "time" : {
        "resolution" : {
          "num" : 1,
          "denom" : 1000000
        },
        "absoluteReference": "1970-01-01"
      }
    }
  }
}
~~~~

Transferred data for one signal value: 

~~~~
time (uint64)
~~~~


### Data signal:

~~~~ {.javascript}
{
  "method": "signal",
  "params" : {
    "tableId": "counter_table",
    "definition" : {
      "name": "counter",
      "dataType": "uint32",
      "rule: "linear",
      "linear": {
        "delta" : 2
      }
    }
  }
}
~~~~

`counter` has a linear rule with a step width of 2, hence only the initial counter position will be transferred.

Transferred signal data (sent for index 0):

~~~~
0 (valueIndex: uint64),
initial counter position (uint32)
~~~~


## An Absolute Rotary Encoder

- The signal value is expressed as a base data type
- `angle` is explicit, it can go back and forth
- `time` is explicit.


### Time signal:
~~~~ {.javascript}
{
  "method": "signal",
  "params" : {
    "tableId": "angle_table",
    "definition" : {
      "name": "angle_time",
      "rule: "explicit",
      "dataType": "uint64",
      "time" : {
        "resolution" : {
          "num" : 1,
          "denom" : 1000000
        },
        "absoluteReference": "1970-01-01"
      }
    }
  }
}
~~~~

Transferred data for one signal value: 

A uint64 containining the time.

~~~~
time (uint64)
~~~~


### Data:

~~~~ {.javascript}
{
  "method": "signal",
  "params" : {
    "tableId": "angle_table",
    "definition" : {
      "name": "angle",
      "rule: "explicit",
      "dataType": "real64"
    }
  }
}
~~~~

Transferred signal data for one signal value:

~~~~
angle (real64)
~~~~



## An Incremental Rotary Encoder with start Position

- The signal value is expressed as a base data type
- The counter representing the angle follows a linear rule, it can go back and forth
- Absolute start position when crossing a start position.
- No initial absolute value.
- The `time` is explicit.



### Time signal:
~~~~ {.javascript}
{
  "method": "signal",
  "params" : {
    "tableId": "angle2_table",
    "definition" : {
      "name": "angle2_time",
      "rule: "explicit",
      "dataType": "uint64",
      "time" : {
        "resolution" : {
          "num" : 1,
          "denom" : 1000000
        },
        "absoluteReference": "1970-01-01"
      }
    }
  }
}
~~~~

Transferred data for one signal value: 

~~~~
time (uint64)
~~~~


### Data signal:
~~~~ {.javascript}
{
  "method": "signal",
  "params" : {
    "tableId": "angle2_table",
    "definition" : {
      "name": "angle2",
      "dataType": "int32",
      "rule: "linear",
      "linear": {
        "delta": 1
      }
    }
  }
}
~~~~


This is similar to the simple counter. 
`angle` changes by a known amount of 1. Only time stamps are being transferred.

Transferred signal data (Sent at start)

~~~~
0 (valueIndex: uint64),
angle (uint32)
~~~~


We get a zero value for the counter every time the zero index is being crossed:

~~~~
valueIndex (uint64),
angle (uint32)
~~~~

If the rotation direction changes, we get a (partial) meta information with a new `delta` for the linear rule:

~~~~ {.javascript}
{
  "method": "signal",
  "params" : {
    "definition" : {
      "linear" : {
        "delta": -1
      }
    }
  }
}
~~~~

This type of counter is useful when having a high counting rate with only few changes of direction. The handling is complicated because there are lots of meta information being send.

\pagebreak

## Two signals measured at certain times

- Time signal: Id: "time1"


~~~~ {.javascript}
{
  "method": "signal",
  "params" : {
    "tableId": "a_table",
    "definition" : {
      "name": "Time1",
      "rule: "explicit",
      "dataType": "uint64",
      "time" : {
        "resolution" : {
          "num" : 1,
          "denom" : 1000000
        },
        "absoluteReference": "1970-01-01"
      }
    }
  }
}
~~~~

Transferred signal data for one signal value:
~~~~
   time (uint64)
~~~~

(if time had been linear there would only have been sent time information once)

- Data signal: Id: "data1"

~~~~ {.javascript}
{
  "method": "signal",
  "params" : {
    "tableId": "a_table",
    "definition" : {
      "name": "data1",
      "rule: "explicit"
      "dataType": "real64"
    }
  }
}
~~~~


Transferred signal data for one signal value:
~~~~
   value (real64)
~~~~

- Data signal: Id: "data2" 
~~~~ {.javascript}
{
  "method": "signal",
  "params": {
    "tableId": "a_table",
    "definition": {
      "name": "Data2",
      "rule: "explicit",
      "dataType": "real64"
    }
  }
} 
~~~~

Transferred signal data for one signal value:
~~~~
   value (real64)
~~~~
 
\pagebreak

## Two signals measured at certain rpms. 

Time for the measurement is also included to show that this also is possible

- RPM signal: Id: "rpm1"

~~~~ {.javascript}
{
  "method": "signal",
  "params": {
    "tableId": "rpm_table",
    "definition": {
      "name": "rpm",
      "rule: "explicit",
      "dataType": "real64"
    } 
  }
} 
~~~~

Transferred signal data for one signal value:
~~~~
   rpm (real64)
~~~~

- Time signal: Id: "time"

~~~~ {.javascript}
{
  "method": "signal",
  "params" : {
    "tableId": "rpm_table",
    "definition" : {
      "name": "time",
      "rule: "explicit",
      "dataType": "uint64",
      "time" : {
        "resolution" : {
          "num" : 1,
          "denom" : 1000000
        },
        "absoluteReference": "1970-01-01"
      }
    }
  }
}
~~~~

Transferred signal data for one signal value:
~~~~
   time (uint64)
~~~~

- Data signal: Id: "data1"

~~~~ {.javascript}
{
  "method": "signal",
  "params": {
    "tableId": "rpm_table",
    "definition": {
      "name": "data1",
      "rule: "explicit",
      "dataType": "real64"
    }
  }
} 
~~~~

Transferred signal data for one signal value:
~~~~
   value (real64)
~~~~

- Data signal: Id: "data2"

~~~~ {.javascript}
{
  "method": "signal",
  "params": {
    "tableId": "rpm_table",
    "definition": {
      "name": "data2",
      "rule: "explicit",
      "dataType": "real64"
    }
  }
} 
~~~~

Transferred signal data for one signal value:
~~~~
   value (real64)
~~~~
 

\pagebreak

## A Spectrum

The signal consists of a spectum

- Each value consists of two elements:
	* Frequency which is implicit linear, it has an absolute start value of 100.
	* Amplitude which is explicit
- The time is explicit. Each complete spectrum has one time stamp.


Several amplitude values over the frequency.
There is one base data type having one dimension.



### Time signal:
~~~~ {.javascript}
{
  "method": "signal",
  "params" : {
    "tableId": "spectrum_table",
    "definition" : {
      "name": "spectrum_time",
      "rule: "explicit",
      "dataType": "uint64",
      "time" : {
        "resolution" : {
          "num" : 1,
          "denom" : 1000000
        },
        "absoluteReference": "1970-01-01"
      }
    }
  }
}
~~~~

Transferred data: 

~~~~
   time (uint64)
~~~~


### Data signal:
~~~~ {.javascript}
{
  "method": "signal",
  "params" : {
    "tableId": "spectrum_table",
    "definition" : {
      "name": "amplitude",      
      "dataType" : "real64",
      "rule" : "explicit",
      "dimensions" : [      
        {
          "name" : "frequency",
          "rule" : "linear",
          "linear" : { 
            "start" : 0.0,
            "delta" : 10.0,
            "size" : 1024   	                    
          }
        }
      ]
    }
  }
}
~~~~

- member `amplitude`: Describes the measured values (i.e. amplitude, attenuation).
- dimensions of `amplitude`:
	- `frequency` describes the range in the spectral domain (i.e. frequency)

Each value consists of 1024 amplitude real64 values.


~~~~
   amplitude 1 (real64)
   amplitude 2 (real64)
   ...
   amplitude 1024 (real64)
~~~~

\pagebreak


## A Spectrum with Peak Values

The signal consists of a spectum and an array of peak values. Number of peaks is fixed 16.
If the number of peaks does change, there will be a meta information telling about the new amount (`count`) of peaks!

Meta information describing the signal:


### Time signal:
~~~~ {.javascript}
{
  "method": "signal",
  "params" : {
    "tableId": "spectrumWithPeakValues_table",
    "definition" : {
      "name": "spectrumWithPeakValues_time",
      "rule: "explicit",
      "dataType": "uint64",
      "time" : {
        "resolution" : {
          "num" : 1,
          "denom" : 1000000
        },
        "absoluteReference": "1970-01-01"
      }
    }
  }
}
~~~~

Transferred data: 

~~~~
   time (uint64)
~~~~


### Data:
~~~~ {.javascript}
{
  "method": "signal",
  "params": {
    "tableId": "spectrumWithPeakValues_table",
    "definition" : {
      "name": "spectrumWithPeakValues",
      "struct": [
        {
          "name": "amplitude",
          "dataType" : "real64",
          "rule" : "explicit",
          "dimensions" : [      
            {
              "name" : "frequency",
              "rule" : "linear",
              "linear" : {
                "start" : 0.0,
                "delta" : 10.0,
                "size" : 1024   	                    
              }
            }
          ]
        },
	    {
		  "name": "peakValues",
		  "dataType": "struct",
          "rule" : "explicit",
		  "struct": [
		    {
			  "dataType": "real64",
		   	  "name": "frequency",
		   	  "rule" : "explicit"
		    },
		    {
		  	  "name": "amplitude",
			  "dataType": "real64",
			  "rule" : "explicit"
		    }
		  ]
		  "dimensions": [
  	   	    {
			  "rule": "linear",
			  "linear": {
  			    "start" : 0,
				"delta" : 1
				"size": 16
              }
		    }
		  ]
	    }     
      ]
    }   
  }
}
~~~~


Transferred signal data for one signal value:

- 1024 spectrum amplitude real64 values. No spectrum frequncy values because those are implicit.
- 16 amplitude, frequency pairs.


~~~~
   spectrum amplitude 1 (real64)
   spectrum amplitude 2 (real64)
   ...
   spectrum amplitude 1024 (real64)

   frequency point 1 (real64)
   amplitude point 1 (real64)
   frequency point 2 (real64)
   amplitude point 2 (real64)
   ...
   frequency point 16 (real64)
   amplitude point 16 (real64)
~~~~


\pagebreak






## A Matrix

The signal consists of a matrix of the following form:


		A		B		C		D

1		measval

2				measval		
			
3						measval


One matrix is generated per second.


### Time signal:

This is a relative time without an epoch and start value 0. resolution is one per second.

~~~~ {.javascript}
{
  "method": "signal",
  "params" : {
    "tableId": "a_table",
    "definition" : {
      "name": "Time1",
      "dataType": "uint64",
      "rule: "linear",
      "linear : {
        "start" : 0,
        "delta" : 1
      },
      "time" : {
        "resolution" : {
          "Num" : 1,
          "Denom" : 1
        },
    }
  }
}
~~~~

Transferred data: 

A uint64 containining index of the time (0) and a uint64 with the time at that index (0),
after that no more time information is sent unless there is a break in the linear time such as a pause.

~~~~
valueIndex (uint64)
time (uint64)
~~~~




### Data signal:
~~~~ {.javascript}
{
  "method": "signal",
  "params" : {
    "tableId": "matrix_table",
    "definition" : {
      "name": "measval",      
      "dataType" : "real64",
      "rule" : "explicit",
      "dimensions" : [      
        {
          "name" : "Xdim",
          "dataType" : "string",
          "rule" : "list",
          "list" : { 
            "values" : ["A", "B", "C", "D"]
          }
        },
        {
          "name" : "Ydim",
          "dataType" : "uint8",
          "rule" : "linear",
          "linear" : {
            "start" : 1,
            "delta" : 1,
            "size": 3
          }
        }
      ]
    }
  }
}
~~~~

-Matrix contains double values
-1st dimension of the matrix is descibed using a list of 4 strings.
-2nd dimension of the matrix is described using a linear rule with size 3

Transferred data:

Each combined matrix signal value consists of 4 * 3 real64 values

~~~~
   measval 1 (real64)
   measval 2 (real64)
   ...
   measval 12 (real64)
~~~~


\pagebreak




## Statistics

Statistics consists of N "counters" each covering a value interval. If the signal value is within a counter interval, then that counter is incremented.
For instance the interval from 50 to 99 dB might be covered by 10 counters. Each of these intervals then would cover 5 dB.

Often there also is a lower than lowest and higher than highest counter, and for performance reasons, there might be a total counter.


Example: 50 - 99 dB statistics:
It is made up of a struct containing an histogram with 10 classes (bins) and three additional counters for the lower than, higher than and total count.




### Time signal:
~~~~ {.javascript}
{
  "method": "signal",
  "params" : {
    "tableId": "soundLevelStatistics_table",
    "definition" : {
      "name": "soundLevelStatistics_time",
      "rule: "explicit",
      "dataType": "uint64",
      "time" : {
        "resolution" : {
          "num" : 1,
          "denom" : 1000000
        },
        "absoluteReference": "1970-01-01"
      }
    }
  }
}
~~~~

Transferred data: 

~~~~
   time (uint64)
~~~~


### Data:
~~~~ {.javascript}
{
  "method": "signal",
  "params": {
    "tableId": "soundLevelStatistics_table",
    "definition" : {
      "name": "soundLevelStatistics",
      "rule: "explicit",
      "dataType": "struct",
      "struct": [
        {
          "name": "count",      
          "dataType" : "uint64",
          "rule" : "explicit",
          "dimensions" : [      
            {
              "name" : "class",
              "dataType": "uint32",
              "unit": {
                "displayName" : "db",
              },
              "rule" : "linear", 
              "linear" : {
                "start" : {
                  "low" : 0,
                  "high": 4
                },
                "delta" : 5
                "size" : 10   	                    
              } 
            }
          ]
        },
        {
          "name": "lowerThanCounter",
          "dataType": "uint64",
          "rule" : "explicit"
        },
        {
          "name": "higherThanCounter",
          "dataType": "uint64",
          "rule" : "explicit"
        },
        {
          "name": "totalCounter",
          "dataType": "uint64",
          "rule" : "explicit"
        }
      ]
    }
  }
}
~~~~

There is a struct describing a histrogram. It has the following members:
- `count` is a vector of 10 counters over the ranges 50db..54db, 55db..59db, 60db..64db, ..., 95db..99db
- `lowerThanCounter` counter for events lower than the 1st counting bin
- `higherThanCounter` counter for events higher than the last counting bin
- `totalCounter` counter of all events.

Transferred signal data for one signal value:

- 10 uint64 for the 10 counters
- 1 uint64 for the higher than counter
- 1 uint64 for the lower than counter
- 1 uint64 for the total counter

~~~~
   counter 1 (uint64)
   counter 2 (uint64)
   ...
   counter 50 (uint64)
   higher than counter (uint64)
   lower than counter (uint64)
   total counter (uint64)
~~~~

\pagebreak

## Run up

This is an array of 15 structs containing a FFT and an exciter frequency.
FFT amplitudes and exciter frequency are explicit.

We'll get the following signal specific meta information:


### Time signal:
~~~~ {.javascript}
{
  "method": "signal",
  "params" : {
    "tableId": "runUp_table",
    "definition" : {
      "name": "runUp_time",
      "rule: "explicit",
      "dataType": "uint64",
      "time" : {
        "resolution" : {
          "num" : 1,
          "denom" : 1000000
        },
        "absoluteReference": "1970-01-01"
      }
    }
  }
}
~~~~

Transferred data: 

~~~~
time (uint64)
~~~~


### Data signal:
~~~~ {.javascript}
{
  "method": "signal",
  "params": {
    "tableId": "runUp_table",
    "definition" : {
      "name": "runUp",
      "rule: "explicit",
      "dataType" : "struct",
      "struct": [
        {
          "name": "exciterFrequency",
          "dataType": "real64",
          "rule" : "explicit"
        },
        {
          "name": "amplitude",      
          "dataType" : "real64",
          "rule" : "explicit"
          "dimensions" : [      
            {
              "name" : "frequency",
              "rule" : "linear", 
              "linear" : {
                "start" : 0.0,
                "delta" : 10.0,
                "size" : 100   	                    
              }
            }
          ]
        }
      ]
      "dimensions": [
   	    {
          "name": "run",
		  "rule": "linear",
          "linear": {
  			"start": 0,
			"delta": 1,
			"size": 15
          }
		}
	  ]      
    }
  }
}
~~~~


Transferred signal data for one signal value:
15 frequencies with the corresponding spectra containing 100 amplitude values each.

~~~~

   frequency 1 (real64)
   amplitude 1 belonging to frequency 1 (real64)
   amplitude 2 belonging to frequency 1 (real64)
   ...
   amplitude 100 belonging to frequency 1 (real64)
   
   frequency 2 (real64)
   amplitude 1 belonging to frequency 2 (real64)
   amplitude 2 belonging to frequency 2 (real64)
   ...
   amplitude 100 belonging to frequency 2 (real64)
   
   ...

   frequency 15 (real64)
   amplitude 1 belonging to frequency 15 (real64)
   amplitude 2 belonging to frequency 15 (real64)
   ...
   amplitude 100 belonging to frequency 15 (real64)
~~~~

\pagebreak

## Point in Cartesian Space

Depending on the the number of dimensions n, 
The value is a struct of n real64 values. In this example we choose 3 dimensions x, y, and z.

We'll get the following signal specific meta information:

### Time signal:
~~~~ {.javascript}
{
  "method": "signal",
  "params" : {
    "tableId": "coordinate_table",
    "definition" : {
      "name": "coordinate_time",
      "rule: "explicit",
      "dataType": "uint64",
      "time" : {
        "resolution" : {
          "num" : 1,
          "denom" : 1000000
        },
        "absoluteReference": "1970-01-01"
      }
    }
  }
}
~~~~

Transferred data: 

~~~~
   time (uint64)
~~~~


### Data signal:
~~~~ {.javascript}
{
  "method": "signal",
  "params": {
    "tableId": "coordinate_table",
    "definition": {
      "name": "coordinate",
      "dataType": "struct",
      "struct": [
        {
          "name": "x",
          "dataType": "real64",
          "rule: "explicit"
        },
        {
          "name": "y",
          "dataType": "real64",
          "rule: "explicit"
        },
        {
          "name": "z",
          "dataType": "real64",
          "rule: "explicit"
        }
      ]
    }    
  }
}
~~~~

Transferred signal data for one signal value: Three real64 values.

~~~~
   x (real64)
   y (real64)
   z (real64)
~~~~

\pagebreak


# Signal Definition Notifications

When subscribing a signal, the current signal definition is being delivered. While acquisition is running the signal definition might change. This results in notifications telling about the changed values.
Since only parts of the signal definition change, only the changed parts are transferred.
Here are some examples:

## Change of signal output rate

In this case the `delta` of the linear rule for the signal changes and will be notified.

~~~~ {.javascript}
{
  "method": "signal",
  "params" : {
    "definition" : {
      "linear": {
        "delta": 4294967296
      }
    }
  }
}
~~~~

## Resynchronisation of the device

In this case the `start` of the linear rule changes to a new absolute start time. Since the synchronization affects the whole device, this will be notified to all subscribed signals of the device.

The new time will simply be sent as a implicit data packet (i.e. a packet that includes the index where the time changed)

~~~~
valueIndex (uint64)
time (uint64)
~~~~

# Resolution Examples

Here are some time family examples:

- 1200 Hz = [1/1200]
- 44100 Hz = [1/44100]
- 0.5 Hz = [ 2/1 ]


