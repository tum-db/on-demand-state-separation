FROM ubuntu:21.10

# Install apt dependencies
RUN apt-get update -qq \
    && DEBIAN_FRONTEND=noninteractive apt-get install tzdata \
    && apt-get install -y \
        binutils \
        ccache \
        cmake \
        g++-11 \
        make \
        ninja-build \
        lsb-release \
        gnupg \
        wget \
    && wget --quiet -O - https://www.postgresql.org/media/keys/ACCC4CF8.asc | apt-key add - \
    && sh -c 'echo "deb http://apt.postgresql.org/pub/repos/apt $(lsb_release -cs)-pgdg main" > /etc/apt/sources.list.d/pgdg.list' \
    && apt-get clean \    
    && apt-get install -y \
    libpq-dev \
    && rm -rf /var/lib/apt/lists/* \
    && update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-11 11 \
    && update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-11 11

WORKDIR /usr/local/src

# Build the driver binary
COPY multimigration multimigration
RUN set -x \
    && cd multimigration \
    && mkdir build \
    && cd build \
    && cmake -DCMAKE_BUILD_TYPE=Release -DPostgreSQL_TYPE_INCLUDE_DIR=/usr/include/postgresql/ .. \
    && make -j \
    && cd ../..

# Build the production image
FROM ubuntu:21.10
RUN apt-get update -qq \
    && DEBIAN_FRONTEND=noninteractive apt-get install tzdata \
    && apt-get install -y \
        gnupg \
        wget \
    && wget --quiet -O - https://www.postgresql.org/media/keys/ACCC4CF8.asc | apt-key add - \
    && sh -c 'echo "deb http://apt.postgresql.org/pub/repos/apt $(lsb_release -cs)-pgdg main" > /etc/apt/sources.list.d/pgdg.list' \
    && apt-get clean \    
    && apt-get install -y \
    libpq5 \
    && rm -rf /var/lib/apt/lists/*

COPY --from=0 /usr/local/src/multimigration/build/multimigration /usr/local/bin/multimigration
COPY runMultimig.sh /var/lib/multimig/runMultimig.sh
COPY queries /var/lib/multimig/queries

WORKDIR /var/lib/multimig
