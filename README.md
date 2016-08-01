SONiC NAS QoS
===============

Quality of service utilities for the SONiC project

Description
-----------

This repo contains base QoS functionality for the SONiC software. It provides functions to configure policer, queue, scheduler, mapping as well as port QoS rules.

Building
---------
Please see the instructions in the sonic-nas-manifest repo for more details on the common build tools.  [Sonic-nas-manifest](https://stash.force10networks.com/projects/SONIC/repos/sonic-nas-manifest/browse)

Build Requirements:
 - sonic-common
 - sonic-base-model
 - sonic-nas-common
 - sonic-object-library
 - sonic-logging
 - sonic-nas-ndi-api
 - sonic-nas-ndi

Dependent Packages:
  libsonic-logging-dev libsonic-logging1 libsonic-model1 libsonic-model-dev libsonic-common1 libsonic-common-dev libsonic-object-library1 libsonic-object-library-dev sonic-sai-api-dev libsonic-nas-common1 libsonic-nas-common-dev sonic-ndi-api-dev  libsonic-nas-ndi1 libsonic-nas-ndi-dev libsonic-sai-common1 libsonic-sai-common-utils1


BUILD CMD: sonic_build  --dpkg libsonic-logging-dev libsonic-logging1 libsonic-model1 libsonic-model-dev libsonic-common1 libsonic-common-dev libsonic-object-library1 libsonic-object-library-dev sonic-sai-api-dev libsonic-nas-common1 libsonic-nas-common-dev sonic-ndi-api-dev  libsonic-nas-ndi1 libsonic-nas-ndi-dev --apt libsonic-sai-common1 libsonic-sai-common-utils1 -- clean binary

(c) Dell 2016
