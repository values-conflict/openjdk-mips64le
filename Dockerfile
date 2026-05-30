FROM eclipse-temurin:17-jdk AS temurin

FROM debian:trixie-slim

RUN set -eux; \
	apt-get install -y --update --no-install-recommends \
		autoconf \
		file \
		g++-mips64el-linux-gnuabi64 \
		gcc-mips64el-linux-gnuabi64 \
		libasound2-dev \
		libcups2-dev \
		libfontconfig1-dev \
		libx11-dev \
		libxext-dev \
		libxi-dev \
		libxrandr-dev \
		libxrender-dev \
		libxtst-dev \
		make \
		unzip \
		zip \
		\
		time \
	; \
	apt-get dist-clean

ENV JAVA_HOME=/opt/java/openjdk
COPY --from=temurin $JAVA_HOME $JAVA_HOME
ENV PATH $JAVA_HOME/bin:$PATH
