#!/bin/sh
su whiles -c '/usr/bin/x10watch /dev/x10/g11 -0 "echo 0 > /dev/x10/g1" -1 "echo 1 > /dev/x10/g1" &'
su whiles -c '/usr/bin/x10watch /dev/x10/g12 -0 "echo 0 > /dev/x10/g1" -1 "echo 1 > /dev/x10/g1" &'
