#!/bin/bash
#---------------------------------------------------------------------------
# Copyright (c) 2022 TUM. All rights reserved.
#---------------------------------------------------------------------------
TYPE=${1:-namenode}
HOST=${2:-localhost}
IFACE=${3:-eth0}
HUGEPATH=${4}

# Build docker image
docker build --tag=owncrail:latest --ulimit nofile=2048 .

# If a filesystem of hugepages is mounted use it. If not, use a tmpfs instead.
if [ -z ${HUGEPATH+x} ]; then
    docker run -it --rm --network host -e NAMENODE_HOST=${HOST} -e NAMENODE_PORT=9060 -e INTERFACE=${IFACE} --mount type=tmpfs,destination=/dev/hugepages owncrail ${TYPE}
else
    docker run -it --rm --network host -e NAMENODE_HOST=${HOST} -e NAMENODE_PORT=9060 -e INTERFACE=${IFACE} -v ${HUGEPATH}:/dev/hugepages owncrail ${TYPE}
fi
