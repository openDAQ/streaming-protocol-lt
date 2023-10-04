---
title:  openDAQ Stream Protocol Client Requirements
subtitle: Draft
titlepage: true
toc: true
toc-own-page: true
author: Version 0.0.0
papersize: a4
fontsize: 9pt
numbersections: true
linkcolor: blue
---

# Introduction

openDAQ Streaming Protocol is a very generic protocol. This document states the minimum requirements for an openDAQ Streaming Client.
It is a living document. Over time more chapters will be added as the hardware evolves.

## Requirements

### Common 

- Measured data will be encoded in little endian.
- Each signal has an implicit time / domain that follows a linear rule.
- Each signal has it's own start time for the implicit time rule. Start times might not fit into the output rate grid, since channels might have different delay in signal processing.
- We use the UNIX epoch which is the 1st january of 1970
- Since we are using PTP synchronization we need a time resolution of 1ns for synchrnoization.

## Promises

When subscribing a signal, the device delivers a signal meta information containing the complete signal description. Later changes might deliver the changed parts only.

## References to other signals

For the interpretation of a measured value typically three signals are necessary, once the signal which delivers the value itself, a signal which sets the values in relation to a domain (for example time signal) and possibly a third signal which delivers a status. 

Each value signal must at least always have a reference to a domain signal. The client must therefore wait until it interprets the data that it assembles the information from the different signals.

__Remark__: Domain signals can be used for several value signals.
