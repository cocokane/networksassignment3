#!/usr/bin/env python

from mininet.topo import Topo
from mininet.net import Mininet
from mininet.link import TCLink
from mininet.cli import CLI
from mininet.log import setLogLevel, info
import time, os
from mininet.node import OVSController, OVSSwitch

#############################################
# Topology Definition 
#############################################
class CustomRoutingTopoNAT(Topo):
    def build(self):
        s1 = self.addSwitch('s1')
        s2 = self.addSwitch('s2')
        s3 = self.addSwitch('s3')
        s4 = self.addSwitch('s4')

        # Internal hosts
        h1 = self.addHost('h1', ip='10.1.1.2/24')
        h2 = self.addHost('h2', ip='10.1.1.3/24')
        # External hosts
        h3 = self.addHost('h3', ip='10.0.0.4/24')
        h4 = self.addHost('h4', ip='10.0.0.5/24')
        h5 = self.addHost('h5', ip='10.0.0.6/24')
        h6 = self.addHost('h6', ip='10.0.0.7/24')
        h7 = self.addHost('h7', ip='10.0.0.8/24')
        h8 = self.addHost('h8', ip='10.0.0.9/24')
        # NAT host
        h9 = self.addHost('h9', ip='172.16.10.10/24')

        # External hosts    
        for host, switch in list(zip([h3, h4], [s2]*2)) + list(zip([h5, h6], [s3]*2)) + list(zip([h7, h8], [s4]*2)):
            self.addLink(host, switch, delay='5ms')

        # Internal hosts connect via NAT
        self.addLink(h9, s1, delay='5ms')
        self.addLink(h1, h9, delay='5ms')
        self.addLink(h2, h9, delay='5ms')

        # Inter-switch
        self.addLink(s1, s2, delay='7ms')
        self.addLink(s2, s3, delay='7ms')
        self.addLink(s3, s4, delay='7ms')
        self.addLink(s4, s1, delay='7ms')
        self.addLink(s1, s3, delay='7ms')

#####################################################
# NAT Setup on H9 
#####################################################
def setup_nat_on_h9(net):
    h1 = net.get('h1')
    h2 = net.get('h2')
    h9 = net.get('h9')

    # Configure external hosts
    for extHost in ['h3', 'h4', 'h5', 'h6', 'h7', 'h8']:
        net.get(extHost).cmd("ip route add default via 10.0.0.1")

    # Setup NAT public interface
    h9.cmd("ip addr add 10.0.0.1/24 dev h9-eth0")
    h9.cmd("ip addr add 10.0.0.10/24 dev h9-eth0")

    # Internal bridge
    h9.cmd("ip link add name br-int type bridge")
    h9.cmd("ip link set br-int up")
    h9.cmd("ip link set h9-eth1 master br-int")
    h9.cmd("ip link set h9-eth2 master br-int")
    h9.cmd("ip addr add 10.1.1.1/24 dev br-int")

    # Set routes for internal hosts
    h1.cmd("ip route add default via 10.1.1.1")
    h2.cmd("ip route add default via 10.1.1.1")

    # Enable IP forwarding
    h9.cmd("sysctl -w net.ipv4.ip_forward=1")

    # NAT rules
    h9.cmd("iptables -t nat -F")
    h9.cmd("iptables -t nat -A POSTROUTING -s 10.1.1.0/24 -o h9-eth0 -j MASQUERADE")
    h9.cmd("iptables -A FORWARD -i h9-eth0 -o br-int -m state --state RELATED,ESTABLISHED -j ACCEPT")
    h9.cmd("iptables -A FORWARD -i br-int -o h9-eth0 -j ACCEPT")

    # DNAT Rules for External → Internal traffic
    # Forward 5001 (from external to h1)
    h9.cmd("iptables -t nat -A PREROUTING -i h9-eth0 -p tcp --dport 5001 -j DNAT --to-destination 10.1.1.2:5001")
    h9.cmd("iptables -A FORWARD -p tcp -d 10.1.1.2 --dport 5001 -j ACCEPT")

    # Forward 5002 (from external to h2)
    h9.cmd("iptables -t nat -A PREROUTING -i h9-eth0 -p tcp --dport 5002 -j DNAT --to-destination 10.1.1.3:5002")
    h9.cmd("iptables -A FORWARD -p tcp -d 10.1.1.3 --dport 5002 -j ACCEPT")

    info("\n+++ NAT and DNAT configuration on h9 complete. +++\n")

#############################################################
# Running the Network
#############################################################
def run_network():
    # os.system('mn -c')
    topo = CustomRoutingTopoNAT()
    net = Mininet(topo=topo, controller=OVSController, link=TCLink, switch=OVSSwitch)
    net.start()

    setup_nat_on_h9(net)

    info("\n+++ Enabling STP +++\n")
    for swName in ['s1', 's2', 's3', 's4']:
        net.get(swName).cmd(f"ovs-vsctl set Bridge {swName} stp_enable=true")

    info("\n+++ Waiting 30s for convergence +++\n")
    time.sleep(30)

    # Ping Tests
    info("\n=== Test 1: h1 → h5 ===\n")
    print(net.get('h1').cmd("ping -w 10 10.0.0.6"))
    info("\n=== Test 2: h2 → h3 ===\n")
    print(net.get('h2').cmd("ping -w 10 10.0.0.4"))
    info("\n=== Test 3: h8 → h1 (via DNAT) ===\n")
    print(net.get('h8').cmd("ping -w 10 10.0.0.10"))
    info("\n=== Test 4: h6 → h2 (via DNAT) ===\n")
    print(net.get('h6').cmd("ping -w 10 10.0.0.10"))

    # iPerf3 Tests
    for i in range(3):
        info(f"\n+++ iPerf Test {i+1}: h1 (server) ⇐⇒ h6 (client) +++\n")
        net.get('h1').cmd("pkill -f iperf3")
        net.get('h6').cmd("pkill -f iperf3")
        net.get('h1').cmd("iperf3 -s -p 5001 &")
        time.sleep(2)
        print(net.get('h6').cmd("iperf3 -c 10.0.0.10 -t 10 -p 5001"))

        info(f"\n+++ iPerf Test {i+1}: h8 (server) ⇐⇒ h2 (client) +++\n")
        net.get('h8').cmd("pkill -f iperf3")
        net.get('h2').cmd("pkill -f iperf3")
        net.get('h8').cmd("iperf3 -s -p 5002 &")
        time.sleep(2)
        print(net.get('h2').cmd("iperf3 -c 10.0.0.9 -t 10 -p 5002"))

    CLI(net)
    net.stop()

if __name__ == '__main__':
    setLogLevel('info')
    run_network()
