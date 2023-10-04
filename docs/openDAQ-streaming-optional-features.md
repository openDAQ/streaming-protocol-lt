---
title:  openDAQ Stream Protocol Optional Features
subtitle: Draft
titlepage: false
toc: false
toc-own-page: true
author: Version 0.0
papersize: a4
fontsize: 9pt
numbersections: true
---

An openDAQ Streaming Protocol Server may supported additional fatures. 

# Ringbuffer Fill Level

Is send at will. The value of `fill` is a number
between 0 and 100 which indicates the stream`s associated data buffer
fill level. A fill value of 0 means the buffer is empty. A fill value of 100
means the buffer is full and the associated stream (and the associated
socket) will be closed as soon as all previously acquired data has been
send. This meta information is for monitoring purposes only and it is
not guaranteed to get a fill = 100 before buffer overrun.

## Fill Meta Information

~~~~ {.javascript}
{
  "method": "fill",
  "params": [38]
}
~~~~

## Fill Feature Object

If this feature is supported, the [Init Meta information`s](#init-meta)
"supported" field must have an entry named "fill" with this value:

~~~~ {.javascript}
true
~~~~
