#!/usr/bin/env python

from mininet.topo import Topo
from mininet.net import Mininet
from mininet.link import TCLink
from mininet.cli import CLI
from mininet.log import setLogLevel, info
import time, os
from mininet.node import OVSController, OVSSwitch

#############################################
# Topology Definition (Unchanged)
#############################################
class CustomRoutingTopoNAT(Topo):
    def build(self):
        # Create four switches
        s1 = self.addSwitch('s1')
        s2 = self.addSwitch('s2')
        s3 = self.addSwitch('s3')
        s4 = self.addSwitch('s4')

        # Internal hosts in the private subnet (10.1.1.0/24)
        h1 = self.addHost('h1', ip='10.1.1.2/24')
        h2 = self.addHost('h2', ip='10.1.1.3/24')
        # External hosts in the public subnet (10.0.0.0/24)
        h3 = self.addHost('h3', ip='10.0.0.4/24')
        h4 = self.addHost('h4', ip='10.0.0.5/24')
        h5 = self.addHost('h5', ip='10.0.0.6/24')
        h6 = self.addHost('h6', ip='10.0.0.7/24')
        h7 = self.addHost('h7', ip='10.0.0.8/24')
        h8 = self.addHost('h8', ip='10.0.0.9/24')
        # NAT host H9 with its public IP from the 172.16.10.0 subnet
        h9 = self.addHost('h9', ip='172.16.10.10/24')

        # Connect external hosts as before
        self.addLink(h3, s2, delay='5ms')
        self.addLink(h4, s2, delay='5ms')
        self.addLink(h5, s3, delay='5ms')
        self.addLink(h6, s3, delay='5ms')
        self.addLink(h7, s4, delay='5ms')
        self.addLink(h8, s4, delay='5ms')

        # Internal hosts (h1 and h2) now connect to the NAT host h9 instead of directly to s1.
        self.addLink(h9, s1, delay='5ms')
        self.addLink(h1, h9, delay='5ms')
        self.addLink(h2, h9, delay='5ms')

        # Inter-switch links remain unchanged.
        self.addLink(s1, s2, delay='7ms')
        self.addLink(s2, s3, delay='7ms')
        self.addLink(s3, s4, delay='7ms')
        self.addLink(s4, s1, delay='7ms')
        self.addLink(s1, s3, delay='7ms')

#####################################################
# NAT Setup on H9 (Revised and Well Commented)
#####################################################
def setup_nat_on_h9(net):
    """
    Set up NAT functionality on host H9.
    H9 bridges internal (private) and external (public) networks,
    acting as a NAT gateway for internal hosts h1 and h2.
    """
    # Retrieve our internal hosts and NAT host from the network.
    int_h1 = net.get('h1')
    int_h2 = net.get('h2')
    nat_node = net.get('h9')

    # For external hosts (h3 to h8), define the default gateway.
    # Here we use 10.0.0.1 as the gateway for simplicity.
    for extHost in ['h3', 'h4', 'h5', 'h6', 'h7', 'h8']:
        host = net.get(extHost)
        host.cmd("ip route add default via 10.0.0.1")

    # Assign a gateway IP on NAT host's external interface (h9-eth0).
    # This IP is used by external hosts to send packets toward the NAT.
    nat_node.cmd("ip addr add 10.0.0.1/24 dev h9-eth0")

    # Create an internal bridge within H9 to link traffic coming from h1 and h2.
    nat_node.cmd("ip link add name br-int type bridge")  # Create a bridge called 'br-int'
    nat_node.cmd("ip link set dev br-int up")             # Bring the bridge up

    # Connect the NAT host interfaces for h1 and h2 to the bridge.
    # These are assumed to be h9-eth1 and h9-eth2.
    nat_node.cmd("ip link set dev h9-eth1 master br-int")
    nat_node.cmd("ip link set dev h9-eth2 master br-int")

    # Assign an IP to the internal bridge to serve as the NAT gateway IP for h1 and h2.
    nat_node.cmd("ip addr add 10.1.1.1/24 dev br-int")
    # Optionally, add a secondary IP to the external interface if needed.
    nat_node.cmd("ip addr add 10.0.0.10/24 dev h9-eth0")

    # Update the default routes for the internal hosts to point to the NAT gateway.
    int_h1.cmd("ip route add default via 10.1.1.1")
    int_h2.cmd("ip route add default via 10.1.1.1")

    # Enable IP forwarding on the NAT host so it can pass traffic between networks.
    nat_node.cmd("sysctl -w net.ipv4.ip_forward=1")

    # Set up NAT using iptables so that packets leaving the internal network appear
    # to come from the NAT host's external IP. This uses masquerading.
    nat_node.cmd("iptables -t nat -F")  # Flush existing NAT rules.
    nat_node.cmd("iptables -t nat -A POSTROUTING -s 10.1.1.0/24 -o h9-eth0 -j MASQUERADE")
    # Allow established connections from external responses to reach the internal hosts.
    nat_node.cmd("iptables -A FORWARD -i h9-eth0 -o br-int -m state --state RELATED,ESTABLISHED -j ACCEPT")
    # Allow traffic from internal hosts to forward to the external network.
    nat_node.cmd("iptables -A FORWARD -i br-int -o h9-eth0 -j ACCEPT")

    # Updated console output message for clarity.
    info("\n+++ NAT configuration on host h9 complete. Internal hosts now use 10.1.1.1 as gateway. +++\n")

