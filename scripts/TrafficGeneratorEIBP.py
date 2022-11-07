#!/usr/bin/env python
from scapy.all import *
from subprocess import call
import time
import os
import configparser
import argparse
import sys

frameCounter = {} # Global dictionary to hold frame/packet payload content for analysis on the receiving end

def main():
    # ArgumentParser object to read in command line arguments
    argParser = argparse.ArgumentParser(description = "Traffic generator (IEEE 802.3 and TCP/IP Protocol Suite) for switch testing purposes")

    # Determine where the traffic is going
    argParser.add_argument("-n", "--node") # Use the node argument for ease of use or...
    # ... use the IPv4 argument
    argParser.add_argument("-i", "--IPv4")

    # Determine what type of traffic needs to be sent
    argParser.add_argument("-r", "--receive", action = "store_true")
    argParser.add_argument("-a", "--analyze")                            # argument of capture needed

    # Determine factors supporting the traffic
    argParser.add_argument("-c", "--count", type = int) # argument of the number of frames to send or loops in the performance function)
    argParser.add_argument("-e", "--port", type = int) # By default it's eth1 for GENI nodes, but if for some reason it needs to change, use this argument
    argParser.add_argument("-d", "--delay", type = float)

    # Parse the arguments
    args = argParser.parse_args()

    # Gather the necessary info that is generic across all functions
    count = args.count
    port = args.port
    delay = args.delay

    if(args.receive):
        recvTraffic()

    elif(args.IPv4):
        dstLogicalAddr = args.IPv4
        sendTraffic(dstLogicalAddr, count, delay)

    elif(args.analyze):
        captureFile = args.analyze
        print("Working on capture file {0}...".format(captureFile))
        analyzeTraffic(captureFile)

    else:
        sys.exit("Syntax error: incorrect arguments (use -h for help)") # Error out if the arguments are bad or missing

    return None


def sendTraffic(dstLogicalAddr, count, delay):
    # Information needed to generate a custom payload and build protocol headers
    srcPhysicalAddr = get_if_hwaddr("eth1")

    # Added UDP at the end to maybe calm this down a bit.
    PDUToSend = Ether(src = srcPhysicalAddr)/IP(dst = dstLogicalAddr)/ICMP(type=1)
    generateContinousTraffic(PDUToSend, count, srcPhysicalAddr, delay)

    return None


def generateContinousTraffic(PDUToSend, numberOfFramesToSend, srcPhysicalAddr, delay):
    seqNum = 0
    payloadDelimiterSize = 2
    maxPayloadLength = 1400 #changed to 1400 because of fragmentation problems
    payloadPadding = 0
    complete = False

    if(numberOfFramesToSend is None):
        numberOfFramesToSend = -1

    while(not complete):
        try:
            seqNum += 1

            frameLength = len(str(seqNum) + srcPhysicalAddr) + len(PDUToSend) + payloadDelimiterSize
            if(frameLength < maxPayloadLength):
                payloadPadding = maxPayloadLength - frameLength
            else:
                payloadPadding = 0

            # UPDATED: changed randString to a string of A's the size of the payload padding
            frameWithCustomPayload = PDUToSend/Raw(load = "{0}|{1}|{2}".format(srcPhysicalAddr, seqNum, 'A' * payloadPadding))
            sendp(frameWithCustomPayload, iface="eth1", count = 1, verbose = False)

            sys.stdout.write("\rSent {0} frames".format(seqNum))
            sys.stdout.flush()

            if(seqNum == numberOfFramesToSend):
                complete = True
                print("\nFinished\n")

            if(delay is not None):
                time.sleep(delay)

        except KeyboardInterrupt:
            complete = True
            print("\nFinished\n")

    return None


