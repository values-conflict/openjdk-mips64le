FROM eclipse-temurin:17-jdk AS jdk17
FROM eclipse-temurin:25-jdk AS jdk25

FROM debian:bookworm-slim

RUN set -eux; \
	apt-get update; \
	apt-get install -y --no-install-recommends \
		autoconf \
		file \
		g++ \
		g++-mips64el-linux-gnuabi64 \
		gcc \
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
		time \
		unzip \
		zip \
	; \
	rm -rf /var/lib/apt/lists/*

RUN set -eux; \
	\
# uname shim: strip -microsoft- from uname -r so OpenJDK configure detects linux not wsl
	printf '%s\n' \
		'#!/bin/sh' \
		'set -eu' \
		'case "${1-}" in' \
		'    -r) /bin/uname "$@" | sed "s/-microsoft-/-/g" ;;' \
		'    *)  exec /bin/uname "$@" ;;' \
		'esac' \
		| tee /usr/local/bin/uname \
	; \
	chmod +x /usr/local/bin/uname; \
	uname; \
	uname -r; \
	\
# alsa stub: mips64el shared lib exporting all symbols from x86-64 libasound
# libjsound.so links against -lasound at build time; real libasound.so.2 loads at runtime
	nm --dynamic /usr/lib/x86_64-linux-gnu/libasound.so.2 \
		| awk '$2 == "T" { sub(/@.*/, "", $3); print "void __attribute__((visibility(\"default\"))) " $3 "(void){}" }' \
		| sort --unique \
		> /tmp/alsa-stub.c \
	; \
	symbolsStubExpect="$(wc --lines < /tmp/alsa-stub.c)"; \
	echo "alsa stub: $symbolsStubExpect symbols"; \
	mkdir --parents /opt/alsa-stub; \
	mips64el-linux-gnuabi64-gcc -shared -fPIC \
		-o /opt/alsa-stub/libasound.so \
		/tmp/alsa-stub.c \
	; \
	symbolsStubActual="$(nm --dynamic --defined-only /opt/alsa-stub/libasound.so | wc --lines)"; \
	echo "alsa stub built: $symbolsStubActual symbols"; \
	[ "$symbolsStubActual" = "$symbolsStubExpect" ]; \
	rm /tmp/alsa-stub.c

COPY --from=jdk17 /opt/java/openjdk /opt/java/jdk17
COPY --from=jdk25 /opt/java/openjdk /opt/java/jdk25
RUN /opt/java/jdk17/bin/java --version && /opt/java/jdk25/bin/java --version
