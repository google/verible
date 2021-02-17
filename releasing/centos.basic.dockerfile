ARG _VERSION_

FROM centos:$_VERSION_ AS base

# Install basic tools
RUN yum install -y \
    file           \
    git            \
    redhat-lsb     \
    tar            \
    wget

#--

FROM base AS eight

# Install C++ compiler
RUN yum -y group install --nogpgcheck "Development Tools" || \
    yum -y groupinstall --nogpgcheck "Development Tools"

RUN yum install -y bison flex

RUN gcc --version
RUN g++ --version

#--

FROM base

# Link libstdc++ statically so people don't have to install devtoolset-8
# just to use verible.
ENV BAZEL_LINKOPTS "-static-libstdc++:-lm -static-libstdc++:-lrt"
ENV BAZEL_LINKLIBS "-l%:libstdc++.a"

# Get a newer GCC version
RUN yum install -y --nogpgcheck centos-release-scl
RUN yum install -y --nogpgcheck devtoolset-8

RUN yum install -y bison flex

# Build a newer version of flex
RUN wget --no-verbose "https://github.com/westes/flex/files/981163/flex-2.6.4.tar.gz" && \
    tar -xvf flex-2.6.4.tar.gz && \
    cd flex-2.6.4 && \
    ./configure --prefix=/usr && \
    make -j && \
    make install && \
    cd .. && \
    rm -rf flex-*
RUN flex --version

SHELL [ "scl", "enable", "devtoolset-8" ]
