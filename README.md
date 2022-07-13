# VLDB 2022 Artifacts

This repository contains the required scripts to recreate all benchmarks for our paper "On-Demand State Separation for Cloud Data Warehousing". We describe the necessary server cluster and give step-by-step instructions on how to prepare and perform all benchmarks below.

Copyright (c) 2022 TUM. All rights reserved.
## Setup
Shared setup for all experiments conducted for the paper.

### Cluster requirements
The experiments require at least four servers connected through high-speed Ethernet (We recommend at least 10GBit/s) or Infiniband using Infiniband-over-IP.
All experiments are conducted with data residing entirely in memory. Therefore, we further require 256 GB of main memory at the two database servers and at least 128 GB for the servers hosting the network cache.
For data generation, 256 GB of storage is required at the servers hosting Umbra.

### Build requirements
We provide docker images for Umbra and a pre-configured image for the Apache Crail-based network cache. The configuration used for Crail can be found in the `thirdparty` directory. The `docker`
subdirectory contains the build scripts used to generate the Docker images. To run the benchmarks, the following dependencies are required on all servers:

- Docker
- bash

### Variables
The following IP addresses and network device names of all servers involved are required parameters for our scripts and should be substituted in the steps below by their value in your system:

- `IPUD`: The IPv4 adress of the umbra server destination. (e.g., `10.0.0.1`)
- `IPCN`: The IPv4 adress of the crail namenode. (e.g., `10.0.0.1`)
- `DNCN`: The network device name of the crail namenode. (e.g., `eth0`)
- `DNCD`: The network device name of the crail datanode. (e.g., `eth0`)

### Preparing the cache
To run the benchmarks, first the cache has to be started. To start the cache it is advised by Crail to create a `hugetlb` mount. If no configuration change on the server is possible, we provide a fallback option using a `tmpfs` that is created by docker.
However, we strongly recommend a `hugetlb` mount.
All experiments in the paper were performed with such a mount configured and we cannot guarantee performance or stability for a deployment with tmpfs.

