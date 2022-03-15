#!/bin/bash
echo "Test connection"
if ping -c 3 8.8.8.8
then
    if [ $1 == install ]; then
        sudo apt update
        sudo apt install git -y
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

        sudo systemctl enable docker.service
        sudo systemctl enable containerd.service
        
        sudo usermod -aG docker $USER

        DOCKER_CONFIG=${DOCKER_CONFIG:-$HOME/.docker}
        mkdir -p $DOCKER_CONFIG/cli-plugins
        curl -SL "https://github.com/docker/compose/releases/download/v2.2.3/docker-compose-linux-aarch64" -o $DOCKER_CONFIG/cli-plugins/docker-compose
        chmod +x $DOCKER_CONFIG/cli-plugins/docker-compose

        sudo systemctl reboot
    elif [ $1 == docker ]; then
        echo "DOCKER CHECK"
        docker version
        echo "DOCKER COMPOSE CHECK"
        docker compose version
        echo "SETUP REPOSITORY"
        git clone https://github.com/marekkles/TOI-project1.git
        cd TOI-project1
        docker compose build
        docker compose up -d
        #TODO AUTO SETUP
    elif [ $1 == hotspot ]; then
        echo "Setup hotspot"

        ADDR_THIS="10.10.0.1"
        ADDR_START="10.10.0.2"
        ADDR_END="10.10.0.20"
        SSID="rpiIotGateway"
        PASS="abcdefgh"

        sudo apt install hostapd
        sudo systemctl unmask hostapd
        sudo systemctl enable hostapd
        sudo apt install dnsmasq
        sudo DEBIAN_FRONTEND=noninteractive apt install -y netfilter-persistent iptables-persistent

        WLAN0_DHCPCD=$'
interface wlan0
    static ip_address='$ADDR_THIS$'/24
    nohook wpa_supplicant
'
        echo "$WLAN0_DHCPCD" | sudo tee -a /etc/dhcpcd.conf
        echo "net.ipv4.ip_forward=1" | sudo tee /etc/sysctl.d/routed-ap.conf
        sudo iptables -t nat -A POSTROUTING ! -o wlan0 -j MASQUERADE
        sudo iptables -A FORWARD -i wlan0 ! -o wlan0 -j ACCEPT
        sudo iptables -A FORWARD -i wlan0   -o wlan0 -j ACCEPT

        sudo netfilter-persistent save

        sudo mv /etc/dnsmasq.conf /etc/dnsmasq.conf.orig
        DNSMASQ=$'
interface=wlan0 # Listening interface
dhcp-range='$ADDR_START','$ADDR_END$',255.255.255.0,24h
                # Pool of IP addresses served via DHCP
domain=wlan     # Local wireless DNS domain
address=/gw.wlan/10.10.0.1
                # Alias for this router
'
        echo "$DNSMASQ" | sudo tee /etc/dnsmasq.conf
        sudo rfkill unblock wlan
        HOSTAPD=$'
country_code=CZ
interface=wlan0
ssid='$SSID$'
hw_mode=g
channel=7
macaddr_acl=0
auth_algs=1
ignore_broadcast_ssid=0
wpa=2
wpa_passphrase='$PASS$'
wpa_key_mgmt=WPA-PSK
wpa_pairwise=TKIP
rsn_pairwise=CCMP
'
        echo "$HOSTAPD" | sudo tee /etc/hostapd/hostapd.conf
        echo "Created hotspot: ssid=$SSID wpa_passphrase=$PASS gateway=$ADDR_THIS"
        sudo systemctl reboot
    else
        echo "Unknown option use:\n" >&2
        echo "./rpisetup.sh install|docker|hotspot" >&2
        echo "  install - install docker, docker compose and reboot" >&2
        echo "  docker - setup docker environment" >&2
        echo "  hotspot - setup hostspot" >&2
    fi
else
    echo "Connection is not established" >&2
fi

