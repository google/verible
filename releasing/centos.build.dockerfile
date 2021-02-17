ARG _VERSION_

FROM verible:centos-$_VERSION_-basic AS base

# Install bazel
RUN yum install -y --nogpgcheck \
    java-1.8.0-openjdk-devel \
    unzip \
    zip

RUN java -version
RUN javac -version

#--

FROM base AS six

ARG _BAZEL_VERSION_

RUN yum install -y --nogpgcheck rh-python36
SHELL [ "scl", "enable", "rh-python36", "devtoolset-8" ]

RUN python --version
RUN python3 --version

# Build bazel
RUN wget --no-verbose "https://github.com/bazelbuild/bazel/releases/download/$_BAZEL_VERSION_/bazel-$_BAZEL_VERSION_-dist.zip"
RUN unzip bazel-$_BAZEL_VERSION_-dist.zip -d bazel-$_BAZEL_VERSION_-dist
RUN cd bazel-$_BAZEL_VERSION_-dist; \
    EXTRA_BAZEL_ARGS="--copt=-D__STDC_FORMAT_MACROS --host_copt=-D__STDC_FORMAT_MACROS" ./compile.sh
RUN cp bazel-$_BAZEL_VERSION_-dist/output/bazel /usr/local/bin
RUN bazel --version

SHELL [ "scl", "enable", "devtoolset-8" ]

RUN bazel --version

#--

FROM base

ARG _VERSION_

ADD "https://copr.fedorainfracloud.org/coprs/vbatts/bazel/repo/epel-$_VERSION_/vbatts-bazel-epel-$_VERSION_.repo" /etc/yum.repos.d
RUN yum install -y --nogpgcheck bazel3

RUN bazel --version
