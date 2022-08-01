FROM ubuntu:22.04 as builder
RUN SPEX_SOURCE_DIR=$HOME/dev/space-expansion && \
    SPEX_BUILD_DIR=$HOME/dev/space-expansion-build && \
    apt   update && \
    apt   install -y cmake git python3 python3-pip && \
    pip3  install conan && \
    conan profile new default --detect && \
    conan profile update settings.compiler.libcxx=libstdc++11 default && \
    mkdir -p $HOME/dev && \
    git   clone https://github.com/ziminas1990/space-expansion.git $SPEX_SOURCE_DIR && \
    mkdir $SPEX_BUILD_DIR && cd $SPEX_BUILD_DIR && \
    conan install $SPEX_SOURCE_DIR/server/conanfile.txt --build=missing -s build_type=Release && \
    cmake $SPEX_SOURCE_DIR/server && cmake --build . -- -j4 && \
    cp    $SPEX_BUILD_DIR/bin/space-expansion-server /root && \
    cp    $SPEX_SOURCE_DIR/server/space-expansion.cfg /root
    
FROM ubuntu:22.04
COPY --from=builder /root/space-expansion-server /usr/bin/
COPY --from=builder /root/space-expansion.cfg .
RUN apt  update && apt install -y mcedit && \
    cp   /space-expansion.cfg /space-expansion.cfg.backup && \
    echo "alias server_run='/usr/bin/space-expansion-server /space-expansion.cfg'" >> ~/.bashrc && \
    echo "alias config_edit='mcedit /space-expansion.cfg'" >> ~/.bashrc && \
    echo "alias config_restore='rm /space-expansion.cfg && cp /space-expansion.cfg.backup /space-expansion.cfg'" >> ~/.bashrc && \
    touch /etc/motd && \
    echo "############################################################" >> /etc/motd && \
    echo "#                                                          #" >> /etc/motd && \
    echo "#           Welcome to the space-expansion server!         #" >> /etc/motd && \
    echo "# Run the following commands:                              #" >> /etc/motd && \
    echo "# 'server_run'     - to start a server                     #" >> /etc/motd && \
    echo "# 'config_edit'    - to edit server configuration          #" >> /etc/motd && \
    echo "#                    (press F2 to save and F10 to exit)    #" >> /etc/motd && \
    echo "# 'config_restore' - to restore default configuration      #" >> /etc/motd && \
    echo "#                    in case it was accidentaly corrupted  #" >> /etc/motd && \
    echo "############################################################" >> /etc/motd && \
    echo "cat /etc/motd" >> /etc/bash.bashrc
