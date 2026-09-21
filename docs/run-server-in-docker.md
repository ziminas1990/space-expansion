# Running space-expansion-server using Docker

## Ubuntu

Please, follow the [official guide](https://docs.docker.com/engine/install/ubuntu/) to install Docker engine.

A docker image with `space-expansion-server` is [published in dockerhub](https://hub.docker.com/repository/docker/ziminas1990/space-expansion-server).

The container starts the server immediately. Configuration is not baked into the image: mount `space-expansion.cfg` at `/etc/space-expansion/space-expansion.cfg`.

A sample config is available in the image at `/usr/share/space-expansion/space-expansion.cfg`, or in this repository at `server/space-expansion.cfg`.

To copy the sample config out of the image:

```bash
docker run --rm --entrypoint cat ziminas1990/space-expansion-server:latest \
  /usr/share/space-expansion/space-expansion.cfg > space-expansion.cfg
```

To run the server:

```bash
docker run --rm \
  -p 6842:6842/udp \
  -p 17392:17392/udp \
  -p 25000-25200:25000-25200/udp \
  -v "$PWD/space-expansion.cfg:/etc/space-expansion/space-expansion.cfg:ro" \
  ziminas1990/space-expansion-server:latest
```

To build the image locally from this repository:

```bash
docker build -t space-expansion-server -f server/Dockerfile server/
```


## Windows

Currently, `space-expansion-server` docker image is not avaliable for windows. But you may download a minimalistic virtual machine with ubuntu server that already has everything you need to start `space-expansion-server`.

First, create a virtual machine with ubuntu server:

1. Install [Oracle VirtualBox](https://www.virtualbox.org/).
2. Download the [space-expansion-server.ova](https://disk.yandex.ru/d/d3shKv7U33wIZw).
3. Run the VirtualBox and follow the ["How to Import and Export OVA Files in VirtualBox"](https://www.maketecheasier.com/import-export-ova-files-in-virtualbox/) guide to import `space-expansion-server.ova`.

Now to run a `space-expansion-server`:

1. Run the machine and login useing `space` as login and `expansion` as password.
2. Run `docker_start` to start the docker immediatelly; you may also run `docker_update` to update docker container with a latest version.
3. Mount a configuration file at `/etc/space-expansion/space-expansion.cfg`. The container starts the server immediately.
