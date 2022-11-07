"""
Module to define all of the functions needed to successfully parse through a GENI RSPEC file
Written and modified by various students over the years, written into a module by Peter Willis (pjw7904@rit.edu)
"""
from multiprocessing import connection
from xml.dom import minidom
from collections import defaultdict
import paramiko # External library for SSH API
import sys
import os
import configparser
import ipaddress

def orchestrateRemoteCommands(remoteGENINode, GENISliceDict, cmdList, getOutput=False, waitForResult=True):
    """
    Execution of a single or collection of commands on a GENI node.
    :param remoteGENINode: Name of the remote node
    :param GENISliceDict: Dictionary of GENI slice connection information
    :param cmdList: Either a string or string list of remote Bash commands
    :param getOutput: Return the output of the command
    :param waitForResult: Blocking call to wait for completion of command
    :return cmdOutput: Result of output
    :return: Return nothing
    """
    keyLocation = getConfigInfo("Local Utilities", "privateKey")
    psswd = getConfigInfo("GENI Credentials", "password")
    user = getConfigInfo("GENI Credentials", "username")

    remoteShell = paramiko.SSHClient()
    remoteShell.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    mykey = paramiko.RSAKey.from_private_key_file(keyLocation, password = psswd)

    host, port = getGENIHostConnectionArgs(GENISliceDict, remoteGENINode)
    remoteShell.connect(host, username = user, pkey = mykey, port = int(port))
    #print("Connected to: {0}".format(remoteGENINode))

    if(getOutput):
        if(type(cmdList) is str):
            stdin, stdout, stderr = remoteShell.exec_command(cmdList)
            cmdOutput = stdout.read()
            return cmdOutput
        else:
            sys.exit("not a valid command, please check what you are passing to the function")

    else:
        if(type(cmdList) is str):
            stdin, stdout, stderr = remoteShell.exec_command(cmdList)
            if(waitForResult):
                print("Result: {0}".format(getExitStatus(stdout.channel.recv_exit_status())))

        elif(type(cmdList) is list):
            print("#####SCRIPT COMMAND LIST#####")
            print(*cmdList, sep = "\n")
            print("#############################")

            for command in cmdList:
                print("sending command")
                stdin, stdout, stderr = remoteShell.exec_command(command)
                if(waitForResult):
                    print("Waiting for result")
                    print("Result: {0}".format(getExitStatus(stdout.channel.recv_exit_status())))

        else:
            sys.exit("not a valid command, please check what you are passing to the function")

    remoteShell.close()
    print("\n")

    if(getOutput):
        return cmdOutput

    else:
        return None


def getGENIFile(remoteGENINode, GENISliceDict, remoteFileSrc, localFileDst):
    """
    """

    psswd = getConfigInfo("GENI Credentials", "password")
    keyLocation = getConfigInfo("Local Utilities", "privateKey")
    user = getConfigInfo("GENI Credentials", "username")

    host, port = getGENIHostConnectionArgs(GENISliceDict, remoteGENINode)
    transport = paramiko.Transport((host, int(port)))
    mykey = paramiko.RSAKey.from_private_key_file(keyLocation, password = psswd)

    transport.connect(username = user, pkey = mykey)
    SFTP = paramiko.SFTPClient.from_transport(transport)

    if(type(remoteFileSrc) is str):
        SFTP.get(remoteFileSrc, localFileDst) # Make sure that the remote and local destinations are PURELY STRINGS, it will fail otherwise

    elif(type(remoteFileSrc) is list):
        for theFile in remoteFileSrc:
            SFTP.get(theFile, localFileDst)

    else:
        "Not a valid file path string or list of file paths"

    SFTP.close()
    transport.close()

    return None

