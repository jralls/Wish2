#!/bin/sh
#

cat x10_core.c | grep DISTRIBUTION_VERSION | awk '{print $6}' | cut -c2-
