#!/usr/bin/env python

from mininet.topo import Topo
from mininet.net import Mininet
from mininet.link import TCLink
from mininet.cli import CLI
from mininet.log import setLogLevel
import time
import os
from mininet.node import OVSController, OVSSwitch

class CustomRoutingTopo(Topo):
    def build(self):
        s1 = self.addSwitch('s1')
        s2 = self.addSwitch('s2')
        s3 = self.addSwitch('s3')
        s4 = self.addSwitch('s4')

        h1 = self.addHost('h1', ip='10.0.0.2/24')
        h2 = self.addHost('h2', ip='10.0.0.3/24')
        h3 = self.addHost('h3', ip='10.0.0.4/24')
        h4 = self.addHost('h4', ip='10.0.0.5/24')
        h5 = self.addHost('h5', ip='10.0.0.6/24')
        h6 = self.addHost('h6', ip='10.0.0.7/24')
        h7 = self.addHost('h7', ip='10.0.0.8/24')
        h8 = self.addHost('h8', ip='10.0.0.9/24')

        self.addLink(h1, s1, cls=TCLink, delay='5ms')
        self.addLink(h2, s1, cls=TCLink, delay='5ms')
        self.addLink(h3, s2, cls=TCLink, delay='5ms')
        self.addLink(h4, s2, cls=TCLink, delay='5ms')
        self.addLink(h5, s3, cls=TCLink, delay='5ms')
        self.addLink(h6, s3, cls=TCLink, delay='5ms')
        self.addLink(h7, s4, cls=TCLink, delay='5ms')
        self.addLink(h8, s4, cls=TCLink, delay='5ms')

        self.addLink(s1, s2, cls=TCLink, delay='7ms')
        self.addLink(s2, s3, cls=TCLink, delay='7ms')
        self.addLink(s3, s4, cls=TCLink, delay='7ms')
        self.addLink(s4, s1, cls=TCLink, delay='7ms')
        self.addLink(s1, s3, cls=TCLink, delay='7ms')

def run():
    os.system('mn -c')
    topo = CustomRoutingTopo()
    net = Mininet(topo=topo, controller=OVSController, link=TCLink, switch=OVSSwitch)

    net.start()

    time.sleep(5)
    print("\nRunning Ping Tests with 30-second intervals:")

    h1, h3 = net.get('h1', 'h3')
    h2, h4 = net.get('h2', 'h4')
    h5, h7 = net.get('h5', 'h7')
    h6, h8 = net.get('h6', 'h8')

    print("Waiting 30 seconds for STP to converge...")
    time.sleep(30)
    for i in range(3):
        # print(f"\nRound {i+1}: Ping h3 -> h1")
        # print(h3.cmd('ping -w 30 %s' % h1.IP()))
        # time.sleep(30)

        # print(f"\nRound {i+1}: Ping h5 -> h7")
        # print(h5.cmd('ping -w 30 %s' % h7.IP()))
        # time.sleep(30)

        print(f"\nRound {i+1}: Ping h8 -> h2")
        print(h8.cmd('ping -w 30 %s' % h2.IP()))
        time.sleep(30)

    CLI(net)
    net.stop()

if __name__ == '__main__':
    setLogLevel('info')
    run()