def recvTraffic():
    global frameCounter
    srcPhysicalAddr = get_if_hwaddr("eth1")
    filterToUse = "ether src not {} and {}" # example full cmd: tcpdump -i eth1 ether src not 02:de:3a:3f:a2:fd and icmp
    commandToUse = 'sudo tshark -i eth1 -w results.pcap -F libpcap {}'

    try:
        # UPDATE: changed ethertype from MTP to IPv4
        filterForEIBPTraffic = '"icmp[0] == 1"'
        filterToUse = filterToUse.format(srcPhysicalAddr, filterForEIBPTraffic)
        commandToUse = commandToUse.format(filterToUse)
        call(commandToUse, shell=True)

    except KeyboardInterrupt:
        print("\nExited program")

    return None


def analyzeTraffic(capture):
    frameCounter = {}

    capture = rdpcap(capture)

    for frame in capture:
        if(frame[ICMP].type == 1):
            payload = frame[Raw].load

        else:
            sys.exit("Can't find a payload")

        payload = str(payload, 'utf-8')
        payloadContent = payload.split("|")
        source = payloadContent[0]
        newSeqNum = int(payloadContent[1])

        if source not in frameCounter:
            # Updated Sequence Number, List of missed frames, Total number of frames sent, list of out of order frames, lost of duplicate frames
            frameCounter[source] = [newSeqNum, [], 1, [], []]

        else:
            currentSeqNum = frameCounter[source][0]          # The current sequence number for the source address
            expectedNextSeqNum = frameCounter[source][0] + 1 # The next expected sequence number for the source address

            if(currentSeqNum == newSeqNum and newSeqNum == 1):
                continue

            if(newSeqNum in frameCounter[source][1]):
                frameCounter[source][1].remove(newSeqNum)
                frameCounter[source][3].append(newSeqNum)
                frameCounter[source][2] += 1
                continue

            if(newSeqNum not in frameCounter[source][1] and (newSeqNum < currentSeqNum or newSeqNum == currentSeqNum)): # NEW STUF TO LOOK FOR DUPLICATES
                frameCounter[source][4].append(newSeqNum)
                frameCounter[source][2] += 1
                continue

            missedFrames = newSeqNum - expectedNextSeqNum # get frames 1-5, get frame 10, missing 6-9

            while(missedFrames != 0):
                missingSeqNum = currentSeqNum + missedFrames
                frameCounter[source][1].append(missingSeqNum)
                missedFrames -= 1

            frameCounter[source][0] = newSeqNum # Update the current sequence number
            frameCounter[source][2] += 1        # Update how many frames we have received from this source in total

    f = open("trafficResult.txt", "w+")
    for source in frameCounter:
        endStatement = "{0} frames lost from source {1} {2} | {3} received | {4} Not sequential {5} | {6} duplicates {7}\n"
        outputMissingFrames = ""
        outputUnorderedFrames = ""
        outputDuplicateFrames = ""

        if(frameCounter[source][1]):
            frameCounter[source][1].sort()
            outputMissingFrames = frameCounter[source][1]

        if(frameCounter[source][3]):
            frameCounter[source][3].sort()
            outputUnorderedFrames = frameCounter[source][3]

        if(frameCounter[source][4]):
            frameCounter[source][4].sort()
            outputDuplicateFrames = frameCounter[source][4]

        f.write(endStatement.format(len(frameCounter[source][1]), source, outputMissingFrames, frameCounter[source][2], len(frameCounter[source][3]), outputUnorderedFrames, len(frameCounter[source][4]), outputDuplicateFrames))

    f.close()

    return None


def getTrafficInfo(args):
    dstLogicalAddr = "None"  # Network Layer destination address (almost always IPv4 address)

    if(args.node):
        with open(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'addrInfo.cnf').replace("\\", "/")) as fp:
            config = configparser.ConfigParser()
            config.readfp(fp)
            dstLogicalAddr = config.get(args.node, "l3address")
    else:
        dstLogicalAddr = args.IPv4

    return dstLogicalAddr

if __name__ == "__main__":
    main()