def uploadToGENINode(remoteGENINode, GENISliceDict, localSrc, remoteDst):
    """
    """

    keyLocation = getConfigInfo("Local Utilities", "privateKey")
    psswd = getConfigInfo("GENI Credentials", "password")
    user = getConfigInfo("GENI Credentials", "username")

    host, port = getGENIHostConnectionArgs(GENISliceDict, remoteGENINode)
    transport = paramiko.Transport((host, int(port)))
    mykey = paramiko.RSAKey.from_private_key_file(keyLocation, password = psswd)

    transport.connect(username = user, pkey = mykey)
    SFTP = paramiko.SFTPClient.from_transport(transport)

    #  recursively upload a full directory [https://gist.github.com/johnfink8/2190472 put_all function, modified]
    if(os.path.isdir(localSrc)):
        os.chdir(os.path.split(localSrc)[0])
        parent = os.path.split(localSrc)[1]

        for walker in os.walk(parent):
            try:
                SFTP.mkdir(os.path.join(remoteDst, walker[0]).replace("\\", "/")) # Windows POSIX change
            except:
                pass

            for file in walker[2]:
                SFTP.put(os.path.join(walker[0], file).replace("\\", "/"), os.path.join(remoteDst, walker[0], file).replace("\\", "/")) # Windows POSIX change (src, dst)

    else:
        fullDest = os.path.join(remoteDst, os.path.basename(localSrc)).replace("\\", "/") # Windows POSIX change
        SFTP.put(localSrc, fullDest)

    SFTP.close()
    transport.close()

    return None

def downloadFromGENI(remoteGENINode,GENISliceDict, remoteSrc, localDest):
    keyLocation = getConfigInfo("Local Utilities", "privateKey")
    psswd = getConfigInfo("GENI Credentials", "password")
    user = getConfigInfo("GENI Credentials", "username")
    mykey = paramiko.RSAKey.from_private_key_file(keyLocation, password = psswd)
    host, port = getGENIHostConnectionArgs(GENISliceDict, remoteGENINode)
    transport = paramiko.Transport((host, int(port)))
    transport.connect(username = user, pkey = mykey)
    _connection = paramiko.SFTPClient.from_transport(transport)
    _connection.get(remoteSrc,localDest,callback=None)
    _connection.close()
    return None


def buildDictonary(rspec):
    connectionInfo = {} # Holds connection and network information for nodes in RSPEC
    addressingInfo = defaultdict(list) # Temporary collection to hold network information

    runOnNodes = getConfigInfo("MTP Utilities", "runOnNodes")
    
    nodeList = runOnNodes.split(",")

    # Parse RSPEC
    getConnectionInfo(rspec, connectionInfo, addressingInfo, nodeList)
    getAddressingInfo(connectionInfo, addressingInfo) # Adds addressing info into connection dictonary

    return connectionInfo

def getConnectionInfo(rspec, connectionInfo, addressingInfo, nodeList):
    xmlInfo = minidom.parse(rspec)

    # Get the top-level XML tag Node, which contains node information
    nodes = xmlInfo.getElementsByTagName('node')

    # Iterate through each node in the topology and grab its information
    for node in nodes:
        # Get name of node (ex: node-1)
        nodeName = node.attributes['client_id'].value.upper()

        # Limit what is added to the dictonary based on a list of nodes or a prefix
        if nodeName not in nodeList:
            continue

        # Get the login credentials for the node, which is the FQDN and port
        loginCreds = node.getElementsByTagName("login")
        hostname = (loginCreds[0].attributes["hostname"]).value
        port = (loginCreds[0].attributes["port"]).value

        # Each entry in the dictonary is a 3-tuple/triple containing the FQDN, port, and network information
        connectionInfo[nodeName] = (hostname, port, {})

        # Get the IP address and mask for each network interface attached to the node
        networkInterfaceInfo = node.getElementsByTagName("ip")
        for interface in networkInterfaceInfo:
            ipv4addr = (interface.attributes["address"]).value
            subnetMask = (interface.attributes["netmask"]).value

            # With the address and mask, determine the network the interface is attached to
            # example: 10.10.1.5/24 address --> 10.10.1.0/24 network
            fulladdr = "{}/{}".format(ipv4addr, subnetMask)
            networkAddress = ipaddress.ip_network(fulladdr, strict=False)

            # Store the networking info in the other dictonary for use later
            addressingInfo[networkAddress].append((nodeName, ipv4addr))

    return

