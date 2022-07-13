#!/bin/bash
#---------------------------------------------------------------------------
# Copyright (c) 2022 TUM. All rights reserved.
#---------------------------------------------------------------------------

# Build docker image
docker build --tag=umbravldb:latest --ulimit nofile=2048 .
docker run --rm -it --network host --mount type=tmpfs,destination=/tmpspace umbravldb:latest
