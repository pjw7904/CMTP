#!/usr/bin/env python
from platform import node
from GENIutils import *
import json,time

def main():
    RSPEC = getConfigInfo("Local Utilities", "RSPEC")
    username = getConfigInfo("GENI Credentials", "username")
    codeDirectory = getConfigInfo("MTP Utilities", "localCodeDirectory")
    #endnodeInfoLocation = getConfigInfo("Local Utilities", "endnodeInfoLocation")
    codeDestination = getConfigInfo("GENI Credentials", "remoteCodeDirectory")
    endNodeNamingSyntax = getConfigInfo("MTP Utilities", "endNodeName")
    GENIDict = buildDictonary(RSPEC)

    print("\n+---------Number of Nodes: {0}--------+".format(len(GENIDict)))

    remoteWorkingDirectory = os.path.join(codeDestination, os.path.basename(codeDirectory)).replace("\\", "/")
    runMTPImplementation(GENIDict, remoteWorkingDirectory)

           
    print("+-------------------------------------+\n")


def get_node_tiers(data):
    node_tier_list = []

    queue = [data['topology']]
    
    while len(queue) != 0:
        temp_list = []
        while len(queue) != 0:
            temp_list.append(queue.pop(0))

        tot = []
        for pod in temp_list:
            for spine in pod['topSpines']:
                tot.append(spine)
        
        node_tier_list.append(tot)
            
        for pod in temp_list:
            if "pods" in pod:
                for pod_t in pod["pods"]:
                    queue.append(pod_t)
    
    node_tier_list.append(data['topology']['leaves'])

    return node_tier_list



def runMTPImplementation(GENIDict, workingDirectory):
    cmdList = []

    f = open("node_config.json")
    data = json.load(f)
    f.close()
    node_tier_list = get_node_tiers(data)    

    #Get time to run program and add a minute for runtime
    currentTime = int(time.time())
    runTime = currentTime + 60


    # MTP nodes Startup commands
    #startMTP = "cd ~/SRC ; sudo ./run -t {0}".format(runTime)
    startMTP = "cd ~/SRC; screen -dmS mtp -L -Logfile MTP_{0}.log bash -c 'sudo ./MTPstart -t {1}; exec bash'"


    for i in range(len(node_tier_list)):
        for currentRemoteNode in node_tier_list[i]:
            cmdList[:] = []
            cmdList.append("cd ~/SRC; sudo rm MTP_{0}.log".format(currentRemoteNode))
            cmdList.append("cd ~/SRC ; gcc main.c mtp_build.c mtp_send.c mtp_struct.c mtp_utils.c -o MTPstart")        
            if i == len(node_tier_list) - 1:
                cmdList.append('cd ~/SRC ; echo "isTor:true" > mtp_dcn.conf')
                cmdList.append('cd ~/SRC ; echo "isTopSpine:false" >> mtp_dcn.conf')
            else:
                cmdList.append('cd ~/SRC ; echo "isTor:false" > mtp_dcn.conf')
                if i == 0:
                    cmdList.append('cd ~/SRC ; echo "isTopSpine:true" >> mtp_dcn.conf')
                else:
                    cmdList.append('cd ~/SRC ; echo "isTopSpine:false" >> mtp_dcn.conf')
            
            cmdList.append('cd ~/SRC ; echo "tier:{0}" >> mtp_dcn.conf'.format(len(node_tier_list) - i))
            if i == len(node_tier_list) - 1:
                cmdList.append('cd ~/SRC ; echo "torEthPortName:{0}" >> mtp_dcn.conf'.format(data['topology']['leavesNetworkPortDict'][currentRemoteNode]))
            cmdList.append('sudo sysctl -w net.ipv4.ip_forward=0')
            cmdList.append(startMTP.format(currentRemoteNode,runTime))
        
            orchestrateRemoteCommands(currentRemoteNode, GENIDict, cmdList)       
        time.sleep(3)         


if __name__ == "__main__":
    main()