def getAddressingInfo(connectionInfo, addressingInfo):
    # In the connection info, index 2 in the 3-tuple is the network information
    NETWORK_INFO_INDEX = 2

    # For each subnet found in the RSPEC
    for subnet in addressingInfo.items():
        subnetAddr = subnet[0]
        subnetInfo = subnet[1]

        # For each node attached to the subnet
        for node in subnetInfo:
            localNode = node[0]
            localAddr = node[1]

            # For each of the other nodes attached to the subnet
            for otherNode in subnetInfo:
                remoteNode = otherNode[0]
                remoteAddr = otherNode[1]
                if(remoteNode != localNode):
                    newInfo = {"remoteIPAddr": remoteAddr, "localIPAddr": localAddr, "subnet": subnetAddr}
                    connectionInfo[localNode][NETWORK_INFO_INDEX][remoteNode] = newInfo
   
    return

def printDictonaryContent(GENISliceDict,searchPort=None):
    HOSTNAME = 0
    PORT = 1
    NET_INFO = 2

    for node in sorted(GENISliceDict):
        output_dict = ""
        if searchPort is not None:
            output_dict = searchPort(node,GENISliceDict)
        print("node: {}".format(node))
        print("\thostname: {}".format(GENISliceDict[node][HOSTNAME]))
        print("\tport: {}".format(GENISliceDict[node][PORT]))

        for neighbor in GENISliceDict[node][NET_INFO]:
            print("\tsubnet: {}".format(GENISliceDict[node][NET_INFO][neighbor]["subnet"]))
            print("\t\tlocal IP address: {}".format(GENISliceDict[node][NET_INFO][neighbor]["localIPAddr"]))
            print("\t\tneighbor: {}".format(neighbor))
            print("\t\tneighbor IP address: {}".format(GENISliceDict[node][NET_INFO][neighbor]["remoteIPAddr"]))
            if searchPort is not None:
                print("\t\tConnected port:",output_dict[GENISliceDict[node][NET_INFO][neighbor]["localIPAddr"]])


    return

def getGENIHostConnectionArgs(GENISliceDict, node):
    """
    Gets the FQDN and port used by a GENI node in a given GENI slice via a custom dictonary.

    Args:
        GENI_dict (dict): A dictonary comprised of information about GENI nodes in a slice. The dictonary must be created with buildDictonary() to be valid.
        node (str): The hostname of a given GENI node (example: node-1)

    Returns:
        string (host): The FQDN of the requested GENI node.
        integer (port): The port number of the requested GENI node.
    """

    connectionInfo = GENISliceDict[node]
    host, port = connectionInfo[0], connectionInfo[1]

    return host, port


def getExitStatus(status):
    """
    Gets the status of a command run remotely via the Paramiko implementation of SSH

    Args:
        status (int): The result of a Paramiko command via stdout.channel.recv_exit_status(). A status of 0 means the command was run sucessfully, while a status of 1 means the command was run unsucessfully

    Returns:
        string: An easy-to-understand text interpretation of the exit status of a command
    """

    result = ""

    if(str(status) == "0"):
        result = "SUCCESS"

    elif(str(status) == "1"):
        result = "FAILED"

    else:
        result = "UNKNOWN"

    return result

def getNodeInfo(infoSection, infoType):
    '''
    Deprecated as of 2022, do not use. Keeping for historical purposes.
    '''
    with open(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'addrInfo.cnf').replace("\\", "/")) as fp:
        config = configparser.ConfigParser()
        config.readfp(fp)
        result = config.get(infoSection, infoType)

    return result


def getConfigInfo(infoSection, infoType):
    with open(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'creds.cnf').replace("\\", "/")) as fp: # Windows POSIX change
        config = configparser.ConfigParser()
        config.readfp(fp)
        result = config.get(infoSection, infoType)
    return result
