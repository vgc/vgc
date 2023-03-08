This folder provides Dockerfiles for building Docker containers useful for VGC:

- `vgc-ci-ubuntu-18.04`: a Docker container to build VGC on Ubuntu 18.04.

Below are instructions how to build these containers and publish them to Docker Hub.

# Install and configure Docker

1. Install Docker on your local machine, see: https://docs.docker.com/get-docker/

2. (Optional but recommended) Configure the Docker's credentials store for
   more secure login to Docker Hub. See: https://docs.docker.com/engine/reference/commandline/login/#credentials-store

   For example, if your local machine is on Ubuntu, you can do something like this:

   ```
   # Install GPG and Pass
   sudo apt install gnupg pass

   # Generate RSA key. When prompted: RSA and RSA, 4096, Your Name <yourname@example.com>
   gpg --full-generate-key

   # Download the Docker Credential Helper to ~/bin and make it executable
   VERSION=v0.7.0
   NAME=docker-credential-pass-$VERSION.linux-amd64
   URL=https://github.com/docker/docker-credential-helpers/releases/download/$VERSION/$NAME
   mkdir -p ~/bin
   cd ~/bin
   wget $URL
   chmod u+x $NAME
   ```

   Then add `~/bin` to your PATH, for example by adding the following to your `~/.bashrc`:

   ```
   export PATH="$HOME/bin:$PATH"
   ```

   Then configure Docker to use the freshly installed credential helper,
   by adding the following to your `~/.docker/config.json`:

   ```
   {
       "credsStore": "pass-v0.7.0.linux-amd64"
   }
   ```

3. Login to Docker via `docker login -u <docker-user>` (e.g., `<docker-user> = vgcsoftware`)

# Build the Docker image

```
cd <folder-with-dockerfile>
docker build -t <image> .
```

Note: `<image>` can be any name of your choice, but typically you can choose
the same as `<folder-with-dockerfile>`.

Concrete example:

```
cd vgc-ci-ubuntu-18.04
docker build -t vgc-ci-ubuntu-18.04 .
```

# Test the Docker image

```
docker run -it <image> /bin/bash
```

This brings you to an interactive Bash session, when you can try commands
 running on the container. Type `exit` to end the session.

Concrete example:

```
docker run -it vgc-ci-ubuntu-18.04 /bin/bash
```

followed by the following in the interactive Bash session:

```
BUILD_TYPE=Release
QT_VERSION=5.12.5
PARALLEL_JOBS=5
mkdir -p /__w/vgc && cd /__w/vgc
git clone --recurse-submodules --depth 100 https://github.com/vgc/vgc.git
cd vgc && mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE="$BUILD_TYPE" -DQt5_DIR="/opt/qt/$QT_VERSION/gcc_64/lib/cmake/Qt5"
cmake --build . --parallel $PARALLEL_JOBS
cmake --build . --target check --parallel $PARALLEL_JOBS
cmake --build . --target deploy --parallel $PARALLEL_JOBS
```

# Push the Docker image to Docker Hub

```
docker tag <image> <docker-user>/<image>:<version>
docker push <docker-user>/<image>:<version>
```

You can check on Docker Hub that the image was correcly uploaded.

Concrete example:

```
docker tag vgc-ci-ubuntu-18.04 vgcsoftware/vgc-ci-ubuntu-18.04:2023-03-08.0
docker push vgcsoftware/vgc-ci-ubuntu-18.04:2023-03-08.0
```

# Use the Docker images with GitHub Actions

```
name: Ubuntu 18.04
on: [push, pull_request]
jobs:
  build:
    runs-on: ubuntu-latest
    container: <docker-user>/<image>:<version>
    steps:

    # Fix "fatal: detected dubious ownership in repository at '/__w/<user>/<repo>'""
    - name: Add GitHub workspace to Git safe directories
      run: git config --global --add safe.directory `pwd`

    - uses: actions/checkout@v2

    - name: Build
      working-directory: ${{github.workspace}}
      run: |
        mkdir build && cd build
        cmake --version
        cmake .. -DCMAKE_BUILD_TYPE=Release
        cmake --build .
```
