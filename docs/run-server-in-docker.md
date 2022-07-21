# Running space-expansion-server in docker
A docker image with `space-expansion-server` is [published in dockerhub](https://hub.docker.com/repository/docker/ziminas1990/space-expansion-server).  
To run `space-expansion-server` in docker just run:
```
docker -ri --rm ziminas1990/space-expansion-server:latest
```

Once you are in container, you have three commands:
1. `server_run` - start a server;
2. `config_edit` - open server's configuration file in `mcedit` tool;
3. `config_restore` - reset config to defaults; may be useful in case config was corrupted accidentaly.