A guide on how to create such a mount (for Debian) can be found here: [https://wiki.debian.org/Hugepages](https://wiki.debian.org/Hugepages).
The path to the `hugetlb` file system with at least 16GB of space is specified as a fourth parameter (e.g., `/hugepages`). To trigger the fallback enter only the first three parameters.

In a first step, start the crail namenode using the script in the `docker/crail` directory:
```
cd docker/crail
./startCrail.sh namenode $IPCN $DNCN /hugepages
```
Then start the crail datanode on the second server:
```
cd docker/crail
./startCrail.sh datanode $IPCN $DNCD /hugepages
```

### Preparing Umbra
Once both crail namenode and datanode are online, Umbra can be started. 
First start Umbra on both the source and destination servers.
The overall setup and data generation steps are identical and have to be executed on both servers:

Start the Umbra docker container using the script in the `docker/umbra` directory:
```
cd docker/umbra
./startUmbraDocker.sh
```

Next, now in the docker container prepare the data. For this, the docker container provides a script that takes one parameter, the scale factor.
All experiments in the paper are run using scale factor `100`.
```
./scripts/prepareData.sh 100
```
For scalefactor 100, this step will take several minutes.

## Single Query Experiments
This section describes the setup for all single-query experiments. Previous preparation can be shared between both single and multi-query experiments.

### Starting the Umbra destination server
Before starting the benchmark, the migration target has to be running. In the prepared Umbra docker container for the destination server, launch the server using the provided script.
The first parameter specifies the degree of parallelism (4 or 8 in our experiments), the second specifies the scale factor for which data was generated (100 in all experiments).
```
./scripts/startServer.sh 4 100 $IPUD $IPCN
```

### Starting the benchmark
Finally, the benchmark can be launched on the Umbra source server using the provided command:
```
./scripts/startBenchmark.sh $IPCN $IPUD 4 100 $NAME
```
`4` and `100` again specify the degree of parallelism Umbra is allowed to use and the scalefactor of the data.
`$NAME` is the name of the CSV file containing the result and can be chosen by you. We recommend unique names per experiment, e.g., `data48.csv` for the experiment with 4 worker threads on the Umbra source and 8 worker threads on the Umbra destination server.

Each run of the benchmark will take several hours at scalefactor 100.

### Parameters for all benchmarks in the paper
To recreate all experiments in the paper, three runs are required with different degrees of parallelism on Umbra source and destination servers:

- First experiment: 4 Threads on both source and destination
- Second experiment: 8 Threads on both source and destination
- Third experiment: 4 Threads on the source and 8 threads on the destination

After each run has finished, the benchmark script terminates automatically and the results can be inspected or extracted from the file you have specified.
To change the degree of parallelism the Umbra destination server has to be restarted by sending an interrupt `Ctrl + C` and repeating the `Starting the Umbra destination server` step with the desired degree of parallelism.

### Structure of the CSV results
Each line of the log contain 5 values.

| Query | Pipeline | Entry Type | Migrated Table ID | Metric |
|-------|----------|------------|-------------------|--------|

We will describe all columns in the following.

#### Query
Denotes the TPC-DS query and variant this line belongs to.

#### Pipeline
The pipeline id after which the migration was triggered.

#### Entry Type
The category that was measured. This includes all runtime and size overheads. The following table describes all possible entries including the unit measured and if the entry is relevant for an individual table or the query as a whole.

| Entry Type | Unit         | Relevant per Table | Description                                                                                                              |
|------------|--------------|--------------------|--------------------------------------------------------------------------------------------------------------------------|
| TRD        | Microseconds | ✓                  | Transfer Run Duration: Time to scan an operator and migrate it via the network, excluding compilation                    |
| FMD        | Microseconds | ✖                  | Full Migration Duration: Time from start of migration until results are reported at the target server                    |
| PMD        | Microseconds | ✖                  | Prepare Migration Duration: Includes calculating operators to migrate, compiling and enqueuing migration tasks to run    |
| MFS        | Bytes        | ✓                  | Migration File Size: The size of the data sent via the network for this migrated table                                   |
| MDS        | Bytes        | ✓                  | Migration Data Size: The size of tuples to be sent via the network for the migrated table                                |
| MOS        | Bytes        | ✓                  | Migration Offset Size: The size of the offsets to be sent via the network for the migrated table                         |
| TMD        | Microseconds | ✓                  | Table Migration Duration: The time to send the table's data to crail                                                     |
| TPD        | Microseconds | ✓                  | Table migration Preparation Duration: Time to prepare network migration (calculating offsets, creating crail file, etc.) |
| TFD        | Microseconds | ✓                  | Table Fetch Duration: The time to fetch the table's data from crail                                                      |
| QDM        | Microseconds | ✖                  | Query Duration Migration: The time to execute the query in full for a migrated query                                     |
| QDN        | Microseconds | ✖                  | Query Duration Non-Migration: The time to execute the query in full for a non-migrated query for reference               |
| RPD        | Microseconds | ✖                  | Remote Parse Duration: The time to parse the sent JSON plan at the target server                                         |
| RCD        | Microseconds | ✖                  | Remote Compilation Duration: the time to compile the sent plan at the target server                                      |
| RED        | Microseconds | ✖                  | Remote Execution Duration: The time from finishing compilation to fully printing all results at the target server        |
| RRD        | Microseconds | ✖                  | Remote Run Duration: The full time spent on the target server to process the query                                       |
| BMD        | Microseconds | ✖                  | Before Migration Duration: the time spent on the query before the migration is initiated                                 |
#### Migrated Table ID
The ID of the migrated table, only present if the entry is relevant for individual tables.

#### Metric
The value measured, as specified in the entry table.

## Mulit-Query Experiment
Prepare the cluster and database as described in the Setup section.

### Starting the Umbra destination server
Before starting the benchmark, the migration target has to be running. In the prepared Umbra docker container for the destination server, launch the server using the provided script.
The first parameter specifies the degree of parallelism (4 or 8 in our experiments), the second specifies the scale factor for which data was generated (100 in all experiments).
```
./scripts/startServerMultimig.sh 4 100 $IPUD $IPCN
```

### Preparing the benchmark driver
On the source server, launch the docker container containing the multimigration benchmark driver (also contained in the tools folder of this repository).

```
cd docker/driver
./startUmbraDriver.sh
```

### Launching the Umbra source server
In the Umbra container on the source, launch the Umbra source server:

```
./scripts/startPGServer.sh 4 100 $IPUD $IPCN
```

### Starting the benchmark
In the benchmark driver container, launch the benchmark:
```
./runMultimig.sh 4 5
```
The first parameter specifies the maximum number of parallel queries, the third parameter specifies the number of repetitions to run.

### Structure of the CSV results
The overall structure is similar to those of single-query experiments. However, we are interested in entries with other entry-types. Entries with entry-types from single-query experiments not listed below are not necessary valid in this experiment.

Each line of the log contain 5 values.

| Run | Parallel | Entry Type | Migrated Table ID | Metric |
|-------|----------|------------|-------------------|--------|

We will describe all columns in the following.


#### Run
Enumerates all runs performed by the benchmark driver.

#### Parallel
Number of parallel queries in this run.

#### Migrated table id
Not relevant for this experiment as no per-table entries are guaranteed to be valid.

#### Entry Type
The category that was measured. This includes all runtime and size overheads. The following table describes all possible entries including the unit measured and if the entry is relevant for an individual table or the query as a whole.

| Entry Type | Unit         | Relevant per Table | Description                                                                                                              |
|------------|--------------|--------------------|--------------------------------------------------------------------------------------------------------------------------|
| TRT        | Timestamp    | ✖                  | Transfer Run Timestamp: Timestamp of the moment that transfer has finished for the current query. Will occur once per parallel query                    |
| TTT        | Timestamp    | ✖                  | Transfer Trigger Timestamp: Timestamp of the moment a migration request was sent to the database                    |

#### Metric
The value measured, as specified in the entry table.
