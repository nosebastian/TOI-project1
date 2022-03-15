#!/bin/sh

if [ $1 = "forward-on" ]; then
    iptables -t nat -A POSTROUTING ! -o $2 -s 10.55.0.0/29 -j MASQUERADE
    iptables -A FORWARD -i $2 ! -o $2 -j ACCEPT
    iptables -A FORWARD -i $2   -o $2 -j ACCEPT
elif [ $1 = "forward-off" ]; then 
    iptables -t nat -D POSTROUTING ! -o $2 -s 10.55.0.0/29 -j MASQUERADE
    iptables -D FORWARD -i $2 ! -o $2 -j ACCEPT
    iptables -D FORWARD -i $2   -o $2 -j ACCEPT
elif [ $1 = "ip-on" ]; then
    ip address add 10.55.0.1/29 brd + dev $2
elif [ $1 = "ip-off" ]; then
    ip address del 10.55.0.1/29 brd + dev $2
else
    echo "ERROR: Cannot execute command ${1}, please use:\n" >&2
    echo "ip.sh forward-on|forward-off|ip-on|ip-off DEVICE\n" >&2
    echo "    forward-on|forward-off|ip-on|ip-off" >&2
    echo "        forward-on  - set NAT iptable rules" >&2
    echo "        forward-off - delete NAT iptable rules" >&2
    echo "        ip-on       - set ip addres of interface ip-off" >&2
    echo "        ip-off      - delete ip addres of interface" >&2
    echo "    DEVICE - A selected network device" >&2
    exit 1
fi


