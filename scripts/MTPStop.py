#!/usr/bin/env python
#Grabs MTP convergence time information from specified nodes
#updated by Peter Willis (pjw7904@rit.edu)
from GENIutils import *

def main():
    RSPEC = getConfigInfo("Local Utilities", "RSPEC")
    username = getConfigInfo("GENI Credentials", "username")
    GENIDict = buildDictonary(RSPEC)

    # Command to stop the screen running the MTP implementation or the end node software running
    stopMTP = "sudo pkill screen"

    print("\n+---------Number of Nodes to Stop: {0}--------+".format(len(GENIDict)))
    for currentRemoteNode in GENIDict:
        if currentRemoteNode[0] != 'C':
            orchestrateRemoteCommands(currentRemoteNode, GENIDict, stopMTP)
    print("+-------------------------------------+\n")

if __name__ == "__main__":
    main()
