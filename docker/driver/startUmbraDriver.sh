#!/bin/bash
#---------------------------------------------------------------------------
# Copyright (c) 2022 TUM. All rights reserved.
#---------------------------------------------------------------------------

# Build docker image
docker build --tag=umbravldbdriver:latest --ulimit nofile=2048 .
docker run --rm -it --network host umbravldbdriver:latest
