---
title:  openDAQ Stream Protocol Workflow
subtitle: Draft
titlepage: false
toc: false
toc-own-page: true
author: Version 0.0
papersize: a4
fontsize: 9pt
numbersections: true
---

When connecting to the openDAQ streaming server, the client will receive some 
information telling about the stream (see stream related meta information in the openDAQ Streaming Protocol Specification for details).

This includes the available signals. As a result, the client knows the signal ids of all signals that may be subscribed.

The command interface is used to subscribe/unsubscribe any available signal at any time.

After a signal was subscribed, there will be meta information describing the subscribed signal.
Afterwards produced signal data will be send. The client will be able to interpret this using the meta information received before.
