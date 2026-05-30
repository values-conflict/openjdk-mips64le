FROM debian:trixie-slim

RUN set -eux; \
	apt-get install -y --update --no-install-recommends \
		autoconf \
		file \
		g++-mips64el-linux-gnuabi64 \
		gcc-mips64el-linux-gnuabi64 \
		make \
		openjdk-21-jdk-headless \
		unzip \
		zip \
	; \
	apt-get dist-clean
