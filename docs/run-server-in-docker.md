# Running space-expansion-server using Docker

## Ubuntu
Please, follow the [official guide](https://docs.docker.com/engine/install/ubuntu/) to install Docker engine in your system, or just run:

A docker image with `space-expansion-server` is [published in dockerhub](https://hub.docker.com/repository/docker/ziminas1990/space-expansion-server).  
To run `space-expansion-server` container just run:
```
docker run -ti --rm -p 6842:6842/udp -p25000-25200:25000-25200/udp ziminas1990/space-expansion-server:latest
```

Once you are in the container, you may execute the following commands:
1. `server_run` - start a server;
2. `config_edit` - open server's configuration file in `mcedit` tool;
3. `config_restore` - reset config to defaults; may be useful in case config was corrupted accidentaly.


## Windows
Currently, `space-expansion-server` docker image is not avaliable for windows. But you may download a minimalistic virtual machine with ubuntu server that already has everything you need to start `space-expansion-server`.

First, create a virtual machine with ubuntu server:
1. Install [Oracle VirtualBox](https://www.virtualbox.org/).
2. Download the [space-expansion-server.ova](https://disk.yandex.ru/d/d3shKv7U33wIZw).
3. Run the VirtualBox and follow the ["How to Import and Export OVA Files in VirtualBox"](https://www.maketecheasier.com/import-export-ova-files-in-virtualbox/) guide to import `space-expansion-server.ova`.

Now to run a `space-expansion-server`:
1. Run the machine and login useing `space` as login and `expansion` as password.
2. Run `docker_start` to start the docker immediatelly; you may also run `docker_update` to update docker container with a latest version.
3. In docker run `server_start` to start the server; you may also run `config_edit` to edit server's configuration, or `config_restore` to reset configuration to defaults.
