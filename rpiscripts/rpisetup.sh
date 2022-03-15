#!/bin/bash
echo "Test connection"
if ping -c 3 8.8.8.8
then
    sudo apt update
    sudo apt upgrade -y
    sudo apt-get remove docker docker-engine docker.io containerd runc
    sudo apt-get install \
        ca-certificates \
        curl \
        gnupg \
        lsb-release -y
    curl -fsSL https://download.docker.com/linux/debian/gpg | sudo gpg --dearmor -o /usr/share/keyrings/docker-archive-keyring.gpg
    echo \
        "deb [arch=$(dpkg --print-architecture) signed-by=/usr/share/keyrings/docker-archive-keyring.gpg] https://download.docker.com/linux/debian \
        $(lsb_release -cs) stable" | sudo tee /etc/apt/sources.list.d/docker.list > /dev/null
    sudo apt-get update
    sudo apt-get install containerd.io docker-ce docker-ce-cli -y || echo "Failed but continue"

    sudo systemctl stop docker.service
    sudo systemctl stop containerd.service
    sudo systemctl start containerd.service
    sudo systemctl start docker.service
    sudo systemctl enable docker.service
    sudo systemctl enable containerd.service


    sudo groupadd docker
    sudo usermod -aG docker $USER
    newgrp docker
    sudo newgrp docker

    docker version
    
    DOCKER_CONFIG=${DOCKER_CONFIG:-$HOME/.docker}
    mkdir -p $DOCKER_CONFIG/cli-plugins
    curl -SL "https://github.com/docker/compose/releases/download/v2.2.3/docker-compose-linux-aarch64" -o $DOCKER_CONFIG/cli-plugins/docker-compose
    chmod +x $DOCKER_CONFIG/cli-plugins/docker-compose

    docker compose version

    sudo apt install git -y
    git clone https://github.com/marekkles/TOI-project1.git
    cd TOI-project1
    ls
    #TODO
else
    echo "Connection is not established" >&2
fi
