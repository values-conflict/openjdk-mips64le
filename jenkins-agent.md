# jenkins-agent testing

Context and procedure for testing the MIPS/QEMU JDK build against a real Jenkins remoting
channel.  This document is written for an LLM guiding Tianon -- the LLM has no access to
Docker or the Jenkins UI; Tianon does those steps and reports the results back.

## Background

The target workload for this port is the Jenkins remoting agent -- a pure-Java headless process
that connects outbound to a Jenkins controller and executes build steps over a bidirectional
channel.  Testing it exercises the full remoting stack: WebSocket handshake, Java object
serialization, and sustained GC pressure -- none of which the simpler phase tests cover.

Tianon runs a local Jenkins controller in Docker on his host (outside the build container).
The MIPS/QEMU agent runs inside the build container and connects to the controller via the
Docker bridge network.  The two ends run on different architectures, which is exactly the real
deployment scenario.

## Two distinct situations

### Jenkins is already running

Ask Tianon to provide three things:

1. **Container IP** -- run on the host (outside the build container):
   ```console
   $ docker container inspect jenkins --format '{{.NetworkSettings.IPAddress}}'
   ```
2. **Node secret** -- visible in the Jenkins UI on the node's "Connect agent" / JNLP page
   (`/computer/<node-name>/` → "Connect agent" button shows the full `-secret <value>`)
3. **Node name** -- whatever name was chosen when the node was created (e.g. `test`)

If `jenkins-agent.jar` is not already in the workspace root, ask Tianon to run this inside
the build container (substituting the IP from step 1):

```console
$ curl --output jenkins-agent.jar http://<jenkins-ip>:8080/jnlpJars/agent.jar
```

### Jenkins is not running (starting from scratch)

The container is `--rm` with a `tmpfs` home directory, so all Jenkins state -- including the
setup wizard completion, node configuration, and node secrets -- is lost on every restart.
Everything below needs to be repeated.

Ask Tianon to run this on his host:

```console
$ docker run -it --rm --name jenkins --user 1234:5678 --security-opt no-new-privileges --read-only --tmpfs /tmp --tmpfs /var/lib/jenkins:uid=1234,gid=5678 -p 8181:8080 tianon/jenkins
```

Then ask him to:
1. open `http://localhost:8181/` in a browser
2. complete the initial setup wizard
3. create a permanent agent node (any name; "test" is conventional here)
4. open the new node's page and find the secret (the "Connect agent" button shows the full
   connection command including `-secret <value>`)

Once that's done, collect the container IP, secret, and node name as described above under
"Jenkins is already running."

The agent connects to the container's bridge IP on port 8080 directly -- not to localhost:8181
(that's just for Tianon's browser).

## Running the tests

All commands run inside the build container.  Substitute the actual IP, secret, and node name.

**x86 baseline (rules out network/protocol issues before testing MIPS):**

```console
$ /opt/java/jdk25/bin/java -jar jenkins-agent.jar \
	-url http://<jenkins-ip>:8080/ \
	-secret <node-secret> \
	-name <node-name> \
	-webSocket
```

Expected output: `INFO: WebSocket connection open` followed by `INFO: Connected`.  JNA
"restricted method" and "stack guard" warnings are harmless.  Let it run for ~15 seconds then
Ctrl-C; clean exit is good.

**MIPS/QEMU:**

```console
$ QEMU_CPU=Loongson-3A1000 QEMU_LD_PREFIX=/usr/mips64el-linux-gnuabi64 \
	timeout --kill-after=5s 60 \
	tianon-jdk25u-mips64/build/linux-mips64el-server-release/images/jdk/bin/java \
	-jar jenkins-agent.jar \
	-url http://<jenkins-ip>:8080/ \
	-secret <node-secret> \
	-name <node-name> \
	-webSocket
```