#############################################################
# Running the Network with Modified Console Outputs & Tests
#############################################################
def run_network():
    # Uncomment the following line if a previous Mininet instance might be running.
    # os.system('mn -c')  

    # Instantiate the network using the previously defined topology.
    topo = CustomRoutingTopoNAT()
    net = Mininet(topo=topo, controller=OVSController, link=TCLink, switch=OVSSwitch)

    # Call our NAT configuration function.
    setup_nat_on_h9(net)

    net.start()

    # Starting STP on all switches with updated output messaging.
    info("\n+++ Activating Spanning Tree Protocol (STP) on all switches +++\n")
    for swName in ['s1', 's2', 's3', 's4']:
        sw = net.get(swName)
        sw.cmd("ovs-vsctl set Bridge {} stp_enable=true".format(swName))

    info("\n+++ Network stabilization in progress... waiting 30 seconds +++\n")
    time.sleep(30)

    # Running connectivity tests with updated messages.
    info("\n=== Test 1: Internal host h1 pinging external host h5 ===\n")
    print(net.get('h1').cmd("ping -w 30 %s" % net.get('h5').IP()))
    info("\n=== Test 2: Internal host h2 pinging external host h3 ===\n")
    print(net.get('h2').cmd("ping -w 30 %s" % net.get('h3').IP()))
    info("\n=== Test 3: External host h8 pinging internal host h1 ===\n")
    print(net.get('h8').cmd("ping -w 30 %s" % net.get('h1').IP()))
    info("\n=== Test 4: External host h6 pinging internal host h2 ===\n")
    print(net.get('h6').cmd("ping -w 30 %s" % net.get('h2').IP()))

    # Running a set of iPerf3 tests with updated prompts.
    for run_count in range(1, 3):
        info("\n+++ iPerf3 Test Set %d: h1 (server) and h6 (client), 120 seconds +++\n" % run_count)
        net.get('h1').cmd("pkill -f iperf3")
        net.get('h6').cmd("pkill -f iperf3")
        net.get('h1').cmd("iperf3 -s -p 5001 &")
        time.sleep(5)
        print(net.get('h6').cmd("iperf3 -c 10.1.1.2 -t 120 -p 5001"))

        info("\n+++ iPerf3 Test Set %d: h8 (server) and h2 (client), 120 seconds +++\n" % run_count)
        net.get('h8').cmd("pkill -f iperf3")
        net.get('h2').cmd("pkill -f iperf3")
        net.get('h8').cmd("iperf3 -s -p 5002 &")
        time.sleep(5)
        print(net.get('h2').cmd("iperf3 -c 10.0.0.9 -t 120 -p 5002"))
    
    CLI(net)
    net.stop()

#############################################################
# Main: Set Log Level and Run Network
#############################################################
if __name__ == '__main__':
    setLogLevel('info')
    run_network()
