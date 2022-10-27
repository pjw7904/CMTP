"""
Module to define all of the functions needed to successfully parse through a GENI RSPEC file
Written and modified by various students over the years, written into a module by Peter Willis (pjw7904@rit.edu)
"""
from __future__ import print_function
from xml.dom import minidom
from time import gmtime, strftime
from subprocess import call
from scapy.all import *
from collections import defaultdict
from datetime import datetime, time, timedelta
import paramiko
import pdb
import sys
import os
import re
import zipfile
import re
import time
import datetime
import subprocess
import configparser
import argparse

def orchestrateRemoteCommands(remoteGENINode, GENISliceDict, cmdList, getOutput=False):
    """
    """

    keyLocation = getConfigInfo("Local Utilities", "privateKey")
    psswd = getConfigInfo("GENI Credentials", "password")
    user = getConfigInfo("GENI Credentials", "username")

    remoteShell = paramiko.SSHClient()
    remoteShell.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    mykey = paramiko.RSAKey.from_private_key_file(keyLocation, password = psswd)

    host, port = getGENIHostConnectionArgs(GENISliceDict, remoteGENINode)
    remoteShell.connect(host, username = user, pkey = mykey, port = int(port))
    print("Connected to: {0}".format(remoteGENINode))

    if(getOutput):
        if(type(cmdList) is str):
            stdin, stdout, stderr = remoteShell.exec_command(cmdList)
            cmdOutput = stdout.read()

        else:
            sys.exit("not a valid command, please check what you are passing to the function")

    else:
        if(type(cmdList) is str):
            stdin, stdout, stderr = remoteShell.exec_command(cmdList)
            print("Result: {0}".format(getExitStatus(stdout.channel.recv_exit_status())))

        elif(type(cmdList) is list):
            print("#####SCRIPT COMMAND LIST#####")
            print(*cmdList, sep = "\n")
            print("#############################")

            for command in cmdList:
                stdin, stdout, stderr = remoteShell.exec_command(command)
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


def buildDictonary(rspec_file):
    """
    Builds a dictionary data structure based on a given GENI RSPEC file with the
    necessary information to parse out GENI host information. The cumulation of the work done
    via the formKeys() function and the formValues() function. The xmldoc module is used to
    initally parse the file, as the file is XML compliant.

    Returns:
        dictonary: The GENI slice information parsed into a dictonary.
    """

    xmldoc = minidom.parse(rspec_file)

    keys = formKeys('node', 'client_id', xmldoc)
    vals = formValues('node', 'interface', 'address', 'mac_address', xmldoc)

    RSPECDict = dict(zip(keys, vals))

    return RSPECDict


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


def formKeys(node, client_id, xmlInfo):
    """
    Define the keys to be used in a GENI slice dictonary.

    Args:
        node (str): The name of the XML element to parse the node name with.
        client_id (str): The name of the XML element to parse the client ID with.
        xmlInfo (xml): The result of using the xmldoc library on a valid GENI slice RSPEC.

    Returns:
        list: The GENI node name and interface names to be used in a dictonary as keys.
    """

    must_keys = []

    node_lst = xmlInfo.getElementsByTagName(node)

    for node in node_lst:
        interface = node.attributes[client_id]
        must_keys.append(interface.value)

    return must_keys


def formValues(node, interface, ip, mac, xmlInfo):
    """
    Define the values to be used in a GENI slice dictonary.

    Args:
        node (str): The name of the XML element to parse the node name with.
        interface (str): The name of the XML element to parse the node interfaces with.
        ip (str): The name of the XML element to parse the IPv4 addresses with.
        mac (str): The name of the XML element to parse the MAC addresses with.
        xmlInfo (xml): The result of using the xmldoc library on a valid GENI slice RSPEC.

    Returns:
        list: The GENI node name and interface names to be used in a dictonary as values.
    """

    must_values = []
    node_lst = xmlInfo.getElementsByTagName(node)

    def hostInfo(GENI_node):
        """
        Get the FQDN and and port needed to connect to a remote GENI node via Paramiko (SSH).

        Args:
            GENI_node (str): The name of the GENI node.

        Returns:
            string: The tuple of the host FQDN and port.
        """

        host_all = []
        login_info = GENI_node.getElementsByTagName("login") # login is the tag name

        for elem in login_info:
            hostname = (elem.attributes["hostname"]).value
            port = (elem.attributes["port"]).value

        host = str(hostname), str(port)

        return host

    def ipStrip(GENI_node):
        """
        Get the IPv4 addresses associated with a GENI node

        Args:
            GENI_node (str): The name of the GENI node.

        Returns:
            tuple: A collection of interface names and IPv4 addresses for the GENI node
        """

        cat = []
        int_lst = GENI_node.getElementsByTagName("interface")

        for item in int_lst:
            intr = (item.attributes["client_id"]).value
            cat.append(intr)

        ip_lst = GENI_node.getElementsByTagName("ip")

        for ip in ip_lst:
            ip_address = (ip.attributes["address"]).value
            cat.append(ip_address)

        return tuple(cat)

    for node in node_lst:
        hostname = hostInfo(node) # Uses the nested hostInfo() function
        ipaddr = ipStrip(node) # Uses the nested ipStrip() function
        full = hostname + ipaddr

        must_values.append(full)

    return must_values


def getConfigInfo(infoSection, infoType):
    """
    """

    with open(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'creds.cnf').replace("\\", "/")) as fp: # Windows POSIX change
        config = configparser.ConfigParser()
        config.readfp(fp)
        result = config.get(infoSection, infoType)

    return result
